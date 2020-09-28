#include <jolt/test.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/threading/lock.hpp>

using namespace jolt::threading;

SETUP { initialize(); }

struct test_data {
    Lock lock;
    volatile bool complete = false;
};

void lock_handler(void *param) noexcept {
    test_data *tdata = reinterpret_cast<test_data *>(param);

    tdata->lock.acquire();
    tdata->complete = true;
    tdata->lock.release();
}

TEST(acquire_release_mt) {
    test_data tdata;
    Thread t{&lock_handler};

    tdata.lock.acquire();
    t.start(&tdata);
    sleep(100);
    assert2(!tdata.complete, "Lock was acquired twice");
    tdata.lock.release();
    sleep(100);
    assert2(tdata.complete, "Lock wasn't released");
}

void try_acquire_handler(void *param) noexcept {
    test_data *tdata = reinterpret_cast<test_data *>(param);

    if(!tdata->lock.try_acquire()) {
        tdata->complete = true;
    }
}

TEST(try_acquire) {
    test_data tdata;
    Thread t{&try_acquire_handler};

    tdata.lock.acquire();
    t.start(&tdata);
    t.join();

    assert(tdata.complete);
}
