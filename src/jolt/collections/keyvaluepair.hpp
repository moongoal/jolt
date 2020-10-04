#ifndef JLT_COLLECTIONS_KEYVALUEPAIR_HPP
#define JLT_COLLECTIONS_KEYVALUEPAIR_HPP

namespace jolt {
    namespace collections {
        /**
         * A generic key-value pair.
         *
         * @tparam K The type of the key.
         * @tparam V The type of the value.
         */
        template<typename K, typename V>
        class KeyValuePair {
          public:
            using key_type = K;
            using value_type = V;
            using key_reference = K &;
            using value_reference = V &;
            using const_key_reference = const K &;
            using const_value_reference = const V &;

            key_type m_key;
            value_type m_value;

            constexpr KeyValuePair(const_key_reference key, const_value_reference value) :
              m_key{key}, m_value{value} {}

            const_key_reference get_key() const { return m_key; }
            value_reference get_value() { return m_value; }
            const_value_reference get_value() const { return m_value; }
            void set_value(const_value_reference new_value) { m_value = new_value; }

            value_reference operator*() { return m_value; }
            const_value_reference operator*() const { return m_value; }

            bool operator==(const KeyValuePair other) const { return m_key == other.m_key; }
            bool operator==(const_key_reference other_key) const { return m_key == other_key; }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_KEYVALUEPAIR_HPP */
