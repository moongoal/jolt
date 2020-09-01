#include "util.hpp"

namespace jolt {
    void *align_raw_ptr(void *const ptr, size_t const alignment) {
        const uintptr_t mask = static_cast<uintptr_t>(alignment) - 1;
        
        return reinterpret_cast<void *>((mask + reinterpret_cast<uintptr_t>(ptr)) & ~mask);
    }
} // namespace jolt
