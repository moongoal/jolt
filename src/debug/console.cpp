#include <text/stringbuilder.hpp>
#include "console.hpp"

using namespace jolt::text;

namespace jolt {
    namespace debug {
        void Console::echo(const text::String &message, bool newline) {
            if(!m_sink) {
                return;
            }

            StringBuilder sb;

            sb.add(message);

            if(newline) {
                sb.add(u8"\n");
            }

            String s = sb.to_string();

            m_sink->write(
              reinterpret_cast<const uint8_t *>(s.get_raw()), s.get_length() * sizeof(utf8c));
        }

        void Console::warn(const text::String &message, bool newline) {
            if(!m_sink) {
                return;
            }

            StringBuilder sb{u8"Warning: "};

            sb.add(message);

            if(newline) {
                sb.add(u8"\n");
            }

            String s = sb.to_string();

            m_sink->write(
              reinterpret_cast<const uint8_t *>(s.get_raw()), s.get_length() * sizeof(utf8c));
        }

        void Console::err(const text::String &message, bool newline) {
            if(!m_sink) {
                return;
            }

            StringBuilder sb{u8"Error: "};

            sb.add(message);

            if(newline) {
                sb.add(u8"\n");
            }

            String s = sb.to_string();

            m_sink->write(
              reinterpret_cast<const uint8_t *>(s.get_raw()), s.get_length() * sizeof(utf8c));
        }

        bool Console::interpret_command(const text::String &cmdline) { return true; }
    } // namespace debug
} // namespace jolt
