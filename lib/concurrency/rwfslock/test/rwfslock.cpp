#include <gtest/gtest.h>

extern "C" {
#include "rwfslock.h"
}

#include <thread>
#include <chrono>

TEST(cmn_rwfslock, multiple_rdlock_are_allowed) {
    cmn_rwfslock_t lock = cmn_rwfslock_init();
    for (int i = 0; i < 10; i++) {
        cmn_rwfslock_rdlock(lock, "./pippo");
    }
    cmn_rwfslock_destroy(lock);
    ASSERT_EQ(1, 1);
}

TEST(cmn_rwfslock, wrlock_is_exclusive) {
    cmn_rwfslock_t lock = cmn_rwfslock_init();
    bool t1_has_lock = false;
    bool t2_has_lock = false;
    std::thread t1([lock, &t1_has_lock] {
        cmn_rwfslock_wrlock(lock, "./pippo");
        t1_has_lock = true;
    });
    std::thread t2([lock, &t2_has_lock] {
        cmn_rwfslock_wrlock(lock, "./pippo");
        t2_has_lock = true;
    });
    std::thread timer([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
    t1.detach();
    t2.detach();
    timer.join();
    cmn_rwfslock_destroy(lock);
    ASSERT_EQ(t1_has_lock && t2_has_lock, false);
}

TEST(cmn_rwfslock, lock_is_unique_per_file) {
    cmn_rwfslock_t lock = cmn_rwfslock_init();
    std::thread t1([lock] {
        cmn_rwfslock_wrlock(lock, "./pippo");
    });
    std::thread t2([lock] {
        cmn_rwfslock_wrlock(lock, "./pluto");
    });
    t1.join();
    t2.join();
    cmn_rwfslock_destroy(lock);
    ASSERT_EQ(1, 1);
}
