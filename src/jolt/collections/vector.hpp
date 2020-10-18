#ifndef JLT_COLLECTIONS_VECTOR_HPP
#define JLT_COLLECTIONS_VECTOR_HPP

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <initializer_list>
#include <utility>
#include <jolt/api.hpp>
#include <jolt/debug.hpp>
#include <jolt/util.hpp>
#include <jolt/memory/allocator.hpp>
#include "iterator.hpp"

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

            struct noclone_t {};

            template<typename E>
            using base_iterator = Iterator<E, ArrayIteratorImpl<E>>;
            using iterator = base_iterator<T>;
            using const_iterator = base_iterator<const T>;

            static constexpr noclone_t noclone{};

          private:
            pointer m_data;                //< Pointer to the data.
            unsigned int m_length;         //< Length of the data.
            unsigned int m_capacity;       //< Capacity of the array.
            memory::flags_t m_alloc_flags; //< Allocation flags.

          protected:
            /**
             * Release any resources held by this vector.
             */
            void dispose() {
                if(m_data) {
                    memory::free(m_data, m_length);

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
            JLT_NODISCARD Vector(pointer data, unsigned int const length, noclone_t) :
              m_data{data}, m_length{length}, m_capacity{length}, m_alloc_flags{
                                                                    memory::get_alloc_flags(data)} {
                jltassert(data);
            }

            /**
             * Create a new instance of this class.
             *
             * @param lst The initializer list of items containing the initial state.
             */
            JLT_NODISCARD Vector(const std::initializer_list<value_type> &lst) :
              Vector(lst.begin(), lst.size()) {}

            /**
             * Create a new empty instance of this class.
             *
             * @param capacity The number of items the vector will be able to hold before resizing
             * its internal array.
             */
            JLT_NODISCARD explicit Vector(unsigned int const initial_capacity = DEFAULT_CAPACITY) :
              m_data{memory::allocate<value_type>(initial_capacity)}, m_length{0},
              m_capacity{initial_capacity}, m_alloc_flags{memory::get_current_force_flags()} {}

            /**
             * Create a new instance of this class.
             *
             * @param data Pointer to the data to use.
             * @param length Length (as number of items) of `data`.
             */
            JLT_NODISCARD Vector(const_pointer const data, unsigned int const length) :
              m_data{memory::allocate<value_type>(max(length, DEFAULT_CAPACITY))}, m_length{length},
              m_capacity{max(length, DEFAULT_CAPACITY)}, m_alloc_flags{memory::get_current_force_flags()} {
                jltassert(data);

                if constexpr(std::is_trivial<value_type>::value) {
                    memcpy(m_data, data, length * sizeof(value_type));
                } else {
                    for(unsigned int i = 0; i < length; ++i) { memory::construct(m_data + i, *(data + i)); }
                }
            }

            template<typename It>
            JLT_NODISCARD Vector(It const begin, It const end) :
              m_data{memory::allocate<value_type>(end - begin)}, m_length{end - begin},
              m_capacity{end - begin}, m_alloc_flags{memory::get_current_force_flags()} {
                add_all(begin, end);
            }

            JLT_NODISCARD Vector(const Vector<value_type> &other) : Vector(other.m_data, other.m_length) {}

            JLT_NODISCARD Vector(Vector<value_type> &&other) :
              m_data{other.m_data}, m_length{other.m_length}, m_capacity{other.m_capacity} {
                other.m_data = nullptr;
            }

            ~Vector() { dispose(); }

            /**
             * Return the length of the vector.
             */
            JLT_NODISCARD unsigned int get_length() const { return m_length; }

            void set_length(unsigned int length) {
                ensure_capacity(length);

                if constexpr(!std::is_trivial<value_type>::value) {
                    if(length < m_length) {
                        const_iterator end = cend();

                        for(iterator it = begin() + m_length; it != end; ++it) { (*it).~value_type(); }
                    }
                }

                m_length = length;
            }
            /**
             * Return the capacity of the vector.
             */
            JLT_NODISCARD unsigned int get_capacity() const { return m_capacity; }

            Vector<value_type> &operator=(const Vector<value_type> &other) {
                memory::push_force_flags(m_alloc_flags);

                if(m_capacity >= other.m_capacity) {
                    clear();
                } else {
                    dispose();

                    m_data = memory::allocate<value_type>(other.m_capacity);
                    m_capacity = other.m_capacity;
                }

                m_length = other.m_length;
                m_alloc_flags = other.m_alloc_flags;

                if constexpr(std::is_trivial<value_type>::value) {
                    memcpy(m_data, other.m_data, sizeof(value_type) * m_length);
                } else {
                    const_iterator const other_data_end = other.cend();
                    iterator my_data = begin();

                    for(const_iterator other_data = other.begin(); other_data != other_data_end;
                        ++other_data) {
                        memory::construct((my_data++).get_pointer(), *other_data);
                    }
                }

                memory::pop_force_flags();
                return *this;
            }

            Vector<value_type> &operator=(Vector<value_type> &&other) {
                dispose();

                m_data = other.m_data;
                m_length = other.m_length;
                m_capacity = other.m_capacity;
                m_alloc_flags = other.m_alloc_flags;

                other.m_data = nullptr;

                return *this;
            }

            JLT_NODISCARD reference operator[](unsigned int const i) {
                jltassert(i < m_length);

                return m_data[i];
            }

            JLT_NODISCARD Vector<value_type> operator+(const Vector<value_type> &other) const {
                memory::push_force_flags(m_alloc_flags);

                unsigned int const new_length = m_length + other.m_length;
                pointer const new_vec = memory::allocate<value_type>(new_length);

                if constexpr(std::is_trivial<value_type>::value) {
                    memcpy(new_vec, m_data, m_length * sizeof(value_type));
                    memcpy(new_vec + m_length, other.m_data, other.m_length * sizeof(value_type));
                } else {
                    pointer nvp = new_vec;
                    const_iterator const this_end = cend();
                    const_iterator const other_end = other.cend();

                    for(iterator p = begin(); p != this_end; ++p) {
                        memory::construct(nvp++, value_type(*p));
                    }

                    for(iterator p = other.begin(); p != other_end; ++p) {
                        memory::construct(nvp++, value_type(*p));
                    }
                }

                memory::pop_force_flags();

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
             * Reserve capacity some capacity.
             *
             * @param new_capacity The minimum capacity to reserve.
             */
            void reserve_capacity(unsigned int const new_capacity) {
                if(new_capacity > m_capacity) {
                    unsigned int const old_capacity = m_capacity;
                    m_capacity = new_capacity;

                    m_data = memory::reallocate(m_data, old_capacity, m_capacity);
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
            void add(const_reference item, unsigned int const position) { add_all(&item, 1, position); }

            /**
             * Add many items.
             *
             * @param begin Iterator to the beginning of the sequence.
             * @param end Iterator to the end of the sequence.
             * @param position The index at which to add the items.
             */
            template<typename It>
            void add_all(It const begin, It const end, unsigned int const position) {
                add_all(begin.get_pointer(), end - begin, position);
            }

            /**
             * Add many items.
             *
             * @param items Pointer to the array of items to add.
             * @param length The number of items to add.
             * @param position The index at which to add the items.
             */
            void add_all(const_pointer const items, unsigned int const length, unsigned int const position) {
                ensure_capacity(length);

                if constexpr(std::is_trivial<value_type>::value) {
                    memmove(
                      m_data + position + length,
                      m_data + position,
                      (m_length - position) * sizeof(value_type));
                    memcpy(m_data + position, items, length * sizeof(value_type));
                } else {
                    for(long long i = static_cast<long long>(m_length) - 1; i >= position; --i) {
                        pointer const cur = m_data + i;

                        memory::construct(cur + length, value_type(std::move(*cur)));
                    }

                    pointer const base_pos = m_data + position;

                    for(unsigned int i = 0; i < length; ++i) {
                        memory::construct(base_pos + i, *(items + i));
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
             * Add many items at the end of the vector.
             *
             * @param items Pointer to the array of items to add.
             * @param length The number of items to add.
             */
            template<typename It>
            void push_all(It const begin, It const end) {
                add_all(begin, end, m_length);
            }

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
            JLT_NODISCARD int find(const_reference item) const {
                for(unsigned int i = 0; i < m_length; ++i) {
                    if(m_data[i] == item) {
                        return static_cast<int>(i);
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

                if constexpr(!std::is_trivial<value_type>::value) {
                    m_data[i].~value_type();
                }

                for(; i < m_length - 1; ++i) { m_data[i] = std::move(m_data[i + 1]); }

                --m_length;
            }

            /**
             * Remove all the items from the vector.
             */
            void clear() {
                if constexpr(!std::is_trivial<value_type>::value) {
                    const_iterator end = cend();

                    for(iterator it = begin(); it != end; ++it) { (*it).~value_type(); }
                }

                m_length = 0;
            }

            JLT_NODISCARD bool contains(const_reference value) const { return find(value) >= 0; }

            JLT_NODISCARD constexpr iterator begin() { return iterator{m_data}; }
            JLT_NODISCARD constexpr iterator end() { return iterator{m_data + m_length}; }

            JLT_NODISCARD constexpr const_iterator begin() const { return const_iterator{m_data}; }
            JLT_NODISCARD constexpr const_iterator end() const { return const_iterator{m_data + m_length}; }

            JLT_NODISCARD constexpr const_iterator cbegin() const { return const_iterator{m_data}; }
            JLT_NODISCARD constexpr const_iterator cend() const { return const_iterator{m_data + m_length}; }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_VECTOR_HPP */
