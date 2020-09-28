#ifndef JLT_TEXT_STRING_HPP
#define JLT_TEXT_STRING_HPP

#include <utility>
#include "unicode.hpp"

namespace jolt {
    namespace text {
        /**
         * Immutable string class.
         */
        class JLTAPI UTF8String {
          public:
            using noclone_t = int;

            constexpr static noclone_t noclone = 0;

          private:
            utf8c *m_str;            //< String data.
            size_t const m_str_len;  //< Length as number of code points.
            size_t const m_str_size; //< Length as number of bytes.
            bool m_own = true;       //< True if the memory of m_str is owned by the object.

          protected:
            /**
             * Disposes the allocated resources. Will do nothing if called more than once.
             *
             * @remarks The object is invalidated after calling this function and must not be used.
             */
            void dispose();

          public:
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
            UTF8String(utf8c const *const s, size_t const s_size, noclone_t noclone);

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
            UTF8String(char const *const s, size_t const s_size, noclone_t noclone);

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
              m_str{const_cast<utf8c *>(s)}, m_str_len{utf8_len(s, N - 1)},
              m_str_size{N - 1}, m_own{false} {}

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
             * Check whether two strings contain the same data.
             */
            bool operator==(const UTF8String &other) const;

            /**
             * Concatenate two strings.
             */
            UTF8String operator+(const UTF8String &other) const;

            /**
             * Join multiple strings.
             *
             * @param left The left string.
             * @param right The right string.
             * @param other The remaining strings to join.
             *
             * @return A new string containing the concatenated values of the inputs.
             */
            template<typename... S>
            static UTF8String join(const UTF8String &left, const UTF8String &right, S... other) {
                if constexpr(sizeof...(other) == 0) {
                    return left + right;
                } else {
                    return left + join(right, other...);
                }
            }

            operator const utf8c *() const { return get_raw(); }
            const utf8c &operator[](size_t const i) const { return m_str[i]; }
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
              std::is_same<C, char>::value || std::is_same<C, utf8c>::value,
              "Invalid string literal");

            size_t len;

            if constexpr(std::is_same<C, char>::value) {
                len = strlen(raw);
            } else { // utf8c
                len = utf8_len(raw);
            }

            return UTF8String{raw, len};
        }

        /**
         * Concatenate two strings.
         */
        template<typename C, size_t N>
        UTF8String operator+(const C (&left)[N], const UTF8String &right) {
            static_assert(
              std::is_same<C, char>::value || std::is_same<C, utf8c>::value,
              "Invalid string literal");

            return UTF8String{left} + right;
        }

        /**
         * Concatenate two strings.
         */
        template<typename C, size_t N>
        UTF8String operator+(const UTF8String &left, const C (&right)[N]) {
            static_assert(
              std::is_same<C, char>::value || std::is_same<C, utf8c>::value,
              "Invalid string literal");

            return left + UTF8String{right};
        }

        using String = UTF8String;

        extern JLTAPI String EmptyString;
    } // namespace text
} // namespace jolt

#endif /* JLT_TEXT_STRING_HPP */
