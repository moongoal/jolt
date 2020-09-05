#include "checks.hpp"
#include "arena.hpp"

namespace jolt {
    namespace memory {
        /**
         * Return a boolean value stating whether two existing nodes are adjacent in terms of memory
         * address.
         */
        static inline bool are_nodes_adjacent(ArenaFreeListNode *const left,
                                              ArenaFreeListNode *const right) {
            return reinterpret_cast<uint8_t *>(left) + left->m_size ==
                   reinterpret_cast<uint8_t *>(right);
        }

        /**
         * Create a free list node at a given memory location.
         *
         * @param node The memory location at which to create the new node.
         * @param size The size of the free memory chunk.
         * @param prev The previous node or nullptr.
         * @param next The next node or nullptr.
         */
        static void create_free_list_node(ArenaFreeListNode *const node,
                                          uint32_t const size,
                                          ArenaFreeListNode *const prev,
                                          ArenaFreeListNode *const next) {
            new(node) ArenaFreeListNode(size, prev, next);

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
        static void remove_free_list_node(ArenaFreeListNode *const node) {
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
         * @param right The noed with the highest memory address.
         */
        static void merge_adj_free_list_nodes(ArenaFreeListNode *const left,
                                              ArenaFreeListNode *const right) {

            left->m_size += right->m_size;
            left->m_next = right->m_next;

            if(right->m_next) {
                right->m_next->m_prev = left;
            }
        }

        ArenaFreeListNode *Arena::find_left_closest_node(void *const ptr) const {
            ArenaFreeListNode *res = get_free_list();

            for(ArenaFreeListNode *node = get_free_list(); node; node = node->m_next) {
                void *const node_end_ptr = reinterpret_cast<uint8_t *>(node) + node->m_size;

                if(node_end_ptr < ptr && node_end_ptr > res) {
                    res = node;
                }
            }

            for(ArenaFreeListNode *node = get_free_list(); node; node = node->m_prev) {
                void *const node_end_ptr = reinterpret_cast<uint8_t *>(node) + node->m_size;

                if(node_end_ptr < ptr && node_end_ptr > res) {
                    res = node;
                }
            }

            // This is needed because if only one node is available and is > ptr
            // It'll still be in `res` at this point.
            return choose(res,
                          static_cast<ArenaFreeListNode *>(nullptr),
                          res && (reinterpret_cast<uint8_t *>(res) + res->m_size) <= ptr);
        }

        ArenaFreeListNode *Arena::find_right_closest_node(void *const ptr,
                                                          size_t const size) const {
            ArenaFreeListNode *res = get_free_list();
            void *const end_ptr = reinterpret_cast<uint8_t *>(ptr) + size;

            for(ArenaFreeListNode *node = get_free_list(); node; node = node->m_next) {
                if(node > end_ptr && node < res) {
                    res = node;
                }
            }

            for(ArenaFreeListNode *node = get_free_list(); node; node = node->m_prev) {
                if(node > end_ptr && node < res) {
                    res = node;
                }
            }

            // Same as in sibling function.
            return choose(res, static_cast<ArenaFreeListNode *>(nullptr), res && res >= end_ptr);
        }

        Arena::Arena(size_t const memory_size) :
            Heap(memory_size), m_allocated_size(sizeof(ArenaFreeListNode)) {
            commit(sizeof(ArenaFreeListNode));

            m_free_list = new(get_base()) ArenaFreeListNode(memory_size, nullptr, nullptr);
        }

        void *Arena::allocate(uint32_t const size, flags_t const flags, uint32_t const alignment) {
            uint32_t adjusted_size = size + sizeof(ArenaAllocHeader)
#ifdef JLT_WITH_MEM_CHECKS
                                     + JLT_MEM_CANARY_VALUE_SIZE;
#endif // JLT_WITH_MEM_CHECKS
            ;
            adjusted_size = max(adjusted_size, static_cast<uint32_t>(sizeof(ArenaFreeListNode)));

            // Allocation meta
            ArenaFreeListNode *const free_slot = find_free_list_node(adjusted_size + alignment - 1);

            jltassert(free_slot);
            ensure_free_memory_consistency(free_slot);

            // Avoid memory leaks by aggregating any extra space too small to host its own free list
            // node
            adjusted_size = choose(adjusted_size,
                                   static_cast<uint32_t>(free_slot->m_size),
                                   free_slot->m_size - adjusted_size >= sizeof(ArenaFreeListNode));

            auto const raw_alloc_ptr = reinterpret_cast<uint8_t *>(free_slot);
            auto const alloc_ptr = reinterpret_cast<uint8_t *>(
                align_raw_ptr(raw_alloc_ptr + sizeof(ArenaAllocHeader), alignment));
            auto const hdr_ptr = reinterpret_cast<ArenaAllocHeader *>(alloc_ptr) - 1;
            uint32_t const padding =
                reinterpret_cast<uint8_t *>(alloc_ptr) - raw_alloc_ptr - sizeof(ArenaAllocHeader);
            auto const alloc_end_ptr = reinterpret_cast<uint8_t *>(hdr_ptr) + adjusted_size;

            // Pool meta
            auto const committed_mem_end_ptr =
                reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();

            if(alloc_end_ptr > committed_mem_end_ptr) {
                size_t const base_extend_size = alloc_end_ptr - committed_mem_end_ptr;

                commit(choose(base_extend_size + sizeof(ArenaFreeListNode),
                              base_extend_size,
                              adjusted_size != free_slot->m_size));
            }

            ArenaFreeListNode *cur_slot;

            if(adjusted_size != free_slot->m_size) {
                // There's enough room to split this free list node into two
                cur_slot = reinterpret_cast<ArenaFreeListNode *>(alloc_end_ptr);

                // Move node forward
                create_free_list_node(cur_slot,
                                      free_slot->m_size - adjusted_size - padding,
                                      free_slot->m_prev,
                                      free_slot->m_next);
            } else {
                cur_slot = free_slot->m_next;

                remove_free_list_node(free_slot);
            }

            // Update the free list pointer if necessary
            // NOTE: It may look tempting to move this line in the else block above
            // but that would be wrong because that add_free_list_node() call is
            // actually *moving* the node to a different address.
            m_free_list = choose(m_free_list, cur_slot, m_free_list != free_slot);

            new(hdr_ptr) ArenaAllocHeader(adjusted_size - sizeof(ArenaAllocHeader)
#ifdef JLT_WITH_MEM_CHECKS
                                              - JLT_MEM_CANARY_VALUE_SIZE
#endif // JLT_WITH_MEM_CHECKS
                                          ,
                                          padding);

#ifdef JLT_WITH_MEM_CHECKS
            JLT_FILL_OVERFLOW(alloc_ptr, size);
#endif // JLT_WITH_MEM_CHECKS

            return alloc_ptr;
        }

        void Arena::free(void *const ptr) {
            auto const hdr_ptr = reinterpret_cast<ArenaAllocHeader *>(ptr) - 1;

#ifdef JLT_WITH_MEM_CHECKS
            jltassert(hdr_ptr->m_free_canary == JLT_MEM_CANARY_VALUE);
            JLT_CHECK_OVERFLOW(ptr, hdr_ptr->m_alloc_sz);
#endif // JLT_WITH_MEM_CHECKS

            size_t const total_alloc_size = hdr_ptr->m_alloc_sz + hdr_ptr->m_alloc_offset +
                                            sizeof(ArenaAllocHeader)
#ifdef JLT_WITH_MEM_CHECKS
                                            + JLT_MEM_CANARY_VALUE_SIZE
#endif // JLT_WITH_MEM_CHECKS
                ;
            void *const raw_alloc_ptr =
                reinterpret_cast<uint8_t *>(hdr_ptr) - hdr_ptr->m_alloc_offset;
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

            JLT_FILL_AFTER_FREE(reinterpret_cast<uint8_t *>(left_closest_node) +
                                    sizeof(ArenaFreeListNode),
                                left_closest_node->m_size - sizeof(ArenaFreeListNode));

            // Keep this outside of the previous block as this check will encompass the case
            // when both left & right closest are null.
            m_free_list = choose(m_free_list, left_closest_node, m_free_list != right_closest_node);
        }

        ArenaFreeListNode *Arena::find_free_list_node(size_t size) const {
            for(ArenaFreeListNode *ptr = m_free_list; ptr != nullptr; ptr = ptr->m_next) {
                if(ptr->m_size >= size) {
                    return ptr;
                }
            }

            for(ArenaFreeListNode *ptr = m_free_list; ptr != nullptr; ptr = ptr->m_prev) {
                if(ptr->m_size >= size) {
                    return ptr;
                }
            }

            return nullptr;
        }

        void Arena::ensure_free_memory_consistency(ArenaFreeListNode *const node) const {
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
        }
    } // namespace memory
} // namespace jolt
