#ifndef JLT_TEXT_STRING_HPP
#define JLT_TEXT_STRING_HPP

#include <utility>
#include <limits>
#include <jolt/api.hpp>
#include <jolt/hash.hpp>
#include "unicode.hpp"

namespace jolt {
    namespace collections {
        template<typename T>
        class Array;
    }

    namespace text {
        /**
         * Immutable string class.
         */
        class JLTAPI UTF8String {
          public:
            struct noclone_t {};
            using string_array = collections::Array<UTF8String>;

            constexpr static const unsigned int MAX_SPLITS = std::numeric_limits<unsigned int>::max();
            constexpr static const noclone_t noclone{};

          private:
            utf8c *m_str;      //< String data.
            size_t m_str_len;  //< Length as number of code points.
            size_t m_str_size; //< Length as number of bytes.
            bool m_own = true; //< True if the memory of m_str is owned by the object.

            /**
             * Initialize a new instance of this class.
             *
             * @param s The string.
             * @param s_size The size of the string as the number of `utf8c` elements.
             * @param s_len The length of the string as the number of code points.
             *
             * @remarks This constructor will not create a copy of the string and will *NOT* acquire ownership
             * of `s`.
             */
            UTF8String(
              utf8c const *const s,
              size_t const s_size,
              size_t const s_len,
              JLT_MAYBE_UNUSED noclone_t noclone);

            /**
             * Disposes the allocated resources. Will do nothing if called more than once.
             *
             * @remarks The object is invalidated after calling this function and must not be used.
             */
            void dispose();

          public:
            /**
             * Initialize a new empty string.
             */
            UTF8String();

            /**
             * Initialize a new instance of this class.
             *
             * @param s The string.
             * @param s_size The size of the string as the number of `utf8c` elements.
             * @param noclone The constant noclone.
             *
             * @remarks This constructor will not create a copy of the string but will acquire
             * ownership of the input `s`.
             */
            UTF8String(utf8c const *const s, size_t const s_size, JLT_MAYBE_UNUSED noclone_t noclone);

            /**
             * Initialize a new instance of this class.
             *
             * @param s The string.
             * @param s_size The size of the string as the number of `utf8c` elements.
             * @param noclone The constant noclone.
             *
             * @remarks This constructor will not create a copy of the string but will acquire
             * ownership of the input `s`.
             */
            UTF8String(char const *const s, size_t const s_size, JLT_MAYBE_UNUSED noclone_t noclone);

            /**
             * Initialize a new instance of this class.
             *
             * @tparam N The length of the string literal.
             *
             * @param s The string literal.
             *
             * @remarks This constructor will not create a copy of the string. It will use the
             * literal's storage as its data.
             */
            template<size_t N>
            constexpr UTF8String(const utf8c (&s)[N]) :
              m_str{const_cast<utf8c *>(s)}, m_str_len{utf8_len(s, N - 1)}, m_str_size{N - 1}, m_own{false} {}

            /**
             * Initialize a new instance of this class.
             *
             * @tparam N The length of the string literal.
             *
             * @param s The string literal.
             *
             * @remarks This constructor will not create a copy of the string. It will use the
             * literal's storage as its data.
             */
            template<size_t N>
            constexpr UTF8String(const char (&s)[N]) :
              m_str{const_cast<utf8c *>(reinterpret_cast<utf8c const *>(s))}, m_str_len{N - 1},
              m_str_size{N - 1}, m_own{false} {}

            /**
             * Initialize a new instance of this class.
             *
             * @param s The string.
             * @param s_size The size of the string as the number of `utf8c` elements.
             *
             * @remarks This constructor will create a copy of the string.
             */
            UTF8String(utf8c const *const s, size_t const s_size);

            /**
             * Initialize a new instance of this class.
             *
             * @param s The string.
             * @param s_size The size of the string as the number of `utf8c` elements.
             * @param s_len The length of the string as the number of code points.
             *
             * @remarks This constructor will create a copy of the string.
             */
            UTF8String(utf8c const *const s, size_t const s_size, size_t const s_len);

            /**
             * Initialize a new instance of this class.
             *
             * @param s The string.
             * @param s_size The size of the string as the number of `utf8c` elements.
             *
             * @remarks This constructor will create a copy of the string.
             */
            UTF8String(char const *const s, size_t const s_size);

            /**
             * Initialize a new instance of this class.
             *
             * @param other The string.
             *
             * @remarks This constructor will create a copy of the string.
             */
            UTF8String(const UTF8String &other);

            /**
             * Initialize a new instance of this class.
             *
             * @param other The string.
             *
             * @remarks This constructor will not create a copy of the string and the newly
             * initialized object will steal ownership of the storage underlying the input argument.
             */
            UTF8String(UTF8String &&other);

            ~UTF8String();

            /**
             * Return the length of the string.
             */
            size_t get_length() const { return m_str_len; }

