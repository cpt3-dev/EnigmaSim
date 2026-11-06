// Win32 + wgl backend for the platform abstraction.
// All OS calls for windowing, input, dialogs and timing live in this file.

#include "platform_window.hpp"

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace enigma::platform {
namespace {

constexpr wchar_t kWindowClassName[] = L"EnigmaSimWindowClass";

// wgl extension typedefs (loaded by hand, like the GL functions).
using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC(APIENTRY*)(HDC, HGLRC, const int*);
using PFNWGLSWAPINTERVALEXTPROC = BOOL(APIENTRY*)(int);
using PFNWGLCHOOSEPIXELFORMATARBPROC = BOOL(APIENTRY*)(HDC, const int*,
                                                       const FLOAT*, UINT,
                                                       int*, UINT*);

constexpr int kWglContextMajorVersionArb = 0x2091;
constexpr int kWglContextMinorVersionArb = 0x2092;
constexpr int kWglContextProfileMaskArb = 0x9126;
constexpr int kWglContextCoreProfileBitArb = 0x00000001;
constexpr int kWglContextDebugBitArb = 0x00000002;

// WGL_ARB_pixel_format constants.
constexpr int kWglDrawToWindowArb = 0x2001;
constexpr int kWglSupportOpenGlArb = 0x2010;
constexpr int kWglDoubleBufferArb = 0x2011;
constexpr int kWglPixelTypeArb = 0x2013;
constexpr int kWglColorBitsArb = 0x2014;
constexpr int kWglDepthBitsArb = 0x2022;
constexpr int kWglStencilBitsArb = 0x2023;
constexpr int kWglTypeRgbaArb = 0x202B;
constexpr int kWglSampleBuffersArb = 0x2041;
constexpr int kWglSamplesArb = 0x2042;

constexpr int kColorBits = 32;
constexpr int kDepthBits = 24;
constexpr int kStencilBits = 8;
constexpr int kBootstrapWindowSize = 64;

std::string& threadLastError() {
    static thread_local std::string error;
    return error;
}

PIXELFORMATDESCRIPTOR makeLegacyDescriptor() {
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = kColorBits;
    pfd.cDepthBits = kDepthBits;
    pfd.cStencilBits = kStencilBits;
    pfd.iLayerType = PFD_MAIN_PLANE;
    return pfd;
}

// Everything the bootstrap on the throwaway window produces and the real
// window's GL setup consumes.
struct GlBootstrap {
    PFNWGLCREATECONTEXTATTRIBSARBPROC createContextAttribs = nullptr;
    int pixelFormat = 0;  // 0 -> caller falls back to legacy ChoosePixelFormat
};

// Creates a hidden throwaway window to host a temporary legacy context. The
// temporary context exposes the wgl extensions needed to (a) pick an
// MSAA-capable pixel format for the real window and (b) create the final
// 3.3 Core context. The real window's DC may only receive SetPixelFormat
// once, so the format has to be selected before it is applied there.
bool bootstrapGl(const WindowDesc& desc, GlBootstrap& out) {
    HWND dummy = nullptr;
    HDC dummyDc = nullptr;
    HGLRC tempContext = nullptr;
    const auto fail = [&](const std::string& message) {
        if (tempContext != nullptr) {
            wglMakeCurrent(dummyDc, nullptr);
            wglDeleteContext(tempContext);
        }
        if (dummyDc != nullptr) {
            ReleaseDC(dummy, dummyDc);
        }
        if (dummy != nullptr) {
            DestroyWindow(dummy);
        }
        threadLastError() = message;
        return false;
    };

    dummy = CreateWindowExW(0, kWindowClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0,
                            kBootstrapWindowSize, kBootstrapWindowSize, nullptr,
                            nullptr, GetModuleHandleW(nullptr), nullptr);
    if (dummy == nullptr) {
        return fail("CreateWindowExW (bootstrap window) failed (error " +
                    std::to_string(GetLastError()) + ")");
    }
    dummyDc = GetDC(dummy);
    if (dummyDc == nullptr) {
        return fail("GetDC (bootstrap window) failed");
    }

    const PIXELFORMATDESCRIPTOR legacyDescriptor = makeLegacyDescriptor();
    const int legacyFormat = ChoosePixelFormat(dummyDc, &legacyDescriptor);
    if (legacyFormat == 0) {
        return fail("ChoosePixelFormat (bootstrap window) failed (error " +
                    std::to_string(GetLastError()) + ")");
    }
    if (!SetPixelFormat(dummyDc, legacyFormat, &legacyDescriptor)) {
        return fail("SetPixelFormat (bootstrap window) failed (error " +
                    std::to_string(GetLastError()) + ")");
    }

    tempContext = wglCreateContext(dummyDc);
    if (tempContext == nullptr) {
        return fail("wglCreateContext (bootstrap window) failed (error " +
                    std::to_string(GetLastError()) + ")");
    }
    if (!wglMakeCurrent(dummyDc, tempContext)) {
        return fail("wglMakeCurrent (bootstrap window) failed");
    }

    const auto createContextAttribs =
        reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
            reinterpret_cast<void*>(wglGetProcAddress("wglCreateContextAttribsARB")));
    if (createContextAttribs == nullptr) {
        return fail(
            "OpenGL driver does not support WGL_ARB_create_context; an "
            "OpenGL 3.3 Core capable driver is required.");
    }
    out.createContextAttribs = createContextAttribs;

    const auto choosePixelFormat =
        reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(
            reinterpret_cast<void*>(wglGetProcAddress("wglChoosePixelFormatARB")));
    if (choosePixelFormat != nullptr && desc.msaaSamples > 0) {
        const int attribs[] = {
            kWglDrawToWindowArb,   TRUE,
            kWglSupportOpenGlArb,  TRUE,
            kWglDoubleBufferArb,   TRUE,
            kWglPixelTypeArb,      kWglTypeRgbaArb,
            kWglColorBitsArb,      kColorBits,
            kWglDepthBitsArb,      kDepthBits,
            kWglStencilBitsArb,    kStencilBits,
            kWglSampleBuffersArb,  TRUE,
            kWglSamplesArb,        desc.msaaSamples,
            0,
        };
        int formats[1] = {0};
        UINT formatCount = 0;
        if (choosePixelFormat(dummyDc, attribs, nullptr, 1, formats,
                              &formatCount) &&
            formatCount > 0) {
            out.pixelFormat = formats[0];
        }
        // Otherwise: no MSAA-capable format; the caller falls back to the
        // legacy descriptor so the app still runs without antialiasing.
    }

    wglMakeCurrent(dummyDc, nullptr);
    wglDeleteContext(tempContext);
    ReleaseDC(dummy, dummyDc);
    DestroyWindow(dummy);
    return true;
}

std::wstring toWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
                                         static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                        wide.data(), size);
    return wide;
}

