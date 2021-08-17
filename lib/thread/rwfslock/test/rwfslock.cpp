#include <gtest/gtest.h>

extern "C" {
#include "rwfslock.h"
}

#include <thread>
#include <chrono>

TEST(cmn_rwfslock, multiple_rdlock_are_allowed) {
	struct cmn_rwfslock lock;
	cmn_rwfslock_init(&lock);
	for (int i = 0; i < 10; i++) {
		cmn_rwfslock_rdlock(&lock, "./pippo");
	}
	ASSERT_EQ(1, 1);
}

TEST(cmn_rwfslock, wrlock_is_exclusive) {
	struct cmn_rwfslock lock;
	cmn_rwfslock_init(&lock);
	bool t1_has_lock = false;
	bool t2_has_lock = false;
	std::thread t1([&lock, &t1_has_lock] {
		cmn_rwfslock_wrlock(&lock, "./pippo");
		t1_has_lock = true;
	});
	std::thread t2([&lock, &t2_has_lock] {
		cmn_rwfslock_wrlock(&lock, "./pippo");
		t2_has_lock = true;
	});
	std::thread timer([] {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	});
	t1.detach();
	t2.detach();
	timer.join();
	ASSERT_EQ(t1_has_lock && t2_has_lock, false);
}

TEST(cmn_rwfslock, lock_is_unique_per_file) {
	std::thread t[10];
	struct cmn_rwfslock lock;
	cmn_rwfslock_init(&lock);
	std::thread t1([&lock] {
		cmn_rwfslock_wrlock(&lock, "./pippo");
	});
	std::thread t2([&lock] {
		cmn_rwfslock_wrlock(&lock, "./pluto");
	});
	t1.join();
	t2.join();
	ASSERT_EQ(1, 1);
}
