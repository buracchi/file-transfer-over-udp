#include <gtest/gtest.h>

extern "C" {
#include "communication_manager.h"
#include "nproto/nproto_service_ipv4.h"
#include "tproto/tproto_service_tcp.h"
#include "request_handler.h"
}

#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>

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

static void foo(struct cmn_request_handler* request_handler, struct cmn_socket2* socket);

constexpr int request_number = 2;

TEST(cmn_communication_manager, test) {
	__cmn_request_handler_ops_vtbl.handle_request = foo;
	struct cmn_communication_manager cm;
	cmn_communication_manager_init(&cm, 4);
	std::thread server([&cm] {
		cmn_communication_manager_start(
			&cm,
			cmn_nproto_service_ipv4,
			cmn_tproto_service_tcp,
			"127.0.0.1:1234",
			(struct cmn_request_handler*)&simple_handler
		);
	});
	std::vector<std::thread> clients;
	for (int i = 0; i < request_number; i++) {
		clients.push_back(std::thread([] {
			uint8_t buff;
			struct cmn_socket2 socket;
			cmn_socket2_init(&socket, cmn_nproto_service_ipv4, cmn_tproto_service_tcp);
			cmn_socket2_connect(&socket, "127.0.0.1:1234");
			cmn_socket2_recv(&socket, &buff, 1);
			cmn_socket2_destroy(&socket);
		}));
	}
	std::for_each(clients.begin(), clients.end(), [](std::thread &client) {
        client.join();
    });
	cmn_communication_manager_stop(&cm);
	server.join();
	cmn_communication_manager_destroy(&cm);
	ASSERT_EQ(simple_handler.counter, request_number);
}

static void foo(struct cmn_request_handler* request_handler, struct cmn_socket2* socket) {
	struct simple_handler* handler = (struct simple_handler*)request_handler;
	uint8_t buff = 1;
	handler->m.lock();
	handler->counter++;
	handler->m.unlock();
	std::cout << std::this_thread::get_id() << std::endl;
	cmn_socket2_send(socket, &buff, 1);
}
