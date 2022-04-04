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

auto is_binary_file(std::string_view haystack) {
  // If the haystack has NUL characters, it's likely a binary file.
  return (haystack.find('\0') != std::string_view::npos);
}

auto needle_search(std::string_view needle,
                   std::string_view::const_iterator haystack_begin,
                   std::string_view::const_iterator haystack_end,
                   bool ignore_case) {
  if (haystack_begin != haystack_end) {

    if (ignore_case) {
      return std::search(haystack_begin, haystack_end, needle.begin(),
                         needle.end(), [](char c1, char c2) {
                           return std::toupper(c1) == std::toupper(c2);
                         });
    } else {
      return std::search(haystack_begin, haystack_end, needle.begin(),
                         needle.end());
    }
  } else {
    return haystack_end;
  }
}

// find case insensitive substring
auto needle_search_case_insensitive(std::string_view str,
                                    std::string_view query) {
  if (str.size() < query.size())
    return std::string_view::npos;

  auto it = std::search(
      str.begin(), str.end(), query.begin(), query.end(),
      [](char c1, char c2) { return std::toupper(c1) == std::toupper(c2); });

  return it != str.end() ? it - str.begin() : std::string_view::npos;
}

void print_colored(std::string_view str, std::string_view query,
                   bool ignore_case) {
  auto pos = ignore_case ? needle_search_case_insensitive(str, query)
                         : str.find(query);
  if (pos == std::string_view::npos) {
    std::cout << termcolor::white << termcolor::bold << str << termcolor::reset;
    return;
  }
  std::cout << termcolor::white << termcolor::bold << str.substr(0, pos)
            << termcolor::reset;
  std::cout << termcolor::red << termcolor::bold
            << str.substr(pos, query.size()) << termcolor::reset;
  print_colored(str.substr(pos + query.size()), query, ignore_case);
}

auto file_search(std::string_view filename, std::string_view haystack,
                 std::string_view needle, bool ignore_case,
                 bool print_line_numbers, bool print_only_file_matches,
                 bool print_only_matching_parts) {
  // Start from the beginning
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();
  auto it = haystack_begin;

  while (it != haystack_end) {
    // Search for needle
    it = needle_search(needle, it, haystack_end, ignore_case);

    if (it != haystack_end) {

      // Avoid printing lines from binary files with matches
      if (is_binary_file(haystack)) {
        std::cout << termcolor::white << termcolor::bold << "Binary file "
                  << termcolor::cyan << filename << termcolor::white
                  << " matches\n"
                  << termcolor::reset;
        return;
      }

      // -l option
      // Print only filenames of files that contain matches.
      if (print_only_file_matches) {
        std::cout << termcolor::blue << termcolor::bold << filename << "\n"
                  << termcolor::reset;
        return;
      }

      // Found needle in haystack
      auto newline_before = haystack.rfind('\n', it - haystack_begin);
      auto newline_after = std::find(it, haystack_end, '\n');

      if (print_line_numbers) {
        auto line_number =
            std::count_if(haystack_begin, haystack_begin + newline_before + 1,
                          [](char c) { return c == '\n'; }) +
            1;
        std::cout << termcolor::cyan << termcolor::bold << filename << ":"
                  << termcolor::magenta << line_number << termcolor::red << ":"
                  << termcolor::reset;
      } else {
        std::cout << termcolor::cyan << termcolor::bold << filename << ":"
                  << termcolor::reset;
      }

      if (print_only_matching_parts) {
        std::cout << termcolor::red << termcolor::bold
                  << haystack.substr(it - haystack_begin, needle.size())
                  << termcolor::reset << "\n";
      } else {
        // Get line from newline_before and newline_after
        auto line = haystack.substr(newline_before + 1,
                                    newline_after -
                                        (haystack_begin + newline_before) - 1);
        print_colored(line, needle, ignore_case);
        std::cout << "\n";
      }

      // Move to next line and continue search
      it = newline_after + 1;
      continue;
    } else {
      break;
    }
  }
}

auto read_file_and_search(fs::path const &path, std::string_view needle,
                          bool ignore_case, bool print_line_numbers,
                          bool print_only_file_matches,
                          bool print_only_matching_parts) {
  try {
    const auto absolute_path = fs::absolute(path);
    const auto file_size = fs::file_size(absolute_path);
    if (file_size < needle.size()) {
      return;
    }

    std::string filename = absolute_path.string();
    std::ifstream is(filename);
    auto haystack = std::string(std::istreambuf_iterator<char>(is),
                                std::istreambuf_iterator<char>());
    file_search(path.c_str(), haystack, needle, ignore_case, print_line_numbers,
                print_only_file_matches, print_only_matching_parts);
  } catch (const std::exception &e) {
    std::cout << termcolor::red << termcolor::bold << "Error: " << e.what()
              << "\n"
              << termcolor::reset;
  }
}

