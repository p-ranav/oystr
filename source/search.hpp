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
                        bool print_only_file_matches,
                        bool print_only_file_without_matches);

std::size_t read_file_and_search(std::filesystem::path const& path,
                                 std::string_view needle,
                                 bool print_count,
                                 bool enforce_max_count,
                                 std::size_t max_count,
                                 bool print_only_file_matches,
                                 bool print_only_file_without_matches);

bool filename_has_extension_from_list(
    const std::filesystem::path& fext,
    const std::vector<std::string>& extensions);

template<typename T>
void directory_search(const T&& iterator,
                      std::string_view query,
                      const std::vector<std::string>& include_extension,
                      const std::vector<std::string>& exclude_extension,
                      bool print_count,
                      bool enforce_max_count,
                      std::size_t max_count,
                      bool print_only_file_matches,
                      bool print_only_file_without_matches)
{
  std::size_t count = 0;
  for (auto const& dir_entry : iterator) {
    try {
      if (std::filesystem::is_regular_file(dir_entry)) {
        // Check if file extension is in `include_extension` list
        // Check if file extension is NOT in `exclude_extension` list
        if ((include_extension.empty()
             || filename_has_extension_from_list(dir_entry.path().extension(),
                                                 include_extension))
            && (exclude_extension.empty()
                || !filename_has_extension_from_list(
                    dir_entry.path().extension(), exclude_extension)))
        {
          count += read_file_and_search(dir_entry.path(),
                                        query,
                                        print_count,
                                        enforce_max_count,
                                        max_count,
                                        print_only_file_matches,
                                        print_only_file_without_matches);
        }
      }
    } catch (std::exception& e) {
      continue;
    }
  }
  if (!print_only_file_matches && !print_only_file_without_matches) {
    fmt::print("\n{} results\n", count);
  }
}

}  // namespace search
