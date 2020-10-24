#include <cstdlib>
#include <Windows.h>
#include "keyboard.hpp"

static size_t constexpr MAX_KEYBOARD_KEYS = VK_OEM_CLEAR - VK_BACK + 1;
static bool g_key_states[MAX_KEYBOARD_KEYS];

namespace jolt {
    namespace input {
        void initialize() { memset(g_key_states, 0, sizeof(bool) * MAX_KEYBOARD_KEYS); }

        void key_down(KeyCode const key_code) { g_key_states[static_cast<int>(key_code)] = true; }

        void key_up(KeyCode const key_code) { g_key_states[static_cast<int>(key_code)] = false; }

        bool *get_key_states() { return g_key_states; }
    } // namespace input
} // namespace jolt
