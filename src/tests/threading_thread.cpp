#include <atomic>
#include <utility>
#include <chrono>
#include <emmintrin.h>
#include <mmintrin.h>
#include <jolt/test.hpp>
#include <jolt/threading/threadguard.hpp>

using namespace jolt::threading;

SETUP { initialize(); }

void run_handler(void *param) noexcept {
    std::atomic<int> *x = reinterpret_cast<std::atomic<int> *>(param);

    x->store(1);
}

TEST(start) {
    using duration = std::chrono::duration<double>;
    constexpr int timeout = 5;

    std::atomic<int> x = 0;
    Thread t{&::run_handler};
    ThreadGuard t_guard{t};

    auto t_begin = std::chrono::steady_clock::now();

    t.start(&x);

    while(!x.load()) {
        auto t_end = std::chrono::steady_clock::now();
        duration tdelta = t_end - t_begin;

        assert(tdelta.count() < timeout);
        _mm_pause();
    }
}

TEST(operator_assign) {
    Thread t{&run_handler};
    Thread t2 = std::move(t);

    assert(t2.get_handler() == &::run_handler);
    assert(t2.get_state() == ThreadState::Created);
    assert(t.get_state() == ThreadState::Invalid);
}

TEST(ctor_move) {
    const char *const thr_name = "My super thread";
    Thread t{&run_handler, thr_name};
    Thread t2{std::move(t)};

    assert(t2.get_handler() == &::run_handler);
    assert(t2.get_state() == ThreadState::Created);
    assert(t2.get_name() == thr_name);
    assert(t.get_state() == ThreadState::Invalid);
    assert(t.get_name() == thr_name);
}

// TEST(ctor_default) {
//     Thread t;

//     assert(t.get_state() == ThreadState::Invalid);
//     assert(t.get_handler() == nullptr);
//     assert(t.get_id() == INVALID_THREAD_ID);
//     assert(t.get_os_id() == INVALID_OS_THREAD_ID);
//     assert(t.get_name() == UNNAMED_THREAD_NAME);
// }

void join_handler(void *param) noexcept { sleep(1000); }

TEST(join) {
    Thread t{&join_handler};

    auto t_begin = std::chrono::steady_clock::now();
    t.start(nullptr);
    t.join();
    auto t_end = std::chrono::steady_clock::now();
    std::chrono::duration<double> t_delta = t_end - t_begin;

    assert(t_delta.count() >= 1 && t_delta.count() < 10);
}

TEST(try_join) {
    Thread t{&join_handler};

    t.start(nullptr);
    bool const should_fail = t.try_join(10);
    assert2(!should_fail, "Didn't fail where it should have");

    bool const should_succeed = t.try_join(1200);
    assert2(should_succeed, "Didn't succeed where it should have");
}
