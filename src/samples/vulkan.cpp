#include <jolt/jolt.hpp>

using namespace jolt;

int main(int argc, char **argv) {
    graphics::GraphicsEngineInitializationParams gparams{0};

    initialize();
    console.set_output_stream(&io::standard_error_stream);
    ui::Window wnd{"Vulkan initialization"};

    gparams.app_name = "Vulkan initialization";
    gparams.wnd = &wnd;

    //wnd.show();

    //for(size_t i = 0; i < 60; i++) { ui::Window::cycle(); }

    graphics::initialize(gparams);

    graphics::shutdown();
    shutdown();
}
