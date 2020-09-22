#ifndef JLT_COLLECTIONS_VECTOR_HPP
#define JLT_COLLECTIONS_VECTOR_HPP

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <initializer_list>
#include <utility>
#include <debug.hpp>
#include <util.hpp>
#include <memory/allocator.hpp>

namespace jolt {
    namespace collections {

        /**
         * A resizable collection where the data is stored in arrays.
         */
        template<typename T>
        class Vector {
          public:
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;
            using noclone_t = int;

            static_assert(std::is_move_constructible<value_type>::value);
            static_assert(std::is_copy_constructible<value_type>::value);

            static constexpr noclone_t noclone = 0;

          private:
            pointer m_data;          //< Pointer to the data.
            unsigned int m_length;   //< Length of the data.
            unsigned int m_capacity; //< Capacity of the array.

          protected:
            /**
             * Release any resources held by this vector.
             */
            void dispose() {
                if(m_data) {
                    clear();
                    jolt::memory::free(m_data);

                    m_data = nullptr;
                }
            }

          public:
            static constexpr unsigned int DEFAULT_CAPACITY = 16; //< The default capacity.

            /**
             * Create a new instance of this class.
             * This constructor doesn't copy the external data but gets ownership of it and will
             * free `data` after usage.
             *
             * @param data Pointer to the data to use.
             * @param length Length (as number of items) of `data`.
             * @param noclone The `noclone` constant.
             */
            Vector(pointer data, unsigned int const length, noclone_t) :
              m_data{data}, m_length{length}, m_capacity{length} {
                jltassert(data);
            }

            /**
             * Create a new instance of this class.
             *
             * @param lst The initializer list of items containing the initial state.
             */
            Vector(std::initializer_list<value_type> lst) : Vector(lst.begin(), lst.size()) {}

            /**
             * Create a new empty instance of this class.
             *
             * @param capacity The number of items the vector will be able to hold before resizing
             * its internal array.
             */
            explicit Vector(unsigned int const initial_capacity = DEFAULT_CAPACITY) :
              m_data{jolt::memory::allocate<value_type>(initial_capacity)}, m_length{0},
              m_capacity{initial_capacity} {}

            /**
             * Create a new instance of this class.
             *
             * @param data Pointer to the data to use.
             * @param length Length (as number of items) of `data`.
             */
            Vector(const_pointer const data, unsigned int const length) :
              m_data{jolt::memory::allocate<value_type>(max(length, DEFAULT_CAPACITY))},
              m_length{length}, m_capacity{max(length, DEFAULT_CAPACITY)} {
                jltassert(data);

                if constexpr(std::is_trivial<value_type>::value) {
                    memcpy(m_data, data, length * sizeof(value_type));
                } else {
                    for(unsigned int i = 0; i < length; ++i) {
                        new(m_data + i) value_type(*(data + i));
                    }
                }
            }

            Vector(const Vector<value_type> &other) : Vector(other.m_data, other.m_length) {}

            Vector(Vector<value_type> &&other) :
              m_data{other.m_data}, m_length{other.m_length}, m_capacity{other.m_capacity} {
                other.m_data = nullptr;
            }

            ~Vector() { dispose(); }

            /**
             * Return the length of the vector.
             */
            unsigned int get_length() const { return m_length; }

            /**
             * Return the capacity of the vector.
             */
            unsigned int get_capacity() const { return m_capacity; }

            Vector<value_type> &operator=(const Vector<value_type> &other) {
                if(m_capacity >= other.m_capacity) {
                    clear();
                } else {
                    dispose();

                    m_data = jolt::memory::allocate<value_type>(other.m_capacity);
                    m_capacity = other.m_capacity;
                }

                m_length = other.m_length;

                if constexpr(std::is_trivially_copyable<value_type>::value) {
                    memcpy(m_data, other.m_data, sizeof(value_type) * m_length);
                } else {
                    pointer const other_data_end = other.m_data + other.m_length;
                    pointer my_data = m_data;

                    for(pointer other_data = other.m_data; other_data < other_data_end;
                        ++other_data) {
                        new(my_data++)(*other_data);
                    }
                }

                return *this;
            }

            Vector<value_type> &operator=(Vector<value_type> &&other) {
                dispose();

                m_data = other.m_data;
                m_length = other.m_length;
                m_capacity = other.m_capacity;

                other.m_data = nullptr;

                return *this;
            }

            reference operator[](unsigned int const i) {
                jltassert(i < m_length);

                return m_data[i];
            }

