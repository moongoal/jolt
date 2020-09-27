#define _CRT_SECURE_NO_WARNINGS
#include <debug.hpp>
#include "stream.hpp"

namespace jolt {
    namespace io {
        StandardOutputStream standard_output_stream;
        StandardErrorStream standard_error_stream;

        FileStream::FileStream(const text::String &path, FileMode const mode) : m_mode(mode) {
            char fo_mode[4] = {(mode == FileMode::Read ? 'r' : 'w'), 'b', '+', 0};

            m_file = fopen((char *)path.get_raw(), fo_mode);
            m_eof = !feof(m_file);
        }

        FileStream::FileStream(FILE *const file, FileMode const mode) :
          m_file{file}, m_mode{mode}, m_eof{!feof(file)} {}

        FileStream::~FileStream() { close(); }

        size_t FileInputStream::read(uint8_t *const buf, size_t const buf_sz) {
            size_t x = fread(buf, sizeof(uint8_t), buf_sz, m_file);

            if(!x) {
                m_eof = feof(m_file);
            }

            return x;
        }

        size_t FileOutputStream::write(const uint8_t *const buf, size_t const buf_sz) {
            jltassert(m_mode == FileMode::Write);

            return fwrite(buf, sizeof(uint8_t), buf_sz, m_file);
        }

        void FileStream::close() {
            if(m_file) {
                fclose(m_file);

                m_file = nullptr;
            }
        }
    } // namespace io
} // namespace jolt
