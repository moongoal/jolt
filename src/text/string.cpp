#define _CRT_SECURE_NO_WARNINGS
#include <cstring>
#include <memory/allocator.hpp>
#include "string.hpp"

using namespace jolt::memory;

namespace jolt {
    namespace text {
        UTF8String::UTF8String(utf8c const *const s, size_t const s_size, noclone_t noclone) :
          m_str{const_cast<utf8c *>(s)}, m_str_len{utf8_len(s, s_size)}, m_str_size{s_size} {}

        UTF8String::UTF8String(utf8c const *const s, size_t const s_size) :
          m_str{allocate<utf8c>(s_size)}, m_str_len{utf8_len(s, s_size)}, m_str_size{s_size} {
            memcpy(m_str, s, s_size);
        }

        UTF8String::UTF8String(const UTF8String &other) :
          m_str{allocate<utf8c>(other.m_str_size)}, m_str_len{other.m_str_len},
          m_str_size{other.m_str_size} {
            memcpy(m_str, other.m_str, m_str_size);
        }

        UTF8String::UTF8String(UTF8String &&other) :
          m_str{other.m_str}, m_str_len{other.m_str_len},
          m_str_size{other.m_str_size}, m_own{other.m_own} {
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
            // TODO: Add Unicode normalization support
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
            utf8c *const new_str = allocate<utf8c>(total_size);

            memcpy(new_str, m_str, m_str_size);
            memcpy(new_str + m_str_size, other.m_str, other.m_str_size);

            return UTF8String{new_str, total_size, noclone};
        }
    } // namespace text
} // namespace jolt
