#ifndef JLT_VFS_VFS_HPP
#define JLT_VFS_VFS_HPP

#include <jolt/util.hpp>
#include <jolt/hash.hpp>
#include <jolt/path.hpp>
#include <jolt/text/string.hpp>
#include <jolt/collections/hashmap.hpp>
#include "driver.hpp"
#include "fs-driver.hpp"

namespace jolt {
    namespace vfs {
        /**
         * A virtual file system implementation supporting multiple drivers.
         */
        class JLTAPI VirtualFileSystem {
          public:
            using mp_table =
              collections::HashMap<path::Path, Driver *, hash::ObjectHash<>>; //< Mount point table.

          private:
            mp_table m_mounts; //< Table of active mount points.

#ifdef _DEBUG
            jolt::vfs::FSDriver *m_driver_build; //< The default-mounted /build driver.
#endif

            Driver *get_path_driver(path::Path const &path) const;

          public:
            /**
             * Initialize a new VFS instance.
             *
             * @remarks Debug builds all have a /build directory automatically mounted upon creation.
             */
            VirtualFileSystem();
            VirtualFileSystem(VirtualFileSystem const &) = delete;
            VirtualFileSystem(VirtualFileSystem &&other);

            /**
             * Destroy a VFS instance.
             *
             * @remarks All mount points are automatically unmounted by calling `unmount_all()`.
             */
            ~VirtualFileSystem();

            /**
             * Create a new mount point.
             *
             * @param path The virtual path where to create the mount point.
             * @param driver The driver to mount.
             */
            void mount(path::Path const &path, Driver &driver);

            /**
             * Remove a mount point.
             *
             * @param path The mount point path to remove.
             */
            void unmount(path::Path const &path);

            /**
             * Remove all mount points.
             */
            void unmount_all();

            /**
             * Return a boolean value stating whether a path corresponds to a mount point.
             *
             * @param path The path to test.
             *
             * @return True if the given path is a mount point, false if not.
             */
            bool is_mount_point(path::Path const &path) const;

            /**
             * Open a stream from a file in the VFS.
             *
             * @param path The virtual path to the file.
             * @param mode Stream mode flags.
             */
            io::Stream *open(path::Path const &path, io::ModeFlags const mode = io::MODE_READ);

            /**
             * List all the files in the VFS.
             */
            Driver::file_name_vec list_all() const;

            /**
             * List all the files in a virtual folder.
             */
            Driver::file_name_vec list(path::Path const &path, bool const recurse = false) const;
        };
    } // namespace vfs
} // namespace jolt

#endif /* JLT_VFS_VFS_HPP */
