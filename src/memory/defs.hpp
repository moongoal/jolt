#ifndef JLT_MEMORY_DEFS_HPP
#define JLT_MEMORY_DEFS_HPP

#include <cstdint>

namespace jolt {
    namespace memory {
        using flags_t = uint32_t; // The allocation flags type.

        /**
         * The allocation flags values.
         */
        enum AllocFlags : flags_t {
            ALLOC_NONE,    // No flags specified
            ALLOC_PERSIST, // Allocate within persistent objects space
            ALLOC_BIG      // Allocate within big objects space
        };
    } // namespace memory
} // namespace jolt

#endif // JLT_MEMORY_DEFS_HPP