int16_t lowordSigned(LPARAM value) {
    return static_cast<int16_t>(static_cast<int16_t>(LOWORD(value)));
}

class Win32Window : public PlatformWindow {
public:
    ~Win32Window() override { destroyGlResources(); }

    bool pumpEvents() override;
    void swapBuffers() override { SwapBuffers(dc_); }
    void setVsync(bool enabled) override;
    std::pair<int, int> clientSize() const override;
    std::pair<int, int> cursorPosition() const override;
    void requestClose() override;

    bool initialize(const WindowDesc& desc);

private:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void destroyGlResources();
    void onResize(int width, int height);

    HWND hwnd_ = nullptr;
    HDC dc_ = nullptr;
    HGLRC context_ = nullptr;
    PFNWGLSWAPINTERVALEXTPROC wglSwapInterval_ = nullptr;
    bool quitRequested_ = false;
    bool vsyncEnabled_ = false;
};

bool Win32Window::initialize(const WindowDesc& desc) {
    // Make the process DPI-aware so the GUI is not upscaled by DWM.
    // Loaded dynamically: the API is missing on Windows 7.
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(void*);
    if (const HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        const auto setDpiAware =
            reinterpret_cast<SetProcessDpiAwarenessContextFn>(
                reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")));
        if (setDpiAware != nullptr) {
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == ((HANDLE)-4)
            setDpiAware(reinterpret_cast<void*>(-4));
        }
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC | CS_DBLCLKS;  // own DC keeps the GL pixel format stable
    wc.lpfnWndProc = &Win32Window::wndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));  // IDC_ARROW (W)
    wc.lpszClassName = kWindowClassName;
    if (RegisterClassExW(&wc) == 0) {
        threadLastError() = "RegisterClassExW failed (error " +
                            std::to_string(GetLastError()) + ")";
        return false;
    }

    const std::wstring title = toWide(desc.title);
    // lpCreateParams passes `this` so the static wndProc can resolve the instance.
    hwnd_ = CreateWindowExW(0, kWindowClassName, title.c_str(), WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, desc.width, desc.height,
                            nullptr, nullptr, wc.hInstance, this);
    if (hwnd_ == nullptr) {
        threadLastError() = "CreateWindowExW failed (error " +
                            std::to_string(GetLastError()) + ")";
        return false;
    }

    dc_ = GetDC(hwnd_);
    if (dc_ == nullptr) {
        threadLastError() = "GetDC failed";
        return false;
    }

    // Bootstrap on a throwaway window: temporary legacy context -> wgl
    // extensions -> MSAA-capable pixel format (when available).
    GlBootstrap bootstrap{};
    if (!bootstrapGl(desc, bootstrap)) {
        return false;
    }

    // The real window's DC accepts SetPixelFormat exactly once, so the
    // format selected above is applied here and never changed again.
    const PIXELFORMATDESCRIPTOR descriptor = makeLegacyDescriptor();
    int pixelFormat = bootstrap.pixelFormat;
    if (pixelFormat == 0) {
        // No ARB multisample format: fall back to the legacy descriptor.
        pixelFormat = ChoosePixelFormat(dc_, &descriptor);
        if (pixelFormat == 0) {
            threadLastError() = "ChoosePixelFormat failed (error " +
                                std::to_string(GetLastError()) + ")";
            return false;
        }
    }
    if (!SetPixelFormat(dc_, pixelFormat, &descriptor)) {
        threadLastError() = "SetPixelFormat failed (error " +
                            std::to_string(GetLastError()) + ")";
        return false;
    }

    const int attribs[] = {
        kWglContextMajorVersionArb, desc.glMajor,
        kWglContextMinorVersionArb, desc.glMinor,
        kWglContextProfileMaskArb,  kWglContextCoreProfileBitArb,
        kWglContextDebugBitArb,     1,  // report driver errors via glGetError
        0,
    };
    context_ = bootstrap.createContextAttribs(dc_, nullptr, attribs);
    if (context_ == nullptr) {
        threadLastError() =
            "Failed to create an OpenGL " + std::to_string(desc.glMajor) + "." +
            std::to_string(desc.glMinor) +
            " Core context (error " + std::to_string(GetLastError()) +
            "). A GPU driver with OpenGL 3.3 support is required.";
        return false;
    }
    if (!wglMakeCurrent(dc_, context_)) {
        wglDeleteContext(context_);
        context_ = nullptr;
        threadLastError() = "wglMakeCurrent (OpenGL 3.3 context) failed";
        return false;
    }

    wglSwapInterval_ = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(
        reinterpret_cast<void*>(wglGetProcAddress("wglSwapIntervalEXT")));

    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
    SetFocus(hwnd_);
    return true;
}

