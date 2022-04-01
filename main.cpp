#include <mio.hpp>
#include <termcolor.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

// -n to show line numbers
// -r to recurse into subdirectories
// -i to ignore case

int main(int argc, char* argv[]) {

	auto root = fs::path(argv[1]);

	for (auto const& dir_entry : fs::recursive_directory_iterator(root))
	{
		// if file
		if (fs::is_regular_file(dir_entry)) {

			auto absolute_path = fs::absolute(dir_entry.path());
			std::string filename = absolute_path.string();
			std::string_view query = argv[2];

			try {
				auto mmap = mio::mmap_source(filename);
				if (!mmap.is_open() || !mmap.is_mapped())
					return 1;
				std::string_view buffer = mmap.data();
				auto buffer_size = mmap.mapped_length();

				auto it = buffer.find(query);
				while (it != std::string_view::npos) {

					// at least one result

					// find line number
					// use std::count_if
					auto line_start = it;
					auto line_end = buffer.find('\n', line_start);
					auto line_number = std::count_if(buffer.begin(), buffer.begin() + line_start, [](char c) { return c == '\n'; }) + 1;

					std::cout << dir_entry.path().c_str() << ":" << line_number << ":";

					auto start = 0;
					for (auto i = it; i >= 0; --i) {
						// find the first '\n' before `it`
						if (buffer[i] == '\n') {
							start = i;
							break;
						}
					}
					std::cout << buffer.substr(start + 1, it - start - 1);
					std::cout << termcolor::red << termcolor::bold << buffer.substr(it, query.size()) << termcolor::reset
							  << buffer.substr(it + query.size(), line_end - it - query.size())
							  << "\n";

					it = buffer.find(query, it + 1);
				}

			} catch (std::exception& e) {
				continue;
			}
		}
	}
}