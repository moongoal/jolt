#ifndef JLT_COLLECTIONS_LINKEDLIST_HPP
#define JLT_COLLECTIONS_LINKEDLIST_HPP

#include <initializer_list>
#include <jolt/api.hpp>
#include <jolt/memory/allocator.hpp>
#include "iterator.hpp"

namespace jolt {
    namespace collections {
        template<typename Node>
        struct LinkedListIteratorImpl {
            using item_type = typename Node::value_type;

            static constexpr Node *forward(Node *const cur, size_t const n) {
                Node *result = cur;

                for(size_t i = 0; i < n; ++i) { result = result->get_next(); }

                return result;
            }

            static constexpr Node *backward(Node *const cur, size_t const n) {
                Node *result = cur;

                for(size_t i = 0; i < n; ++i) { result = result->get_previous(); }

                return result;
            }

            JLT_NODISCARD static constexpr int compare(const Node *const left, const Node *const right) {
                return left != right;
            }

            template<typename NodeT>
            JLT_NODISCARD static constexpr auto &resolve(NodeT *const node) {
                return node->get_value();
            }

            JLT_NODISCARD static constexpr size_t
            count_elements(Node const *const from, Node const *const to) {
                if(from == to && from == nullptr) {
                    return 0;
                }

                size_t count = 1;

                for(Node const *n = from; n != to && n; n = n->get_next()) { ++count; }

                return count;
            }
        };

        /**
         * Doubly linked list.
         *
         * @tparam T The type of item contained in each node of the list.
         */
        template<typename T>
        class LinkedList {
          public:
            using value_type = T;
            using pointer = T *;
            using const_pointer = const T *;
            using reference = T &;
            using const_reference = const T &;

            class Node {
                friend class LinkedList<T>;

              public:
                using value_type = value_type;

              private:
                value_type m_value;
                Node mutable *m_next;
                Node mutable *m_prev;

              public:
                JLT_NODISCARD Node(const_reference value, Node *const next, Node *const prev) :
                  m_value{value}, m_next{next}, m_prev{prev} {}

                JLT_NODISCARD Node *get_next() const { return m_next; }
                JLT_NODISCARD Node *get_previous() const { return m_prev; }
                JLT_NODISCARD reference get_value() { return m_value; }
                JLT_NODISCARD const_reference get_value() const { return m_value; }
                void set_value(const_reference value) { m_value = value; }
            };

            template<typename E>
            using base_iterator = Iterator<E, LinkedListIteratorImpl<E>>;
            using iterator = base_iterator<Node>;
            using const_iterator = base_iterator<const Node>;

          private:
            Node *m_first;                 //< First node in the list.
            Node *m_last;                  //< Last node in the list.
            size_t m_length;               //< List length.
            memory::flags_t m_alloc_flags; //< Allocator flags.

          public:
            /**
             * Create a new empty list.
             */
            JLT_NODISCARD LinkedList() :
              m_first{nullptr}, m_last{nullptr}, m_length{0}, m_alloc_flags{
                                                                memory::get_current_force_flags()} {}

            /**
             * Create a new instance of this class.
             *
             * @param lst The initializer list.
             */
            JLT_NODISCARD LinkedList(const std::initializer_list<value_type> &lst) :
              LinkedList{lst.begin(), lst.end()} {}

            /**
             * Create a new instance of this class.
             *
             * @param begin The iterator pointing to the first element to add.
             * @param end The iterator pointing right after the last element to add.
             */
            template<typename It>
            JLT_NODISCARD LinkedList(It const &begin, It const &end) : LinkedList{} {
                add_all(begin, end);
            }

            /**
             * Create a new list, copy of another.
             *
             * @param other The other list.
             */
            JLT_NODISCARD LinkedList(const LinkedList &other) : LinkedList{other.cbegin(), other.cend()} {}

            /**
             * Create a new list, taking ownership of the data of another.
             *
             * @param other The other list.
             *
             * @remarks After this constructor returns, the other list will be empty.
             */
            JLT_NODISCARD LinkedList(LinkedList &&other) :
              m_first{other.m_first}, m_last{other.m_last}, m_length{other.m_length}, m_alloc_flags{
                                                                                        other.m_alloc_flags} {
                other.m_first = other.m_last = nullptr;
                other.m_length = 0;
            }

            ~LinkedList() { clear(); }

            /**
             * Add a new item at the end of the list.
             *
             * @param item The item to add.
             */
            Node *add(const_reference item) { return add_after(item, m_last); }

