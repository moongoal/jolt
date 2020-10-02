#include <jolt/memory/allocator.hpp>
#include <jolt/collections/hashmap.hpp>
#include <jolt/debug.hpp>
#include "window.hpp"

using namespace jolt;
using namespace jolt::debug;
using namespace jolt::text;
using namespace jolt::collections;

using window_map = HashMap<HWND, jolt::ui::Window *, jolt::hash::Identity<HWND>>;

static constexpr const char MAIN_CLASS_NAME[] = "JoltMainWindow";

static HINSTANCE g_hinstance = GetModuleHandleA(NULL);
static ATOM g_window_class = NULL;
static window_map *g_windows = nullptr;

static void register_window_class() {
    WNDCLASSEXA cls = {
      sizeof(WNDCLASSEXA),          // cbSize
      CS_GLOBALCLASS | CS_OWNDC,    // style
      jolt::ui::Window::WindowProc, // lpfnWndProc
      0,                            // cbClsExtra
      0,                            // cbWndExtra
      g_hinstance,                  // hInstance
      NULL,                         // hIcon
      NULL,                         // hCursor
      NULL,                         // hbrBackground
      NULL,                         // lpszMenuName
      MAIN_CLASS_NAME,              // lpszClassName
      NULL                          // hIconSm
    };

    g_window_class = RegisterClassExA(&cls);

    jltassert(g_window_class);
}

static void unregister_window_class() { UnregisterClassA(MAIN_CLASS_NAME, g_hinstance); }

namespace jolt {
    namespace ui {
        HINSTANCE get_hinstance() { return g_hinstance; }

        Window::Window(const text::String &name, const Rect &dimensions, const Point &location) :
          m_name{name}, m_handle{NULL}, m_size{dimensions}, m_location{location} {
            create();
        }

        void Window::show(bool const visible) {
            console.info("Showing window " + m_name);

            if(visible) {
                ShowWindow(m_handle, SW_SHOW);
            } else {
                CloseWindow(m_handle);
            }
        }

        Window::~Window() {
            if(m_handle) {
                close();
            }
        }

        void Window::close() {
            console.info("Destroying window \"" + m_name + "\"");

            DestroyWindow(m_handle);
        }

        void Window::create() {
            console.info("Creating window \"" + m_name + "\"");

            m_handle = CreateWindowExA(
              WS_EX_APPWINDOW,
              MAIN_CLASS_NAME,
              reinterpret_cast<const char *>(
                m_name.get_raw()), // TODO: Allow non-ASCII characters in the title
              WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
              m_location.m_x,
              m_location.m_y,
              static_cast<int>(m_size.m_w),
              static_cast<int>(m_size.m_h),
              NULL,
              NULL,
              g_hinstance,
              this);

            jltassert(m_handle);

            console.info("Created window \"" + m_name + "\"");
        }

        LRESULT CALLBACK Window::WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
#define WND_DEF() DefWindowProcA(wnd, msg, wparam, lparam)

            switch(msg) {
            case WM_CREATE: {
                auto screate = reinterpret_cast<LPCREATESTRUCTA>(lparam);

                g_windows->add(wnd, reinterpret_cast<jolt::ui::Window *>(screate->lpCreateParams));
            }
                return 0;

            case WM_MOVE: {
                jolt::ui::Window *w = *g_windows->get_value(wnd);

                w->m_location.m_x = static_cast<int>(LOWORD(lparam));
                w->m_location.m_y = static_cast<int>(HIWORD(lparam));
            }
                return 0;

            case WM_SIZE: {
                jolt::ui::Window *w = *g_windows->get_value(wnd);

                w->m_size.m_w = static_cast<int>(LOWORD(lparam));
                w->m_size.m_h = static_cast<int>(HIWORD(lparam));
            }
                return 0;

            case WM_DESTROY: {
                jolt::ui::Window *w = *g_windows->get_value(wnd);

                w->m_handle = NULL;
                g_windows->remove(wnd);

                if(g_windows->get_length() == 0) {
                    PostQuitMessage(0);
                }
            }
                return 0;

            default: return WND_DEF();
            }

            jltassert(false);

#undef WND_DEF
        }

        bool Window::cycle() {
            MSG msg;

            for(size_t n_msg = 16; PeekMessageA(&msg, NULL, 0, 0, FALSE) && n_msg > 0; --n_msg) {
                BOOL const res = GetMessageA(&msg, NULL, 0, 0);

                if(!res) {
                    return false;
                }

                jltassert(res != -1);

                DispatchMessageA(&msg);
            }

            return true;
        }

        void initialize() {
            g_windows = jolt::memory::allocate<window_map>(1, jolt::memory::ALLOC_PERSIST);
            jolt::memory::construct(g_windows, 2);

            register_window_class();
        }

        void shutdown() {
            unregister_window_class();

            jolt::memory::free(g_windows);
        }
    } // namespace ui
} // namespace jolt
