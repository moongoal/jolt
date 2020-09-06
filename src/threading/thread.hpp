#include <cstdint>
#include <util.hpp>

namespace jolt {
    namespace threading {
        using thread_id = uint32_t;

        class JLTAPI Thread {
            thread_id const m_id;

          public:
            explicit Thread(thread_id id);

            thread_id get_id() const { return m_id; }

            static Thread &current();
        };

        void initialize();
    } // namespace threading
} // namespace jolt
