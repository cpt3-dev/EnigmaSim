// Application entry point:
// Renders the procedural Enigma machine scene (rotors, Ringstellung collars,
// reflector, lampboard, plugboard, bakelite casing) with an orbit camera.
//
// Controls:
//   Left drag    orbit camera        Right drag   pan camera
//   Mouse wheel  zoom                Double-click reset view
//   F / T / C    front / top / cutaway preset views
//   1..4         switch machine configuration presets
//   Space        step the rotors (one keypress, no letter routing yet)
//   Escape       quit

#include "camera.hpp"
#include "enigma.hpp"
#include "gl_loader.hpp"
#include "gl_objects.hpp"
#include "math.hpp"
#include "platform_window.hpp"
#include "renderer.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace {
namespace gl = enigma::gl;
namespace platform = enigma::platform;
namespace scene = enigma::scene;
namespace core = enigma::core;
namespace math = enigma::math;

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr float kFieldOfViewDegrees = 45.0f;
constexpr float kNearPlane = 0.1f;
constexpr float kFarPlane = 60.0f;
constexpr float kMaxFrameDelta = 0.1f;  // clamp after stalls (s)
constexpr float kClearRed = 0.07f;
constexpr float kClearGreen = 0.08f;
constexpr float kClearBlue = 0.10f;
constexpr int kMsaaSamples = 4;

// Virtual-key codes as delivered by the platform layer.
constexpr std::uint32_t kKeyEscape = 0x1B;
constexpr std::uint32_t kKeySpace = 0x20;
constexpr std::uint32_t kKey1 = '1';
constexpr std::uint32_t kKey4 = '4';
constexpr std::uint32_t kKeyF = 'F';
constexpr std::uint32_t kKeyT = 'T';
constexpr std::uint32_t kKeyC = 'C';
constexpr std::uint32_t kMouseButtonLeft = 0;
constexpr std::uint32_t kMouseButtonRight = 1;

constexpr int kPresetCount = 4;

// Machine presets switched with the number keys.
// Rotor order is left -> right, matching core::MachineConfig.
std::array<core::MachineConfig, kPresetCount> buildPresetConfigs() {
    std::array<core::MachineConfig, kPresetCount> presets{};

    presets[0].type = core::MachineType::EnigmaI;
    presets[0].rotors = {{"I", 'A', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}};
    presets[0].reflector = "UKW-B";

    presets[1].type = core::MachineType::EnigmaI;
    presets[1].rotors = {{"II", 'B', 'K'}, {"IV", 'D', 'Q'}, {"V", 'F', 'E'}};
    presets[1].reflector = "UKW-B";
    presets[1].plugboard = {{{'A', 'H'}, {'D', 'Q'}, {'F', 'N'}}};

    presets[2].type = core::MachineType::M3;
    presets[2].rotors = {
        {"III", 'B', 'X'}, {"V", 'E', 'M'}, {"VIII", 'H', 'R'}};
    presets[2].reflector = "UKW-B";
    presets[2].plugboard = {{{'B', 'L'}}};

    presets[3].type = core::MachineType::M4;
    presets[3].rotors = {{"Beta", 'A', 'V'}, {"I", 'A', 'J'},
                         {"II", 'A', 'N'}, {"III", 'A', 'A'}};
    presets[3].reflector = "UKW-B-thin";
    presets[3].plugboard = {{{'A', 'E'}, {'M', 'R'}}};

    return presets;
}

bool reportGlError(const char* where, gl::GLenum& lastReported) {
    const gl::GLenum error = gl::glGetError();
    if (error != gl::GL_NO_ERROR && error != lastReported) {
        lastReported = error;
        std::fprintf(stderr, "GL error 0x%04X at %s\n", error, where);
        return false;
    }
    return true;
}
}  // namespace

