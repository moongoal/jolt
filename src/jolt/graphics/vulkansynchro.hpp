#ifndef JLT_GRAPHICS_VULKANSYNCHRO_HPP
#define JLT_GRAPHICS_VULKANSYNCHRO_HPP

#include "defs.hpp"

namespace jolt {
    namespace graphics {
        class VulkanRenderer;

        constexpr const uint64_t SYNCHRO_WAIT_MAX = std::numeric_limits<uint64_t>::max();

        class JLTAPI VulkanFence {
            VulkanRenderer const &m_renderer;
            VkFence m_fence = VK_NULL_HANDLE;

            void create(bool const signaled);

          public:
            /**
             * Acquire an externally obtained fence.
             *
             * @param renderer The renderer the fence was produced from.
             * @param fence The fence.
             */
            VulkanFence(VulkanRenderer const &renderer, VkFence fence) :
              m_renderer{renderer}, m_fence{fence} {}

            /**
             * Create a new fence.
             *
             * @param renderer The renderer.
             * @param signaled A value stating whether to initialise the fence to the signaled
             * state.
             */
            explicit VulkanFence(VulkanRenderer const &renderer, bool const signaled = false) :
              m_renderer{renderer} {
                create(signaled);
            }

            VulkanFence(const VulkanFence &other) = delete;

            ~VulkanFence() { dispose(); }

            /**
             * Reset the fence to the not signeled status.
             */
            void reset();

            /**
             * Destroy the fence. This is automatically called by the destructor.
             */
            void dispose();

            /**
             * Wait for a fence to be signaled.
             *
             * @param timeout The amount of time (in nanoseconds) to wait for the fence before
             * giving up. Set to 0 to poll and return immediately.
             *
             * @return True if the fence is signalled, false if it's not.
             */
            bool wait(uint64_t const timeout = 0) const;

            /**
             * An alias for wait(0).
             *
             * @see wait().
             */
            bool is_signaled() const { return wait(0); }

            VulkanRenderer const &get_renderer() const { return m_renderer; }
            VkFence get_fence() const { return m_fence; }

            /**
             * Same as `reset()` but for multiple fences.
             *
             * @see reset().
             */
            static void reset_multiple(VulkanFence const *const fences, uint32_t const n);

            /**
             * Same as `wait()` but for multiple fences.
             *
             * @param fences The fence array to wait for.
             * @param timeout The amount of time (in nanoseconds) to wait for the fence before
             * giving up. Set to 0 to poll and return immediately.
             * @param all True to wait for all the fences, false to wait for any.
             *
             * @return True if all (any) fences are signalled, false if not.
             */
            static bool wait_multiple(
              VulkanFence *fences,
              uint32_t const n,
              uint64_t const timeout = 0,
              bool const all = true);

            operator VkFence() const { return m_fence; }
        };

        class JLTAPI VulkanSemaphore {
            VulkanRenderer const &m_renderer;
            VkSemaphore m_semaphore = VK_NULL_HANDLE;
            VkSemaphoreType m_type;

            void create(VkSemaphoreType const sem_type, uint64_t const value);

          public:
            /**
             * Acquire an externally obtained semaphore.
             *
             * @param renderer The renderer the semaphore was produced from.
             * @param semaphore The semaphore.
             * @param type The semaphore type.
             */
            VulkanSemaphore(
              VulkanRenderer const &renderer, VkSemaphore semaphore, VkSemaphoreType const type) :
              m_renderer{renderer},
              m_semaphore{semaphore}, m_type{type} {}

            /**
             * Create a new semaphore.
             *
             * @param renderer The renderer.
             * @param sem_type The semaphore type.
             * @param value The value used to initialize a timeline semaphore. Ignored otherwise.
             */
            explicit VulkanSemaphore(
              VulkanRenderer const &renderer,
              VkSemaphoreType const sem_type = VK_SEMAPHORE_TYPE_BINARY,
              uint64_t const value = 0) :
              m_renderer{renderer},
              m_type{sem_type} {
                create(sem_type, value);
            }

            VulkanSemaphore(const VulkanSemaphore &other) = delete;

            ~VulkanSemaphore() { dispose(); }

            VkSemaphoreType get_type() const { return m_type; }

            /**
             * Return the counter value of a semaphore.
             *
             * @remarks This must only be used on counter semaphores.
             */
            uint64_t get_counter() const;

            /**
             * Destroy the semaphore. This is automatically called by the destructor.
             */
            void dispose();

            /**
             * Wait for a semaphore.
             *
             * @param timeout The amount of time (in nanoseconds) to wait for the semaphore before
             * giving up. Set to 0 to poll and return immediately.
             *
             * @return The value of the semaphore counter.
             *
             * @remarks This must only be called on timeline semaphores.
             */
            uint64_t wait(uint64_t const timeout = 0) const;

            /**
             * An alias for wait(0).
             *
             * @see wait().
             */
            bool is_signaled() const { return wait(0); }

            VulkanRenderer const &get_renderer() const { return m_renderer; }
            VkSemaphore get_semaphore() const { return m_semaphore; }

            /**
             * Same as `wait()` but for multiple semaphores.
             *
             * @param semaphores The semaphore array to wait for.
             * @param out_counters An output array of `n` elements that will be populated with the
             * semaphore counters.
             * @param timeout The amount of time (in nanoseconds) to wait for the semaphore before
             * giving up. Set to 0 to poll and return immediately.
             * @param all True to wait for all the semaphores, false to wait for any.
             */
            static void wait_multiple(
              VulkanSemaphore const *const semaphores,
              uint32_t const n,
              uint64_t *const out_counters,
              uint64_t const timeout = 0,
              bool const all = true);

            /**
             * Signal a semaphore.
             *
             * @param value The value to signal.
             */
            void signal(uint64_t value);

            operator VkSemaphore() const { return m_semaphore; }
        };

        struct VulkanWaitSemaphoreActionSynchro {
            uint32_t wait_semaphore_count =
              0; //< Number of elements pointed to by `wait_semaphores`.
            VkSemaphore wait_semaphores[JLT_MAX_SEMAPHORES]; /*< Semaphores to wait before
                                                                performing the action. */
            VkPipelineStageFlags
              wait_semaphores_stages[JLT_MAX_SEMAPHORES]; /*< A bitmask indicating where each
                                                             `wait_semaphore` must be awaited at. */
        };

        struct VulkanSignalSemaphoreActionSynchro {
            uint32_t signal_semaphore_count =
              0; //< Number of elements pointed to by `signalSemaphores`.
            VkSemaphore signal_semaphores[JLT_MAX_SEMAPHORES]; /*< Semaphores to be signeled after
                                                                  the action has been completed. */
        };

        struct VulkanSemaphoreActionSynchro :
          public VulkanWaitSemaphoreActionSynchro,
          public VulkanSignalSemaphoreActionSynchro {};

        struct VulkanActionSynchro : public VulkanSemaphoreActionSynchro {
            VkFence fence =
              VK_NULL_HANDLE; //< The fence to signal after the action has been completed.
        };
    } // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKANSYNCHRO_HPP */
