#include <utility>
#include <jolt/debug.hpp>
#include "shader-mgr.hpp"

namespace jolt {
    namespace graphics {
        ShaderManager::ShaderManager(vfs::VirtualFileSystem &vfs) : m_vfs{vfs} {}

        void ShaderManager::scan_shaders() {
            vfs::Driver::file_name_vec files = m_vfs.list_all();

            register_multiple_shaders(files);
        }

        void ShaderManager::register_shader(path::Path const &path) {
            io::Stream *stream = m_vfs.open(path);

            jltassert2(stream, "Unable to open shader");
            jltassert2(stream->supports_size(), "Unable to determine shader's size");

            size_t const sz = stream->get_size();
            shader_data data{sz, shader_data::cap_ctor};

            data.set_length(sz);
            stream->read(&data[0], sz);

            stream->close();
            jltfree(stream);

            m_table.add(path.hash<hash_function>(), data);
        }

        void ShaderManager::register_multiple_shaders(vfs::Driver::file_name_vec const &files) {
            for(auto const &path : files) {
                if(path.ends_with(".spv")) {
                    register_shader(path);
                }
            }
        }

        ShaderManager::shader_data const &ShaderManager::get_shader(hash::hash_t id) {
            shader_data const *const data = m_table.get_value(id);

            jltassert2(data, "Shader not found");

            return *data;
        }
    } // namespace graphics
} // namespace jolt
