#include <gtest/gtest.h>

extern "C" {
#include "rwfslock.h"
}

#include <thread>

TEST(cmn_rwfslock, multiple_rdlock_are_allowed) {
	struct cmn_rwfslock lock;
	cmn_rwfslock_init(&lock);
	for (int i = 0; i < 10; i++) {
		cmn_rwfslock_rdlock(&lock, "./pippo");
	}
	ASSERT_EQ(1, 1);
}

// To run this test, enable it and verify that deadlock occurs (test freezes).
TEST(cmn_rwfslock, DISABLED_wrlock_is_exclusive) {
	std::thread t[10];
	struct cmn_rwfslock lock;
	cmn_rwfslock_init(&lock);
	std::thread t1([&lock] {
		cmn_rwfslock_wrlock(&lock, "./pippo");
	});
	std::thread t2([&lock] {
		cmn_rwfslock_wrlock(&lock, "./pippo");
	});
	t1.join();
	t2.join();
	ASSERT_EQ(1, 1);
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
