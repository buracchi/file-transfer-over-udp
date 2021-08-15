#include <gtest/gtest.h>

extern "C" {
#include "tpool.h"
}

#include <mutex>

std::mutex mtx;

static void* foo(void*);

TEST(cmn_tpool, tpool_execute_routine) {
	mtx.lock();
	bool work_completed = false;
	struct cmn_tpool tpool;
	cmn_tpool_init(&tpool, 2);
	cmn_tpool_add_work(&tpool, foo, (void*)&work_completed);
	mtx.lock();
	cmn_tpool_wait(&tpool);
	ASSERT_EQ(work_completed, true);
}

static void* foo(void* arg) {
	bool* work_completed = (bool*)arg;
	mtx.unlock();
	*work_completed = true;
	return NULL;
}
