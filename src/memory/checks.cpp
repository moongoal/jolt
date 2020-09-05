#include "checks.hpp"

namespace jolt {
    namespace memory {
        bool check_use_after_free(void *const ptr, size_t const size) {
            auto cptr = reinterpret_cast<uint8_t *>(ptr);
            void *const cend = cptr + size;

            for(; cptr < cend; ++cptr) {
                if(*cptr != JLT_MEM_FILLER_VALUE) {
                    return false;
                }
            }

            return true;
        }
    } // namespace memory
} // namespace jolt
