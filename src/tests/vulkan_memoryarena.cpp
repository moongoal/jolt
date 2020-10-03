#include <jolt/test.hpp>
#include <jolt/jolt.hpp>

using namespace jolt;
using namespace jolt::graphics;

constexpr VkDeviceSize const HEAP_SIZE = 16 * 1024 * 1024;

VulkanRenderer renderer;
ui::Window *ui_window;

GraphicsEngineInitializationParams gparams{"Jolt test", 1, 0, 0, nullptr};

SETUP {
    initialize();

    ui_window = jltnew(ui::Window, "Test window - don't close");

    gparams.wnd = ui_window;
    renderer.initialize(gparams);
}

CLEANUP {
    renderer.shutdown();
    ui_window->close();

    shutdown();
}

TEST(ctor) {
    VulkanArena arena{renderer, HEAP_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    assert2(arena.get_allocated_size() == 0, "Allocated size not 0");
    assert2(arena.get_free_list().get_length() == 1, "Free list not length 1");
    assert2(arena.get_alloc_list().get_length() == 0, "Metadata list not length 0");
}

TEST(allocate) {
    VulkanArena arena{renderer, HEAP_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    VkDeviceSize const alloc1 = arena.allocate(1024, 1);
    assert2(arena.get_allocated_size() == 1024, "Invalid allocated size 1");
    assert2(arena.get_free_list().get_length() == 1, "Free list not length 1");
    assert2(arena.get_alloc_list().get_length() == 1, "Metadata list not length 0");

    VkDeviceSize const alloc2 = arena.allocate(1024, 1);
    assert2(arena.get_allocated_size() == 2048, "Invalid allocated size 2");
    assert2(arena.get_free_list().get_length() == 1, "Free list not length 1");
    assert2(arena.get_alloc_list().get_length() == 2, "Metadata list not length 2");

    arena.free(alloc1);
    assert2(arena.get_allocated_size() == 1024, "Invalid allocated size 3");
    assert2(arena.get_free_list().get_length() == 2, "Free list not length 2");
    assert2(arena.get_alloc_list().get_length() == 1, "Metadata list not length 1");

    VkDeviceSize const alloc3 = arena.allocate(1025, 1);
    assert2(arena.get_allocated_size() == 2049, "Invalid allocated size 3");
    assert2(arena.get_free_list().get_length() == 2, "Free list not length 2");
    assert2(arena.get_alloc_list().get_length() == 2, "Metadata list not length 2");
}

TEST(allocate__invalid) {
    VulkanArena arena{renderer, HEAP_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    VkDeviceSize const alloc1 = arena.allocate(HEAP_SIZE + 1, 1);

    assert(alloc1 == VulkanArena::INVALID_ALLOC);
}

TEST(allocate__free__max) {
    VulkanArena arena{renderer, HEAP_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    VkDeviceSize const alloc1 = arena.allocate(HEAP_SIZE, 1);
    assert2(arena.get_allocated_size() == HEAP_SIZE, "Invalid allocated size 1");
    assert2(arena.get_free_list().get_length() == 0, "Free list not length 1");
    assert2(arena.get_alloc_list().get_length() == 1, "Metadata list not length 0");

    arena.free(alloc1);
    assert2(arena.get_allocated_size() == 0, "Allocated size not 0");
    assert2(arena.get_free_list().get_length() == 1, "Free list not length 1");
    assert2(arena.get_alloc_list().get_length() == 0, "Metadata list not length 0");
}

TEST(free) {
    VulkanArena arena{renderer, HEAP_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    VkDeviceSize const alloc1 = arena.allocate(1024, 1);
    VkDeviceSize const alloc2 = arena.allocate(1024, 1);

    arena.free(alloc1);
    assert2(arena.get_allocated_size() == 1024, "Invalid allocated size 3");
    assert2(arena.get_free_list().get_length() == 2, "Free list not length 2");
    assert2(arena.get_alloc_list().get_length() == 1, "Metadata list not length 1");

    VkDeviceSize const alloc3 = arena.allocate(1025, 1);
    arena.free(alloc3);
    arena.free(alloc2);

    assert2(arena.get_allocated_size() == 0, "Allocated size not 0");
    assert2(arena.get_free_list().get_length() == 1, "Free list not length 1");
    assert2(arena.get_alloc_list().get_length() == 0, "Metadata list not length 0");
}
