#include <cstring>
#include <jolt/util.hpp>
#include "checks.hpp"
#include "arena.hpp"

namespace jolt {
    namespace memory {
        /**
         * Return a boolean value stating whether two existing nodes are adjacent in terms of memory
         * address.
         */
        static inline bool are_nodes_adjacent(ArenaFreeListNode *const left, ArenaFreeListNode *const right) {
            return reinterpret_cast<uint8_t *>(left) + left->m_size == reinterpret_cast<uint8_t *>(right);
        }

        /**
         * Create a free list node at a given memory location.
         *
         * @param node The memory location at which to create the new node.
         * @param size The size of the free memory chunk.
         * @param prev The previous node or nullptr.
         * @param next The next node or nullptr.
         */
        static void create_free_list_node(
          ArenaFreeListNode *const node,
          size_t const size,
          ArenaFreeListNode *const prev,
          ArenaFreeListNode *const next) {
            jltassert(size >= sizeof(ArenaFreeListNode));

            new(node) ArenaFreeListNode{size, prev, next};

            if(prev) {
                prev->m_next = node;
            }

            if(next) {
                next->m_prev = node;
            }
        }

        /**
         * Remove a node from the free list.
         *
         * @param node The pointer to the node to remove.
         */
        static void delete_free_list_node(ArenaFreeListNode *const node) {
#ifdef JLT_WITH_MEM_CHECKS
            jltassert(node->m_free_canary == JLT_MEM_ARENA_FLN_CANARY_VALUE);
#endif // JLT_WITH_MEM_CHECKS

            if(node->m_prev) {
                node->m_prev->m_next = node->m_next;
            }

            if(node->m_next) {
                node->m_next->m_prev = node->m_prev;
            }
        }

        /**
         * Merge two adjacent free list nodes into one.
         *
         * @param left The node with the lowest memory address.
         * @param right The node with the highest memory address.
         */
        static void merge_adj_free_list_nodes(ArenaFreeListNode *const left, ArenaFreeListNode *const right) {
            jltassert(are_nodes_adjacent(left, right));

            left->m_size += right->m_size;
            left->m_next = right->m_next;

            if(right->m_next) {
                right->m_next->m_prev = left;
            }

            JLT_FILL_AFTER_FREE(right, sizeof(ArenaFreeListNode));
        }

        ArenaFreeListNode *Arena::find_left_closest_node(void *const ptr) const {
            ArenaFreeListNode *node = get_free_list();

            for(; node && node < ptr && node->m_next; node = node->m_next)
                ;

            for(; node && node >= ptr; node = node->m_prev)
                ;

            return node;
        }

        ArenaFreeListNode *Arena::find_right_closest_node(void *const ptr, size_t const size) const {
            ArenaFreeListNode *node = get_free_list();
            void *const end_ptr = reinterpret_cast<uint8_t *>(ptr) + size;

            for(; node && node > end_ptr && node->m_prev; node = node->m_prev)
                ;

            for(; node && node < end_ptr; node = node->m_next)
                ;

            return node;
        }

        Arena::Arena(size_t const memory_size) :
          Heap{memory_size}, m_allocated_size{sizeof(ArenaFreeListNode)} {
            commit(sizeof(ArenaFreeListNode));

            m_free_list = reinterpret_cast<ArenaFreeListNode *>(get_base());
            create_free_list_node(m_free_list, memory_size, nullptr, nullptr);
        }

