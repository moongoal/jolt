#ifndef JLT_THREADING_THREAD_HPP
#define JLT_THREADING_THREAD_HPP

#include <cstdint>
#include <limits>
#include <atomic>

#ifdef _WIN32
    #include <Windows.h>
#endif // _WIN32

#include <util.hpp>

namespace jolt {
    namespace threading {
        using thread_id = uint32_t;
        using os_thread_id = DWORD;
        constexpr thread_id INVALID_THREAD_ID = std::numeric_limits<thread_id>::max();
        constexpr os_thread_id INVALID_OS_THREAD_ID = 0;
        extern JLTAPI const char *const UNNAMED_THREAD_NAME;

        /**
         * Thread handler pointer type.
         *
         * This implementation-independent handler type represents
         * a function, entry-point for a thread. It accepts a parameter that will be passed through
         * the `Thread::start()` member function.
         */
        using thread_handler_ptr = void (*)(void *param);

        /** Thread state. */
        enum class ThreadState {
            /** The thread **object** (not necessarily the underlying thread) has been created but
             * has not been run.
             */
            Created,

            /** The thread is currently executing. */
            Running,

            /** The thread has been terminated. */
            Terminated,

            /** The thread object is in an invalid state. */
            Invalid
        };

        class JLTAPI Thread {
            static volatile std::atomic<thread_id> s_next_id; //< Next available internal ID

            volatile std::atomic<thread_id> m_id;       //< Internal thread ID
            volatile std::atomic<os_thread_id> m_os_id; //< OS thread ID
            volatile const char *m_name;                /**< The current thread name */
            volatile void *m_param;                     /**< Current param */
            volatile std::atomic<ThreadState> m_state;  /**< State of the thread object */
            volatile thread_handler_ptr m_handler; /**< Pointer to the thread starting function */

#ifdef _WIN32
            volatile HANDLE m_win_handle; /**< Windows handle to the thread */
#endif                                    // _WIN32

          public:
            /**
             * Initialize a new instance of this class.
             *
             * @param os_id The thread's OS ID.
             * @param state The thread's state.
             * @param thread_name A string containing the thread's name.
             */
            Thread(
              thread_id os_id,
              ThreadState const state,
              const char *const thread_name = UNNAMED_THREAD_NAME);

            Thread(Thread &&other) :
              m_id{other.m_id.load(std::memory_order_acquire)},
              m_os_id{other.m_os_id.load(std::memory_order_acquire)}, m_handler{std::move(
                                                                        other.m_handler)},
              m_state{other.m_state.load(std::memory_order_acquire)}, m_param{std::move(
                                                                        other.m_param)},
              m_win_handle{std::move(other.m_win_handle)}, m_name{std::move(other.m_name)} {
                other.m_id.store(INVALID_THREAD_ID, std::memory_order_release);
                other.m_state.store(ThreadState::Invalid, std::memory_order_release);
            }

            /**
             * Initialize a new instance of this class.
             *
             * @param handler The thread handler
             */
            explicit Thread(
              thread_handler_ptr const handler,
              const char *const thread_name = UNNAMED_THREAD_NAME) noexcept :
              m_id{s_next_id++},
              m_os_id{INVALID_OS_THREAD_ID}, m_handler{handler}, m_state{ThreadState::Created},
              m_param{nullptr}, m_win_handle{NULL}, m_name{thread_name} {}

            Thread(Thread &other) = delete;
            Thread &operator=(Thread &other) = delete;

            /**
             * Get the thread ID as assigned by the threading system.
             *
             * @return The thread ID or `INVALID_THREAD_ID`. The main thread has always ID 0.
             */
            thread_id get_id() const { return m_id.load(std::memory_order_acquire); }

            /**
             * Get the OS thread ID as assigned by the threading system.
             *
             * @return The OS thread ID or `INVALID_OS_THREAD_ID`.
             */
            os_thread_id get_os_id() const { return m_os_id.load(std::memory_order_acquire); }

            /** Get the thread handler. */
            thread_handler_ptr get_handler() const { return m_handler; }

            /** Get the thread state. */
            ThreadState get_state() const { return m_state.load(std::memory_order_acquire); }

            /**
             * Start the thread.
             *
             * @param param The optional parameter to pass to the newly created thread's function.
             */
            void start(void *param);

            /** Wait for the thread to terminate. */
            void join();

            /** Wait for the thread to terminate.
             *
             * @param timeout_ms The timeout (in milliseconds) to wait before giving up.
             *
             * @return True if the thread has been joined, false if the function has given up.
             */
            bool try_join(unsigned const timeout_ms);

            /**
             * Set the affinity mask for the thread.
             *
             * @param mask An affinity mask where a 0 bit indicates the thread should not run on the
             * given processor and 1 indicates it should.
             */
            void set_affinity(uint64_t mask) const;

            /**
             * Get the name of the thread.
             *
             * @return A string containing the name of the thread or `UNNAMED_THREAD_NAME` if the
             * thread has not been given a name.
             */
            volatile const char *get_name() const { return m_name; }

            /** Return the current thread. */
            static Thread &get_current();

            /**
             * Thread start service function.
             *
             * @param t The thread to run
             *
             * @remarks This is an internal method and is not intended to be used outside of the
             * threading library.
             */
            static void start_new_thread(Thread &t);
        };

        /**
         * Initialize the threading system.
         */
        void JLTAPI initialize();

        /**
         * Make the current thread sleep for a certain amount of ms.
         *
         * @param duration_ms The minimum amount of time to wait in ms.
         */
        void JLTAPI sleep(size_t duration_ms);

        /**
         * Get the number of available logical processors on the machine.
         *
         * @return The number of available processors.
         */
        JLTAPI unsigned get_available_processor_count();

        /**
         * Get the processor affinity mask for the current process.
         *
         * @return The affinity mask of the currently running process.
         */
        JLTAPI uint64_t get_process_affinity_mask();
    } // namespace threading
} // namespace jolt

#endif /* JLT_THREADING_THREAD_HPP */
