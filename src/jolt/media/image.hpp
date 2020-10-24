#ifndef JLT_MEDIA_IMAGE_HPP
#define JLT_MEDIA_IMAGE_HPP

#include <cstdint>
#include <jolt/memory/allocator.hpp>

namespace jolt {
    namespace media {
        enum ImageType { IMAGE_TYPE_1D, IMAGE_TYPE_2D, IMAGE_TYPE_3D };

        struct ImageHeader {
            ImageType image_type;
            uint32_t width, height, depth;
        };

        /**
         * Pixel data. All images internally will be stored as arrays of pixels with RGBA components expressed
         * as 8 bits unsigned integers.
         */
        struct Pixel {
            uint8_t r, g, b, a;
        };

        class Image {
            ImageHeader const m_header;
            Pixel const *const m_data;

          public:
            /**
             * Create a new image from its data.
             *
             * @param head Image header.
             * @param data Image data as an array allocated with `jltallocarray()`.
             */
            Image(ImageHeader const &head, Pixel const *const data) : m_header{head}, m_data{data} {}
            ~Image() { jltfreearray(m_data); }

            ImageHeader const &get_header() const { return m_header; }
            Pixel const *get_data() const { return m_data; }

            /**
             * Get the size of the image data in bytes.
             */
            size_t get_size() const {
                return m_header.width * m_header.height * m_header.depth * sizeof(Pixel);
            }
        };
    } // namespace media
} // namespace jolt

#endif /* JLT_MEDIA_IMAGE_HPP */