        void *Arena::allocate(uint32_t const size, flags_t const flags, uint32_t const alignment) {
            uint32_t base_alloc_sz_no_padding = get_total_allocation_size(size, 0);
            base_alloc_sz_no_padding = max<uint32_t>(base_alloc_sz_no_padding, sizeof(ArenaFreeListNode));
            uint32_t const max_padding = alignment - 1;

            // Allocation meta
            ArenaFreeListNode *const free_slot = find_free_list_node(base_alloc_sz_no_padding + max_padding);

            jltassert(free_slot);
            ensure_free_memory_consistency(free_slot);

            auto const raw_alloc_ptr = reinterpret_cast<uint8_t *>(free_slot);
            auto const alloc_ptr =
              reinterpret_cast<uint8_t *>(align_raw_ptr(raw_alloc_ptr + sizeof(AllocHeader), alignment));
            uint32_t const padding =
              reinterpret_cast<uint8_t *>(alloc_ptr) - raw_alloc_ptr - sizeof(AllocHeader);
            uint32_t const base_alloc_sz = base_alloc_sz_no_padding + padding;
            size_t const slot_remaining_sz = free_slot->m_size - base_alloc_sz;

            // Avoid memory leaks by aggregating any extra space too small to host its own free list
            // node
            uint32_t const total_alloc_sz = choose(
              base_alloc_sz,
              static_cast<uint32_t>(free_slot->m_size),
              slot_remaining_sz >= sizeof(ArenaFreeListNode));

            auto const hdr_ptr = get_header(alloc_ptr);
            auto const alloc_end_ptr = raw_alloc_ptr + total_alloc_sz;

            // Heap meta
            auto const committed_mem_end_ptr = reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();
            bool const absorb_entire_node = total_alloc_sz == free_slot->m_size;

            if(alloc_end_ptr > committed_mem_end_ptr) {
                size_t const base_extend_size = alloc_end_ptr - committed_mem_end_ptr;
                size_t const total_extend_size =
                  choose(base_extend_size + sizeof(ArenaFreeListNode), base_extend_size, !absorb_entire_node);

                commit(total_extend_size);
            }

            ArenaFreeListNode *cur_slot;

            if(absorb_entire_node) {
                cur_slot = choose(free_slot->m_next, free_slot->m_prev, free_slot->m_next);

                delete_free_list_node(free_slot);
            } else {
                // There's enough room to split this free list node into two
                cur_slot = reinterpret_cast<ArenaFreeListNode *>(alloc_end_ptr);

                // Move node forward
                create_free_list_node(cur_slot, slot_remaining_sz, free_slot->m_prev, free_slot->m_next);
            }

            // Update the free list pointer if necessary
            // NOTE: It may look tempting to move this line in the else block above
            // but that would be wrong because that add_free_list_node() call is
            // actually *moving* `free_slot` to a different address.
            m_free_list = choose(m_free_list, cur_slot, m_free_list != free_slot);

            new(hdr_ptr) AllocHeader(
              base_alloc_sz_no_padding - sizeof(AllocHeader) - JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE,
              flags,
              padding,
              alignment);

            JLT_FILL_OVERFLOW(alloc_ptr, hdr_ptr->m_alloc_sz);
            m_allocated_size += total_alloc_sz;

            return alloc_ptr;
        }

        void Arena::free(void *const ptr) {
            auto const hdr_ptr = get_header(ptr);

#ifdef JLT_WITH_MEM_CHECKS
            jltassert(hdr_ptr->m_free_canary == JLT_MEM_ALLOC_HDR_CANARY_VALUE);
#endif // JLT_WITH_MEM_CHECKS
            JLT_CHECK_OVERFLOW(ptr, hdr_ptr->m_alloc_sz);

            uint32_t const total_alloc_size = get_total_allocation_size(ptr);
            void *const raw_alloc_ptr = reinterpret_cast<uint8_t *>(hdr_ptr) - hdr_ptr->m_alloc_offset;
            auto const node = reinterpret_cast<ArenaFreeListNode *>(raw_alloc_ptr);
            ArenaFreeListNode *left_closest_node = find_left_closest_node(raw_alloc_ptr);
            ArenaFreeListNode *const right_closest_node =
              left_closest_node ? left_closest_node->m_next
                                : find_right_closest_node(raw_alloc_ptr, total_alloc_size);

            create_free_list_node(node, total_alloc_size, left_closest_node, right_closest_node);

            if(left_closest_node && are_nodes_adjacent(left_closest_node, node)) {
                merge_adj_free_list_nodes(left_closest_node, node);
            } else {
                // The following statement is to simplify the merging logic
                // by allowing the next if condition to be evaluated
                // even if the left-closest is not merged with the node.
                left_closest_node = node;
            }

            if(right_closest_node && are_nodes_adjacent(left_closest_node, right_closest_node)) {
                merge_adj_free_list_nodes(left_closest_node, right_closest_node);
            }

            auto const fill_start_ptr =
              reinterpret_cast<uint8_t *>(left_closest_node) + sizeof(ArenaFreeListNode);
            size_t fill_sz = left_closest_node->m_size - sizeof(ArenaFreeListNode);
            uint8_t *const fill_end_ptr = fill_start_ptr + fill_sz;
            auto const committed_end_ptr = reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();

            if(fill_end_ptr > committed_end_ptr) {
                fill_sz = committed_end_ptr - fill_start_ptr;
            }

            JLT_FILL_AFTER_FREE(fill_start_ptr, fill_sz);
            // Keep this outside of the previous block as this check will encompass the case
            // when both left & right closest are null.
            m_free_list = choose(m_free_list, left_closest_node, m_free_list != right_closest_node);
            m_allocated_size -= total_alloc_size;
        }

