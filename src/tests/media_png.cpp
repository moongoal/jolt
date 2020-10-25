#include <jolt/test.hpp>
#include <jolt/features.hpp>
#include <jolt/jolt.hpp>
#include <jolt/media/png.hpp>

using namespace jolt;
using namespace jolt::media;

size_t memory_sz;

SETUP {
    initialize();
    memory_sz = memory::get_allocated_size();
}

CLEANUP { shutdown(); }

TEST(load_image_png) {
    io::FileStream stream{JLT_ASSETS_DIR "/images/png-load-test.png", io::Mode::MODE_READ};
    Image img = load_image_png(stream);
    stream.close();

    assert2(img.get_header().image_type == IMAGE_TYPE_2D, "Image type");
    assert2(img.get_header().width == 16, "Width");
    assert2(img.get_header().height == 16, "Height type");
    assert2(img.get_header().depth == 1, "Depth");
    assert2(img.get_data()->r == 10, "Red");
    assert2(img.get_data()->g == 20, "Green");
    assert2(img.get_data()->b == 30, "Blue");
    assert2(img.get_data()->a == 0xff, "Alpha");
}

TEST(memory_leak) { // Must always be last
    assert(memory_sz == memory::get_allocated_size());
}
