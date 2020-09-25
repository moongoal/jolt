#ifndef JLT_HASH_HASH_HPP
#define JLT_HASH_HASH_HPP

#include <cstdint>
#include <util.hpp>

namespace jolt {
    namespace hash {
        using hash_t = uint64_t;

        struct XXHash {
            static JLTAPI hash_t hash(const void *const data, size_t const size);
        };
    } // namespace hash
} // namespace jolt

#endif /* JLT_HASH_HASH_HPP */