        ArenaFreeListNode *Arena::find_free_list_node(size_t size) const {
            for(ArenaFreeListNode *ptr = get_free_list(); ptr; ptr = ptr->m_next) {
                if(ptr->m_size >= size) {
                    return ptr;
                }
            }

            for(ArenaFreeListNode *ptr = get_free_list(); ptr; ptr = ptr->m_prev) {
                if(ptr->m_size >= size) {
                    return ptr;
                }
            }

            return nullptr;
        }

        void Arena::ensure_free_memory_consistency(ArenaFreeListNode *const node) const {
#ifdef JLT_WITH_MEM_CHECKS
            auto const far_end_ptr = reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();
            auto const node_end_ptr = reinterpret_cast<uint8_t *>(node) + node->m_size;
            auto const free_mem_ptr = reinterpret_cast<uint8_t *>(node) + sizeof(ArenaFreeListNode);
            size_t size_to_check = node->m_size - sizeof(ArenaFreeListNode);

            if(node_end_ptr > far_end_ptr) {
                // Reduce the checked area as part of this node's memory goes beyond
                // the actually allocated memory
                ptrdiff_t const excess = node_end_ptr - far_end_ptr;
                size_to_check -= excess;
            }

            jltassert(JLT_CHECK_MEM_USE_AFTER_FREE(free_mem_ptr, size_to_check));
#endif // JLT_WITH_MEM_CHECKS
        }

        void *Arena::reallocate_shrink(void *const ptr, uint32_t const new_size, AllocHeader *const ptr_hdr) {
            uint32_t const extent = ptr_hdr->m_alloc_sz - new_size;

            if(extent >= sizeof(ArenaFreeListNode) && new_size >= sizeof(ArenaFreeListNode)) {
                void *const new_node_raw_ptr =
                  reinterpret_cast<uint8_t *>(ptr) + new_size + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
                ArenaFreeListNode *const prev_node = find_left_closest_node(ptr);
                ArenaFreeListNode *const next_node =
                  prev_node
                    ? prev_node->m_next
                    : find_right_closest_node(ptr, ptr_hdr->m_alloc_sz + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);
                auto const new_node_ptr = reinterpret_cast<ArenaFreeListNode *>(new_node_raw_ptr);
                ptr_hdr->m_alloc_sz = new_size;

                JLT_FILL_AFTER_FREE(new_node_raw_ptr, extent);

                create_free_list_node(new_node_ptr, extent, prev_node, next_node);

                if(next_node && are_nodes_adjacent(new_node_ptr, next_node)) {
                    merge_adj_free_list_nodes(new_node_ptr, next_node);
                }

                // Update free list if necessary
                m_free_list = choose(m_free_list, new_node_ptr, m_free_list);
                m_allocated_size -= extent;

                JLT_FILL_OVERFLOW(ptr, new_size);
            }

            return ptr;
        }

