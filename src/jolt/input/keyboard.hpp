#ifndef JLT_INPUT_KEYBOARD_HPP
#define JLT_INPUT_KEYBOARD_HPP

#include <cstdint>
#include <jolt/api.hpp>

#define JLT_KEYCODE_MIN 0x08
#define JLT_KEYCODE_MAX = 0xfe

namespace jolt {
    namespace input {
        /**
         * Key codes.
         *
         * @remarks This is a list of codes for keys on the keyboard, not characters.
         */
        enum class KeyCode : uint8_t {
            Back = 0x08,
            Tab = 0x09,
            Clear = 0x0c,
            Return = 0x0d,
            Shift = 0x10,
            Control = 0x11,
            Alt = 0x12,
            Pause = 0x13,
            Capital = 0x14,
            Kana = 0x15,
            Junja = 0x17,
            Final = 0x18,
            Hanja = 0x19,
            Kanji = 0x19,
            Escape = 0x1b,
            Convert = 0x1c,
            NonConvert = 0x1d,
            Accept = 0x1e,
            ModeChange = 0x1f,
            Space = 0x20,
            Prior = 0x21,
            Next = 0x22,
            End = 0x23,
            Home = 0x24,
            Left = 0x25,
            Up = 0x26,
            Right = 0x27,
            Down = 0x28,
            Select = 0x29,
            Print = 0x2a,
            Execute = 0x2b,
            Snapshot = 0x2c,
            Insert = 0x2d,
            Delete = 0x2e,
            Help = 0x2f,
            Number0 = 0x30,
            Number1,
            Number2,
            Number3,
            Number4,
            Number5,
            Number6,
            Number7,
            Number8,
            Number9,
            A = 0x41,
            B,
            C,
            D,
            E,
            F,
            G,
            H,
            I,
            J,
            K,
            L,
            M,
            N,
            O,
            P,
            Q,
            R,
            S,
            T,
            U,
            V,
            W,
            X,
            Y,
            Z,
            Lwin = 0x5b,
            Rwin = 0x5c,
            Apps = 0x5d,
            Sleep = 0x5f,
            Numpad0 = 0x60,
            Numpad1 = 0x61,
            Numpad2 = 0x62,
            Numpad3 = 0x63,
            Numpad4 = 0x64,
            Numpad5 = 0x65,
            Numpad6 = 0x66,
            Numpad7 = 0x67,
            Numpad8 = 0x68,
            Numpad9 = 0x69,
            Multiply = 0x6a,
            Add = 0x6b,
            Separator = 0x6c,
            Subtract = 0x6d,
            Decimal = 0x6e,
            Divide = 0x6f,
            F1 = 0x70,
            F2 = 0x71,
            F3 = 0x72,
            F4 = 0x73,
            F5 = 0x74,
            F6 = 0x75,
            F7 = 0x76,
            F8 = 0x77,
            F9 = 0x78,
            F10 = 0x79,
            F11 = 0x7a,
            F12 = 0x7b,
            F13 = 0x7c,
            F14 = 0x7d,
            F15 = 0x7e,
            F16 = 0x7f,
            F17 = 0x80,
            F18 = 0x81,
            F19 = 0x82,
            F20 = 0x83,
            F21 = 0x84,
            F22 = 0x85,
            F23 = 0x86,
            F24 = 0x87,
            NumLock = 0x90,
            Scroll = 0x91,
            LShift = 0xa0,
            RShift = 0xa1,
            LControl = 0xa2,
            RControl = 0xa3,
            LAlt = 0xa4,
            RAlt = 0xa5,
            BrowserBack = 0xa6,
            BrowserForward = 0xa7,
            BrowserRefresh = 0xa8,
            BrowserStop = 0xa9,
            BrowserSearch = 0xaa,
            BrowserFavorites = 0xab,
            BrowserHome = 0xac,
            VolumeMute = 0xad,
            VolumeDown = 0xae,
            VolumeUp = 0xaf,
            MediaNextTrack = 0xb0,
            MediaPrevTrack = 0xb1,
            MediaStop = 0xb2,
            MediaPlayPause = 0xb3,
            LaunchMail = 0xb4,
            LaunchMediaSelect = 0xb5,
            LaunchApp1 = 0xb6,
            LaunchApp2 = 0xb7,
            Semicolon = 0xba,
            Plus = 0xbb,
            Comma = 0xbc,
            Minus = 0xbd,
            Period = 0xbe,
            ForwardSlash = 0xbf,
            Tilde = 0xc0,
            SquareBracketOpen = 0xdb,
            BackwardSlash = 0xdc,
            SquareBracketClose = 0xdd,
            Quote = 0xde,
            Ax = 0xe1,
            AngleBracket = 0xe2,
            Processkey = 0xe5,
            Reset = 0xe9,
            Jump = 0xea,
            Pa1 = 0xeb,
            Pa2 = 0xec,
            Pa3 = 0xed,
            Wsctrl = 0xee,
            Cusel = 0xef,
            Attn = 0xf0,
            Finish = 0xf1,
            Copy = 0xf2,
            Auto = 0xf3,
            Enlw = 0xf4,
            Backtab = 0xf5,
            Crsel = 0xf7,
            Exsel = 0xf8,
            Ereof = 0xf9,
            Play = 0xfa,
            Zoom = 0xfb,
            Noname = 0xfc
        };

        /**
         * Initialize the keyboard handling subsystem.
         */
        void JLTAPI initialize();

        /**
         * Fire a key down event.
         */
        void JLTAPI key_down(KeyCode const key_code);

        /**
         * Fire a key up event.
         */
        void JLTAPI key_up(KeyCode const key_code);

        /**
         * Get the key state array.
         *
         * @return The keyboard subsystem's key state array. Each true entry is a key in the pressed state,
         * each false entry is a key in the unpressed state.
         */
        bool *JLTAPI get_key_states();

        /**
         * Check whether a key is in the pressed state.
         */
        inline bool is_key_down(KeyCode const key_code) {
            return get_key_states()[static_cast<int>(key_code) - JLT_KEYCODE_MIN];
        }

        /**
         * Check whether a sequence of keys is in the pressed state.
         */
        bool are_all_keys_down(auto const &begin, auto const &end) {
            bool const *const states = get_key_states();

            for(auto it = begin; it != end; ++it) {
                if(!states[static_cast<int>(*it) - JLT_KEYCODE_MIN]) {
                    return false;
                }
            }

            return true;
        }

        /**
         * Check whether any entry in a sequence of keys is in the pressed state.
         */
        bool is_any_key_down(auto const &begin, auto const &end) {
            bool const *const states = get_key_states();

            for(auto it = begin; it != end; ++it) {
                if(states[static_cast<int>(*it) - JLT_KEYCODE_MIN]) {
                    return true;
                }
            }

            return false;
        }
    } // namespace input
} // namespace jolt

#endif /* JLT_INPUT_KEYBOARD_HPP */
