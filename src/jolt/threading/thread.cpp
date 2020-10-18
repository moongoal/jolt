#include <Windows.h>
#include <jolt/debug.hpp>
#include "thread.hpp"

namespace {
    extern "C" DWORD WINAPI start_new_thread(LPVOID thread_ptr) {
        jolt::threading::Thread::start_new_thread(*reinterpret_cast<jolt::threading::Thread *>(thread_ptr));

        return (DWORD)0;
    }
} // namespace

namespace jolt {
    namespace threading {
        const char *const UNNAMED_THREAD_NAME = "Unnamed thread";

        static Thread g_main_thread(::GetCurrentThreadId(), ThreadState::Running, "Main");
        static thread_local Thread *t_current_thread = &g_main_thread;
        volatile std::atomic<thread_id> Thread::s_next_id = 0;

        Thread::Thread(thread_id os_id, ThreadState const state, const char *const thread_name) :
          m_id{s_next_id++}, m_os_id{os_id}, m_name{thread_name}, m_param{nullptr}, m_state{state},
          m_handler{nullptr} {}

        Thread &Thread::get_current() { return *t_current_thread; }

        void Thread::start_new_thread(Thread &t) {
            thread_id tid = t.get_id();
            jltassert(tid != INVALID_THREAD_ID);

            t_current_thread = &t;

            t.m_handler(const_cast<void *>(t.m_param));
            t.m_state.store(ThreadState::Terminated, std::memory_order_release);
        }

        void Thread::start(void *param) {
            jltassert(m_state.load(std::memory_order_acquire) == ThreadState::Created);

            m_param = param;
            HANDLE thandle = CreateThread(NULL, 0, &::start_new_thread, this, NULL, NULL);
            jltassert(thandle != NULL);

            m_os_id.store(GetThreadId(thandle), std::memory_order_release);

            m_win_handle = thandle;
            m_state.store(ThreadState::Running, std::memory_order_release);
        }

        void Thread::join() {
            while(true) {
                if(try_join(std::numeric_limits<unsigned>::max())) {
                    break;
                }
            }
        }

        bool Thread::try_join(unsigned const timeout_ms) {
            thread_id tid = m_id.load(std::memory_order_acquire);

            jltassert(tid != get_current().get_id() && tid != INVALID_THREAD_ID);

            ThreadState const state = m_state.load(std::memory_order_acquire);

            if(state == ThreadState::Terminated) {
                return true;
            }

            jltassert(state == ThreadState::Running);
            jltassert(timeout_ms <= std::numeric_limits<DWORD>::max());

            DWORD result = WaitForSingleObject(m_win_handle, (DWORD)timeout_ms);

            jltassert(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT);

            return result == WAIT_OBJECT_0;
        }

        unsigned get_available_processor_count() {
            return (unsigned)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
        }

        uint64_t get_process_affinity_mask() {
            DWORD_PTR proc_affinity, sys_affinity;

            BOOL result =
              (uint64_t)GetProcessAffinityMask(GetCurrentProcess(), &proc_affinity, &sys_affinity);

            jltassert(result);

            return (uint64_t)proc_affinity;
        }

        void Thread::set_affinity(uint64_t mask) const {
            DWORD_PTR old_affinity_mask = SetThreadAffinityMask(m_win_handle, (DWORD_PTR)mask);

            jltassert(old_affinity_mask);
        }

        void initialize() {}

        void sleep(size_t duration_ms) {
            jltassert(duration_ms <= std::numeric_limits<os_thread_id>::max());

            ::Sleep((os_thread_id)duration_ms);
        }
    } // namespace threading
} // namespace jolt
