#include <gtest/gtest.h>

extern "C" {
#include "ft_service.h"
}

#include <string>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <filesystem>

using namespace std;

TEST(ft_service_get_filelist, return_current_directory_regular_file) {
	string path = filesystem::current_path();
	ft_service_t ftservice = ft_service_init(path.c_str());
	istringstream is(ft_service_get_filelist(ftservice));
	unordered_set<string> actual = unordered_set<string>(
		istream_iterator<string>(is),
		istream_iterator<string>()
	);
	unordered_set<string> expected;
	for (const auto& entry : filesystem::directory_iterator(path)) {
		if (entry.is_regular_file()) {
			expected.insert(entry.path().filename());
		}
	}
	ASSERT_EQ(actual, expected);
}
