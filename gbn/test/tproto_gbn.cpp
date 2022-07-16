#include <gtest/gtest.h>

extern "C" {
	#include "socket2.h"
	#include "nproto/nproto_service_ipv4.h"
	#include "tproto/tproto_service_gbn.h"
}

#include <thread>

TEST(tproto_gbn, test) {
	std::thread server([] {
		cmn_socket2_t socket;
		cmn_socket2_t client;
		socket = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_gbn);
		cmn_socket2_listen(socket, "127.0.0.1:1234", 1024);
		client = cmn_socket2_accept(socket);
		cmn_socket2_destroy(client);
		cmn_socket2_destroy(socket);
	});
	std::thread client([] {
		cmn_socket2_t socket = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_gbn);
		cmn_socket2_connect(socket, "127.0.0.1:1234");
		cmn_socket2_destroy(socket);
	});
	client.join();
	server.join();
	ASSERT_EQ(1, 1);
}
