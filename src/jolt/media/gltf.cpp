#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <jolt/debug.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/collections/array.hpp>
#include "gltf.hpp"

using namespace jolt;

static void *mem_alloc(JLT_MAYBE_UNUSED void *user, cgltf_size size) {
    return memory::allocate_array<uint8_t>(size);
}

static void mem_free(JLT_MAYBE_UNUSED void *user, void *ptr) {
    memory::free_array(reinterpret_cast<uint8_t *>(ptr));
}

namespace jolt::media {
    void load_model_gltf(io::Stream &stream) {
        cgltf_data *data = nullptr;
        cgltf_options options{};

        options.type = cgltf_file_type::cgltf_file_type_gltf;
        options.memory.alloc = mem_alloc;
        options.memory.free = mem_free;

        collections::Array<uint8_t> raw_data{static_cast<size_t>(stream.get_size())};
        stream.read(raw_data, stream.get_size());

        cgltf_result result = cgltf_parse(&options, raw_data, raw_data.get_length(), &data);
        jltassert2(result != cgltf_result_success, "Unable to load GLTF data");

        // ...

        cgltf_free(data);
    }
} // namespace jolt::media
