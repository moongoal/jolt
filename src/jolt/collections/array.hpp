#ifndef JLT_COLLECTIONS_ARRAY_HPP
#define JLT_COLLECTIONS_ARRAY_HPP

#include <jolt/memory/allocator.hpp>
#include <jolt/collections/iterator.hpp>

namespace jolt {
    namespace collections {
        template<typename T>
        class Array {
          public:
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;

            template<typename E>
            using base_iterator = Iterator<E, ArrayIteratorImpl<E>>;
            using iterator = base_iterator<T>;
            using const_iterator = base_iterator<const T>;

          private:
            pointer const m_ptr;   //< Pointer to the data.
            size_t const m_length; //< Length of the array.

          public:
            explicit constexpr Array(size_t const length) :
              m_ptr{memory::allocate<value_type>(length)}, m_length{length} {}

            constexpr Array(pointer const ptr, size_t const length) :
              m_ptr{ptr}, m_length{length} {}

            ~Array() { memory::free(m_ptr); }

            constexpr reference operator[](size_t const i) {
                jltassert(i < m_length);

                return m_ptr[i];
            }

            constexpr const_reference operator[](size_t const i) const {
                jltassert(i < m_length);

                return m_ptr[i];
            }

            operator pointer() { return m_ptr; }
            operator const_pointer() const { return m_ptr; }

            constexpr size_t get_length() const { return m_length; }

            constexpr iterator begin() { return iterator{m_ptr}; }
            constexpr iterator end() { return iterator{m_ptr + m_length}; }

            constexpr const_iterator begin() const { return const_iterator{m_ptr}; }
            constexpr const_iterator end() const { return const_iterator{m_ptr + m_length}; }

            constexpr const_iterator cbegin() const { return const_iterator{m_ptr}; }
            constexpr const_iterator cend() const { return const_iterator{m_ptr + m_length}; }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_ARRAY_HPP */
