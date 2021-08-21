#include <gtest/gtest.h>

extern "C" {
#include "socket2.h"
#include "nproto/nproto_service_ipv4.h"
#include "tproto/tproto_service_tcp.h"
#include "try.h"
}

#include <thread>
#include <mutex>
#include <string>
#include <chrono>

TEST(cmn_socket2, receive_message) {
	std::mutex mtx;
	std::string message = "MESSAGE";
	mtx.lock();
	std::string actual = "";
	std::thread sender([&mtx, message] {
		cmn_socket2_t server;
		cmn_socket2_t client;
		try(server = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp), nullptr);
		while (cmn_socket2_listen(server, "0.0.0.0:1234", 1024) && (errno == EADDRINUSE)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		};
		try(client = cmn_socket2_accept(server), nullptr);
		try(cmn_socket2_send(client, (const uint8_t*) message.c_str(), message.length()), -1);
		mtx.lock();
		try(cmn_socket2_destroy(server), !0);
		try(cmn_socket2_destroy(client), !0);
		return EXIT_SUCCESS;
	});
	std::thread receiver([&mtx, message, &actual] {
		cmn_socket2_t socket;
		char* buff = (char*) malloc(message.length() + 1);
		memset(buff, 0, message.length() + 1);
		try(socket = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp), nullptr);
		try(cmn_socket2_connect(socket, "127.0.0.1:1234"), !0);
		try(cmn_socket2_recv(socket, (uint8_t*) buff, message.length()), -1);
		mtx.unlock();
		try(cmn_socket2_destroy(socket), !0);
		actual = buff;
		return EXIT_SUCCESS;
	});
	sender.join();
	receiver.join();
	ASSERT_EQ(actual, message);
}
