#include <gtest/gtest.h>

extern "C" {
#include <ft_service.h>
}

#include <string>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <filesystem>

using namespace std;

TEST(get_filelist, return_directory_regular_file) {
	string path = filesystem::current_path();
	istringstream is(get_filelist(path.c_str()));
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
