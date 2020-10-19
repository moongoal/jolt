#ifndef JLT_VFS_DRIVER_HPP
#define JLT_VFS_DRIVER_HPP

#include <jolt/util.hpp>
#include <jolt/path.hpp>
#include <jolt/collections/vector.hpp>
#include <jolt/text/string.hpp>
#include <jolt/io/stream.hpp>

namespace jolt {
    namespace vfs {
        class Driver {
          public:
            using file_name_vec = collections::Vector<path::Path>;

          private:
            bool const
              m_supports_write; //< Boolean flag stating whether this driver supports writing to files.
            path::Path const m_virt_path; //< The virtual path of the mount point.

            /**
             * Implementation for the `open()` function.
             *
             * @see open().
             */
            virtual io::Stream *open_impl(const path::Path &path, io::ModeFlags const mode) = 0;

            /**
             * Implementation for the `list()` function.
             *
             * @see list().
             */
            virtual file_name_vec list_impl() const = 0;

          protected:
            /**
             * Create a new driver.
             *
             * @param virtual_path The virtual path of the mount point.
             * @param supports_write True if this driver supports writing to and creating files.
             */
            explicit Driver(path::Path const &virtual_path, bool const supports_write) :
              m_supports_write{supports_write}, m_virt_path{virtual_path} {}

          public:
            virtual ~Driver() {}

            /**
             * Open a file.
             *
             * @param path The virtual file to the path.
             * @param mode The stream mode flags.
             */
            io::Stream *open(const path::Path &path, io::ModeFlags const mode);

            /**
             * List all the files.
             */
            file_name_vec list() const { return list_impl(); }

            /**
             * Check whether this driver supports opening a file in write mode.
             *
             * If the driver supports write mode, the `open()` function can be used to open or create files.
             *
             * @return True if the driver supports write mode, false if it doesn't.
             */
            bool supports_write() const { return m_supports_write; }

            /**
             * Get the virtual root path for this driver.
             *
             * @return The virtual absolute path in the VFS representing the mount point for this driver's
             * instance.
             */
            path::Path const &get_virtual_path() const { return m_virt_path; }
        };
    } // namespace vfs
} // namespace jolt

#endif /* JLT_VFS_DRIVER_HPP */
