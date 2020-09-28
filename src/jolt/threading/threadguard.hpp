#ifndef JLT_THREADING_THREADGUARD_HPP
#define JLT_THREADING_THREADGUARD_HPP

#include "thread.hpp"

namespace jolt {
    namespace threading {
        /**
         * Thread guard to prevent a thread object from being destructed without being joined first.
         */
        template<typename T>
        class ThreadGuard {
          public:
            using thread_type = T;
            using thread_reference = T &;

          private:
            T &m_t; /**< The guarded thread */

          public:
            ThreadGuard(thread_reference t) noexcept : m_t{t} {}
            ~ThreadGuard() noexcept { m_t.join(); }
        };
    } // namespace threading
} // namespace jolt

#endif /* JLT_THREADING_THREADGUARD_HPP */
