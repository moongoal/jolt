#include <jolt/features.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/debug.hpp>
#include "vfs.hpp"

using namespace jolt::path;

#ifdef _DEBUG
    #define BUILD_DRIVER_PATH "/build"
#endif // _DEBUG

namespace jolt {
    namespace vfs {
        VirtualFileSystem::VirtualFileSystem() {
#ifdef _DEBUG
            m_driver_build = memory::allocate<FSDriver>(1, memory::ALLOC_PERSIST);
            memory::construct(m_driver_build, BUILD_DRIVER_PATH, JLT_BUILD_DIR);

            mount(BUILD_DRIVER_PATH, *m_driver_build);
#endif // _DEBUG
        }

        VirtualFileSystem::VirtualFileSystem(VirtualFileSystem &&other) :
          m_mounts{std::move(other.m_mounts)}, m_driver_build{other.m_driver_build} {
            other.m_driver_build = nullptr;
        }

        VirtualFileSystem::~VirtualFileSystem() {
            unmount_all();

#ifdef _DEBUG
            if(m_driver_build) {
                jltfree(m_driver_build);
            }
#endif // _DEBUG
        }

        void VirtualFileSystem::mount(Path const &path, Driver &driver) {
            jltassert2(!is_mount_point(path), "Mount point already registered");

            m_mounts.add(path, &driver);
        }

        void VirtualFileSystem::unmount(Path const &path) { m_mounts.remove(path); }

        void VirtualFileSystem::unmount_all() { m_mounts.clear(); }

        bool VirtualFileSystem::is_mount_point(Path const &path) const { return m_mounts.contains_key(path); }

        io::Stream *VirtualFileSystem::open(
          JLT_MAYBE_UNUSED const Path &path, JLT_MAYBE_UNUSED io::ModeFlags const mode) {
            Driver *driver = get_path_driver(path);

            if(driver) {
                io::Stream *stream = driver->open(path, mode);

                if(stream && !stream->has_error()) {
                    return stream;
                }
            }

            return nullptr;
        }

        Driver *VirtualFileSystem::get_path_driver(Path const &path) const {
            for(auto const &[key, value] : m_mounts) {
                if(path.starts_with(key)) {
                    return value;
                }
            }

            return nullptr;
        }

        Driver::file_name_vec VirtualFileSystem::list_all() const {
            auto it = m_mounts.begin();
            auto const it_end = m_mounts.end();

            Driver::file_name_vec files{(*it).get_value()->list()};

            for(++it; it != it_end; ++it) {
                Driver::file_name_vec const driver_files{(*it).get_value()->list()};

                files.push_all(driver_files.begin(), driver_files.end());
            }

            return files;
        }

        Driver::file_name_vec VirtualFileSystem::list(Path const &path, bool const recurse) const {
            Driver *driver = get_path_driver(path);
            Driver::file_name_vec files = path ? driver->list(path, recurse) : Driver::file_name_vec{};

            return files;
        }
    } // namespace vfs
} // namespace jolt
