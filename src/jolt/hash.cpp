#include <xxhash.h>
#include "hash.hpp"

static constexpr uint64_t DEFAULT_SEED = 2147483647;

namespace jolt {
    namespace hash {
        uint64_t XXHash::hash(const void *const data, size_t const size) {
            return static_cast<hash_t>(XXH64(data, size, DEFAULT_SEED));
        }
    } // namespace hash
} // namespace jolt
