#ifndef JLT_THREADING_LOCK_HPP
#define JLT_THREADING_LOCK_HPP

#include <limits>
#include <util.hpp>
#include <debug.hpp>

#ifdef _WIN32
    #include <Windows.h>
#else
    #error OS not supported
#endif // _WIN32

namespace jolt {
    namespace threading {
        /**
         * A lock that busy-waits at first and then yields control until the resource is available.
         */
        class Lock {
            CRITICAL_SECTION m_lock;

          public:
            /**
             * Initialize a new instance of this class.
             *
             * @param spin_count The number of times to spin before yielding control
             */
            explicit Lock(size_t spin_count = 0) noexcept {
                jltassert(spin_count <= std::numeric_limits<DWORD>::max());

                ::InitializeCriticalSectionAndSpinCount(&m_lock, (DWORD)spin_count);
            }

            ~Lock() noexcept { ::DeleteCriticalSection(&m_lock); }

            Lock(Lock &other) = delete;
            Lock &operator=(Lock &other) = delete;

            /**
             * Try to acquire the lock and if it's already held, give up immediately.
             *
             * @return True if the lock has been acquired, false if it hasn't because some other
             * thread alraedy holds it
             */
            JLT_INLINE bool try_acquire() noexcept {
                return (bool)::TryEnterCriticalSection(&m_lock);
            }

            /**
             * Wait until the lock is available and then acquire it.
             */
            JLT_INLINE void acquire() noexcept { ::EnterCriticalSection(&m_lock); }

            /**
             * Release the lock.
             */
            JLT_INLINE void release() noexcept { ::LeaveCriticalSection(&m_lock); }
        };
    } // namespace threading
} // namespace jolt

#endif /* JLT_THREADING_LOCK_HPP */
