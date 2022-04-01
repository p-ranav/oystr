#pragma once
#include <filesystem>
#include <string_view>
namespace fs = std::filesystem;

namespace search {

void file_search(fs::directory_entry const& dir_entry, std::string_view query, bool ignore_case, bool show_line_numbers);

void directory_search(fs::path const& path, std::string_view query, bool ignore_case, bool show_line_numbers);

void recursive_directory_search(fs::path const& path, std::string_view query, bool ignore_case, bool show_line_numbers);

} // namespace search