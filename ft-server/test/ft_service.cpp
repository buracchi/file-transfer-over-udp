#include <gtest/gtest.h>

extern "C" {
#include <ft_service.h>
}

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

TEST(get_filelist, return_list) {
	std::string path = std::filesystem::current_path();
	std::vector<std::string> filelist;

	std::string filelist = "";
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (entry.is_regular_file()) {
			filelist.push_back(entry.path().filename());
		}
	}
	ASSERT_STREQ(filelist.c_str(), get_filelist(path.c_str()));
}
