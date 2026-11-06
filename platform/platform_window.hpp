#pragma once
// Platform abstraction: OS window, OpenGL context, input events, dialogs.
// The Win32 + wgl backend is implemented here; another backend (e.g. X11 +
// GLX) can be added behind this same interface. Business code must not call
// Win32 APIs directly -- everything goes through this header.

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace enigma::platform {

struct WindowDesc {
    std::string title;
    int width = 1280;   // logical window size in device-independent pixels
    int height = 800;
    int glMajor = 3;    // required OpenGL version (3.3 Core)
    int glMinor = 3;
    bool vsync = true;
    int msaaSamples = 4;  // requested multisample count; 0 disables MSAA.
                          // The backend falls back to a non-multisample
                          // pixel format when the request cannot be met.
};

// One queued input/window event. Coordinates are client-area coordinates.
struct Event {
    enum class Type : int {
        KeyDown,          // code = Win32 virtual-key code, x = 1 when auto-repeat
        KeyUp,            // code = Win32 virtual-key code
        Char,             // code = UTF-16 code unit from TranslateMessage
        MouseDown,        // code = 0 left / 1 right / 2 middle
        MouseUp,          // code = same encoding as MouseDown
        MouseDoubleClick, // code = same encoding as MouseDown
        MouseMove,        // x, y = client coordinates
        MouseWheel,       // code = signed wheel delta (multiples of 120)
        Resize,           // x = new client width, y = new client height
    };

    Type type{};
    uint32_t code = 0;
    int x = 0;
    int y = 0;
};

// Abstract window + OpenGL context. Create via PlatformWindow::create().
class PlatformWindow {
public:
    virtual ~PlatformWindow() = default;

    // Creates the OS window and an OpenGL <major>.<minor> Core context.
    // Returns nullptr on failure; call lastError() for a human-readable
    // reason (the caller shows the error dialog and exits non-zero).
    [[nodiscard]] static std::unique_ptr<PlatformWindow> create(const WindowDesc& desc);

    // Pumps the OS message loop into the event queue. Returns false once the
    // window has been closed and the app should shut down.
    [[nodiscard]] virtual bool pumpEvents() = 0;

    // Event queue for the current frame; emptied at the next pumpEvents().
    [[nodiscard]] const std::vector<Event>& events() const { return events_; }

    // True while the given virtual key is held down.
    [[nodiscard]] bool isKeyDown(uint32_t virtualKey) const {
        return keysDown_[virtualKey & 0xFFu];
    }

    virtual void swapBuffers() = 0;
    virtual void setVsync(bool enabled) = 0;
    [[nodiscard]] virtual std::pair<int, int> clientSize() const = 0;
    // Current cursor position in client coordinates (queried on demand,
    // unlike the queued MouseMove events).
    [[nodiscard]] virtual std::pair<int, int> cursorPosition() const = 0;
    virtual void requestClose() = 0;

protected:
    std::vector<Event> events_;
    std::array<bool, 256> keysDown_{};
};

// Shows a modal native error dialog (MessageBoxW on Windows).
void showErrorDialog(const std::string& title, const std::string& message);

// Detail string for the most recent creation failure of this thread.
[[nodiscard]] const std::string& lastError();

// Monotonic high-resolution clock in seconds (delta-time driven main loop).
[[nodiscard]] double highResTimeSeconds();

}  // namespace enigma::platform
