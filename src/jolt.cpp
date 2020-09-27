#include <jolt.hpp>

namespace jolt {
    void initialize() {
        jolt::threading::initialize();
        jolt::ui::initialize();
    }

    void shutdown() { jolt::ui::shutdown(); }
} // namespace jolt
