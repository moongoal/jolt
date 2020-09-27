#ifndef JLT_UI_RECT_HPP
#define JLT_UI_RECT_HPP

namespace jolt {
    namespace ui {
        struct Rect {
            unsigned int m_w, m_h;

            constexpr Rect(unsigned int w, unsigned int h) : m_w{w}, m_h{h} {}
            constexpr Rect() : Rect{0, 0} {}

            constexpr bool operator==(const Rect &other) const {
                return m_w == other.m_w && m_h == other.m_h;
            }

            constexpr bool operator!=(const Rect &other) const {
                return m_w != other.m_w || m_h != other.m_h;
            }
        };
    } // namespace ui
} // namespace jolt

#endif /* JLT_UI_RECT_HPP */
