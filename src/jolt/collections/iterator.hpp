#ifndef JLT_COLLECTIONS_ITERATOR_HPP
#define JLT_COLLECTIONS_ITERATOR_HPP

#include <type_traits>
#include <jolt/util.hpp>

namespace jolt {
    namespace collections {
        /**
         * Generic iterator.
         *
         * @tparam T The type of element the iterator will use internally.
         * @tparam F The custom functional type of the iterator.
         *
         * Each iterator needs a type T which defines the type of the internal state.
         * The type F must contain one of more static functions to provide custom iteration
         * behaviour plus an `item_type` type alias that represents the type of the actual item the
         * iterator is meant to return when dereferenced.
         *
         * The static functions that can be defined within F are the following:
         * - next: Move the iterator forward
         * - previous: Move the iterator backwards
         * - compare: Compare two iterators
         * - resolve: Return the item the iterator points at
         *
         * See `ArrayIteratorImpl` for a sample implementation.
         */
        template<typename T, typename F>
        struct Iterator {
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;

          private:
            template<typename A, typename B>
            friend struct Iterator;

            pointer m_ptr;

          public:
            Iterator(pointer ptr) : m_ptr{ptr} {}

            template<typename A, typename B>
            Iterator(const Iterator<A, B> &other) : m_ptr{other.m_ptr} {}

            template<typename A, typename B>
            Iterator(Iterator<A, B> &&other) : m_ptr{other.m_ptr} {}

            Iterator &operator++() {
                next();

                return *this;
            }

            JLT_NODISCARD Iterator operator++(int) {
                Iterator other = *this;
                next();

                return other;
            }

            Iterator &operator--() {
                previous();

                return *this;
            }

            JLT_NODISCARD Iterator operator--(int) {
                Iterator other = *this;

                previous();

                return other;
            }

            JLT_NODISCARD constexpr Iterator operator+(size_t n) const {
                return Iterator{F::forward(m_ptr, n)};
            }
            JLT_NODISCARD constexpr Iterator operator-(size_t n) const {
                return Iterator{F::backward(m_ptr, n)};
            }

            Iterator &operator+=(size_t n) {
                m_ptr = F::forward(m_ptr, n);
                return *this;
            }

            Iterator &operator-=(size_t n) {
                m_ptr = F::backward(m_ptr, n);
                return *this;
            }

            template<typename It>
            JLT_NODISCARD constexpr bool operator==(const It &other) const {
                return m_ptr == other.m_ptr;
            }

            template<typename It>
            JLT_NODISCARD constexpr bool operator!=(const It &other) const {
                return m_ptr != other.m_ptr;
            }

            template<typename It>
            JLT_NODISCARD constexpr size_t operator-(const It &other) const {
                return F::count_elements(this->m_ptr, other.m_ptr);
            }

            JLT_NODISCARD constexpr auto &operator*() { return F::resolve(m_ptr); }
            JLT_NODISCARD constexpr const auto &operator*() const { return F::resolve(m_ptr); }

            constexpr void next() { m_ptr = F::forward(m_ptr, 1); }
            constexpr void previous() { m_ptr = F::backward(m_ptr, 1); }
            JLT_NODISCARD constexpr pointer get_pointer() const { return m_ptr; }
        };

        template<typename T, typename E = T>
        struct ArrayIteratorImpl {
            using item_type = E;

            static constexpr T *forward(T *const cur, size_t const n) { return cur + n; }
            static constexpr T *backward(T *const cur, size_t const n) { return cur - n; }
            JLT_NODISCARD static constexpr int compare(T *const left, const T *const right) {
                return choose(1, choose(-1, 0, left < right), left > right);
            }
            JLT_NODISCARD static constexpr item_type &resolve(T *const cur) { return *cur; }

            /**
             * Count the number of items between two iterators.
             *
             * @param from The iterator from which to start the count. Must be positioned before `to`.
             * @param to The iterator where to stop the count. Must be positioned after `from`.
             *
             * @return The number of elements between `from` and `to`.
             */
            JLT_NODISCARD static constexpr size_t count_elements(T const *const from, T const *const to) {
                return to - from;
            }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_ITERATOR_HPP */