            Vector<value_type> operator+(const Vector<value_type> &other) const {
                unsigned int const new_length = m_length + other.m_length;
                pointer const new_vec = jolt::memory::allocate<value_type>(new_length);

                if constexpr(std::is_trivial<value_type>::value) {
                    memcpy(new_vec, m_data, m_length * sizeof(value_type));
                    memcpy(new_vec + m_length, other.m_data, other.m_length * sizeof(value_type));
                } else {
                    pointer nvp = new_vec;
                    pointer const this_end = m_data + m_length;
                    pointer const other_end = other.m_data + other.m_length;

                    for(pointer p = m_data; p < this_end; ++p) { new(nvp++) value_type(*p); }
                    for(pointer p = other.m_length; p < other_end; ++p) {
                        new(nvp++) value_type(*p);
                    }
                }

                return Vector<value_type>{new_vec, new_length, noclone};
            }

            Vector<value_type> &operator+=(const Vector<value_type> &&other) {
                add_all(other.m_data, other.m_length, m_length);

                return *this;
            }

            Vector<value_type> &operator+=(std::initializer_list<value_type> lst) {
                return *this += Vector(lst.begin(), lst.size());
            }

            /**
             * Ensure there is enough capacity to satisfy the requirement.
             *
             * @param new_capacity The capacity requirement.
             */
            void reserve_capacity(unsigned int const new_capacity) {
                if(new_capacity > m_capacity) {
                    m_capacity = new_capacity;

                    if constexpr(std::is_trivial<value_type>::value) {
                        m_data = jolt::memory::reallocate(m_data, m_capacity);
                    } else {
                        if(jolt::memory::will_relocate(m_data, m_capacity)) {
                            pointer const data_new = jolt::memory::allocate<value_type>(m_capacity);

                            for(unsigned int i = 0; i < m_length; ++i) {
                                new(data_new + i) value_type(std::move(m_data[i]));
                                m_data[i].~value_type();
                            }

                            jolt::memory::free(m_data);
                            m_data = data_new;
                        }
                    }
                }
            }

          private:
            /**
             * Ensure the vector has the capacity to hold additional `n` items.
             *
             * @param n The number of extra items to ensure capacity for.
             */
            void ensure_capacity(unsigned int const n) {
                if(m_capacity < m_length + n) {
                    reserve_capacity((m_capacity + n) * 1.3 + DEFAULT_CAPACITY);
                }
            }

          public:
            /**
             * Add an item.
             *
             * @param item The item to add.
             * @param position The index at which to add the new item.
             */
            void add(const_reference item, unsigned int const position) {
                add_all(&item, 1, position);
            }

            /**
             * Add many items.
             *
             * @param items Pointer to the array of items to add.
             * @param length The number of items to add.
             * @param position The index at which to add the items.
             */
            void add_all(
              const_pointer const items, unsigned int const length, unsigned int const position) {
                ensure_capacity(length);

                if constexpr(std::is_trivial<value_type>::value) {
                    memmove(
                      m_data + position + length,
                      m_data + position,
                      (m_length - position) * sizeof(value_type));
                    memcpy(m_data + position, items, length * sizeof(value_type));
                } else {
                    for(long long i = m_length - 1; i >= position; --i) {
                        pointer const cur = m_data + i;

                        new(cur + length) value_type(std::move(*cur));
                    }

                    pointer const base_pos = m_data + position;

                    for(unsigned int i = 0; i < length; ++i) {
                        new(base_pos + i) value_type(*(items + i));
                    }
                }

                m_length += length;
            }

            /**
             * Add an item at the end of the vector.
             *
             * @param item The item to add.
             */
            void push(const_reference item) { add(item, m_length); }

            /**
             * Remove and return an item from the end of the vector.
             */
            reference pop() {
                jltassert(m_length);
                return *(m_data + (--m_length));
            }

            /**
             * Find an item in the vector.
             *
             * @param item The item to find.
             *
             * @return The index of the item in the vector or `-1` if not found.
             */
            int find(const_reference item) {
                for(int i = 0; i < m_length; ++i) {
                    if(m_data[i] == item) {
                        return i;
                    }
                }

                return -1;
            }

            /**
             * Remove an item from the vector.
             *
             * @param item The item to remove.
             */
            void remove(const_reference item) {
                int const i = find(item);

                if(i >= 0) {
                    remove_at(i);
                }
            }

            /**
             * Remove an item from the vector given its position.
             *
             * @param i The index of the item to remove.
             */
            void remove_at(unsigned int i) {
                jltassert(i >= 0 && i < m_length);

                if constexpr(!std::is_trivially_destructible<value_type>::value) {
                    m_data[i].~value_type();
                }

                for(; i < m_length - 1; ++i) { m_data[i] = std::move(m_data[i + 1]); }

                --m_length;
            }

            /**
             * Remove all the items from the vector.
             */
            void clear() {
                if constexpr(!std::is_trivially_destructible<value_type>::value) {
                    const_pointer ptr_end = m_data + m_length;

                    for(pointer ptr = m_data; ptr < ptr_end; ++ptr) { ptr->~value_type(); }
                }

                m_length = 0;
            }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_VECTOR_HPP */
