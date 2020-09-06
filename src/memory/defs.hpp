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
            ALLOC_NONE = 0,             // No flags specified.
            ALLOC_BIG = 0x00000001,     // Allocate within big objects space.
            ALLOC_PERSIST = 0x00000011, // Allocate within persistent objects space.
            ALLOC_SCRATCH = 0x00000111  // Allocate within the scratch memory.
        };

        struct AllocHeader {
            flags_t const m_flags;

          protected:
            AllocHeader(flags_t flags) : m_flags(flags) {}
        };
    } // namespace memory
} // namespace jolt

#endif // JLT_MEMORY_DEFS_HPP
