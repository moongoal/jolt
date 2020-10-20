#define _CRT_SECURE_NO_WARNINGS
#include <jolt/debug.hpp>
#include "stream.hpp"

namespace jolt {
    namespace io {
        StandardOutputStream standard_output_stream;
        StandardErrorStream standard_error_stream;

        Stream::~Stream() = default;

        size_t Stream::write(const uint8_t *const buf, size_t const buf_sz) {
            jltassert2(m_mode & MODE_WRITE, "Attempting to write to non-writable file");

            return write_impl(buf, buf_sz);
        }

        void Stream::close() {
            jltassert2(m_closeable, "Attempting to close a non-closeable stream");

            close_impl();
        }

        FileStream::FileStream(const text::String &path, ModeFlags const mode) : Stream{mode, true} {
#define RW_FLAGS (MODE_READ | MODE_WRITE)
            char fo_mode[4] = {
              ((mode & MODE_READ) ? 'r' : 'w'), 'b', (((mode & RW_FLAGS) == RW_FLAGS) ? '+' : '\0'), 0};

            m_file = fopen((char *)path.get_raw(), fo_mode);

            if(!m_file || ferror(m_file)) {
                set_error();

                return;
            }

            fseek(m_file, 0, SEEK_END);
            set_size(ftell(m_file));
            rewind(m_file);

            m_eof = !feof(m_file);
#undef RW_FLAGS
        }

        FileStream::FileStream(FILE *const file, ModeFlags const mode) :
          Stream{mode, true}, m_file{file}, m_eof{!feof(file)} {}

        FileStream::~FileStream() { close(); }

        size_t FileStream::read_impl(uint8_t *const buf, size_t const buf_sz) {
            size_t x = fread(buf, sizeof(uint8_t), buf_sz, m_file);

            if(!x) {
                m_eof = feof(m_file);

                if(ferror(m_file)) {
                    set_error();
                }
            }

            return x;
        }

        size_t FileStream::write_impl(const uint8_t *const buf, size_t const buf_sz) {
            size_t res = fwrite(buf, sizeof(uint8_t), buf_sz, m_file);

            if(ferror(m_file)) {
                set_error();
            }

            return res;
        }

        void FileStream::close_impl() {
            if(m_file) {
                fclose(m_file);

                m_file = nullptr;
            }
        }
    } // namespace io
} // namespace jolt
