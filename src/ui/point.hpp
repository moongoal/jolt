#ifndef JLT_UI_POINT_HPP
#define JLT_UI_POINT_HPP

namespace jolt {
    namespace ui {
        struct Point {
            int m_x, m_y;

            constexpr Point(int x, int y) : m_x{x}, m_y{y} {}
            constexpr Point() : Point{0, 0} {}

            constexpr Point operator+(const Point &other) const {
                return Point{m_x + other.m_x, m_y + other.m_y};
            }

            constexpr Point operator-(const Point &other) const {
                return Point{m_x - other.m_x, m_y - other.m_y};
            }

            constexpr bool operator==(const Point &other) const {
                return m_x == other.m_x && m_y == other.m_y;
            }

            constexpr bool operator!=(const Point &other) const {
                return m_x != other.m_x || m_y != other.m_y;
            }

            constexpr Point &operator+=(const Point &other) {
                m_x += other.m_x;
                m_y += other.m_y;

                return *this;
            }

            constexpr Point &operator-=(const Point &other) {
                m_x -= other.m_x;
                m_y -= other.m_y;

                return *this;
            }
        };
    } // namespace ui
} // namespace jolt

#endif /* JLT_UI_POINT_HPP */
