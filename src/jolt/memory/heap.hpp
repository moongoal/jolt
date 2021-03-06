#ifndef JLT_MEMORY_HEAP_H
#define JLT_MEMORY_HEAP_H
#include <jolt/debug.hpp>
#include <jolt/api.hpp>

namespace jolt {
    namespace memory {
        /**
         * Memory heap. This class represents a region of reserved memory.
         * The application can use some or all of the memory contained
         * in this heap. When the heap is extended, more memory is
         * allocated from the reserved range. The memory is automatically
         * released after the distruction of the object.
         */
        class JLTAPI Heap {
          private:
            void *const m_base_ptr;  // The base pointer to the chunk of memory
                                     // of the heap.
            size_t const m_size;     // The size of memory contained in the heap.
            size_t m_committed_size; // The size of memory directly usable by
                                     // the application.

          public:
            static constexpr size_t MIN_ALLOC_SIZE = 1024 * 1024; // 1 MiB

            /**
             * Initialize a new instance of this class.
             * Upon initialization, the memory is reserved.
             *
             * @param sz The size of the heap in bytes.
             * @param base The base address where the heap memory will be
             * placed. Set to `nullptr` to allow the system to choose the
             * address.
             */
            explicit Heap(size_t const sz, void *const base = nullptr);
            Heap(const Heap &other) = delete;

            /**
             * Finalize and destroy an instance of this class.
             * Upon finalization, the memory is released.
             */
            virtual ~Heap();

            /**
             * Return the base virtual address of the heap.
             */
            void *get_base() const { return m_base_ptr; }

            /**
             * Return the size of the memory as given to the constructor.
             */
            size_t get_size() const { return m_size; }

            /**
             * Return the size of committed memory in the heap.
             */
            size_t get_committed_size() const { return m_committed_size; }

            /**
             * Check whether the current heap owns a memory location.
             *
             * @param ptr The pointer to the memory location.
             *
             * @return True if the memory location is contained in this heap. False if not.
             */
            bool owns_ptr(void *const ptr) const {
                auto const base = reinterpret_cast<uint8_t *>(get_base());

                return ptr > base && ptr < base + get_size();
            }

            /**
             * Dump the contents of the committed memory to file.
             *
             * @param path The path to the memory dump.
             */
            void dump_to_file(const char *const path);

          protected:
            /**
             * Commit a new chunk of memory.
             *
             * @return The address to the newly committed m_size.
             */
            void *commit(size_t const ext_sz);
        };
    } // namespace memory
} // namespace jolt

#endif // JLT_MEMORY_HEAP_H
