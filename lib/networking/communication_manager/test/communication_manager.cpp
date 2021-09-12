#include <gtest/gtest.h>

extern "C" {
#include "communication_manager.h"
#include "nproto/nproto_service_ipv4.h"
#include "tproto/tproto_service_tcp.h"
#include "types/request_handler.h"
#include "try.h"
}

#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

struct simple_handler {
    struct cmn_request_handler super;
    int counter;
    std::mutex m;
} simple_handler = {
        .super = {
                .__ops_vptr = &__cmn_request_handler_ops_vtbl
        },
        .counter = 0
};

static void foo(cmn_request_handler_t request_handler, cmn_socket2_t socket);

constexpr int request_number = 1024;

TEST(cmn_communication_manager, test) {
    __cmn_request_handler_ops_vtbl.handle_request = foo;
    cmn_communication_manager_t cm = cmn_communication_manager_init(8);
    std::thread server([&cm] {
        cmn_communication_manager_start(cm, "0.0.0.0:1234", (cmn_request_handler_t) &simple_handler);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::vector<std::thread> clients;
    clients.reserve(request_number);
    for (int i = 0; i < request_number; i++) {
        clients.emplace_back([] {
            uint8_t buff;
            cmn_socket2_t socket;
            socket = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp);
            cmn_socket2_connect(socket, "127.0.0.1:1234");
            cmn_socket2_recv(socket, &buff, 1);
            cmn_socket2_destroy(socket);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::for_each(clients.begin(), clients.end(), [](std::thread &client) {
        client.join();
    });
    cmn_communication_manager_stop(cm);
    server.join();
    cmn_communication_manager_destroy(cm);
    ASSERT_EQ(simple_handler.counter, request_number);
}

static void foo(cmn_request_handler_t request_handler, cmn_socket2_t socket) {
    auto *handler = (struct simple_handler *) request_handler;
    uint8_t buff = 1;
    handler->m.lock();
    handler->counter++;
    handler->m.unlock();
    try(cmn_socket2_send(socket, &buff, 1), -1);
}
