#include <gtest/gtest.h>

extern "C" {
	extern bool is_preamble_packet_type_correctly_encapsulated();
	extern bool is_preamble_packet_operation_correctly_encapsulated();
	extern bool is_preamble_packet_result_correctly_encapsulated();
	extern bool is_preamble_packet_arg_correctly_encapsulated();
	extern bool is_preamble_packet_data_packet_lenght_correctly_encapsulated();
}

TEST(ftcp, preamble_packet_type_encapsulation_correctness) {
	ASSERT_EQ(is_preamble_packet_type_correctly_encapsulated(), true);
}

TEST(ftcp, preamble_packet_operation_encapsulation_correctness) {
	ASSERT_EQ(is_preamble_packet_operation_correctly_encapsulated(), true);
}

TEST(ftcp, preamble_packet_result_encapsulation_correctness) {
	ASSERT_EQ(is_preamble_packet_result_correctly_encapsulated(), true);
}

TEST(ftcp, preamble_packet_arg_encapsulation_correctness) {
	ASSERT_EQ(is_preamble_packet_arg_correctly_encapsulated(), true);
}

TEST(ftcp, preamble_packet_data_packet_lenght_encapsulation_correctness) {
	ASSERT_EQ(is_preamble_packet_data_packet_lenght_correctly_encapsulated(), true);
}
