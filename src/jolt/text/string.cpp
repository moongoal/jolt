#define _CRT_SECURE_NO_WARNINGS
#include <cstring>
#include <jolt/memory/allocator.hpp>
#include "stringbuilder.hpp"
#include "string.hpp"

using namespace jolt::memory;

namespace jolt {
    namespace text {
        String const EmptyString{u8""};

        UTF8String::UTF8String() :
          m_str{EmptyString.m_str}, m_str_len{EmptyString.m_str_len},
          m_str_size{EmptyString.m_str_size}, m_own{false} {}

        UTF8String::UTF8String(
          utf8c const *const s, size_t const s_size, JLT_MAYBE_UNUSED noclone_t noclone) :
          m_str{const_cast<utf8c *>(s)},
          m_str_len{utf8_len(s, s_size)}, m_str_size{s_size} {}

        UTF8String::UTF8String(utf8c const *const s, size_t const s_size) :
          m_str{allocate<utf8c>(s_size + 1)}, m_str_len{utf8_len(s, s_size)}, m_str_size{s_size} {
            memcpy(m_str, s, s_size);

            m_str[s_size] = 0;
        }

        UTF8String::UTF8String(utf8c const *const s, size_t const s_size, size_t const s_len) :
          m_str{allocate<utf8c>(s_size + 1)}, m_str_len{s_len}, m_str_size{s_size} {
            memcpy(m_str, s, s_size);

            m_str[s_size] = 0;
        }

        UTF8String::UTF8String(
          utf8c const *const s, size_t const s_size, size_t const s_len, JLT_MAYBE_UNUSED noclone_t noclone) :
          m_str{const_cast<utf8c *>(s)},
          m_str_len{s_len}, m_str_size{s_size}, m_own{false} {}

        UTF8String::UTF8String(char const *const s, size_t const s_size, JLT_MAYBE_UNUSED noclone_t noclone) :
          m_str{const_cast<utf8c *>(reinterpret_cast<utf8c const *>(s))}, m_str_len{s_size}, m_str_size{
                                                                                               s_size} {}

        UTF8String::UTF8String(char const *const s, size_t const s_size) :
          m_str{allocate<utf8c>(s_size + 1)}, m_str_len{s_size}, m_str_size{s_size} {
            memcpy(m_str, s, s_size * sizeof(char));

            m_str[s_size] = 0;
        }

        UTF8String::UTF8String(const UTF8String &other) :
          m_str_len{other.m_str_len}, m_str_size{other.m_str_size}, m_own{other.m_own} {
            if(other.m_own) {
                m_str = allocate<utf8c>(m_str_size + 1);
                memcpy(m_str, other.m_str, m_str_size);

                m_str[m_str_size] = 0;
            } else {
                m_str = other.m_str;
            }
        }

        UTF8String::UTF8String(UTF8String &&other) :
          m_str{other.m_str}, m_str_len{other.m_str_len}, m_str_size{other.m_str_size}, m_own{other.m_own} {
            other.m_own = false;
        }

        UTF8String::~UTF8String() { dispose(); }

        void UTF8String::dispose() {
            if(m_str && m_own) {
                free(m_str);
                m_str = nullptr;
            }
        }

        bool UTF8String::operator==(const UTF8String &other) const {
            if(other.m_str_len != m_str_len) {
                return false;
            } else {
                for(size_t i = 0; i < m_str_size; ++i) {
                    if(m_str[i] != other.m_str[i]) {
                        return false;
                    }
                }
            }

            return true;
        }

        UTF8String UTF8String::operator+(const UTF8String &other) const {
            size_t const total_size = m_str_size + other.m_str_size;
            utf8c *const new_str = allocate<utf8c>(total_size + 1);

            memcpy(new_str, m_str, m_str_size);
            memcpy(new_str + m_str_size, other.m_str, other.m_str_size);

            new_str[total_size] = 0;

            return UTF8String{new_str, total_size, noclone};
        }

