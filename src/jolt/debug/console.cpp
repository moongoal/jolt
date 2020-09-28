#include <jolt/text/stringbuilder.hpp>
#include "console.hpp"

using namespace jolt::text;

namespace jolt {
    debug::Console console;

    namespace debug {
        void Console::print_with_prefix(
          const text::String &prefix, const text::String &message, bool newline) {
            if(!m_sink) {
                return;
            }

            StringBuilder sb{prefix};

            if(prefix != EmptyString) {
                sb.add(": ");
            }

            sb.add(message);

            if(newline) {
                sb.add("\n");
            }

            String s = sb.to_string();

            m_sink->write(
              reinterpret_cast<const uint8_t *>(s.get_raw()), s.get_length() * sizeof(utf8c));
        }

        void Console::echo(const text::String &message, bool newline) {
            print_with_prefix(EmptyString, message, newline);
        }

        void Console::info(const text::String &message, bool newline) {
            if(m_loglevel >= LogLevel::Info) {
                print_with_prefix("Info", message, newline);
            }
        }

        void Console::warn(const text::String &message, bool newline) {
            if(m_loglevel >= LogLevel::Warning) {
                print_with_prefix("Warning", message, newline);
            }
        }

        void Console::err(const text::String &message, bool newline) {
            if(m_loglevel >= LogLevel::Error) {
                print_with_prefix("Error", message, newline);
            }
        }

        void Console::debug(const text::String &message, bool newline) {
            if(m_loglevel >= LogLevel::Debug) {
                print_with_prefix("Debug", message, newline);
            }
        }

        bool Console::interpret_command(const text::String &cmdline) { return true; }
    } // namespace debug
} // namespace jolt
