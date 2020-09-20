#ifndef JLT_THREADING_SPINLOCK_HPP
#define JLT_THREADING_SPINLOCK_HPP

#include <atomic>
#include <limits>
#include <emmintrin.h>
#include <util.hpp>
#include <debug.hpp>
#include "thread.hpp"

namespace jolt {
    namespace threading {
        /**
         * A lock that waits by letting the processor spin - being active while performing no useful
         * work.
         *
         * @remarks Busy-waiting can degrade performance and spin locks should not be used unless
         * the waiting time is assured to be very small.
         */
        class SpinLock {
          public:
            using lock_state_type = bool;
            using retry_amt_type = unsigned;

          private:
            volatile std::atomic<lock_state_type> m_lock;
            volatile thread_id m_owner; /**< ID of owner thread when lock is held */

          public:
            static constexpr lock_state_type ACQUIRED = true;
            static constexpr lock_state_type RELEASED = false;

            constexpr JLT_INLINE SpinLock() noexcept :
              m_lock{RELEASED}, m_owner{INVALID_THREAD_ID} {}

            SpinLock(const SpinLock &other) = delete;
            SpinLock(SpinLock &&other) = delete;
            SpinLock &operator=(const SpinLock &other) = delete;

            /**
             * Try to acquire a spin lock and give up after `max_retries`.
             *
             * @param max_retries The maximum number of times to try to acquire the lock.
             *
             * @return True if the lock has been acquired, false if it hasn't.
             */
            constexpr bool try_acquire(retry_amt_type max_retries) volatile noexcept {
                jltassert(max_retries > 0);

                for(bool lock_state = RELEASED; !m_lock.compare_exchange_weak(
                      lock_state, ACQUIRED, std::memory_order_release, std::memory_order_acquire);
                    lock_state = RELEASED) {
                    _mm_pause();
                    --max_retries;

                    if(!max_retries) {
                        return false;
                    }
                }

                m_owner = Thread::get_current().get_id();

                return true;
            }

            /**
             * Wait indefinitely until the lock has been acquired.
             *
             * @remarks This is not very efficient as this method will busy-wait for a possibly long
             * amount of time.
             */
            JLT_INLINE void acquire() volatile noexcept {
                jltassert(m_owner != Thread::get_current().get_id());

                while(!try_acquire(std::numeric_limits<retry_amt_type>::max()))
                    ; // Wait indefinitely
            }

            /**
             * Release the lock.
             */
            JLT_INLINE void release() volatile noexcept {
                jltassert(m_owner == Thread::get_current().get_id());
                m_owner = INVALID_THREAD_ID;
                m_lock.store(RELEASED, std::memory_order_release);
            }

            /**
             * Return a value stating whether this lock has been acquired.
             */
            JLT_INLINE bool is_acquired() volatile noexcept {
                return m_lock.load(std::memory_order_acquire);
            }
        };
    } // namespace threading
} // namespace jolt

#endif /* JLT_THREADING_SPINLOCK_HPP */
