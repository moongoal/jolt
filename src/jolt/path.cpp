#include <jolt/debug.hpp>
#include "path.hpp"

namespace jolt {
    namespace path {
        const text::String SEPARATOR{"/"};

        Path normalize(Path const &path) {
            Path p{path.replace_all("\\", "/")};

            if(p.ends_with("/")) {
                p = p.slice(0, p.get_length() - 1);
            }

            return p;
        }

        bool is_absolute(Path const &path) { return path[0] == '/'; }
    } // namespace path
} // namespace jolt
