#include <test.hpp>
#include <memory/allocator.hpp>
#include <threading/thread.hpp>
#include <text/stringbuilder.hpp>

using namespace jolt::text;

size_t g_used_memory;

SETUP {
    jolt::threading::initialize();
    g_used_memory = jolt::memory::get_allocated_size();
}

TEST(to_string) {
    StringBuilder sb{u8"First "};
    String s = u8"First Second Third";

    sb.add(u8"Second ");
    sb.add(u8"Third");

    assert(sb.to_string() == s);
}

TEST(memory_leaks) { // Must be last test
    assert(g_used_memory == jolt::memory::get_allocated_size());
}
