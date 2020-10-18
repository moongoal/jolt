#include <jolt/jolt.hpp>
#include <jolt/io/stream.hpp>

using namespace jolt;
using namespace jolt::ui;

int main(JLT_MAYBE_UNUSED int argc, JLT_MAYBE_UNUSED char **argv) {
    jolt::initialize();

    console = debug::Console{nullptr, &io::standard_output_stream};
    Window wnd("Test window");

    wnd.show();

    while(Window::cycle()) { jolt::threading::sleep(50); }

    wnd.close();
    jolt::shutdown();

    return 0;
}
