#ifndef JLT_GRAPHICS_DEFS_HPP
#define JLT_GRAPHICS_DEFS_HPP

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#else
    #error OS not supported.
#endif // _WIN32

#include <cstdint>
#include <limits>
#include <Windows.h>
#include <vulkan/vulkan.h>

#endif /* JLT_GRAPHICS_DEFS_HPP */
