#ifndef JLT_MEDIA_PNG_HPP
#define JLT_MEDIA_PNG_HPP

#include <jolt/api.hpp>
#include <jolt/io/stream.hpp>
#include "image.hpp"

namespace jolt {
    namespace media {
        Image JLTAPI load_image_png(io::Stream &stream);
    }
} // namespace jolt

#endif /* JLT_MEDIA_PNG_HPP */
