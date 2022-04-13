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

bool path_has_substring(std::string_view path,
                        const std::vector<std::string_view>& substrings);

template<typename T>
std::size_t directory_search(const T&& iterator,
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
      std::string_view path_string = (const char*)dir_entry.path().c_str();
      if (std::filesystem::is_regular_file(dir_entry)) {
        if (!dir_entry.path().has_extension()
            || path_has_substring(path_string,
                                  {".dll",
                                   ".exe",
                                   ".o",
                                   ".so",
                                   ".dmg",
                                   ".7z",
                                   ".dmg",
                                   ".gz",
                                   ".iso",
                                   ".jar",
                                   ".rar",
                                   ".tar",
                                   ".zip",
                                   ".sql",
                                   ".sqlite",
                                   ".sys",
                                   ".tiff",
                                   ".tif",
                                   ".bmp",
                                   ".jpg",
                                   ".jpeg",
                                   ".gif",
                                   ".png",
                                   ".eps",
                                   ".raw",
                                   ".cr2",
                                   ".crw",
                                   ".pef",
                                   ".nef",
                                   ".orf",
                                   ".sr2",
                                   ".pdf",
                                   ".psd",
                                   ".ai",
                                   ".indd",
                                   ".arc",
                                   ".meta",
                                   ".pdb",
                                   ".pyc",
                                   ".Spotlight-V100",
                                   ".Trashes",
                                   "ehthumbs.db",
                                   "Thumbs.db",
                                   ".suo",
                                   ".user",
                                   ".lst",
                                   ".userosscache",
                                   ".sln.docstates"}))
        {
          continue;
        }

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
      } else {
        if (path_has_substring(
                path_string,
                {".git",        "build",       "node_modules", ".vscode",
                 ".DS_Store",   "Debug",       "CMakeFiles",   "debug",
                 "debugPublic", "DebugPublic", "Release",      "release",
                 "Releases",    "releases",    "x64",          "x86",
                 "bld",         "bin",         "Bin",          "obj",
                 "Obj",         ".vs",         "libexec",      "__pycache__",
                 "Binaries",    "devel",       "doc",          "/."}))
        {
          continue;
        }

        count += search::directory_search(
            std::move(std::filesystem::directory_iterator(
                path_string,
                std::filesystem::directory_options::skip_permission_denied)),
            query,
            include_extension,
            exclude_extension,
            print_count,
            enforce_max_count,
            max_count,
            print_only_file_matches,
            print_only_file_without_matches);
      }
    } catch (std::exception& e) {
      continue;
    }
  }

  return count;
}

}  // namespace search