void Win32Window::destroyGlResources() {
    if (context_ != nullptr) {
        wglMakeCurrent(dc_, nullptr);
        wglDeleteContext(context_);
        context_ = nullptr;
    }
    if (dc_ != nullptr && hwnd_ != nullptr) {
        ReleaseDC(hwnd_, dc_);
        dc_ = nullptr;
    }
}

void Win32Window::onResize(int width, int height) {
    Event event{};
    event.type = Event::Type::Resize;
    event.x = width;
    event.y = height;
    events_.push_back(event);
}

void Win32Window::setVsync(bool enabled) {
    vsyncEnabled_ = enabled;
    if (wglSwapInterval_ != nullptr) {
        wglSwapInterval_(enabled ? 1 : 0);
    }
}

std::pair<int, int> Win32Window::clientSize() const {
    RECT rect{};
    if (hwnd_ != nullptr && GetClientRect(hwnd_, &rect)) {
        return {static_cast<int>(rect.right - rect.left),
                static_cast<int>(rect.bottom - rect.top)};
    }
    return {0, 0};
}

std::pair<int, int> Win32Window::cursorPosition() const {
    if (hwnd_ == nullptr) {
        return {0, 0};
    }
    POINT point{};
    GetCursorPos(&point);
    ScreenToClient(hwnd_, &point);
    return {point.x, point.y};
}