            /**
             * Return a pointer to the string's raw data.
             */
            const utf8c *get_raw() const { return m_str; }

            /**
             * Split a string using a separator.
             *
             * @param sep The separator string.
             *
             * @return An array of separated strings. If no separation occurs, an array with the entire string
             * is returned.
             */
            string_array split(UTF8String const &sep, unsigned int const max = MAX_SPLITS) const;

            /**
             * Find a substring.
             *
             * @param substr The substring to search.
             * @param start_idx The character index where to start the search.
             *
             * @return The index of the first occurrence of `substr`. -1 if no match is found.
             *
             * @remarks Providing a start_idx out of bounds is not an error and will have the same result as a
             * no-match.
             */
            int find(UTF8String const &substr, size_t const start_idx = 0) const;

            /**
             * Return a value stating whether the string starts with the substring passed as an argument.
             *
             * @param other The substring.
             *
             * @return True if the `other` matches the start of the string, false if not.
             */
            bool starts_with(UTF8String const &other) const;

            /**
             * Return a value stating whether the string ends with the substring passed as an argument.
             *
             * @param other The substring.
             *
             * @return True if the `other` matches the end of the string, false if not.
             */
            bool ends_with(UTF8String const &other) const;

            /**
             * Replace the first occurrence of a substring with another one.
             *
             * @param what The substring to find and replace.
             * @param with The replacement.
             *
             * @return The original string with the replaced value.
             */
            UTF8String replace(UTF8String const &what, UTF8String const &with) const;

            /**
             * Replace all the occurrences of a substring with another one.
             *
             * @param what The substring to find and replace.
             * @param with The replacement.
             *
             * @return The original string with the replaced value.
             */
            UTF8String replace_all(UTF8String const &what, UTF8String const &with) const;

            /**
             * Get a slice of the string.
             *
             * @param start_idx The starting index of the slice.
             * @param len The length of the slice. Set to -1 for the full remaining length.
             *
             * @return A new string containing the slice.
             */
            UTF8String slice(int const start_idx, int len = -1) const;

            /**
             * Check whether two strings contain the same data.
             */
            bool operator==(const UTF8String &other) const;

            /**
             * Concatenate two strings.
             */
            UTF8String operator+(const UTF8String &other) const;

            UTF8String &operator=(const UTF8String &other);

            /**
             * Merge multiple strings into one.
             *
             * @param left The left string.
             * @param right The right string.
             * @param other The remaining strings to join.
             *
             * @return A new string containing the concatenated values of the inputs.
             */
            template<typename... S>
            static UTF8String merge(const UTF8String &left, const UTF8String &right, S &&...other) {
                if constexpr(sizeof...(other) == 0) {
                    return left + right;
                } else {
                    return left + merge(right, std::forward<S>(other)...);
                }
            }

            /**
             * Join multiple strings using a separator.
             *
             * @param glue The separator to use when joining the strings.
             * @param left The left string.
             * @param right The right string.
             * @param other The remaining strings to join.
             *
             * @return A new string containing the concatenated values of the inputs.
             */
            template<typename... S>
            static UTF8String
            join(const UTF8String &glue, const UTF8String &left, const UTF8String &right, S &&...other) {
                if constexpr(sizeof...(other) == 0) {
                    return left + glue + right;
                } else {
                    return left + glue + join(glue, right, std::forward<S>(other)...);
                }
            }

            operator const utf8c *() const { return get_raw(); }
            const utf8c &operator[](size_t const idx) const;

            template<typename H>
            hash::hash_t hash() const {
                return H::hash(m_str, m_str_size);
            }
        };

        /**
         * Build a string object from its raw NUL-terminated representation.
         *
         * @tparam C The character type.
         *
         * @param raw The raw, NUL-terminated string.
         */
        template<typename C>
        static UTF8String s(const C *raw) {
            static_assert(
              std::is_same<C, char>::value || std::is_same<C, utf8c>::value, "Invalid string literal");

            size_t sz = strlen(reinterpret_cast<const char *>(raw));

            return UTF8String{raw, sz};
        }

        /**
         * Concatenate two strings.
         */
        template<typename C, size_t N>
        UTF8String operator+(const C (&left)[N], const UTF8String &right) {
            static_assert(
              std::is_same<C, char>::value || std::is_same<C, utf8c>::value, "Invalid string literal");

            return UTF8String{left} + right;
        }

        /**
         * Concatenate two strings.
         */
        template<typename C, size_t N>
        UTF8String operator+(const UTF8String &left, const C (&right)[N]) {
            static_assert(
              std::is_same<C, char>::value || std::is_same<C, utf8c>::value, "Invalid string literal");

            return left + UTF8String{right};
        }

        using String = UTF8String;

        extern JLTAPI String const EmptyString;
    } // namespace text
} // namespace jolt

#endif /* JLT_TEXT_STRING_HPP */
