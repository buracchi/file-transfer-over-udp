#include <gtest/gtest.h>

extern "C" {
#include "ft_service.h"
}

#include <string>
#include <unordered_set>
#include <iostream>
#include <filesystem>

TEST(ft_service, get_filelist_return_current_directory_regular_filelist) {
	std::string path = std::filesystem::current_path();
	ft_service_t ftservice = ft_service_init(path.c_str());
	std::istringstream is(ft_service_get_filelist(ftservice));
	auto actual = std::unordered_set<std::string>(
		std::istream_iterator<std::string>(is),
		std::istream_iterator<std::string>()
		);
	std::unordered_set<std::string> expected;
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (entry.is_regular_file()) {
			expected.insert(entry.path().filename());
		}
	}
	ASSERT_EQ(actual, expected);
}
