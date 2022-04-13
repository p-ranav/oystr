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
#include <glob.h>
#include <mio.hpp>

namespace search
{
auto is_binary_file(std::string_view haystack);

auto needle_search(std::string_view needle,
                   std::string_view::const_iterator haystack_begin,
                   std::string_view::const_iterator haystack_end);

// find case insensitive substring
auto needle_search_case_insensitive(std::string_view str,
                                    std::string_view query);

void print_colored(std::string_view str, std::string_view query);
std::size_t file_search(std::string_view filename,
                        std::string_view haystack,
                        std::string_view needle,
                        bool print_count,
                        bool enforce_max_count,
                        std::size_t max_count,
                        bool print_line_numbers,
                        bool print_only_file_matches,
                        bool print_only_file_without_matches,
                        bool print_only_matching_parts,
                        bool process_binary_file_as_text);

std::size_t read_file_and_search(std::filesystem::path const& path,
                                 std::string_view needle,
                                 bool print_count,
                                 bool enforce_max_count,
                                 std::size_t max_count,
                                 bool print_line_numbers,
                                 bool print_only_file_matches,
                                 bool print_only_file_without_matches,
                                 bool print_only_matching_parts,
                                 bool process_binary_file_as_text);

void directory_search(std::string_view query,
                      std::vector<std::string>& include_extension,
                      bool print_count,
                      bool enforce_max_count,
                      std::size_t max_count,
                      bool print_line_numbers,
                      bool print_only_file_matches,
                      bool print_only_file_without_matches,
                      bool print_only_matching_parts,
                      bool process_binary_file_as_text);

}  // namespace search
