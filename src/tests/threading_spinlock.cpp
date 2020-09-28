#include <jolt/test.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/threading/spinlock.hpp>

using namespace jolt::threading;

SETUP { initialize(); }

struct test_data {
    volatile SpinLock lock;
    volatile bool complete = false;
    volatile bool can_start = false;
    volatile bool other_thread_started = false;
    volatile bool done = false;
};

void lock_handler(void *param) noexcept {
    test_data *tdata = reinterpret_cast<test_data *>(param);

    while(!tdata->can_start) { _mm_pause(); }

    tdata->other_thread_started = true;
    tdata->lock.acquire();
    tdata->complete = true;
    tdata->lock.release();
    tdata->done = true;
}

TEST(acquire_release_st) {
    SpinLock lock;

    assert2(!lock.is_acquired(), "Invalid initial state");
    lock.acquire();
    assert2(lock.is_acquired(), "Not acquired");
    lock.release();
    assert2(!lock.is_acquired(), "Not released");
}

TEST(is_acquired) {
    SpinLock lock;
    assert(!lock.is_acquired());

    lock.acquire();
    assert(lock.is_acquired());

    lock.release();
    assert(!lock.is_acquired());
}

TEST(ctor) {
    SpinLock lock_released, lock_acquired;

    lock_acquired.acquire();
    assert(!lock_released.is_acquired());
    assert(lock_acquired.is_acquired());
}

TEST(acquire_release_mt) {
    test_data tdata;
    Thread t{&lock_handler};

    tdata.lock.acquire();
    t.start(&tdata);
    tdata.can_start = true;

    while(!tdata.other_thread_started) { _mm_pause(); }
    sleep(50);

    assert2(!tdata.complete, "Lock was acquired twice");
    tdata.lock.release();
    while(!tdata.done) { _mm_pause(); }
    assert2(tdata.complete, "Lock wasn't released");
}

void try_acquire_handler(void *param) noexcept {
    test_data *tdata = reinterpret_cast<test_data *>(param);

    if(!tdata->lock.try_acquire(100)) {
        tdata->complete = true;
    }
}

TEST(try_acquire) {
    test_data tdata;
    Thread t{&try_acquire_handler};

    tdata.lock.acquire();
    tdata.can_start = true;
    t.start(&tdata);
    t.join();

    assert(tdata.complete);
}
