#include <jolt/test.hpp>
#include <jolt/jolt.hpp>
#include <jolt/algorithms.hpp>

using namespace jolt;
using namespace jolt::algorithms;

SETUP { initialize(); }

CLEANUP { shutdown(); }

TEST(quicksort) {
    const int input[] = {1, -2, -3, 4, 4, 5, 0, 1, 2, 0};
    size_t constexpr input_len = sizeof(input) / sizeof(int);
    int const expected_output[input_len] = {-3, -2, 0, 0, 1, 1, 2, 4, 4, 5};
    int actual_output[input_len];

    memcpy(actual_output, input, sizeof(input));

    quicksort(actual_output, input_len, [](int x) { return x; });

    for(size_t i = 0; i < input_len; ++i) { assert(actual_output[i] == expected_output[i]); }
}

TEST(sort__2) {
    const int input[] = {5, -2, -3, 4, 4, -4, 0, 1, 2, 0};
    size_t constexpr input_len = sizeof(input) / sizeof(int);
    int const expected_output[input_len] = {-4, -3, -2, 0, 0, 1, 2, 4, 4, 5};
    int actual_output[input_len];

    memcpy(actual_output, input, sizeof(input));

    quicksort(actual_output, input_len, [](int x) { return x; });

    for(size_t i = 0; i < input_len; ++i) { assert(actual_output[i] == expected_output[i]); }
}

TEST(sort__same_value) {
    const int input[] = {0, 0, 0, 0};
    size_t constexpr input_len = sizeof(input) / sizeof(int);
    int actual_output[input_len];

    memcpy(actual_output, input, sizeof(input));

    quicksort(actual_output, input_len, [](int x) { return x; });

    for(size_t i = 0; i < input_len; ++i) { assert(actual_output[i] == input[i]); }
}

TEST(sort__already_sorted) {
    const int input[] = {0, 1, 3, 9};
    size_t constexpr input_len = sizeof(input) / sizeof(int);
    int actual_output[input_len];

    memcpy(actual_output, input, sizeof(input));

    quicksort(actual_output, input_len, [](int x) { return x; });

    for(size_t i = 0; i < input_len; ++i) { assert(actual_output[i] == input[i]); }
}

TEST(sort__reversed) {
    const int input[] = {5, 4, 3, 2, 1};
    size_t constexpr input_len = sizeof(input) / sizeof(int);
    int const expected_output[input_len] = {1, 2, 3, 4, 5};
    int actual_output[input_len];

    memcpy(actual_output, input, sizeof(input));

    quicksort(actual_output, input_len, [](int x) { return x; });

    for(size_t i = 0; i < input_len; ++i) { assert(actual_output[i] == expected_output[i]); }
}

TEST(sort__struct) {
    struct SortStruct {
        int a;
    };

    const SortStruct input[] = {{1}, {-2}, {-3}, {4}, {4}, {5}, {0}, {1}, {2}, {0}};
    size_t constexpr input_len = sizeof(input) / sizeof(int);
    SortStruct const expected_output[input_len] = {{-3}, {-2}, {0}, {0}, {1}, {1}, {2}, {4}, {4}, {5}};
    SortStruct actual_output[input_len];

    memcpy(actual_output, input, sizeof(input));

    quicksort(actual_output, input_len, [](SortStruct const &x) { return x.a; });

    for(size_t i = 0; i < input_len; ++i) { assert(actual_output[i].a == expected_output[i].a); }
}
