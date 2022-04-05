#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/color.h>
#include <fmt/core.h>
#include <mio.hpp>

#define START_TIME_MEASURE \
  auto start = std::chrono::high_resolution_clock::now();

#define END_TIME_MEASURE \
  auto end = std::chrono::high_resolution_clock::now(); \
  fmt::print( \
      "Elapsed time: {} ms\n", \
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start) \
          .count());

namespace search
{
auto is_binary_file(std::string_view haystack);

auto needle_search(std::string_view needle,
                   std::string_view::const_iterator haystack_begin,
                   std::string_view::const_iterator haystack_end,
                   bool ignore_case);

// find case insensitive substring
auto needle_search_case_insensitive(std::string_view str,
                                    std::string_view query);

void print_colored(std::string_view str,
                   std::string_view query,
                   bool ignore_case);
auto file_search(std::string_view filename,
                 std::string_view haystack,
                 std::string_view needle,
                 bool ignore_case,
                 bool print_count,
                 bool enforce_max_count,
                 std::size_t max_count,
                 bool print_line_numbers,
                 bool print_only_file_matches,
                 bool print_only_file_without_matches,
                 bool print_only_matching_parts,
                 bool process_binary_file_as_text);

bool filename_has_pattern(std::string_view str, std::string_view pattern);

auto include_file(std::string_view filename,
                  const std::vector<std::string>& patterns);

// Return true if the filename should be excluded
auto exclude_file(std::string_view filename,
                  const std::vector<std::string>& patterns);

void read_file_and_search(std::filesystem::path const& path,
                          std::string_view needle,
                          const std::vector<std::string>& include_extension,
                          const std::vector<std::string>& exclude_extension,
                          bool ignore_case,
                          bool print_count,
                          bool enforce_max_count,
                          std::size_t max_count,
                          bool print_line_numbers,
                          bool print_only_file_matches,
                          bool print_only_file_without_matches,
                          bool print_only_matching_parts,
                          bool process_binary_file_as_text);

template<typename T>
void directory_search(const T& iterator,
                      std::string_view query,
                      const std::vector<std::string>& include_extension,
                      const std::vector<std::string>& exclude_extension,
                      bool ignore_case,
                      bool print_count,
                      bool enforce_max_count,
                      std::size_t max_count,
                      bool print_line_numbers,
                      bool print_only_file_matches,
                      bool print_only_file_without_matches,
                      bool print_only_matching_parts,
                      bool process_binary_file_as_text)
{
  for (auto const& dir_entry : iterator) {
    try {
      if (std::filesystem::is_regular_file(dir_entry)) {
        read_file_and_search(dir_entry.path().string(),
                             query,
                             include_extension,
                             exclude_extension,
                             ignore_case,
                             print_count,
                             enforce_max_count,
                             max_count,
                             print_line_numbers,
                             print_only_file_matches,
                             print_only_file_without_matches,
                             print_only_matching_parts,
                             process_binary_file_as_text);
      }
    } catch (std::exception& e) {
      continue;
    }
  }
}

}  // namespace search
