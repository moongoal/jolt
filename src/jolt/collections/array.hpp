#ifndef JLT_COLLECTIONS_ARRAY_HPP
#define JLT_COLLECTIONS_ARRAY_HPP

#include <type_traits>
#include <initializer_list>
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

        template<typename T, size_t N>
        class StaticArray {
          public:
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;
            using array_type = value_type[N];
            using array_reference = array_type &;
            using const_array_reference = array_type const &;

            template<typename E>
            using base_iterator = Iterator<E, ArrayIteratorImpl<E>>;
            using iterator = base_iterator<T>;
            using const_iterator = base_iterator<const T>;

          private:
            value_type m_data[N]; //< Pointer to the data.

          public:
            constexpr StaticArray() = default;

            constexpr StaticArray(std::initializer_list<value_type> lst) {
                auto it = lst.begin();
                auto const it_end = lst.end();
                size_t n_elements = it_end - it;

                for(size_t i = 0; i < N; ++i) { memory::construct(&m_data[i], *(it++)); }
            }

            constexpr reference operator[](size_t const i) {
                jltassert(i < N);

                return m_data[i];
            }

            constexpr const_reference operator[](size_t const i) const {
                jltassert(i < N);

                return m_data[i];
            }

            constexpr operator array_reference() { return m_data; }
            constexpr operator const_array_reference() const { return m_data; }

            constexpr size_t get_length() const { return N; }

            constexpr iterator begin() { return iterator{m_data}; }
            constexpr iterator end() { return iterator{m_data + N}; }

            constexpr const_iterator begin() const { return const_iterator{m_data}; }
            constexpr const_iterator end() const { return const_iterator{m_data + N}; }

            constexpr const_iterator cbegin() const { return const_iterator{m_data}; }
            constexpr const_iterator cend() const { return const_iterator{m_data + N}; }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_ARRAY_HPP */
