#include <algorithm>
#include <cctype>
#include <functional>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <termcolor.hpp>
#include <mio.hpp>
#include <filesystem>
#include <string_view>
namespace fs = std::filesystem;

auto boyer_moore_horspool_search(std::string_view needle,
				 std::string_view::const_iterator haystack_begin,
				 std::string_view::const_iterator haystack_end) {
  if (haystack_begin != haystack_end) {
    return std::search(haystack_begin, haystack_end,
		       std::boyer_moore_horspool_searcher(needle.begin(), needle.end()));
  } else {
    return haystack_end;
  }
}

void print_colored(std::string_view str, std::string_view query)
{
  auto pos = str.find(query);
  if (pos == std::string_view::npos) {
    std::cout << str;
    return;
  }
  std::cout << str.substr(0, pos);
  std::cout << termcolor::red << termcolor::bold
	    << str.substr(pos, query.size()) << termcolor::reset;
  print_colored(str.substr(pos + query.size()), query);
}

auto file_search(fs::path const& path, std::string_view needle) {
  auto absolute_path = fs::absolute(path);
  std::string filename = absolute_path.string();
	
  auto mmap = mio::mmap_source(filename);
  if (!mmap.is_open() || !mmap.is_mapped()) {
    return;
  }
  const std::string_view haystack = mmap.data();  
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();
  auto it = haystack_begin;
  const auto needle_size = needle.size();

  while (it != haystack_end) {
    it = boyer_moore_horspool_search(needle, it, haystack_end);
    if (it != haystack_end) {
      auto newline_before = haystack.rfind('\n', it - haystack_begin);
      
      static auto previous_result = newline_before + 1;
      if (newline_before == previous_result) {
	it += 1;
	continue;
      }
      previous_result = newline_before;

      auto line_number = std::count_if(haystack.begin(), haystack.begin() + newline_before + 1,
				       [](char c) { return c == '\n'; }) + 1;      

      std::cout << path.c_str() << ":";

      if (true) {
	std::cout << line_number << ":";
      }
      
      auto newline_after = std::find(it, haystack.end(), '\n');
      std::string_view before(haystack_begin + newline_before + 1,
			      it - (haystack_begin + newline_before + 1));
      std::cout << before;

      print_colored(haystack.substr(it - haystack_begin,
				    newline_after - it),
		    needle);
      std::cout << "\n";
    }
    else {
      break;
    }
    it++;
  }
}

void directory_search(fs::path const& path, std::string_view query) 
{	
  for (auto const& dir_entry : fs::directory_iterator(path))
    {
      if (fs::is_regular_file(dir_entry)) {
	try {
	  file_search(dir_entry.path(), query);
	} catch (std::exception& e) {
	  continue;
	}
      }
    }
}

void recursive_directory_search(fs::path const& path, std::string_view query) 
{	
  for (auto const& dir_entry : fs::recursive_directory_iterator(path))
    {
      if (fs::is_regular_file(dir_entry)) {
	try {
	  file_search(dir_entry.path(), query);
	} catch (std::exception& e) {
	  continue;
	}
      }
    }
}

int main(int argc, char ** argv) {
  const std::string needle = argv[1];
  const std::string path = argv[2];
  recursive_directory_search(path, needle);
}
