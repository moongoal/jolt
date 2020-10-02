#include <cstdlib>
#include <jolt/debug.hpp>
#include <jolt/collections/array.hpp>
#include "vulkan.hpp"

namespace jolt {
    namespace graphics {
        void VulkanFence::reset() {
            jltassert2(m_fence != VK_NULL_HANDLE, "Trying to reset an invalid fence");
            VkResult result = vkResetFences(m_renderer.get_device(), 1, &m_fence);
            jltassert2(result == VK_SUCCESS, "Unable to reset fence");
        }

        void VulkanFence::reset_multiple(VulkanFence const *const fences, uint32_t const n) {
            collections::Array<VkFence> raw_fences{n};

            for(size_t i = 0; i < n; ++i) {
                raw_fences[i] = fences[i].get_fence();
                jltassert2(raw_fences[i] != VK_NULL_HANDLE, "Trying to reset an invalid fence");
            }

            VkResult result = vkResetFences(fences->get_renderer().get_device(), n, raw_fences);
            jltassert2(result == VK_SUCCESS, "Unable to reset multiple fences");
        }

        bool VulkanFence::wait(uint64_t const timeout) const {
            jltassert2(m_fence != VK_NULL_HANDLE, "Trying to query an invalid fence");

            VkResult result =
              vkWaitForFences(m_renderer.get_device(), 1, &m_fence, VK_TRUE, timeout);

            switch(result) {
            case VK_SUCCESS: return true;

            case VK_TIMEOUT: return false;

            default: console.err("Error waiting for fence"); abort();
            }
        }

        bool VulkanFence::wait_multiple(
          VulkanFence *fences, uint32_t n, uint64_t const timeout, bool const all) {
            collections::Array<VkFence> raw_fences{n};

            for(size_t i = 0; i < n; ++i) {
                raw_fences[i] = fences[i].get_fence();
                jltassert2(raw_fences[i] != VK_NULL_HANDLE, "Trying to query an invalid fence");
            }

            VkResult result =
              vkWaitForFences(fences->get_renderer().get_device(), 1, raw_fences, all, timeout);

            switch(result) {
            case VK_SUCCESS: return true;

            case VK_TIMEOUT: return false;

            default: console.err("Error waiting for multiple fences"); abort();
            }
        }

        void VulkanFence::create(bool const signaled) {
            VkFenceCreateInfo cinfo{
              VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,                                         // sType
              nullptr,                                                                     // pNext
              signaled ? VK_FENCE_CREATE_SIGNALED_BIT : static_cast<VkFenceCreateFlags>(0) // flags
            };

            VkResult result =
              vkCreateFence(m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_fence);
            jltassert2(result == VK_SUCCESS, "Unable to create fence");
        }

        void VulkanFence::dispose() {
            if(m_fence) {
                vkDestroyFence(m_renderer.get_device(), m_fence, get_vulkan_allocator());
                m_fence = VK_NULL_HANDLE;
            }
        }

        uint64_t VulkanSemaphore::get_counter() const {
            uint64_t value;

            VkResult result =
              vkGetSemaphoreCounterValue(m_renderer.get_device(), m_semaphore, &value);
            jltvkcheck(result, "Unable to get semaphore counter value");

            return value;
        }

        uint64_t VulkanSemaphore::wait(uint64_t timeout) const {
            jltassert2(m_semaphore != VK_NULL_HANDLE, "Trying to query an invalid semaphore");
            uint64_t counter;

            VkSemaphoreWaitInfo winfo{
              VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, // sType
              nullptr,                               // pNext
              0,                                     // flags
              1,                                     // semaphoreCount
              &m_semaphore,                          // pSemaphores
              &counter                               // pValues
            };

            VkResult result = vkWaitSemaphores(m_renderer.get_device(), &winfo, timeout);
            jltvkcheck(result, "Error waiting for semaphore");

            return counter;
        }

        void VulkanSemaphore::wait_multiple(
          VulkanSemaphore const *const semaphores,
          uint32_t const n,
          uint64_t *const out_counters,
          uint64_t const timeout,
          bool const all) {
            collections::Array<VkSemaphore> raw_semaphores{n};

            for(size_t i = 0; i < n; ++i) {
                raw_semaphores[i] = semaphores[i].get_semaphore();
                jltassert2(
                  raw_semaphores[i] != VK_NULL_HANDLE, "Trying to query an invalid semaphore");
            }

            VkSemaphoreWaitInfo winfo{
              VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,                                  // sType
              nullptr,                                                                // pNext
              all ? static_cast<VkSemaphoreWaitFlags>(0) : VK_SEMAPHORE_WAIT_ANY_BIT, // flags
              n,              // semaphoreCount
              raw_semaphores, // pSemaphores
              out_counters    // pValues
            };

            VkResult result =
              vkWaitSemaphores(semaphores->get_renderer().get_device(), &winfo, timeout);

            switch(result) {
            case VK_SUCCESS: break;
            case VK_ERROR_SURFACE_LOST_KHR:
            case VK_ERROR_DEVICE_LOST: semaphores->get_renderer().signal_lost(); break;
            default: jolt ::console.err("Error waiting for multiple semaphores"); abort();
            }
        }

        void VulkanSemaphore::create(VkSemaphoreType const sem_type, uint64_t const value) {
            VkSemaphoreTypeCreateInfo tcinfo{
              VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, // sType
              nullptr,                                      // pNext
              sem_type,                                     // semaphoreType
              value                                         // value
            };

            VkSemaphoreCreateInfo cinfo{
              VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, // sType
              &tcinfo,                                 // pNext
              0                                        // flags
            };

            VkResult result = vkCreateSemaphore(
              m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_semaphore);
            jltassert2(result == VK_SUCCESS, "Unable to create semaphore");
        }

        void VulkanSemaphore::dispose() {
            if(m_semaphore) {
                vkDestroySemaphore(m_renderer.get_device(), m_semaphore, get_vulkan_allocator());
                m_semaphore = VK_NULL_HANDLE;
            }
        }

        void VulkanSemaphore::signal(uint64_t value) {
            jltassert2(m_semaphore != VK_NULL_HANDLE, "Attempting to signal an invalid semaphore");
            jltassert2(
              m_type == VK_SEMAPHORE_TYPE_TIMELINE,
              "Attempting to signal a non-timeline semaphore");

            VkSemaphoreSignalInfo sinfo{
              VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO, // sType
              nullptr,                                 // pNext
              m_semaphore,                             // semaphore
              value                                    // value
            };

            VkResult result = vkSignalSemaphore(m_renderer.get_device(), &sinfo);
            jltassert2(result == VK_SUCCESS, "Unable to signal semaphore");
        }
    } // namespace graphics
} // namespace jolt
