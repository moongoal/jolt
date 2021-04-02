#ifndef JLT_MEDIA_GLTF_HPP
#define JLT_MEDIA_GLTF_HPP

#include <jolt/api.hpp>
#include <jolt/io/stream.hpp>

namespace jolt::media {
    void JLTAPI load_model_gltf(io::Stream &stream);
}

#endif /* JLT_MEDIA_GLTF_HPP */
