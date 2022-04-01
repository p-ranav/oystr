#include <search.hpp>
#include <mio.hpp>
#include <termcolor.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <locale>

namespace search {

namespace detail {

// find case insensitive substring
auto find(std::string_view str, std::string_view query)
{
	if (str.size() < query.size())
		return std::string_view::npos;

	auto it = std::search(str.begin(), str.end(), query.begin(), query.end(),
		[](char c1, char c2) {  return std::toupper(c1) == std::toupper(c2); });

	return it != str.end() ? it - str.begin() : std::string_view::npos;
}

void print_colored(std::string_view str, std::string_view query, bool ignore_case)
{
	auto pos = ignore_case ? find(str, query) : str.find(query);
	if (pos == std::string_view::npos) {
		std::cout << str;
		return;
	}
	std::cout << str.substr(0, pos);
	std::cout << termcolor::red << str.substr(pos, query.size()) << termcolor::reset;
	print_colored(str.substr(pos + query.size()), query, ignore_case);
}

} // namespace detail

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

	auto it = 0;
	if (ignore_case) {
		it = detail::find(buffer, query);
	}
	else {
		it = buffer.find(query);
	}

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
		std::cout << termcolor::red << termcolor::bold << buffer.substr(it, query_size) << termcolor::reset;		
		detail::print_colored(buffer.substr(it + query_size, line_end - it - query_size), query, ignore_case);
		std::cout << "\n";				
		
		if (ignore_case) {	
			auto it_next = detail::find(buffer.substr(line_end), query);
			if (it_next == std::string_view::npos)
				break;
			it = line_end + it_next;
		}
		else {
			it = buffer.find(query, line_end);
		}
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