#ifndef JLT_IO_STREAM_HPP
#define JLT_IO_STREAM_HPP

#include <cstdio>
#include <cstdint>
#include <jolt/api.hpp>
#include <jolt/text/string.hpp>

namespace jolt {
    namespace io {
        using ModeFlags = uint32_t;

        enum Mode : ModeFlags { MODE_READ = 1, MODE_WRITE };

        class JLTAPI Stream {
          private:
            ModeFlags m_mode;
            bool const m_closeable;
            bool mutable m_error = false;

            virtual size_t read_impl(uint8_t *const buf, size_t const buf_sz) = 0;

            virtual size_t
            write_impl(JLT_MAYBE_UNUSED const uint8_t *const buf, JLT_MAYBE_UNUSED size_t const buf_sz) {
                return 0;
            }

            virtual void close_impl() {}

          protected:
            void set_error() const { m_error = true; }

          public:
            Stream(ModeFlags const mode, bool const closeable) : m_mode{mode}, m_closeable{closeable} {}
            Stream(const Stream &) = delete;
            Stream(Stream &&) = default;

            virtual ~Stream();

            size_t read(uint8_t *const buf, size_t const buf_sz) { return read_impl(buf, buf_sz); }
            size_t write(const uint8_t *const buf, size_t const buf_sz);
            void close();

            bool has_error() const { return m_error; }
        };

        class JLTAPI FileStream : public Stream {
          private:
            virtual size_t read_impl(uint8_t *const buf, size_t const buf_sz);
            virtual size_t write_impl(const uint8_t *const buf, size_t const buf_sz);
            virtual bool eof_impl() const { return m_eof; }
            virtual void close_impl();

          protected:
            FILE *m_file;
            bool m_eof;

            FileStream(FILE *const file, ModeFlags const mode);

          public:
            FileStream(const text::String &path, ModeFlags const mode);
            FileStream(FileStream &&) = default;
            virtual ~FileStream();

            bool eof() const { return eof_impl(); }
        };

        class StandardErrorStream : public FileStream {
          private:
            virtual void close_impl() {}

          public:
            StandardErrorStream() : FileStream{stderr, MODE_WRITE} {}
        };

        class StandardOutputStream : public FileStream {
          private:
            virtual void close_impl() {}

          public:
            StandardOutputStream() : FileStream{stdout, MODE_WRITE} {}
        };

        class StandardInputStream : public FileStream {
          private:
            virtual void close_impl() {}

          public:
            StandardInputStream() : FileStream{stdin, MODE_READ} {}
        };

        extern JLTAPI StandardOutputStream standard_output_stream;
        extern JLTAPI StandardErrorStream standard_error_stream;
    } // namespace io
} // namespace jolt

#endif /* JLT_IO_STREAM_HPP */
