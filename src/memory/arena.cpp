#include <cstring>
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
            uint32_t adjusted_size = get_total_allocation_size(size, 0);
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
                align_raw_ptr(raw_alloc_ptr + sizeof(AllocHeader), alignment));
            auto const hdr_ptr = get_header(alloc_ptr);
            uint32_t const padding =
                reinterpret_cast<uint8_t *>(alloc_ptr) - raw_alloc_ptr - sizeof(AllocHeader);
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

            new(hdr_ptr)
                AllocHeader(adjusted_size - sizeof(AllocHeader) - JLT_MEM_CANARY_VALUE_SIZE,
                            flags,
                            padding);

            JLT_FILL_OVERFLOW(alloc_ptr, size);
            m_allocated_size += adjusted_size + padding;

            return alloc_ptr;
        }

        void Arena::free(void *const ptr) {
            auto const hdr_ptr = get_header(ptr);

#ifdef JLT_WITH_MEM_CHECKS
            jltassert(hdr_ptr->m_free_canary == JLT_MEM_CANARY_VALUE);
#endif // JLT_WITH_MEM_CHECKS
            JLT_CHECK_OVERFLOW(ptr, hdr_ptr->m_alloc_sz);

            size_t const total_alloc_size = get_total_allocation_size(ptr);
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

            auto const fill_start_ptr =
                reinterpret_cast<uint8_t *>(left_closest_node) + sizeof(ArenaFreeListNode);
            size_t fill_sz = left_closest_node->m_size - sizeof(ArenaFreeListNode);
            uint8_t *const fill_end_ptr = fill_start_ptr + fill_sz;
            auto const committed_end_ptr =
                reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();

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

        void *Arena::reallocate(void *const ptr, size_t const new_size, size_t const alignment) {
            AllocHeader *const ptr_hdr = get_header(ptr);
            size_t const total_alloc_size = get_total_allocation_size(ptr);

            if(new_size == ptr_hdr->m_alloc_sz) {       // You joking?
            } else if(new_size < ptr_hdr->m_alloc_sz) { // Shrink
                size_t const extent = ptr_hdr->m_alloc_sz - new_size;

                if(extent >= sizeof(ArenaFreeListNode)) {
                    void *const new_node_raw_ptr =
                        reinterpret_cast<uint8_t *>(ptr) + new_size + JLT_MEM_CANARY_VALUE_SIZE;
                    ArenaFreeListNode *const prev_node = find_left_closest_node(ptr);
                    ArenaFreeListNode *const next_node =
                        prev_node ? prev_node->m_next
                                  : find_right_closest_node(ptr,
                                                            ptr_hdr->m_alloc_sz +
                                                                JLT_MEM_CANARY_VALUE_SIZE);
                    auto const new_node_ptr =
                        reinterpret_cast<ArenaFreeListNode *>(new_node_raw_ptr);
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
            } else { // Grow
                void *const alloc_end_ptr = reinterpret_cast<uint8_t *>(ptr) + ptr_hdr->m_alloc_sz +
                                            +JLT_MEM_CANARY_VALUE_SIZE;
                size_t const extent = new_size - ptr_hdr->m_alloc_sz;
                ArenaFreeListNode *next_node =
                    find_right_closest_node(ptr, ptr_hdr->m_alloc_sz + JLT_MEM_CANARY_VALUE_SIZE);

                if(!next_node || next_node != alloc_end_ptr || next_node->m_size < extent) {
                    void *const new_alloc_ptr = allocate(new_size, ALLOC_NONE, alignment);

                    memmove(new_alloc_ptr, ptr, ptr_hdr->m_alloc_sz);
                    free(ptr);

                    return new_alloc_ptr;
                } else {
                    if(reinterpret_cast<uint8_t *>(alloc_end_ptr) + extent >
                       reinterpret_cast<uint8_t *>(get_base()) + get_committed_size()) {
                        commit(extent);
                    }

                    if(next_node->m_size - extent < sizeof(ArenaFreeListNode)) {
                        // Absorb entire node since there is not enough space
                        // to create another one
                        ptr_hdr->m_alloc_sz += next_node->m_size;
                        m_allocated_size += next_node->m_size;

                        ensure_free_memory_consistency(next_node);
                        remove_free_list_node(next_node);

                        // Update free list pointer if necessary
                        m_free_list =
                            choose(m_free_list,
                                   choose(next_node->m_prev, next_node->m_next, next_node->m_prev),
                                   m_free_list != next_node);
                    } else {
                        // Subtract the portion of free space we need and move the free list node
                        // forward
                        ArenaFreeListNode *const new_node_ptr =
                            reinterpret_cast<ArenaFreeListNode *>(
                                reinterpret_cast<uint8_t *>(alloc_end_ptr) + extent);

                        ptr_hdr->m_alloc_sz += extent;
                        m_allocated_size += extent;

                        create_free_list_node(new_node_ptr,
                                              next_node->m_size - extent,
                                              next_node->m_prev,
                                              next_node->m_next);

                        // Update free list if necessary
                        m_free_list = choose(m_free_list, new_node_ptr, m_free_list != next_node);
                    }

                    JLT_FILL_OVERFLOW(ptr, ptr_hdr->m_alloc_sz);
                }
            }

            return ptr;
        }
    } // namespace memory
} // namespace jolt
