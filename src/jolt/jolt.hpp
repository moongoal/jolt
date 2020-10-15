#ifndef JLT_JOLT_HPP
#define JLT_JOLT_HPP

#include <jolt/api.hpp>
#include <jolt/util.hpp>
#include <jolt/debug/console.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/ui/window.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    void JLTAPI initialize();
    void JLTAPI shutdown();
} // namespace jolt

#endif /* JLT_JOLT_HPP */
