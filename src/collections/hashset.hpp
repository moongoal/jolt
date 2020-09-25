#ifndef JLT_COLLECTIONS_HASHSET_HPP
#define JLT_COLLECTIONS_HASHSET_HPP

#include <initializer_list>
#include <hash.hpp>
#include "vector.hpp"
#include "keyvaluepair.hpp"

namespace jolt {
    namespace collections {
        template<typename T, typename H = jolt::hash::XXHash>
        class HashSet {
          public:
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;

            static constexpr unsigned int DEFAULT_CAPACITY = Vector<T>::DEFAULT_CAPACITY;

          private:
            Vector<hash::hash_t> m_hashes; //< Hash data.
            Vector<value_type> m_values;   //< Value data.

          public:
            explicit HashSet() = default;

            HashSet(std::initializer_list<T> lst) : HashSet(lst.begin(), lst.end()) {}

            template<typename It>
            HashSet(It const begin, It const end) : HashSet{} {
                add_all(begin, end);
            }

            HashSet(HashSet const &other) : HashSet{other.cbegin(), other.cend()} {}
            HashSet(HashSet &&other) = default;

            /**
             * Get the number of items in the set.
             */
            unsigned int get_length() const { return m_values.get_length(); }

            /**
             * Get the current capacity of the set.
             */
            unsigned int get_capacity() const { return m_values.get_capacity(); }

            /**
             * Reserve some capacity.
             */
            void reserve_capacity(unsigned int const new_capacity) {
                m_hashes.reserve_capacity(new_capacity);
                m_values.reserve_capacity(new_capacity);
            }
            /**
             * Add a value.
             *
             * @param hash The pre-computed hash.
             * @param value The value.
             */
            bool add(hash::hash_t hash, const_reference value) {
                if(m_hashes.contains(hash)) {
                    return false;
                }

                m_hashes.push(hash);
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

                for(It it = begin; it != end; ++it) { result &= add(*it); }

                return result;
            }

            /**
             * Add a value.
             *
             * @param value The value.
             */
            bool add(const_reference value) {
                hash::hash_t hash = H::hash(&value, sizeof(value));

                return add(hash, value);
            }

            /**
             * Check whether a hash is contained in the set.
             *
             * @param hash The hash.
             *
             * @return True if an item with this hash is present in the set, false if not.
             */
            bool contains_hash(hash::hash_t const hash) const { return m_hashes.contains(hash); }

            /**
             * Check whether a value is contained in the set.
             *
             * @param value The value.
             *
             * @return True if the item is present in the set, false if not.
             */
            bool contains(const_reference value) const {
                return m_hashes.contains(H::hash(&value, sizeof(value)));
            }

            /**
             * Remove an item given its hash.
             *
             * @param hash The hash of the item to remove.
             */
            bool remove_hash(hash::hash_t const hash) {
                int i = m_hashes.find(hash);

                if(i < 0) {
                    return false;
                }

                m_hashes.remove_at(i);
                m_values.remove_at(i);

                return true;
            }

            /**
             * Remove an item.
             *
             * @param value The value of the item to remove.
             */
            bool remove(const_reference value) {
                return remove_hash(H::hash(&value, sizeof(value)));
            }

            /**
             * Remove all the items.
             */
            void clear() {
                m_hashes.clear();
                m_values.clear();
            }

            typename Vector<value_type>::const_iterator cbegin() const { return m_values.cbegin(); }
            typename Vector<value_type>::const_iterator cend() const { return m_values.cend(); }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_HASHSET_HPP */
