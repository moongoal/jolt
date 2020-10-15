#ifndef JLT_DEBUG_CONSOLE_HPP
#define JLT_DEBUG_CONSOLE_HPP

#include <jolt/features.hpp>
#include <jolt/api.hpp>
#include <jolt/text/string.hpp>
#include <jolt/io/stream.hpp>

namespace jolt {
    namespace debug {
        enum class LogLevel { Error, Warning, Info, Debug };

        /**
         * The console can be used to process user text input and translate it into commands that
         * will be executed by the game engine and to output text to any output stream.
         */
        class JLTAPI Console {
            io::FileStream *m_source; //< The console input stream.
            io::FileStream *m_sink;   //< The console output stream.
            LogLevel m_loglevel;

            void print_with_prefix(const text::String &prefix, const text::String &message, bool newline);

          public:
            /**
             * Create a new console object.
             *
             * @param source The console input or `nullptr` to disable console input.
             * @param sink The console output or `nullptr` to disable console output.
             */
            explicit Console(
              io::FileStream *const source = nullptr,
              io::FileStream *const sink = nullptr,
              LogLevel loglevel = LogLevel::Warning) :
              m_source{source},
              m_sink{sink},
#ifdef JLT_WITH_DEBUG_LOGGING
              m_loglevel {
                LogLevel::Debug
            }
#else  // JLT_WITH_DEBUG_LOGGING
              m_loglevel {
                loglevel
            }
#endif // JLT_WITH_DEBUG_LOGGING
            {}

            Console(const Console &other) = delete;
            Console &operator=(const Console &other) = delete;
            Console &operator=(Console &&other) = default;

            /**
             * Interpret a command and execute it.
             *
             * @param cmdline The command line to parse.
             *
             * @return True if a command was found and executed, false if the input was invalid.
             */
            bool interpret_command(const text::String &cmdline);

            void echo(const text::String &message, bool newline = true);
            void info(const text::String &message, bool newline = true);
            void warn(const text::String &message, bool newline = true);
            void err(const text::String &message, bool newline = true);
            void debug(const text::String &message, bool newline = true);

            void make_default();

            io::FileStream *get_input_stream() { return m_source; }
            io::FileStream *get_output_straem() { return m_sink; }
            LogLevel get_log_level() const { return m_loglevel; }

            void set_input_stream(io::FileStream *const source) { m_source = source; }
            void set_output_stream(io::FileStream *const sink) { m_sink = sink; }

            void set_log_level(LogLevel loglevel) {
#ifndef JLT_WITH_DEBUG_LOGGING
                m_loglevel = loglevel;
#endif // !JLT_WITH_DEBUG_LOGGING
            }
        };

    } // namespace debug

    extern JLTAPI debug::Console console; //< Default application-wide console
} // namespace jolt

#endif /* JLT_DEBUG_CONSOLE_HPP */
