#ifndef JLT_MEMORY_HEAP_H
#define JLT_MEMORY_HEAP_H
#include <debug.hpp>
#include <util.hpp>

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
            ~Heap();

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

          protected:
            /**
             * Commit a new chunk of memory.
             *
             * @return The address to the newly committed m_size.
             */
            void *extend(size_t const ext_sz);
        };
    } // namespace memory
} // namespace jolt

#endif // JLT_MEMORY_HEAP_H
