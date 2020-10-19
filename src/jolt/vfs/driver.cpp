#include "driver.hpp"

namespace jolt {
    namespace vfs {
        io::Stream *Driver::open(const path::Path &path, io::ModeFlags const mode) {
            jltassert2(
              !(mode & io::MODE_WRITE) || m_supports_write,
              "Attempting to open a file in write-mode using driver that doesn't support writing");

            return open_impl(path, mode);
        }
    } // namespace vfs
} // namespace jolt
