#include <search.hpp>
#include <mio.hpp>
#include <termcolor.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <fstream>

namespace search {

void file_search(fs::directory_entry const& dir_entry, std::string_view query, bool ignore_case, bool show_line_numbers)
{
	auto absolute_path = fs::absolute(dir_entry.path());
	std::string filename = absolute_path.string();
	const auto query_size = query.size();

	auto mmap = mio::mmap_source(filename);
	if (!mmap.is_open() || !mmap.is_mapped()) {
		throw std::runtime_error("Failed to memory map file: " + filename);
	}
	std::string_view buffer = mmap.data();
	auto buffer_size = mmap.mapped_length();

	auto it = buffer.find(query);
	while (it != std::string_view::npos) {

		auto line_start = it;
		auto line_end = buffer.find('\n', line_start);		

		if (show_line_numbers) {
			auto line_number = std::count_if(buffer.begin(), buffer.begin() + line_start, [](char c) { return c == '\n'; }) + 1;
			std::cout << dir_entry.path().c_str() << ":" << line_number << ":";
		}		
		else {
			std::cout << dir_entry.path().c_str() << ":";
		}

		auto start = 0;
		for (auto i = it; i >= 0; --i) {
			// find the first '\n' before `it`
			if (buffer[i] == '\n') {
				start = i;
				break;
			}
		}
		std::cout << buffer.substr(start + 1, it - start - 1);
		std::cout << termcolor::red << termcolor::bold << buffer.substr(it, query_size) << termcolor::reset
					<< buffer.substr(it + query_size, line_end - it - query_size)
					<< "\n";

		it = buffer.find(query, it + 1);
	}
}

void directory_search(fs::path const& path, std::string_view query, bool ignore_case, bool show_line_numbers) 
{
	
	for (auto const& dir_entry : fs::directory_iterator(path))
	{
		// if file
		if (fs::is_regular_file(dir_entry)) {
			try {
				search::file_search(dir_entry, 
							   query,
							   ignore_case,
							   show_line_numbers);
			} catch (std::exception& e) {
				continue;
			}
		}
	}
}

void recursive_directory_search(fs::path const& path, std::string_view query, bool ignore_case, bool show_line_numbers) 
{
	
	for (auto const& dir_entry : fs::recursive_directory_iterator(path))
	{
		// if file
		if (fs::is_regular_file(dir_entry)) {
			try {
				search::file_search(dir_entry, 
							   query,
							   ignore_case,
							   show_line_numbers);
			} catch (std::exception& e) {
				continue;
			}
		}
	}
}



}