#ifndef JLT_COLLECTIONS_ITERATOR_HPP
#define JLT_COLLECTIONS_ITERATOR_HPP

#include <util.hpp>

namespace jolt {
    namespace collections {
        template<typename T, typename F>
        struct Iterator {
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;

          private:
            pointer m_ptr;

          public:
            Iterator(pointer ptr) : m_ptr{ptr} {}

            reference operator++() {
                pointer ptr = m_ptr;
                next();

                return *ptr;
            }

            reference operator++(int) {
                next();

                return *m_ptr;
            }

            constexpr Iterator operator+(size_t n) const { return Iterator{F::forward(m_ptr, n)}; }
            constexpr Iterator operator-(size_t n) const { return Iterator{F::backward(m_ptr, n)}; }

            Iterator &operator+=(size_t n) {
                m_ptr = F::forward(m_ptr, n);
                return *this;
            }

            Iterator &operator-=(size_t n) {
                m_ptr = F::backward(m_ptr, n);
                return *this;
            }

            template<typename It>
            constexpr bool operator==(const It other) const {
                return F::compare(m_ptr, other.get_pointer()) == 0;
            }

            template<typename It>
            constexpr bool operator!=(const It other) const {
                return F::compare(m_ptr, other.get_pointer()) != 0;
            }

            template<typename It>
            constexpr bool operator>(const It other) const {
                return F::compare(m_ptr, other.get_pointer()) > 0;
            }

            template<typename It>
            constexpr bool operator<(const It other) const {
                return F::compare(m_ptr, other.get_pointer()) < 0;
            }

            template<typename It>
            constexpr bool operator>=(const It other) const {
                return F::compare(m_ptr, other.get_pointer()) >= 0;
            }

            template<typename It>
            constexpr bool operator<=(const It other) const {
                return F::compare(m_ptr, other.get_pointer()) <= 0;
            }

            constexpr T &operator*() { return *m_ptr; }

            constexpr void next() { m_ptr = F::forward(m_ptr, 1); }
            constexpr void previous() { m_ptr = F::backward(m_ptr, 1); }
            constexpr pointer get_pointer() const { return m_ptr; }
        };

        template<typename T>
        struct ArrayIteratorImpl {
            static constexpr T *forward(T *const cur, size_t const n) { return cur + n; }
            static constexpr T *backward(T *const cur, size_t const n) { return cur - n; }
            static constexpr int compare(T *const left, const T *const right) {
                return choose(1, choose(-1, 0, left < right), left > right);
            }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_ITERATOR_HPP */
