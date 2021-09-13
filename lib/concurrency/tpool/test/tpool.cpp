#include <gtest/gtest.h>

extern "C" {
#include "tpool.h"
}

#include <mutex>
#include <thread>

static void *foo(void *);

static void *bar(void *);

TEST(cmn_tpool, tpool_execute_routine) {
    cmn_tpool_t tpool;
    std::pair<std::mutex, bool> arg;
    arg.first.lock();
    arg.second = false;
    tpool = cmn_tpool_init(4);
    cmn_tpool_add_work(tpool, foo, (void *) &arg);
    arg.first.lock();
    cmn_tpool_destroy(tpool);
    ASSERT_EQ(arg.second, true);
}

TEST(cmn_tpool, tpool_execute_a_single_work_only_once) {
    cmn_tpool_t tpool;
    constexpr int work_number = 8192;
    std::tuple<std::mutex, std::mutex, int, int> arg;
    std::get<0>(arg).lock();
    std::get<2>(arg) = 0;
    std::get<3>(arg) = work_number;
    tpool = cmn_tpool_init(64);
    for (int i = 0; i < work_number; i++) {
        cmn_tpool_add_work(tpool, bar, (void *) &arg);
    }
    std::get<0>(arg).lock();
    cmn_tpool_destroy(tpool);
    ASSERT_EQ(std::get<2>(arg), work_number);
}

static void *foo(void *arg) {
    auto pair = (std::pair<std::mutex, bool> *) arg;
    pair->first.unlock();
    pair->second = true;
    return nullptr;
}

static void *bar(void *arg) {
    auto tuple = (std::tuple<std::mutex, std::mutex, int, int> *) arg;
    std::get<1>(*tuple).lock();
    std::get<2>(*tuple)++;
    if (std::get<2>(*tuple) == std::get<3>(*tuple)) {
        std::get<0>(*tuple).unlock();
    }
    std::get<1>(*tuple).unlock();
    return nullptr;
}