        bool Arena::will_relocate(void *const ptr, uint32_t const new_size) const {
            AllocHeader *const ptr_hdr = get_header(ptr);
            void *const alloc_end_ptr =
              reinterpret_cast<uint8_t *>(ptr) + ptr_hdr->m_alloc_sz + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
            uint32_t const extent = new_size - ptr_hdr->m_alloc_sz;
            ArenaFreeListNode *next_node =
              find_right_closest_node(ptr, ptr_hdr->m_alloc_sz + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);

            return (!next_node || next_node != alloc_end_ptr || next_node->m_size < extent);
        }

        void *Arena::reallocate_grow(void *const ptr, uint32_t const new_size, AllocHeader *const ptr_hdr) {
            void *const alloc_end_ptr =
              reinterpret_cast<uint8_t *>(ptr) + ptr_hdr->m_alloc_sz + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
            uint32_t const extent = new_size - ptr_hdr->m_alloc_sz;
            ArenaFreeListNode *next_node =
              find_right_closest_node(ptr, ptr_hdr->m_alloc_sz + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE);

            if(!next_node || next_node != alloc_end_ptr || next_node->m_size < extent) {
                void *const new_alloc_ptr = allocate(new_size, ptr_hdr->m_flags, ptr_hdr->m_alignment);

                memmove(new_alloc_ptr, ptr, ptr_hdr->m_alloc_sz);
                free(ptr);

                return new_alloc_ptr;
            } else {
                const uint8_t *const committed_end_ptr =
                  reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();

                // Absorb entire node if it doesn't contain enough space to hold another free list node
                const bool absorb_entire_node = next_node->m_size - extent < sizeof(ArenaFreeListNode);
                const uint32_t total_grow_size =
                  choose<uint32_t>(next_node->m_size, extent, absorb_entire_node);

                uint8_t *const new_alloc_end_ptr =
                  reinterpret_cast<uint8_t *>(alloc_end_ptr) + total_grow_size;

                ptr_hdr->m_alloc_sz += total_grow_size;
                m_allocated_size += total_grow_size;

                if(new_alloc_end_ptr > committed_end_ptr) {
                    commit(new_alloc_end_ptr - committed_end_ptr);
                }

                ensure_free_memory_consistency(next_node);

                if(absorb_entire_node) {
                    ArenaFreeListNode *const prev = next_node->m_prev;
                    ArenaFreeListNode *const next = next_node->m_next;

                    delete_free_list_node(next_node);

                    // Update free list pointer if necessary
                    m_free_list = choose(m_free_list, choose(prev, next, prev), m_free_list != next_node);
                } else {
                    ArenaFreeListNode *const new_node_ptr =
                      reinterpret_cast<ArenaFreeListNode *>(new_alloc_end_ptr);

                    // Move `next_node` forward
                    create_free_list_node(
                      new_node_ptr,
                      next_node->m_size - total_grow_size,
                      next_node->m_prev,
                      next_node->m_next);

                    // Update free list if necessary
                    m_free_list = choose(m_free_list, new_node_ptr, m_free_list != next_node);
                }

                JLT_FILL_OVERFLOW(ptr, ptr_hdr->m_alloc_sz);

                return ptr;
            }
        }

        void *Arena::reallocate(void *const ptr, uint32_t const new_size) {
            AllocHeader *const ptr_hdr = get_header(ptr);

#ifdef JLT_WITH_MEM_CHECKS
            jltassert(ptr_hdr->m_free_canary == JLT_MEM_ALLOC_HDR_CANARY_VALUE);
#endif // JLT_WITH_MEM_CHECKS
            JLT_CHECK_OVERFLOW(ptr, ptr_hdr->m_alloc_sz);

            if(new_size < ptr_hdr->m_alloc_sz) {
                return reallocate_shrink(ptr, new_size, ptr_hdr);
            } else if(new_size > ptr_hdr->m_alloc_sz) {
                return reallocate_grow(ptr, new_size, ptr_hdr);
            }

            return ptr;
        }
    } // namespace memory
} // namespace jolt
