#ifndef JLT_THREADING_LOCKGUARD_HPP
#define JLT_THREADING_LOCKGUARD_HPP

#include <jolt/util.hpp>
#include "lock.hpp"

namespace jolt {
    namespace threading {
        /**
         * A lock guard that ensures a lock is immediately acquired upon guard creation
         * and released upon guard destruction.
         *
         * @tparam T The type of lock.
         */
        template<typename T>
        class LockGuard {
          public:
            using lock_type = T;        /**< Type of lock held */
            using lock_reference = T &; /**< Reference to lock */

          private:
            lock_reference m_lock; /**< Held lock */

          public:
            /**
             * Initialize a new instance of this class.
             *
             * @param lock The lock to guard.
             *
             * @remarks The specified lock is acquired immediately. If the lock is already held by
             * another thread, this guard will wait until the lock is available.
             */
            explicit JLT_INLINE LockGuard(lock_reference lock) noexcept : m_lock{lock} {
                m_lock.acquire();
            }

            LockGuard(LockGuard<lock_type> &other) = delete;
            LockGuard<lock_type> &operator=(LockGuard<lock_type> &other) = delete;

            /**
             * Destroy an existing instance of this class.
             *
             * @remarks The lock is released immediately upon guard destruction.
             */
            JLT_INLINE ~LockGuard() noexcept { m_lock.release(); }
        };
    } // namespace threading
} // namespace jolt

#endif /* JLT_THREADING_LOCKGUARD_HPP */
