#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mio.hpp>
#include <streambuf>
#include <string>
#include <string_view>
#include <termcolor.hpp>
namespace fs = std::filesystem;
#include <argparse.hpp>

auto needle_search(std::string_view needle,
                   std::string_view::const_iterator haystack_begin,
                   std::string_view::const_iterator haystack_end) {
  if (haystack_begin != haystack_end) {
    return std::search(haystack_begin, haystack_end, needle.begin(),
                       needle.end());
  } else {
    return haystack_end;
  }
}

auto file_search(fs::path const &path, std::string_view needle) {
  const auto absolute_path = fs::absolute(path);
  const auto needle_size = needle.size();
  const auto file_size = fs::file_size(absolute_path);
  if (file_size < needle_size) {
    return;
  }

  // Memory map input file
  std::string filename = absolute_path.string();
  auto mmap = mio::mmap_source(filename);
  if (!mmap.is_open() || !mmap.is_mapped()) {
    return;
  }
  const std::string_view haystack = mmap.data();

  // Start from the beginning
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();
  auto it = haystack_begin;

  while (it != haystack_end) {
    // Search for needle
    it = needle_search(needle, it, haystack_end);

    if (it != haystack_end) {
      // Found needle in haystack
      auto newline_before = haystack.rfind('\n', it - haystack_begin);
      auto newline_after = std::find(it, haystack_end, '\n');

      std::cout << path.c_str() << ":"
                << haystack.substr(newline_before + 1,
                                   newline_after -
                                       (haystack_begin + newline_before + 1) -
                                       1)
                << "\n";

      // Move to next line and continue search
      it = newline_after + 1;
      continue;
    } else {
      break;
    }
  }
}

void directory_search(fs::path const &path, std::string_view query) {
  for (auto const &dir_entry : fs::directory_iterator(path)) {
    if (fs::is_regular_file(dir_entry)) {
      try {
        file_search(dir_entry.path(), query);
      } catch (std::exception &e) {
        continue;
      }
    }
  }
}

void recursive_directory_search(fs::path const &path, std::string_view query) {
  for (auto const &dir_entry : fs::recursive_directory_iterator(path)) {
    if (fs::is_regular_file(dir_entry)) {
      try {
        file_search(dir_entry.path(), query);
      } catch (std::exception &e) {
        continue;
      }
    }
  }
}

int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("search");
  program.add_argument("query");
  program.add_argument("path");
  program.add_argument("-i")
      .help("ignore case")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-n")
      .help("show line numbers")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-r")
      .help("recurse into subdirectories")
      .default_value(false)
      .implicit_value(true);

  /*
  TODO:        -l, --files-with-matches
              Suppress  normal  output;  instead  print  the name of each
              input file from  which  output  would  normally  have  been
              printed.  The scanning will stop on the first match.
  TODO:        -o, --only-matching
              Print  only  the  matched  (non-empty)  parts of a matching
              line, with each such part on a separate output line.

  */

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

	auto path = fs::path(program.get<std::string>("path"));
	auto query = program.get<std::string>("query");
	auto ignore_case = program.get<bool>("-i");
	auto show_line_numbers = program.get<bool>("-n");
	auto recurse = program.get<bool>("-r");

  recursive_directory_search(path, query);
}
