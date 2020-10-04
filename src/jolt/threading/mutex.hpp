#ifndef JLT_THREADING_MUTEX_HPP
#define JLT_THREADING_MUTEX_HPP

#include "lock.hpp"

namespace jolt {
    namespace threading {
        template<typename T>
        struct Mutex {
            Lock lock;
            T value;
        };
    } // namespace threading
} // namespace jolt

#endif /* JLT_THREADING_MUTEX_HPP */
