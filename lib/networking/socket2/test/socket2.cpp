#include <gtest/gtest.h>

extern "C" {
#include "socket2.h"
#include "nproto/nproto_service_ipv4.h"
#include "tproto/tproto_service_tcp.h"
}

#include <thread>
#include <mutex>
#include <string>

TEST(cmn_socket2, receive_message) {
	std::mutex mtx;
	std::string message = "MESSAGE";
	mtx.lock();
	std::string actual = "";
	std::thread sender([&mtx, message] {
		struct cmn_socket2 server;
		struct cmn_socket2 client;
		cmn_socket2_init(&server, cmn_nproto_service_ipv4, cmn_tproto_service_tcp);
		cmn_socket2_listen(&server, "0.0.0.0:1234", 1024);
		cmn_socket2_accept(&server, &client);
		cmn_socket2_send(&client, (const uint8_t*) message.c_str(), message.length());
		mtx.lock();
		cmn_socket2_destroy(&server);
		cmn_socket2_destroy(&client);
		return EXIT_SUCCESS;
	});
	std::thread receiver([&mtx, message, &actual] {
		struct cmn_socket2 socket;
		char* buff = (char*) malloc(message.length() + 1);
		memset(buff, 0, message.length() + 1);
		cmn_socket2_init(&socket, cmn_nproto_service_ipv4, cmn_tproto_service_tcp);
		cmn_socket2_connect(&socket, "127.0.0.1:1234");
		cmn_socket2_recv(&socket, (uint8_t*) buff, message.length());
		mtx.unlock();
		cmn_socket2_destroy(&socket);
		actual = buff;
		return EXIT_SUCCESS;
	});
	sender.join();
	receiver.join();
	ASSERT_EQ(actual, message);
}
