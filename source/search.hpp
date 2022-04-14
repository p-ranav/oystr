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

static inline bool has_one_of_suffixes(
    const std::string_view& str, const std::vector<std::string_view>& suffixes)
{
  for (const auto& s : suffixes) {
    if (has_suffix(str, s)) {
      return true;
    }
  }
  return false;
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
        const char* path_cstr = (const char*)dir_entry.path().c_str();
        std::string_view path_string {path_cstr, strlen(path_cstr)};
        const auto file_size = std::filesystem::file_size(dir_entry.path());
        if (dir_entry.path().filename().c_str()[0] == '.'
            || file_size > 200 * 1024 || file_size < query_size)
        {
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

        if (include_extension.empty()) {
          const auto filename = dir_entry.path().filename();
          const std::string_view filename_cstr = (const char*)filename.c_str();

          static std::string_view ignore_list =
              "~.dll,.exe,.o,.so,.dmg,.7z,.gz,.iso,.jar,.rar,.zip,.tar,.sql,."
              "sqlite,"
              ".sys,.tiff,.tif,.bmp,.jpg,.jpeg,.gif,.png,.eps,.raw,.cr2,.crw,."
              "pef,"
              ".nef,.orf,.sr2,.pdf,.ai,.indd,.arc,.meta,.pdb,.pyc,.Spotlight-"
              "V100,"
              ".Trashes,.ehthumbs.db,Thumbs.db,.suo,.user,.lst,.pt,.pak,.qml,."
              "ttf,"
              ".html,appveyor,.ply,.FBX,.fbx,.uasset,.umap,.rc2.res,.bin,.d,."
              "gch,"
              ".org,.project,.workspace,.idea,.epf,sdkconfig,sdkconfig.old,"
              "personal.mak,"
              ".userosscache,.sln.docstates,.a,.bin,.bz2,.dt.yaml,.dtb,.dtbo,."
              "dtb.S,.dwo,"
              ".elf,.gcno,.gz,.ko,.ll,.lst,.lz4,.lzma,.lzo,.mod,.mod.c,.o,."
              "patch,.s,.so,"
              ".so.dbg,.su,.symtypes,.symversions,.xz,.zst,extra_certificates,."
              "pem,.priv,"
              ".x509,.genkey,.kdev4";

#if defined(__AVX512F__)
          if (avx512f_strstr(ignore_list, filename_cstr)
              != std::string_view::npos) {
            continue;
          }
#elif defined(__AVX2__)
          if (needle_search_avx2(
                  filename_cstr, ignore_list.begin(), ignore_list.end())
              != ignore_list.end())
          {
            continue;
          }
#else
          if (needle_search(
                  filename_cstr, ignore_list.begin(), ignore_list.end())
              != ignore_list.end())
          {
            continue;
          }
#endif
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
        const char* path_cstr = (const char*)dir_entry.path().c_str();
        std::string_view path_string {path_cstr, strlen(path_cstr)};
        const auto filename = dir_entry.path().filename();
        const std::string_view filename_cstr = (const char*)filename.c_str();

        static std::string_view ignore_list =
            ".git,build,node_modules,.vscode,.DS_Store,"
            "debugPublic,DebugPublic,Releases,releases,cmake-"
            "build-debug,"
            "__pycache__,Binaries,doc,Simulation/Saved,Documentation,"
            "Doc,Docs,patches,tar-install,install,snap";

#if defined(__AVX512F__)
        if (avx512f_strstr(ignore_list, filename_cstr)
            != std::string_view::npos) {
          continue;
        }
#elif defined(__AVX2__)
        if (needle_search_avx2(
                filename_cstr, ignore_list.begin(), ignore_list.end())
            != ignore_list.end())
        {
          continue;
        }
#else
        if (needle_search(filename_cstr, ignore_list.begin(), ignore_list.end())
            != ignore_list.end())
        {
          continue;
        }
#endif

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
