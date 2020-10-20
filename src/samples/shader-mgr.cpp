#define _CRT_SECURE_NO_WARNINGS

#include <cstdlib>
#include <jolt/features.hpp>
#include <jolt/jolt.hpp>
#include <jolt/text/stringbuilder.hpp>
#include <jolt/graphics/shader-mgr.hpp>

using namespace jolt;
using namespace jolt::text;
using namespace jolt::path;
using namespace jolt::vfs;

void check_shaders(VirtualFileSystem &vfs) {
    graphics::ShaderManager sm{vfs};
    StringBuilder sb{"Loaded "};

    console.echo("Scanning shaders...");
    sm.scan_shaders();
    size_t const n_shaders = sm.get_count();
    char n_shaders_s[64]{};

    _itoa(static_cast<int>(n_shaders), n_shaders_s, 10);

    sb.add(n_shaders_s);
    sb.add(" shaders.");

    console.info(sb.to_string());
}

int main(JLT_MAYBE_UNUSED int argc, JLT_MAYBE_UNUSED char **argv) {
    VirtualFileSystem vfs;

    console.set_output_stream(&io::standard_output_stream);

    if(vfs.is_mount_point(JLT_BUILD_VDIR)) {
        check_shaders(vfs);
    } else {
        console.echo(JLT_BUILD_VDIR " is not a mount point - maybe this is not a debug build?");
    }

    return 0;
}
