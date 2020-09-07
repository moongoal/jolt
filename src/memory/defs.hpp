#ifndef JLT_MEMORY_DEFS_HPP
#define JLT_MEMORY_DEFS_HPP

#include <cstdint>
#include "checks.hpp"

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
            uint32_t m_alloc_sz;
            uint32_t const m_alloc_offset;
            flags_t const m_flags;

#ifdef JLT_WITH_MEM_CHECKS
            JLT_MEM_CANARY_VALUE_TYPE m_free_canary = JLT_MEM_CANARY_VALUE;
#endif // JLT_WITH_MEM_CHECKS

            AllocHeader(uint32_t const alloc_sz, flags_t flags, uint32_t const offset) :
                m_alloc_sz(alloc_sz), m_alloc_offset(offset), m_flags(flags) {}
        };
    } // namespace memory
} // namespace jolt

#endif // JLT_MEMORY_DEFS_HPP
