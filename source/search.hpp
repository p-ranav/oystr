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

#include <termcolor.hpp>

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
                 bool print_line_numbers,
                 bool print_only_file_matches,
                 bool print_only_matching_parts);

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
                          bool print_line_numbers,
                          bool print_only_file_matches,
                          bool print_only_matching_parts);

void directory_search(std::filesystem::path const& path,
                      std::string_view query,
                      const std::vector<std::string>& include_extension,
                      const std::vector<std::string>& exclude_extension,
                      bool ignore_case,
                      bool print_line_numbers,
                      bool print_only_file_matches,
                      bool print_only_matching_parts);

void recursive_directory_search(
    std::filesystem::path const& path,
    std::string_view query,
    const std::vector<std::string>& include_extension,
    const std::vector<std::string>& exclude_extension,
    bool ignore_case,
    bool print_line_numbers,
    bool print_only_file_matches,
    bool print_only_matching_parts);

}  // namespace search