            /**
             * Add a new item right after the specified node.
             *
             * @param item The item to add.
             * @param where The node after which to add the new item. Use `nullptr` to add the item
             * at the beginning of the list.
             */
            Node *add_after(const_reference item, Node *const where) {
                memory::push_force_flags(m_alloc_flags);

                Node *const new_node = memory::allocate_and_construct<Node>(item, nullptr, nullptr);

                if(where) {
                    new_node->m_prev = where;
                    new_node->m_next = where->m_next;

                    where->m_next = new_node;

                    if(new_node->m_next) {
                        new_node->m_next->m_prev = new_node;
                    }
                } else {
                    Node *const old_first = m_first;

                    m_first = new_node;
                    new_node->m_next = old_first;

                    if(old_first) {
                        old_first->m_prev = new_node;
                    }
                }

                if(where == m_last) {
                    m_last = new_node;
                }

                ++m_length;

                memory::pop_force_flags();

                return new_node;
            }

            /**
             * Add all the specified items at the end of the list.
             *
             * @param begin The iterator pointing to the first element to add.
             * @param end The iterator pointing right after the last element to add.
             */
            template<typename It>
            void add_all(It const &begin, It const &end) {
                add_all_after(begin, end, m_last);
            }

            /**
             * Add all the specified items after the specified node.
             *
             * @param begin The iterator pointing to the first element to add.
             * @param end The iterator pointing right after the last element to add.
             * @param where The node after which to add the new items. Use `nullptr` to add the
             * items at the beginning of the list.
             */
            template<typename It>
            void add_all_after(It const &begin, It const &end, Node *const where) {
                Node *last = where;

                for(It it = begin; it != end; ++it) { last = add_after(*it, last); }
            }

            ///@{
            /**
             * Find an item.
             *
             * @param item The value to find.
             *
             * @return The node for the given item or `nullptr` if the item was not found.
             *
             * @remarks Comparison is checked via the equality operator.
             */
            JLT_NODISCARD Node *find(const_reference item) {
                const_iterator end = cend();

                for(iterator it = begin(); it != end; ++it) {
                    if(*it == item) {
                        return it.get_pointer();
                    }
                }

                return nullptr;
            }

            JLT_NODISCARD const Node *find(const_reference item) const {
                LinkedList *const self = const_cast<LinkedList *>(this);

                return self->find(item);
            }
            ///@}

            /**
             * Remove a node from the list.
             *
             * @param node The node to remove.
             */
            void remove(Node &node) {
                Node *const prev = node.get_previous();

                if(prev) {
                    prev->m_next = node.m_next;

                    if(node.m_next) {
                        node.m_next->m_prev = prev;
                    }
                } else {
                    jltassert2(
                      m_first == &node, "Attempting to remove a node that is not part of this linked list");

                    m_first = node.m_next;

                    if(m_first) {
                        m_first->m_prev = nullptr;
                    }
                }

                if(m_last == &node) {
                    m_last = prev;
                }

                --m_length;

                jolt::memory::free(&node);
            }

            /**
             * Remove all the items from the list.
             */
            void clear() {
                Node *node = m_first;

                while(node) {
                    Node *const next = node->m_next;

                    jolt::memory::free(node);

                    node = next;
                }

                m_first = m_last = nullptr;
                m_length = 0;
            }

            JLT_NODISCARD size_t get_length() const { return m_length; }
            JLT_NODISCARD reference get_first() { return m_first->m_value; }
            JLT_NODISCARD reference get_last() { return m_last->m_value; }
            JLT_NODISCARD const_reference get_first() const { return m_first->m_value; }
            JLT_NODISCARD const_reference get_last() const { return m_last->m_value; }
            JLT_NODISCARD Node *get_first_node() const { return const_cast<Node *>(m_first); }
            JLT_NODISCARD Node *get_last_node() const { return const_cast<Node *>(m_last); }

            JLT_NODISCARD constexpr iterator begin() { return iterator{m_first}; }
            JLT_NODISCARD constexpr iterator end() { return iterator{nullptr}; }
            JLT_NODISCARD constexpr const_iterator begin() const { return const_iterator{m_first}; }
            JLT_NODISCARD constexpr const_iterator end() const { return const_iterator{nullptr}; }
            JLT_NODISCARD constexpr const_iterator cbegin() const { return const_iterator{m_first}; }
            JLT_NODISCARD constexpr const_iterator cend() const { return const_iterator{nullptr}; }
        };
    } // namespace collections
} // namespace jolt

#endif /* JLT_COLLECTIONS_LINKEDLIST_HPP */
