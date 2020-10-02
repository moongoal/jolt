#ifndef JLT_UI_WINDOW_HPP
#define JLT_UI_WINDOW_HPP

#ifdef _WIN32
    #include <Windows.h>
#else
    #error Unsupported OS.
#endif // _WIN32

#include <jolt/text/string.hpp>
#include "point.hpp"
#include "rect.hpp"

namespace jolt {
    namespace ui {
        class JLTAPI Window {
            text::String m_name; //< The window name and title.
            HWND m_handle;       //< The window handle.
            Rect m_size;         //< Window size.
            Point m_location;    //< Window location

            /**
             * Create the window.
             */
            void create();

          public:
            static constexpr const Rect DEFAULT_SIZE{1200, 900};
            static constexpr const Point DEFAULT_LOCATION{100, 100};

            /**
             * Create a new window.
             *
             * @param name The window name that will be displayed on the title bar.
             * @param dimension The size of the window.
             * @param location The location of the window.
             */
            explicit Window(
              const text::String &name,
              const Rect &dimension = DEFAULT_SIZE,
              const Point &location = DEFAULT_LOCATION);

            Window(const Window &other) = delete;
            ~Window();

            Window &operator=(const Window &other) = delete;

            /**
             * Return the window handle.
             */
            HWND get_handle() const { return m_handle; }

            /**
             * Show the window.
             *
             * @param visible True if the window must be shown, false if the window must be hidden.
             */
            void show(bool const visible = true);

            /**
             * Close and destroy the window.
             */
            void close();

            static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

            /**
             * Perform a single cycle of the window system loop.
             *
             * @return True if the function can be called again, false if the loop must terminate.
             */
            static bool cycle();
        };

        /**
         * Initialize the windowing subsystem.
         */
        void JLTAPI initialize();

        /**
         * Shutdown the windowing subsystem.
         */
        void JLTAPI shutdown();

        /**
         * Return the Windows instance handle.
         */
        HINSTANCE JLTAPI get_hinstance();

    } // namespace ui
} // namespace jolt

#endif /* JLT_UI_WINDOW_HPP */
