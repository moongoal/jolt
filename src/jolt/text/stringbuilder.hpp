#ifndef JLT_TEXT_STRINGBUILDER_HPP
#define JLT_TEXT_STRINGBUILDER_HPP

#include <jolt/util.hpp>
#include "string.hpp"
#include <jolt/collections/vector.hpp>

namespace jolt {
    namespace text {
        class JLTAPI StringBuilder {
            constexpr static size_t DEFAULT_CAPACITY = 4;

            jolt::collections::Vector<String> m_strings; //< The strings to join.

          public:
            /**
             * Create a new string builder.
             *
             * @param initial_value The value used to initialize the string builder.
             * @param capacity The initial capacity of the underlying data structure.
             */
            StringBuilder(
              const String &initial_value, unsigned int const capacity = DEFAULT_CAPACITY);

            /**
             * Create a new string builder.
             *
             * @param capacity The initial capacity of the underlying data structure.
             */
            explicit StringBuilder(unsigned int const capacity = DEFAULT_CAPACITY);

            StringBuilder(const StringBuilder &other) = delete;
            StringBuilder &operator=(const StringBuilder &other) = delete;

            /**
             * Add a new string.
             *
             * @param value The string to add.
             */
            void add(const String &value) { m_strings.push(value); }

            /**
             * Empty the contents of the string builder.
             */
            void reset() { m_strings.clear(); }

            /**
             * Return a new string containing the contents of the string builder joined together in
             * order.
             */
            String to_string();

            /**
             * @see to_string()
             */
            operator String() { return to_string(); }
        };
    } // namespace text
} // namespace jolt

#endif /* JLT_TEXT_STRINGBUILDER_HPP */