void Win32Window::requestClose() {
    if (hwnd_ != nullptr) {
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
}

bool Win32Window::pumpEvents() {
    events_.clear();
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            quitRequested_ = true;
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return !quitRequested_;
}

LRESULT CALLBACK Win32Window::wndProc(HWND hwnd, UINT message, WPARAM wParam,
                                      LPARAM lParam) {
    // The instance pointer is attached on WM_NCCREATE (and earlier messages
    // fall back to default handling).
    auto* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
        case WM_NCCREATE: {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
        case WM_PAINT: {
            ValidateRect(hwnd, nullptr);  // rendering happens in the main loop
            return 0;
        }
        case WM_SIZE: {
            if (window != nullptr) {
                window->onResize(lowordSigned(lParam),
                                 static_cast<int16_t>(static_cast<int16_t>(HIWORD(lParam))));
            }
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if (window != nullptr) {
                const uint32_t vk = static_cast<uint32_t>(wParam) & 0xFFu;
                window->keysDown_[vk] = true;
                Event event{};
                event.type = Event::Type::KeyDown;
                event.code = vk;
                event.x = (lParam & (1 << 30)) != 0 ? 1 : 0;  // auto-repeat flag
                window->events_.push_back(event);
            }
            return 0;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            if (window != nullptr) {
                const uint32_t vk = static_cast<uint32_t>(wParam) & 0xFFu;
                window->keysDown_[vk] = false;
                Event event{};
                event.type = Event::Type::KeyUp;
                event.code = vk;
                window->events_.push_back(event);
            }
            return 0;
        }
        case WM_CHAR: {
            if (window != nullptr) {
                Event event{};
                event.type = Event::Type::Char;
                event.code = static_cast<uint32_t>(wParam) & 0xFFFFu;
                window->events_.push_back(event);
            }
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK: {
            if (window != nullptr) {
                Event event{};
                const bool doubleClick = message == WM_LBUTTONDBLCLK ||
                                         message == WM_RBUTTONDBLCLK;
                event.type = doubleClick ? Event::Type::MouseDoubleClick
                                         : Event::Type::MouseDown;
                event.code = (message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK)
                                 ? 0u
                                 : (message == WM_RBUTTONDOWN || message == WM_RBUTTONDBLCLK
                                        ? 1u
                                        : 2u);
                event.x = lowordSigned(lParam);
                event.y = static_cast<int16_t>(static_cast<int16_t>(HIWORD(lParam)));
                window->events_.push_back(event);
                SetCapture(hwnd);  // keep receiving moves while dragging later
            }
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
            if (window != nullptr) {
                Event event{};
                event.type = Event::Type::MouseUp;
                event.code = message == WM_LBUTTONUP ? 0u
                             : (message == WM_RBUTTONUP ? 1u : 2u);
                event.x = lowordSigned(lParam);
                event.y = static_cast<int16_t>(static_cast<int16_t>(HIWORD(lParam)));
                window->events_.push_back(event);
                ReleaseCapture();
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (window != nullptr) {
                Event event{};
                event.type = Event::Type::MouseMove;
                event.x = lowordSigned(lParam);
                event.y = static_cast<int16_t>(static_cast<int16_t>(HIWORD(lParam)));
                window->events_.push_back(event);
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            if (window != nullptr) {
                Event event{};
                event.type = Event::Type::MouseWheel;
                event.code = static_cast<uint16_t>(static_cast<uint16_t>(HIWORD(wParam)));
                POINT point{lowordSigned(lParam),
                            static_cast<int16_t>(static_cast<int16_t>(HIWORD(lParam)))};
                ScreenToClient(hwnd, &point);
                event.x = point.x;
                event.y = point.y;
                window->events_.push_back(event);
            }
            return 0;
        }
        case WM_CLOSE: {
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY: {
            if (window != nullptr) {
                window->destroyGlResources();
            }
            PostQuitMessage(0);
            return 0;
        }
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

}  // namespace

std::unique_ptr<PlatformWindow> PlatformWindow::create(const WindowDesc& desc) {
    auto window = std::make_unique<Win32Window>();
    if (!window->initialize(desc)) {
        return nullptr;  // destructor releases any partially-created resources
    }
    return window;
}

void showErrorDialog(const std::string& title, const std::string& message) {
    MessageBoxW(nullptr, toWide(message).c_str(), toWide(title).c_str(),
                MB_OK | MB_ICONERROR);
}

const std::string& lastError() {
    return threadLastError();
}

double highResTimeSeconds() {
    static LARGE_INTEGER frequency = [] {
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        return f;
    }();
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) /
           static_cast<double>(frequency.QuadPart);
}

}  // namespace enigma::platform
