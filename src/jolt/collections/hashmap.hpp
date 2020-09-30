#ifndef JLT_COLLECTIONS_HASHMAP_HPP
#define JLT_COLLECTIONS_HASHMAP_HPP

#include <jolt/debug.hpp>
#include <jolt/hash.hpp>
#include <jolt/memory/allocator.hpp>
#include "valueset.hpp"
#include "keyvaluepair.hpp"
#include "iterator.hpp"

namespace jolt {
    namespace collections {
        /**
         * Iterator implementation for the hash map.
         *
         * @tparam K The key type.
         * @tparam V The value type.
         */
        template<typename K, typename V>
        struct HashMapIterator {
            using value_type = V;
            using pointer = V *;
            using const_pointer = const V *;
            using reference = V &;
            using const_reference = const V &;
            using pair = KeyValuePair<K, V>;
            using const_pair = const KeyValuePair<K, V>;
            using table_entry = ValueSet<pair>;

          private:
            template<typename A, typename B>
            friend class HashMapIterator;

            table_entry *const m_table;
            size_t const m_table_length;
            size_t m_current_table_entry;
            typename table_entry::iterator m_cur_entry_it, m_cur_entry_end;

            bool next_entry() {
                ++m_current_table_entry;

                if(m_current_table_entry >= m_table_length) {
                    return false;
                }

                m_cur_entry_it = m_table[m_current_table_entry].begin();
                m_cur_entry_end = m_table[m_current_table_entry].end();

                return true;
            }

          public:
            HashMapIterator(
              table_entry *const table, size_t const table_length, bool const end = false) :
              m_table{table},
              m_table_length{table_length}, m_current_table_entry{end ? table_length : 0},
              m_cur_entry_it{end ? m_table[m_table_length - 1].end() : m_table[0].begin()},
              m_cur_entry_end{end ? m_table[m_table_length - 1].end() : m_table[0].end()} {}

            HashMapIterator(const HashMapIterator &other) = default;
            HashMapIterator(HashMapIterator &&other) = default;

            HashMapIterator operator++() {
                HashMapIterator other = *this;

                if(m_cur_entry_it == m_cur_entry_end) {
                    next();
                } else {
                    ++m_cur_entry_it;

                    if(m_cur_entry_it == m_cur_entry_end) {
                        next();
                    }
                }

                return other;
            }

            HashMapIterator &operator++(int) {
                if(m_cur_entry_it == m_cur_entry_end) {
                    next();
                } else {
                    ++m_cur_entry_it;

                    if(m_cur_entry_it == m_cur_entry_end) {
                        next();
                    }
                }

                return *this;
            }

            template<typename It>
            constexpr bool operator==(const It other) const {
                return m_cur_entry_it == other.m_cur_entry_it;
            }

            template<typename It>
            constexpr bool operator!=(const It other) const {
                return m_cur_entry_it != other.m_cur_entry_it;
            }

            constexpr auto &operator*() { return *m_cur_entry_it; }
            constexpr const_pair &operator*() const { return *m_cur_entry_it; }

            constexpr void next() { next_entry(); }
            constexpr pair *get_pointer() const { return &*m_cur_entry_it; }
        };

        /**
         * Hash map.
         *
         * @tparam K The key type.
         * @tparam T The value type.
         * @tparam H The hash computation class type.
         */
        template<typename K, typename T, typename H = jolt::hash::XXHash>
        class HashMap {
          public:
            using key_type = K;
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;

            template<typename V>
            using base_iterator = HashMapIterator<K, V>;
            using iterator = base_iterator<T>;
            using const_iterator = const base_iterator<T>;

            constexpr static size_t DEFAULT_CAPACITY = 16;

          private:
            using table_entry = ValueSet<KeyValuePair<K, T>>;

            table_entry *m_table;
            size_t const m_capacity;

            /**
             * Compute the index in the hash table given for a given key.
             *
             * @param key The key.
             *
             * @return The index in the hash table to store/look-up the key to/in.
             */
            size_t compute_index(const key_type &key) const {
                hash::hash_t hash = H::hash(&key, sizeof(key));

                return static_cast<size_t>(hash) % m_capacity;
            }

          public:
            /**
             * Create a new hash map.
             *
             * @param capacity The size of the hash table.
             *
             * @remarks A bigger capacity will use more space but will reduce the chance of the map
             * decaying to a list when many items are inserted.
             */
            explicit HashMap(size_t const capacity = DEFAULT_CAPACITY) :
              m_table{jolt::memory::allocate<table_entry>(capacity)}, m_capacity{capacity} {
                jltassert(capacity);

                for(size_t i = 0; i < capacity; ++i) { memory::construct(&m_table[i]); }
            }

            HashMap(const HashMap &other) :
              HashMap{other.cbegin(), other.cend(), other.m_capacity} {}

            HashMap(HashMap &&other) : m_table{other.m_table}, m_capacity{other.m_capacity} {
                other.m_table = nullptr;
            }

