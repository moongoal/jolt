#ifndef JLT_PATH_HPP
#define JLT_PATH_HPP

#include <jolt/api.hpp>
#include <jolt/text/string.hpp>

namespace jolt {
    namespace path {
        using Path = text::String;

        extern JLTAPI const text::String SEPARATOR; //< Internal path separator

        /**
         * Normalizes a path ensuring every slash is a forward slash and it doesn't end with a slash
         * character.
         *
         * @param path The non-normalized path.
         *
         * @result The normalized path.
         */
        JLT_NODISCARD Path JLTAPI normalize(Path const &path);

        /**
         * Return a value stating whether a path is absolute or relative.
         *
         * @param path The path
         *
         * @return True if the path is absolute, false if it's relative.
         */
        JLT_NODISCARD bool JLTAPI is_absolute(Path const &path);
    } // namespace path
} // namespace jolt

#endif /* JLT_PATH_HPP */
