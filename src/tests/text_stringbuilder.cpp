#include <jolt/test.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/text/stringbuilder.hpp>

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
