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
            pointer const m_ptr; //< Pointer to the data.

          public:
            JLT_NODISCARD explicit constexpr Array(size_t const length) :
              m_ptr{memory::allocate_array<value_type>(length)} {}

            JLT_NODISCARD constexpr Array(pointer const ptr) : m_ptr{ptr} {}

            Array(Array const &) = delete;
            Array(Array &&other) : m_ptr{other.m_ptr} { other.m_ptr = nullptr; }

            ~Array() {
                if(m_ptr) {
                    memory::free_array(m_ptr);
                }
            }

            JLT_NODISCARD constexpr reference operator[](size_t const i) {
                jltassert(i < get_length());

                return m_ptr[i];
            }

            JLT_NODISCARD constexpr const_reference operator[](size_t const i) const {
                jltassert(i < get_length());

                return m_ptr[i];
            }

            JLT_NODISCARD operator pointer() { return m_ptr; }
            JLT_NODISCARD operator const_pointer() const { return m_ptr; }

            JLT_NODISCARD constexpr size_t get_length() const { return memory::get_array_length(m_ptr); }

            JLT_NODISCARD constexpr iterator begin() { return iterator{m_ptr}; }
            JLT_NODISCARD constexpr iterator end() { return iterator{m_ptr + get_length()}; }

            JLT_NODISCARD constexpr const_iterator begin() const { return const_iterator{m_ptr}; }
            JLT_NODISCARD constexpr const_iterator end() const {
                return const_iterator{m_ptr + get_length()};
            }

            JLT_NODISCARD constexpr const_iterator cbegin() const { return const_iterator{m_ptr}; }
            JLT_NODISCARD constexpr const_iterator cend() const {
                return const_iterator{m_ptr + get_length()};
            }

            /**
             * Fill all the items with the given value.
             *
             * @param value The filler value.
             */
            void fill(const_reference value) {
                for(auto &v : *this) { v = value; }
            }
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
            JLT_NODISCARD constexpr StaticArray() = default;

            JLT_NODISCARD constexpr StaticArray(std::initializer_list<value_type> lst) {
                auto it = lst.begin();
                auto const it_end = lst.end();
                size_t const n_elements = it_end - it;

                jltassert(n_elements == N);

                for(size_t i = 0; i < N; ++i) { memory::construct(&m_data[i], *(it++)); }
            }

            JLT_NODISCARD constexpr reference operator[](size_t const i) {
                jltassert(i < N);

                return m_data[i];
            }

            JLT_NODISCARD constexpr const_reference operator[](size_t const i) const {
                jltassert(i < N);

                return m_data[i];
            }

            JLT_NODISCARD constexpr operator array_reference() { return m_data; }
            JLT_NODISCARD constexpr operator const_array_reference() const { return m_data; }

            JLT_NODISCARD constexpr size_t get_length() const { return N; }

            JLT_NODISCARD constexpr iterator begin() { return iterator{m_data}; }
            JLT_NODISCARD constexpr iterator end() { return iterator{m_data + N}; }

            JLT_NODISCARD constexpr const_iterator begin() const { return const_iterator{m_data}; }
            JLT_NODISCARD constexpr const_iterator end() const { return const_iterator{m_data + N}; }

            JLT_NODISCARD constexpr const_iterator cbegin() const { return const_iterator{m_data}; }
            JLT_NODISCARD constexpr const_iterator cend() const { return const_iterator{m_data + N}; }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_ARRAY_HPP */
