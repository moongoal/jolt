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
            size_t sz = 0;

            for(const auto &s : m_strings) { sz += s.get_size(); }

            auto res = allocate_array<utf8c>(sz + 1);
            size_t i = 0;

            for(const auto &s : m_strings) {
                memcpy(res + i, s.get_raw(), sizeof(utf8c) * s.get_size());

                i += s.get_size();
            }

            res[sz] = 0;

            return UTF8String{res, sz, UTF8String::noclone};
        }
    } // namespace text
} // namespace jolt
