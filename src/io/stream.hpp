#ifndef JLT_IO_STREAM_HPP
#define JLT_IO_STREAM_HPP
#include <cstdio>
#include <cstdint>
#include <util.hpp>
#include <text/string.hpp>

namespace jolt {
    namespace io {
        enum class FileMode { Read, Write };

        struct Stream {};

        struct InputStream : public Stream {
            virtual size_t read(uint8_t *const buf, size_t const buf_sz) = 0;
        };

        struct OutputStream : public Stream {
            virtual size_t write(const uint8_t *const buf, size_t const buf_sz) = 0;
        };

        struct Closeable {
            virtual void close() = 0;
        };

        class JLTAPI FileStream : public Stream, public Closeable {
          protected:
            FILE *m_file;
            FileMode const m_mode;
            bool m_eof;

            explicit FileStream(const text::String &path, FileMode const mode);

          public:
            FileStream(FILE *const file, FileMode const mode);
            ~FileStream();

            virtual bool eof() const { return m_eof; }
            virtual void close();
        };

        class JLTAPI FileInputStream : public FileStream, public InputStream {
          public:
            explicit FileInputStream(const text::String &path) : FileStream{path, FileMode::Read} {}
            FileInputStream(FILE *const file) : FileStream{file, FileMode::Read} {}

            virtual size_t read(uint8_t *const buf, size_t const buf_sz);
        };

        class JLTAPI FileOutputStream : public FileStream, public OutputStream {
          public:
            explicit FileOutputStream(const text::String &path) :
              FileStream{path, FileMode::Write} {}

            FileOutputStream(FILE *const file) : FileStream{file, FileMode::Write} {}

            virtual size_t write(const uint8_t *const buf, size_t const buf_sz);
        };

        class StandardErrorStream : public FileOutputStream {
          public:
            StandardErrorStream() : FileOutputStream{stderr} {}

            virtual void close() {}
        };

        class StandardOutputStream : public FileOutputStream {
          public:
            StandardOutputStream() : FileOutputStream{stderr} {}

            virtual void close() {}
        };

        class StandardInputStream : public FileInputStream {
          public:
            StandardInputStream() : FileInputStream{stderr} {}

            virtual void close() {}
        };
    } // namespace io
} // namespace jolt

#endif /* JLT_IO_STREAM_HPP */
