#ifndef JLT_GRAPHICS_DEFS_HPP
#define JLT_GRAPHICS_DEFS_HPP

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR

    #include <Windows.h>
#else
    #error OS not supported.
#endif // _WIN32

/**
 * The maximum number of semaphores that can be operated on simultaneously.
 */
#define JLT_MAX_SEMAPHORES 16

/**
 * The optimal number of swapchain images to request.
 */
#define JLT_OPTIMAL_SWAPCHAIN_IMAGE_COUNT 3

#include <cstdint>
#include <limits>
#include <vulkan/vulkan.h>
#include <jolt/util.hpp>

#define JLT_VULKAN_INVALID32 static_cast<uint32_t>(std::numeric_limits<uint32_t>::max())
#define JLT_VULKAN_INVALID64 static_cast<uint64_t>(std::numeric_limits<uint64_t>::max())

#endif /* JLT_GRAPHICS_DEFS_HPP */
