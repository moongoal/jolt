#ifndef JLT_JOLT_HPP
#define JLT_JOLT_HPP

#include <jolt/api.hpp>
#include <jolt/util.hpp>
#include <jolt/path.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/debug/console.hpp>
#include <jolt/vfs/vfs.hpp>
#include <jolt/ui/window.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    typedef void (*jlt_loop_func_t)(graphics::vulkan::Renderer &renderer);

    void JLTAPI initialize();
    void JLTAPI shutdown();
    void JLTAPI
    main_loop(graphics::vulkan::GraphicsEngineInitializationParams &gparams, jlt_loop_func_t loop_func);
} // namespace jolt

#endif /* JLT_JOLT_HPP */
