#ifndef JLT_COLLECTIONS_VALUESET_HPP
#define JLT_COLLECTIONS_VALUESET_HPP

#include <initializer_list>
#include "vector.hpp"

namespace jolt {
    namespace collections {
        template<typename T>
        class ValueSet {
          public:
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;
            using iterator = typename Vector<T>::const_iterator;
            using const_iterator = typename Vector<T>::const_iterator;

          private:
            Vector<value_type> m_values; //< Values

          public:
            JLT_NODISCARD explicit ValueSet() = default;

            JLT_NODISCARD ValueSet(const std::initializer_list<T> &lst) : ValueSet{lst.begin(), lst.end()} {}

            template<typename It>
            JLT_NODISCARD ValueSet(It const begin, It const end) : ValueSet{} {
                add_all(begin, end);
            }

            JLT_NODISCARD ValueSet(ValueSet const &other) : ValueSet{other.cbegin(), other.cend()} {}
            JLT_NODISCARD ValueSet(ValueSet &&other) = default;

            /**
             * Get the number of items in the set.
             */
            JLT_NODISCARD unsigned int get_length() const { return m_values.get_length(); }

            /**
             * Get the current capacity of the set.
             */
            JLT_NODISCARD unsigned int get_capacity() const { return m_values.get_capacity(); }

            /**
             * Reserve some capacity.
             */
            void reserve_capacity(unsigned int const new_capacity) {
                m_values.reserve_capacity(new_capacity);
            }

            /**
             * Add a value.
             *
             * @param value The value.
             */
            bool add(const_reference value) {
                if(m_values.contains(value)) {
                    return false;
                }

                m_values.push(value);

                return true;
            }

            /**
             * Add multiple values.
             *
             * @param begin Iterator to the beginning of the sequence.
             * @param end Iterator to the end of the sequence.
             */
            template<typename It>
            bool add_all(It const begin, It const end) {
                bool result = true;

                for(It it = begin; it != end; ++it) {
                    bool const add_result = add(*it);

                    result = result && add_result;
                }

                return result;
            }

            /**
             * Check whether a value is contained in the set.
             *
             * @param value The value.
             *
             * @return True if the item is present in the set, false if not.
             */
            JLT_NODISCARD bool contains(const_reference value) const { return m_values.contains(value); }

            /**
             * Remove an item.
             *
             * @param value The value of the item to remove.
             */
            bool remove(const_reference value) {
                int const i = m_values.find(value);

                if(i < 0) {
                    return false;
                }

                m_values.remove_at(i);

                return true;
            }

            /**
             * Remove all the items.
             */
            void clear() { m_values.clear(); }

            JLT_NODISCARD typename Vector<value_type>::const_iterator begin() const {
                return m_values.cbegin();
            }

            JLT_NODISCARD typename Vector<value_type>::const_iterator end() const { return m_values.cend(); }

            JLT_NODISCARD typename Vector<value_type>::const_iterator cbegin() const {
                return m_values.cbegin();
            }

            JLT_NODISCARD typename Vector<value_type>::const_iterator cend() const { return m_values.cend(); }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_VALUESET_HPP */
