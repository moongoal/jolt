#include <jolt/test.hpp>
#include <jolt/jolt.hpp>

using namespace jolt;
using namespace jolt::graphics;

constexpr size_t const Nobjs = 128; //< Object number.
constexpr size_t const Sobjs = 128; //< Object size.

using TestPool = VulkanObjectPool<Sobjs, Nobjs>;

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
    TestPool pool{renderer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    assert2(!pool.is_full(), "Should not be full");
    assert2(pool.get_bitmap().get_length() == 2, "Bitmap slot list not length 2");
}

TEST(allocate) {
    TestPool pool{renderer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    VkDeviceSize const alloc1 = pool.allocate();
    VkDeviceSize const alloc2 = pool.allocate();

    assert2(!pool.is_full(), "Should not be full");
    assert2(pool.get_bitmap().get_length() == 2, "Bitmap slot list not length 2");
    assert2(alloc1 != alloc2, "Same allocation returned twice");
    assert2(alloc1 != JLT_VULKAN_INVALIDSZ, "Invalid alloc");
    assert2(JLT_VULKAN_INVALIDSZ != alloc2, "Invalid alloc");
}

TEST(allocate__invalid) {
    VulkanObjectPool<Sobjs, 1> pool{renderer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    assert2(!pool.is_full(), "Should not be full");

    VkDeviceSize const alloc1 = pool.allocate();

    assert2(pool.is_full(), "Should be full");

    VkDeviceSize const alloc2 = pool.allocate();

    assert2(alloc1 != JLT_VULKAN_INVALIDSZ, "Allocation should be valid");
    assert2(alloc2 == JLT_VULKAN_INVALIDSZ, "Allocation should be invalid");
}

TEST(free) {
    VulkanObjectPool<Sobjs, 1> pool{renderer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    assert2(!pool.is_full(), "Should not be full");

    pool.free(pool.allocate());

    assert2(!pool.is_full(), "Should not be full 2");

    VkDeviceSize const alloc = pool.allocate();

    assert2(pool.is_full(), "Should be full");
    assert2(alloc != JLT_VULKAN_INVALIDSZ, "Should not be invalid");
}