            /**
             * Create a new hash map.
             *
             * @param begin The iterator pointing to the first pair.
             * @param end The iterator pointing to the end of the sequence.
             * @param capacity The capacity.
             */
            template<typename V>
            HashMap(
              base_iterator<V> begin,
              base_iterator<V> end,
              size_t const capacity = DEFAULT_CAPACITY) :
              HashMap{capacity} {
                add_all(begin, end);
            }

            ~HashMap() {
                if(m_table) {
                    jolt::memory::free(m_table, m_capacity);
                }
            }

            /**
             * Add a key/value pair.
             *
             * @param key The key.
             * @param value The value.
             *
             * @remarks This is an alias to `set_value()`.
             */
            void add(const key_type &key, const value_type &value) { set_value(key, value); }

            /**
             * Add mulitple key/value pairs.
             *
             * @param begin The iterator pointing to the first pair.
             * @param end The iterator pointing to the end of the sequence.
             */
            template<typename V>
            void add_all(base_iterator<V> begin, base_iterator<V> end) {
                for(base_iterator<V> it = begin; it != end; ++it) {
                    add((*it).get_key(), (*it).get_value());
                }
            }

            /**
             * Checks whether a key is present.
             *
             * @param key The key to check.
             *
             * @return True if the key is present in the map, false if not.
             */
            bool contains_key(const key_type &key) const {
                size_t const index = compute_index(key);

                for(const auto &pair : m_table[index]) {
                    if(pair.get_key() == key) {
                        return true;
                    }
                }

                return false;
            }

          private:
            const KeyValuePair<K, T> *
            get_pair_for_index(const key_type &key, size_t const index) const {
                for(const auto &pair : m_table[index]) {
                    if(pair.get_key() == key) {
                        return &pair;
                    }
                }

                return nullptr;
            }

            KeyValuePair<K, T> *get_pair_for_index(const key_type &key, size_t const index) {
                const HashMap *const self = this;

                return const_cast<KeyValuePair<K, T> *>(self->get_pair_for_index(key, index));
            }

          public:
            /**
             * Return a key/value pair for a given key.
             *
             * @param key The key.
             *
             * @return The ke/value pair for the given key or `nullptr` if the key is not present.
             */
            const KeyValuePair<K, T> *get_pair(const key_type &key) const {
                size_t const index = compute_index(key);

                return get_pair_for_index(key, index);
            }

            /**
             * Return a key/value pair for a given key.
             *
             * @param key The key.
             *
             * @return The key/value pair for the given key or `nullptr` if the key is not present.
             */
            KeyValuePair<K, T> *get_pair(const key_type &key) {
                const HashMap *const self = this;

                return const_cast<KeyValuePair<K, T> *>(self->get_pair(key));
            }

            /**
             * Return a value for the given key.
             *
             * @param key The key.
             *
             * @return The value for the given key or `nullptr` if the key is not present.
             */
            const_pointer get_value(const key_type &key) const {
                const KeyValuePair<K, T> *const pair = get_pair(key);

                return pair ? &pair->get_value() : nullptr;
            }

            /**
             * Return a value for the given key.
             *
             * @param key The key.
             *
             * @return The value for the given key or `nullptr` if the key is not present.
             */
            pointer get_value(const key_type &key) {
                const HashMap *const self = this;
                return const_cast<pointer>(self->get_value(key));
            }

            /**
             * Set a key/value pair.
             *
             * @param key The key.
             * @param value The value.
             */
            void set_value(const key_type &key, const value_type &value) {
                size_t const index = compute_index(key);
                KeyValuePair<K, T> *pair = get_pair_for_index(key, index);

                if(pair) {
                    pair->set_value(value);
                } else {
                    m_table[index].add(KeyValuePair{key, value});
                }
            }

            /**
             * Remove an item.
             *
             * @param key The key.
             */
            void remove(const key_type &key) {
                size_t const index = compute_index(key);

                for(const auto &pair : m_table[index]) {
                    if(pair.get_key() == key) {
                        m_table[index].remove(pair);
                        return;
                    }
                }
            }

            /**
             * Remove all items.
             */
            void clear() {
                for(size_t i = 0; i < m_capacity; ++i) { m_table[i].clear(); }
            }

            /**
             * Return the number of items.
             */
            size_t get_length() const {
                size_t sz = 0;

                for(size_t i = 0; i < m_capacity; ++i) { sz += m_table[i].get_length(); }

                return sz;
            }

            iterator begin() { return iterator{m_table, m_capacity}; }
            iterator end() { return iterator{m_table, m_capacity, true}; }

            const_iterator begin() const { return const_iterator{m_table, m_capacity}; }
            const_iterator end() const { return const_iterator{m_table, m_capacity, true}; }

            const_iterator cbegin() const { return const_iterator{m_table, m_capacity}; }
            const_iterator cend() const { return const_iterator{m_table, m_capacity, true}; }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_HASHMAP_HPP */
