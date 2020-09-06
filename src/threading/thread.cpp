#include <Windows.h>
#include "thread.hpp"

namespace jolt {
    namespace threading {
        static Thread g_main_thread(::GetCurrentThreadId());
        static thread_local Thread *t_current_thread;

        Thread::Thread(thread_id id) : m_id(id) {}

        Thread &Thread::current() { return *t_current_thread; }

        void initialize() { t_current_thread = &g_main_thread; }
    } // namespace threading
} // namespace jolt
