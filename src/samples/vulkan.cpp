#include <jolt/jolt.hpp>

using namespace jolt;

int main(int argc, char **argv) {
    graphics::GraphicsEngineInitializationParams gparams{0};

    gparams.app_name = "Vulkan test";
    
    initialize();
    console.set_output_stream(&io::standard_error_stream);
    graphics::initialize(gparams);

    graphics::shutdown();
    shutdown();
}
