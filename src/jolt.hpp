#ifndef JLT_JOLT_HPP
#define JLT_JOLT_HPP

#include <util.hpp>
#include <debug/console.hpp>
#include <memory/allocator.hpp>
#include <threading/thread.hpp>
#include <ui/window.hpp>

namespace jolt {
    void JLTAPI initialize();
    void JLTAPI shutdown();
} // namespace jolt

#endif /* JLT_JOLT_HPP */
