#include <cstdint>
#include <new>
#include <features.hpp>
#include <util.hpp>
#include "allocator.hpp"

using namespace jolt;
using namespace jolt::memory;

#ifdef JLT_WITH_OVERLOAD_NEW
void *operator new(size_t count) {
    return _allocate(count, choose(ALLOC_NONE, ALLOC_BIG, count < BIG_OBJECT_MIN_SIZE), count);
}

void *operator new[](size_t count) {
    return _allocate(count, choose(ALLOC_NONE, ALLOC_BIG, count < BIG_OBJECT_MIN_SIZE), count);
}

void *operator new(size_t count, std::align_val_t al) {
    return _allocate(count,
                     choose(ALLOC_NONE, ALLOC_BIG, count < BIG_OBJECT_MIN_SIZE),
                     static_cast<size_t>(al));
}

void *operator new[](size_t count, std::align_val_t al) {
    return _allocate(count,
                     choose(ALLOC_NONE, ALLOC_BIG, count < BIG_OBJECT_MIN_SIZE),
                     static_cast<size_t>(al));
}
#endif // JLT_WITH_OVERLOAD_NEW
