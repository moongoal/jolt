#ifndef JLT_DEBUG_CONSOLE_HPP
#define JLT_DEBUG_CONSOLE_HPP

#include <util.hpp>
#include <text/string.hpp>
#include <io/stream.hpp>

namespace jolt {
    namespace debug {
        /**
         * The console can be used to process user text input and translate it into commands that
         * will be executed by the game engine and to output text to any output stream.
         */
        class JLTAPI Console {
            io::InputStream *const m_source; //< The console input stream.
            io::OutputStream *const m_sink;  //< The console output stream.

          public:
            /**
             * Create a new console object.
             *
             * @param source The console input or `nullptr` to disable console input.
             * @param sink The console output or `nullptr` to disable console output.
             */
            explicit Console(
              io::InputStream *const source = nullptr, io::OutputStream *const sink = nullptr) :
              m_source{source},
              m_sink{sink} {}

            Console(const Console &other) = delete;
            Console &operator=(const Console &other) = delete;

            /**
             * Interpret a command and execute it.
             *
             * @param cmdline The command line to parse.
             *
             * @return True if a command was found and executed, false if the input was invalid.
             */
            bool interpret_command(const text::String &cmdline);

            void echo(const text::String &message, bool newline = true);
            void warn(const text::String &message, bool newline = true);
            void err(const text::String &message, bool newline = true);
        };
    } // namespace debug
} // namespace jolt

#endif /* JLT_DEBUG_CONSOLE_HPP */
