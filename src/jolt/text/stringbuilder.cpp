#include <cstring>
#include <jolt/util.hpp>
#include <jolt/memory/allocator.hpp>
#include "stringbuilder.hpp"

using namespace jolt::memory;

namespace jolt {
    namespace text {
        StringBuilder::StringBuilder(const UTF8String &initial_value, unsigned int const capacity) :
          m_strings{capacity} {
            m_strings.push(initial_value);
        }

        StringBuilder::StringBuilder(unsigned int const capacity) : m_strings{capacity} {}

        UTF8String StringBuilder::to_string() {
            size_t len = 0;

            for(const auto &s : m_strings) { len += s.get_length(); }

            auto res = allocate<utf8c>(len + 1);
            size_t i = 0;

            for(const auto &s : m_strings) {
                memcpy(res + i, s.get_raw(), sizeof(utf8c) * s.get_length());

                i += s.get_length();
            }

            res[len] = 0;

            return UTF8String{res, len, UTF8String::noclone};
        }
    } // namespace text
} // namespace jolt