int main() {
    platform::WindowDesc desc;
    desc.title = "Enigma Machine Simulator";
    desc.width = kWindowWidth;
    desc.height = kWindowHeight;
    desc.glMajor = 3;
    desc.glMinor = 3;
    desc.vsync = true;
    desc.msaaSamples = kMsaaSamples;

    auto window = platform::PlatformWindow::create(desc);
    if (window == nullptr) {
        platform::showErrorDialog(
            "Enigma Machine Simulator",
            "Failed to create a window with an OpenGL 3.3 Core context.\n\n" +
                platform::lastError());
        return 1;
    }

    if (gl::load() != 0) {
        platform::showErrorDialog(
            "Enigma Machine Simulator",
            std::string("Failed to load OpenGL 3.3 entry points.\n\n") +
                gl::loadError());
        return 1;
    }

    window->setVsync(true);
    gl::glEnable(gl::GL_DEPTH_TEST);
    gl::glEnable(gl::GL_CULL_FACE);
    gl::glEnable(gl::GL_MULTISAMPLE);

    scene::EnigmaRenderer renderer;
    std::string sceneLog;
    if (!renderer.initialize(sceneLog)) {
        platform::showErrorDialog("Enigma Machine Simulator",
                                  "Scene setup failed:\n\n" + sceneLog);
        return 1;
    }

    const auto presets = buildPresetConfigs();
    try {
        // Validate every preset up front so preset switches can never throw.
        for (const core::MachineConfig& config : presets) {
            const core::Machine probe(config);
            static_cast<void>(probe);
        }
    } catch (const core::ConfigError& error) {
        platform::showErrorDialog(
            "Enigma Machine Simulator",
            std::string("Invalid built-in machine preset: ") + error.what());
        return 1;
    }

    core::Machine machine(presets[0]);
    renderer.applyConfig(presets[0]);
    scene::OrbitCamera camera;

    const auto initialSize = window->clientSize();
    int viewportWidth = initialSize.first;
    int viewportHeight = initialSize.second;
    gl::glViewport(0, 0, viewportWidth, viewportHeight);

    enum class DragMode { None, Orbit, Pan };
    DragMode drag = DragMode::None;
    int lastMouseX = 0;
    int lastMouseY = 0;

    gl::GLenum lastReportedError = gl::GL_NO_ERROR;
    reportGlError("startup", lastReportedError);

    double previousTime = platform::highResTimeSeconds();
    while (window->pumpEvents()) {
        const double now = platform::highResTimeSeconds();
        float delta = static_cast<float>(now - previousTime);
        previousTime = now;
        if (delta > kMaxFrameDelta) {
            delta = kMaxFrameDelta;  // avoid a jump after a stall
        }

        for (const platform::Event& event : window->events()) {
            switch (event.type) {
                case platform::Event::Type::Resize:
                    viewportWidth = event.x;
                    viewportHeight = event.y;
                    gl::glViewport(0, 0, viewportWidth, viewportHeight);
                    break;
                case platform::Event::Type::KeyDown:
                    if (event.x != 0) {
                        break;  // ignore auto-repeat
                    }
                    if (event.code == kKeyEscape) {
                        window->requestClose();
                    } else if (event.code == kKeySpace) {
                        machine.step();
                    } else if (event.code == kKeyF) {
                        camera.applyPreset(scene::ViewPreset::Front);
                    } else if (event.code == kKeyT) {
                        camera.applyPreset(scene::ViewPreset::Top);
                    } else if (event.code == kKeyC) {
                        camera.applyPreset(scene::ViewPreset::Cutaway);
                    } else if (event.code >= kKey1 && event.code <= kKey4) {
                        const std::size_t index = event.code - kKey1;
                        machine = core::Machine(presets[index]);
                        renderer.applyConfig(presets[index]);
                    }
                    break;
                case platform::Event::Type::MouseDown:
                    if (event.code == kMouseButtonLeft) {
                        drag = DragMode::Orbit;
                    } else if (event.code == kMouseButtonRight) {
                        drag = DragMode::Pan;
                    }
                    lastMouseX = event.x;
                    lastMouseY = event.y;
                    break;
                case platform::Event::Type::MouseUp:
                    drag = DragMode::None;
                    break;
                case platform::Event::Type::MouseDoubleClick:
                    if (event.code == kMouseButtonLeft) {
                        camera.resetToDefault();
                    }
                    break;
                case platform::Event::Type::MouseMove:
                    if (drag != DragMode::None) {
                        const float dx =
                            static_cast<float>(event.x - lastMouseX);
                        const float dy =
                            static_cast<float>(event.y - lastMouseY);
                        if (drag == DragMode::Orbit) {
                            camera.orbit(dx, dy);
                        } else {
                            camera.pan(dx, dy);
                        }
                    }
                    lastMouseX = event.x;
                    lastMouseY = event.y;
                    break;
                case platform::Event::Type::MouseWheel:
                    camera.zoom(static_cast<float>(
                        static_cast<std::int16_t>(event.code & 0xFFFFu)));
                    break;
                case platform::Event::Type::KeyUp:
                case platform::Event::Type::Char:
                    break;
            }
        }

        const float aspect =
            viewportHeight > 0
                ? static_cast<float>(viewportWidth) /
                      static_cast<float>(viewportHeight)
                : 1.0f;
        const math::Mat4 projection =
            math::perspective(math::radians(kFieldOfViewDegrees), aspect,
                              kNearPlane, kFarPlane);

        gl::glClearColor(kClearRed, kClearGreen, kClearBlue, 1.0f);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

        renderer.render(delta, machine, camera.viewMatrix(), projection,
                        camera.eye(), camera.cutawayActive());

        reportGlError("frame", lastReportedError);
        window->swapBuffers();
    }

    // RAII releases all GL objects; the window destructor releases the
    // context. Clean shutdown with exit code 0.
    return 0;
}
