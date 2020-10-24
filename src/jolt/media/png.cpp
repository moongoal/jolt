#include <cstdlib>
#include <libpng16/png.h>
#include <jolt/debug.hpp>
#include <jolt/text/string.hpp>
#include <jolt/collections/array.hpp>
#include "png.hpp"

static void error_handler(JLT_MAYBE_UNUSED png_structp png_ptr, png_const_charp error_msg) {
    jolt::console.err(jolt::text::s(error_msg));

    ::abort();
}

static void warn_handler(JLT_MAYBE_UNUSED png_structp png_ptr, png_const_charp warning_msg) {
    jolt::console.warn(jolt::text::s(warning_msg));
}

static png_voidp malloc_handler(JLT_MAYBE_UNUSED png_structp png_ptr, png_alloc_size_t size) {
    return jolt::memory::allocate_array<uint8_t>(size);
}

static void free_handler(JLT_MAYBE_UNUSED png_structp png_ptr, png_voidp ptr) {
    return jolt::memory::free_array(ptr);
}

namespace jolt {
    namespace media {
        Image JLTAPI load_image_png(io::Stream &stream) {
            jltassert2(stream.supports_size(), "Unable to compute PNG image size");

            size_t const raw_data_sz = stream.get_size();
            collections::Array<uint8_t> raw_data{raw_data_sz};

            JLT_MAYBE_UNUSED size_t const read_data_sz = stream.read(raw_data, raw_data_sz);
            jltassert2(raw_data_sz == read_data_sz, "Unable to read PNG data");

            png_structp png_ptr = png_create_read_struct_2(
              PNG_LIBPNG_VER_STRING,
              nullptr,
              error_handler,
              warn_handler,
              nullptr,
              malloc_handler,
              free_handler);
            png_infop info_ptr = png_create_info_struct(png_ptr);
            png_image image;

            memset(&image, 0, sizeof(png_image));
            image.version = PNG_IMAGE_VERSION;
            
            png_image_begin_read_from_memory(&image, raw_data, raw_data.get_length());
            image.format = PNG_FORMAT_RGBA;
            ImageHeader image_header{
              IMAGE_TYPE_2D, // image_type
              image.width,   // width
              image.height,  // height
              1              // depth
            };
            Pixel *image_data = jltallocarray(Pixel, image.width * image.height);

            png_image_finish_read(&image, nullptr, image_data, 0, nullptr);
            png_image_free(&image);
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

            return Image{image_header, image_data};
        }
    } // namespace media
} // namespace jolt
