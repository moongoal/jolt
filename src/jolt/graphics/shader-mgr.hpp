#ifndef JLT_GRAPHICS_SHADER_MGR_HPP
#define JLT_GRAPHICS_SHADER_MGR_HPP

#include <jolt/api.hpp>
#include <jolt/path.hpp>
#include <jolt/collections/vector.hpp>
#include <jolt/collections/hashmap.hpp>
#include <jolt/vfs/vfs.hpp>

namespace jolt {
    namespace graphics {
        /**
         * A manager for SPIR-V shader data.
         *
         * Each registered shader will be loaded from the VFS and identified by the hash if its virtual path.
         *
         * The hash function used to hash the path is ShaderManager::hash_function.
         */
        class JLTAPI ShaderManager {
          public:
            using hash_function = hash::XXHash; //< The hash function object used to hash the paths.
            using shader_data = collections::Vector<uint8_t>; //< Container of shader raw binary data.
            using shader_table = collections::HashMap<jolt::hash::hash_t, shader_data, jolt::hash::Identity>;

          private:
            vfs::VirtualFileSystem &m_vfs; //< The VFS where to load the shaders from.
            shader_table m_table;          //< The mapping between hash and shader binary data.

          public:
            explicit ShaderManager(vfs::VirtualFileSystem &vfs);

            /**
             * Scan the whole VFS for shaders and register them.
             */
            void scan_shaders();

            /**
             * Register a shader from the VFS given its path.
             *
             * @param path The path to the shader.
             */
            void register_shader(path::Path const &path);

            /**
             * Register multiple shaders. This function will scan all the files in the given vector and only
             * register those ending in '.spv'.
             *
             * @param files A list of files to scan for shaders.
             */
            void register_multiple_shaders(vfs::Driver::file_name_vec const &files);

            /*
             * Return the VFS instance used when creating this shader manager.
             */
            vfs::VirtualFileSystem const &get_vfs() const { return m_vfs; }

            /**
             * Get a shader given its hash.
             */
            shader_data const &get_shader(hash::hash_t id);

            /**
             * Count the number of shaders currently registered.
             */
            size_t get_count() const { return m_table.get_length(); }
        };
    } // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_SHADER_MGR_HPP */