auto mmap_file_and_search(fs::path const &path, std::string_view needle,
                          bool ignore_case, bool print_line_numbers,
                          bool print_only_file_matches,
                          bool print_only_matching_parts) {
  try {
    const auto absolute_path = fs::absolute(path);
    const auto file_size = fs::file_size(absolute_path);
    if (file_size < needle.size()) {
      return;
    }

    // Memory map input file
    std::string filename = absolute_path.string();
    auto mmap = mio::mmap_source(filename);
    if (!mmap.is_open() || !mmap.is_mapped()) {
      return;
    }
    const std::string_view haystack = mmap.data();

    file_search(path.c_str(), haystack, needle, ignore_case, print_line_numbers,
                print_only_file_matches, print_only_matching_parts);
  } catch (const std::exception &e) {
    std::cout << termcolor::red << termcolor::bold << "Error: " << e.what()
              << "\n"
              << termcolor::reset;
  }
}

void directory_search(fs::path const &path, std::string_view query,
                      bool ignore_case, bool print_line_numbers,
                      bool print_only_file_matches,
                      bool print_only_matching_parts, bool use_mmap) {
  for (auto const &dir_entry : fs::directory_iterator(
           path, fs::directory_options::skip_permission_denied)) {
    try {
      if (fs::is_regular_file(dir_entry)) {
        if (use_mmap) {
          mmap_file_and_search(dir_entry, query, ignore_case,
                               print_line_numbers, print_only_file_matches,
                               print_only_matching_parts);
        } else {
          read_file_and_search(dir_entry.path().string(), query, ignore_case,
                               print_line_numbers, print_only_file_matches,
                               print_only_matching_parts);
        }
      }
    } catch (std::exception &e) {
      continue;
    }
  }
}

void recursive_directory_search(fs::path const &path, std::string_view query,
                                bool ignore_case, bool print_line_numbers,
                                bool print_only_file_matches,
                                bool print_only_matching_parts, bool use_mmap) {
  for (auto const &dir_entry : fs::recursive_directory_iterator(
           path, fs::directory_options::skip_permission_denied)) {
    try {
      if (fs::is_regular_file(dir_entry)) {
        if (use_mmap) {
          mmap_file_and_search(dir_entry, query, ignore_case,
                               print_line_numbers, print_only_file_matches,
                               print_only_matching_parts);
        } else {
          read_file_and_search(dir_entry.path().string(), query, ignore_case,
                               print_line_numbers, print_only_file_matches,
                               print_only_matching_parts);
        }
      }
    } catch (std::exception &e) {
      continue;
    }
  }
}

int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("search");
  program.add_argument("query");
  program.add_argument("path");
  program.add_argument("-i")
      .help("Perform case insensitive matching.  By default, search is case "
            "sensitive.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-l", "--files-with-matches")
      .help("Print only filenames of files that contain matches.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--mmap")
      .help("Use mmap instead of read to read input files.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-n", "--line-number")
      .help("Each output line is preceded by its relative line number in the "
            "file, starting at line 1.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-o", "--only-matching")
      .help("Print only the matched (non-empty) parts of a matchiing line.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-R", "-r", "--recursive")
      .help("Recursively search subdirectories listed.")
      .default_value(false)
      .implicit_value(true);

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
  auto print_only_file_matches = program.get<bool>("-l");
  auto print_line_numbers = program.get<bool>("-n");
  auto print_only_matching_parts = program.get<bool>("-o");
  auto recurse = program.get<bool>("-r");
  auto use_mmap = program.get<bool>("--mmap");

  // File
  if (fs::is_regular_file(path)) {
    if (use_mmap) {
      mmap_file_and_search(path, query, ignore_case, print_line_numbers,
                           print_only_file_matches, print_only_matching_parts);
    } else {
      read_file_and_search(path.string(), query, ignore_case,
                           print_line_numbers, print_only_file_matches,
                           print_only_matching_parts);
    }
  } else {
    // Directory
    if (recurse) {
      recursive_directory_search(path, query, ignore_case, print_line_numbers,
                                 print_only_file_matches,
                                 print_only_matching_parts, use_mmap);
    } else {
      directory_search(path, query, ignore_case, print_line_numbers,
                       print_only_file_matches, print_only_matching_parts,
                       use_mmap);
    }
  }
}
