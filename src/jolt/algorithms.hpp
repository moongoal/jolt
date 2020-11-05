#ifndef JLT_ALGORITHMS_HPP
#define JLT_ALGORITHMS_HPP

#include <jolt/collections/vector.hpp>
#include <type_traits>

namespace jolt::algorithms {
    /**
     * Quick-sort elements in an array in place.
     *
     * @param ptr Pointer to the array to sort.
     * @param len Length of the array to sort.
     * @param key A lambda expression accepting a single parameter, an item or reference to an item from the
     * array, and providing as output a comparable key value or reference. Keys will be compared against
     * each-other to sort the items in the array.
     *
     * Example:
     * @code
     * int arr[] = {3, 2, 1};
     * sort(arr, 3, [](int x) { return x; });
     * @endcode
     */
    template<typename value_type>
    void constexpr quicksort(value_type *const ptr, size_t const len, auto key) {
        using pointer = value_type *;
        using key_type = decltype(key(*ptr));

        struct Frame {
            pointer ptr;
            size_t len;
        };

        collections::Vector<Frame> stack;
        stack.push({ptr, len});

        while(stack.get_length()) {
            // Pop so it's more efficient on the vector side
            // and does depth-first scan so it's more cache efficient.
            auto const [p, p_len] = stack.pop(); // p is beginning of array
            pointer pivot = p + (p_len / 2);     // Choose pivot index
            key_type const pivot_value = key(*pivot);
            pointer const p_end = p + p_len;
            pointer l = p, r = p_end - 1; // Left and right pointer

            // Partition
            while(l < r) {
                key_type l_value = key(*l);
                key_type r_value = key(*r);

                while(l_value <= pivot_value && l < pivot) {
                    ++l;
                    l_value = key(*l);
                }

                while(r_value >= pivot_value && r > pivot) {
                    --r;
                    r_value = key(*r);
                }

                if(l_value > r_value) {
                    value_type tmp = *l;
                    *l = *r;
                    *r = tmp;

                    pivot = choose(r, choose(l, pivot, r == pivot), l == pivot);
                }
            }

            // Add next, left partition first
            long long const left_len = pivot - p;
            long long const right_len = p_end - pivot - 1;

            if(left_len > 1) {
                stack.push({p, static_cast<size_t>(left_len)});
            }

            if(right_len > 1) {
                stack.push({pivot + 1, static_cast<size_t>(right_len)});
            }
        }
    }
} // namespace jolt::algorithms

#endif /* JLT_ALGORITHMS_HPP */
