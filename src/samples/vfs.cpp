#include <jolt/features.hpp>
#include <jolt/jolt.hpp>

using namespace jolt;
using namespace jolt::text;
using namespace jolt::path;
using namespace jolt::vfs;

void list_files(VirtualFileSystem &vfs) {
    console.echo("Listing all files in " JLT_BUILD_VDIR "...");

    Driver::file_name_vec files = vfs.list_all();

    for(auto f : files) { console.echo(s("\t") + f); }
}

void check_shader_magic(VirtualFileSystem &vfs) {
    Path const shader_path = "/build/src/shaders/fragment/red.frag.spv";
    uint8_t buff[sizeof(uint32_t)];

    io::Stream *const shader_stream = vfs.open(shader_path, io::MODE_READ);

    if(shader_stream) {
        shader_stream->read(buff, sizeof(uint32_t));
        shader_stream->close();
        jltfree(shader_stream);

        uint32_t *const shader_magic = reinterpret_cast<uint32_t *>(buff);

        if(*shader_magic == 0x07230203) {
            console.echo("Shader verified");
        } else { // Invalid magic
            console.err("Invalid shader file");
        }
    } else {
        console.warn("Shader file does not exist - did you forget to compile the shaders?");
    }
}

int main(JLT_MAYBE_UNUSED int argc, JLT_MAYBE_UNUSED char **argv) {
    VirtualFileSystem vfs;

    console.set_output_stream(&io::standard_output_stream);

    if(vfs.is_mount_point(JLT_BUILD_VDIR)) {
        console.echo(JLT_BUILD_VDIR " is a mount point");

        list_files(vfs);
        check_shader_magic(vfs);
    } else {
        console.echo(JLT_BUILD_VDIR " is not a mount point - maybe this is not a debug build?");
    }

    return 0;
}
