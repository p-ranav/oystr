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

auto needle_search(std::string_view needle,
		   std::string_view::const_iterator haystack_begin,
		   std::string_view::const_iterator haystack_end)
{
  if (haystack_begin != haystack_end)
  {
    return std::search(haystack_begin, haystack_end,
		       needle.begin(),
		       needle.end());
  }
  else
  {
    return haystack_end;
  }
}

auto file_search(fs::path const& path, std::string_view needle)
{
  const auto absolute_path = fs::absolute(path);
  const auto needle_size = needle.size();
  const auto file_size = fs::file_size(absolute_path);
  if (file_size < needle_size)
  {
    return;
  }

  // Memory map input file
  std::string filename = absolute_path.string();
  auto mmap = mio::mmap_source(filename);
  if (!mmap.is_open() || !mmap.is_mapped())
  {
    return;
  }
  const std::string_view haystack = mmap.data();

  // Start from the beginning
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();
  auto it = haystack_begin;  

  while (it != haystack_end)
  {
    // Search for needle
    it = needle_search(needle, it, haystack_end);
    
    if (it != haystack_end)
    {
      // Found needle in haystack
      auto newline_before = haystack.rfind('\n', it - haystack_begin);
      auto newline_after = std::find(it, haystack_end, '\n');

      std::cout << path.c_str() << ":"
		<< haystack.substr(newline_before + 1,
				   newline_after -
				   (haystack_begin + newline_before + 1) - 1)
		<< "\n";

      // Move to next line and continue search
      it = newline_after + 1;
      continue;
    }
    else
    {
      break;
    }    
  }
}

void directory_search(fs::path const& path, std::string_view query) 
{	
  for (auto const& dir_entry : fs::directory_iterator(path))
    {
      if (fs::is_regular_file(dir_entry))
      {
	try
	{
	  file_search(dir_entry.path(), query);
	}
	catch (std::exception& e)
	{
	  continue;
	}
      }
    }
}

void recursive_directory_search(fs::path const& path, std::string_view query) 
{	
  for (auto const& dir_entry : fs::recursive_directory_iterator(path))
  {
    if (fs::is_regular_file(dir_entry))
    {
      try
      {
	file_search(dir_entry.path(), query);
      }
      catch (std::exception& e)
      {
	continue;
      }
    }
  }
}

int main(int argc, char ** argv)
{
  const std::string_view needle = argv[1];
  const std::string_view path = argv[2];
  recursive_directory_search(path, needle);
}
