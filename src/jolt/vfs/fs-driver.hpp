#ifndef JLT_VFS_FS_DRIVER_HPP
#define JLT_VFS_FS_DRIVER_HPP

#include "driver.hpp"

namespace jolt {
    namespace vfs {
        /**
         * File system driver.
         *
         * This driver supports reading and writing from folders on the actual file system.
         */
        class FSDriver : public Driver {
            virtual io::Stream *open_impl(const path::Path &path, io::ModeFlags const mode);
            virtual file_name_vec list_impl() const;
            virtual file_name_vec list_impl(path::Path const &path, bool const recurse = true) const;

            path::Path const m_os_path; //< Root driver path on the actual FS.

            /**
             * Convert a virtual path to a path on the actual FS.
             *
             * @param vpath The virtual path.
             *
             * @return The actual path.
             */
            path::Path virtual_to_actual(path::Path const &vpath) const;

            /**
             * Convert an actual FS path to a virtual path.
             *
             * @param apath The actual path.
             *
             * @return The virtual path.
             */
            path::Path actual_to_virtual(path::Path const &apath) const;

          public:
            /**
             * Create a new instance of this class.
             *
             * @param virtual_path The mount point.
             * @param os_path The root path on the actual FS.
             */
            FSDriver(path::Path const &virtual_path, path::Path const &os_path) :
              Driver{virtual_path, true}, m_os_path{os_path} {}

            /**
             * Return a string representing the path used as root for this driver.
             *
             * @return The path on the actual FS used as root for this driver.
             */
            path::Path const &get_os_path() const { return m_os_path; }
        };
    } // namespace vfs
} // namespace jolt

#endif /* JLT_VFS_FS_DRIVER_HPP */
