#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <streambuf>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <avx2_memchr.hpp>
#include <avx512f_strstr.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <immintrin.h>
#include <mio.hpp>

namespace search
{
bool is_binary_file(std::string_view haystack);

std::string_view::const_iterator needle_search(
    std::string_view needle,
    std::string_view::const_iterator haystack_begin,
    std::string_view::const_iterator haystack_end);

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

static inline bool has_suffix(const std::string_view& str,
                              const std::string_view& suffix)
{
  return str.size() >= suffix.size()
      && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static inline bool has_one_of_suffixes(const std::string_view& str)
{
  static const std::unordered_set<std::string_view> suffixes {
      "~",
      ".dll",
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
      ".pt",
      ".pak",
      ".qml",
      ".ttf",
      ".html",
      "appveyor.yml",
      ".ply",
      ".FBX",
      ".fbx",
      ".uasset",
      ".umap",
      ".rc2.res",
      ".bin",
      ".d",
      ".gch",
      ".orig",
      ".project",
      ".workspace",
      ".idea",
      ".epf",
      "sdkconfig",
      "sdkconfig.old",
      "personal.mak",
      ".userosscache",
      ".sln.docstates",
      ".a",
      ".bin",
      ".bz2",
      ".dt.yaml",
      ".dtb",
      ".dtbo",
      ".dtb.S",
      ".dwo",
      ".elf",
      ".gcno",
      ".gz",
      ".i",
      ".ko",
      ".lex.c",
      ".ll",
      ".lst",
      ".lz4",
      ".lzma",
      ".lzo",
      ".mod",
      ".mod.c",
      ".o",
      ".patch",
      ".s",
      ".so",
      ".so.dbg",
      ".su",
      ".symtypes",
      ".symversions",
      ".tar",
      ".xz",
      ".zst",
      "extra_certificates",
      ".pem",
      ".priv",
      ".x509",
      ".genkey",
      ".kdev4",
      ".defaults",
      ".projbuild",
      ".mk"};

  return suffixes.find(str) != suffixes.end();
}

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
  const auto query_size = query.size();
  std::size_t count = 0;
  for (auto const& dir_entry : iterator) {
    try {
      if (std::filesystem::is_regular_file(dir_entry)) {
        const auto& dir_path = dir_entry.path();
        const char* path_cstr = (const char*)dir_path.c_str();
        std::string_view path_string {path_cstr, strlen(path_cstr)};

        // Ignore dot files
        if (dir_path.filename().c_str()[0] == '.') {
          continue;
        }

        // Ignore files without an extension
        if (!dir_path.has_extension()) {
          continue;
        }

        // Ignore large files
        const auto file_size = std::filesystem::file_size(dir_path);
        if (file_size < query_size || file_size > 400 * 1024) {
          // Ignore files larger than 400 kB
          //
          // Only search text files in source code
          //   Unless they are large JSON/CSV files, single files in source code
          //   are unlikely to be this large (although it does happen often)
          //
          // TODO(pranav): Provide a commandline argument to change this amount
          // fmt::print(fg(fmt::color::yellow), "Skipping large file {}\n",
          // path_string);
          continue;
        }

        // Ignore files without extension, they are unlikely to be source code
        if ((include_extension.empty() && has_one_of_suffixes(path_string))) {
          // fmt::print(fg(fmt::color::yellow), "Skipping {}\n", path_string);
          continue;
        }

        // Check if file extension is in `include_extension` list
        // Check if file extension is NOT in `exclude_extension` list
        if ((include_extension.empty()
             || filename_has_extension_from_list(dir_path.extension(),
                                                 include_extension))
            && (exclude_extension.empty()
                || !filename_has_extension_from_list(dir_path.extension(),
                                                     exclude_extension)))
        {
          count += read_file_and_search(dir_path,
                                        query,
                                        print_count,
                                        enforce_max_count,
                                        max_count,
                                        print_only_file_matches,
                                        print_only_file_without_matches);
        }
      } else {
        const auto& dir_path = dir_entry.path();
        const char* path_cstr = (const char*)dir_path.c_str();
        std::string_view path_string {path_cstr, strlen(path_cstr)};
        const auto filename = dir_path.filename();
        const char* filename_cstr = (const char*)filename.c_str();

        if (strlen(filename_cstr) > 0 && filename_cstr[0] == '.') {
          // dot directory, ignore it
          continue;
        }

        static const std::unordered_set<const char*> ignored_dirs = {
            ".git",
            ".github",
            "build",
            "node_modules",
            ".vscode",
            ".DS_Store",
            "debugPublic",
            "DebugPublic",
            "debug",
            "Debug",
            "Release",
            "release",
            "Releases",
            "releases",
            "cmake-build-debug",
            "__pycache__",
            "Binaries",
            "Doc",
            "doc",
            "Documentation",
            "docs",
            "Docs",
            "bin",
            "Bin",
            "patches",
            "tar-install",
            "CMakeFiles",
            "install",
            "snap",
            "LICENSES",
            "img",
            "images",
            "imgs",
            // Unreal Engine
            "Plugins",
            "Content"};

        bool ignore = false;
        for (const auto& d : ignored_dirs) {
          if (strcmp(filename_cstr, d) == 0) {
            ignore = true;
            break;
          }
        }
        if (ignore) {
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