        const utf8c &UTF8String::operator[](size_t const idx) const {
            utf8c const *p;
            size_t i;

            for(p = get_raw(), i = 0; p != nullptr && i < idx; p = utf8_next_cp(p, get_length() - i++)) {}

            jltassert(p);

            return *p;
        }

        int UTF8String::find(UTF8String const &substr, size_t const start_idx) const {
            if(start_idx < get_length()) {
                utf8c const *p = &((*this)[start_idx]);
                size_t i = 0;

                do {
                    size_t const remaining_len = get_length() - start_idx - i;
                    size_t const remaining_sz = m_str_size - (p - get_raw());

                    if(UTF8String(p, remaining_sz, remaining_len, noclone).starts_with(substr)) {
                        return start_idx + i;
                    }

                    p = utf8_next_cp(p, remaining_len);
                    i++;
                } while(p);
            }

            return -1;
        }

        bool UTF8String::starts_with(UTF8String const &other) const {
            if(get_length() >= other.get_length()) {
                utf8c const *p_this = get_raw();
                utf8c const *p_other = other.get_raw();

                for(size_t i = 0; i < other.get_length(); ++i) {
                    if(*p_this != *p_other) {
                        return false;
                    }

                    p_this = utf8_next_cp(p_this, get_length() - i);
                    p_other = utf8_next_cp(p_other, other.get_length() - i);
                }

                return true;
            }

            return false;
        }

        bool UTF8String::ends_with(UTF8String const &other) const {
            if(get_length() >= other.get_length()) {
                size_t const p_this_pos = get_length() - other.get_length();
                utf8c const *p_this =
                  (p_this_pos != get_length()) ? &((*this)[p_this_pos]) : get_raw() + m_str_size;

                return UTF8String(p_this, p_this - get_raw(), other.get_length(), noclone).starts_with(other);
            }

            return false;
        }

        UTF8String UTF8String::replace(UTF8String const &what, UTF8String const &with) const {
            int const start_idx = find(what);

            if(start_idx != -1 && what.get_length()) {
                size_t const s_end_idx = start_idx + what.get_length();
                utf8c const *const s = &((*this)[start_idx]);
                utf8c const *const s_end = (s_end_idx < get_length()) ? &((*this)[s_end_idx]) : nullptr;
                UTF8String s_begin{
                  get_raw(), static_cast<size_t>(s - get_raw()), static_cast<size_t>(start_idx), noclone};

                StringBuilder sb{s_begin, 3};

                sb.add(with);

                if(s_end) {
                    UTF8String end_str = UTF8String{
                      s_end,
                      static_cast<size_t>(get_raw() + m_str_size - s_end),
                      get_length() - start_idx - with.get_length(),
                      noclone};

                    sb.add(end_str);
                }

                return sb.to_string();
            } else {
                return *this;
            }
        }

        UTF8String UTF8String::replace_all(UTF8String const &what, UTF8String const &with) const {
            UTF8String res{*this};

            if(what.get_length()) {
                do { res = res.replace(what, with); } while(res.find(what) != -1);
            }

            return res;
        }

        UTF8String UTF8String::slice(int const start_idx, int len) const {
            bool const to_end = (len < 0) || (start_idx + len == static_cast<int>(get_length()));

            if(len < 0) {
                len = get_length() - start_idx;
            }

            jltassert(start_idx >= 0 && start_idx + len <= static_cast<int>(get_length()));

            const utf8c *const start = &((*this)[start_idx]);
            const utf8c *const end = to_end ? get_raw() + m_str_size : &((*this)[start_idx + len]);

            return UTF8String{start, static_cast<size_t>(end - start), static_cast<size_t>(len)};
        }

        UTF8String &UTF8String::operator=(const UTF8String &other) {
            dispose();

            // No need to copy if not owned
            if(other.m_own) {
                m_str = allocate<utf8c>(other.m_str_size + 1);
                memcpy(m_str, other.m_str, other.m_str_size);

                m_str[other.m_str_size] = 0;
            } else {
                m_str = other.m_str;
            }

            m_str_len = other.m_str_len;
            m_str_size = other.m_str_size;
            m_own = other.m_own;

            return *this;
        }
    } // namespace text
} // namespace jolt
