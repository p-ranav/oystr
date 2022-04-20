#include <searcher.hpp>
namespace fs = std::filesystem;

#define BOOST_RANGE_ENABLE_CONCEPT_ASSERT 0
#include <boost/filesystem.hpp>
#include <boost/range/adaptors.hpp>

namespace bfs = boost::filesystem;
namespace ba = boost::adaptors;

#if defined(__unix__)

/* We want POSIX.1-2008 + XSI, i.e. SuSv4, features */
#  ifndef _XOPEN_SOURCE
#    define _XOPEN_SOURCE 700
#  endif

/* If the C library can support 64-bit file sizes
   and offsets, using the standard names,
   these defines tell the C library to do so. */
#  ifndef _LARGEFILE64_SOURCE
#    define _LARGEFILE64_SOURCE
#  endif

#  ifndef _FILE_OFFSET_BITS
#    define _FILE_OFFSET_BITS 64
#  endif

#  include <errno.h>

#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif

#  include <ftw.h>
#  include <stdlib.h>
#  include <string.h>
#  include <time.h>
#  include <unistd.h>

/* POSIX.1 says each process has at least 20 file descriptors.
 * Three of those belong to the standard streams.
 * Here, we use a conservative estimate of 15 available;
 * assuming we use at most two for other uses in this program,
 * we should never run into any problems.
 * Most trees are shallower than that, so it is efficient.
 * Deeper trees are traversed fine, just a bit slower.
 * (Linux allows typically hundreds to thousands of open files,
 *  so you'll probably never see any issues even if you used
 *  a much higher value, say a couple of hundred, but
 *  15 is a safe, reasonable value.)
 */
#  ifndef USE_FDS
#    define USE_FDS 15
#  endif

#endif

namespace search
{
std::string_view::const_iterator needle_search(
    std::string_view needle,
    std::string_view::const_iterator haystack_begin,
    std::string_view::const_iterator haystack_end)
{
  if (haystack_begin != haystack_end) {
    return std::search(
        haystack_begin, haystack_end, needle.begin(), needle.end());
  } else {
    return haystack_end;
  }
}

auto find_needle_position(std::string_view str, std::string_view query)
{
  auto it = needle_search(query, str.begin(), str.end());

  return it != str.end() ? std::size_t(it - str.begin())
                         : std::string_view::npos;
}

void print_colored(std::string_view str, std::string_view query)
{
  auto pos = find_needle_position(str, query);
  if (pos == std::string_view::npos) {
    fmt::print("{}", str);
    return;
  }
  fmt::print("{}", str.substr(0, pos));
  fmt::print(fg(fmt::color::red), "{}", str.substr(pos, query.size()));
  print_colored(str.substr(pos + query.size()), query);
}

std::size_t file_search(std::string_view filename,
                        std::string_view haystack,
                        std::string_view needle,
                        bool print_count,
                        bool enforce_max_count,
                        std::size_t max_count,
                        bool print_only_file_matches,
                        bool print_only_file_without_matches)

{
  // Start from the beginning
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();

  auto it = haystack_begin;
  bool first_search = true;
  std::size_t count = 0;
  bool printed_file_name = false;
  std::size_t current_line_number = 1;
  auto last_newline_pos = haystack_begin;

  while (it != haystack_end) {
    // Search for needle

#if defined(__AVX512F__)
    auto pos = avx512f_strstr(std::string_view(it, haystack_end - it), needle);
    if (pos != std::string_view::npos) {
      it += pos;
    } else {
      it = haystack_end;
      break;
    }
#elif defined(__AVX2__)
    it = needle_search_avx2(needle, it, haystack_end);
#else
    it = needle_search(needle, it, haystack_end);
#endif

    if (it != haystack_end && !print_only_file_without_matches) {
      if (!printed_file_name) {
        fmt::print(fg(fmt::color::cyan), "{}\n", filename);
        printed_file_name = true;
      }

      count += 1;

      if (enforce_max_count && count > max_count) {
        fmt::print("\n");
        break;
      }

      // -l option
      // Print only filenames of files that contain matches.
      if (print_only_file_matches) {
        return count;
      }

      // Found needle in haystack
      auto newline_before = haystack.rfind('\n', it - haystack_begin);
#if defined(__AVX2__)
      auto newline_after = find_avx2_more(it, haystack_end, '\n');
#else
      auto newline_after = std::find(it, haystack_end, '\n');
#endif

      if (!print_count) {
        if (last_newline_pos == haystack_begin) {
          last_newline_pos = haystack_begin + newline_before;
          if (newline_before != std::string_view::npos) {
            current_line_number +=
                std::count_if(haystack_begin,
                              last_newline_pos,
                              [](char c) { return c == '\n'; });
          }
        }

        current_line_number += std::count_if(last_newline_pos + 1,
                                             newline_after + 1,
                                             [](char c) { return c == '\n'; });
        fmt::print(fg(fmt::color::magenta), "{:6d} ", current_line_number);
      }

      if (print_count) {
        it = newline_after + 1;
        first_search = false;
        if (enforce_max_count && count == max_count) {
          break;
        }
        continue;
      }

      // Get line from newline_before and newline_after
      auto line_size =
          std::size_t(newline_after - (haystack_begin + newline_before) - 1);
      auto line = haystack.substr(newline_before + 1, line_size);
      print_colored(line, needle);
      fmt::print("\n");

      // Move to next line and continue search
      it = newline_after + 1;
      first_search = false;
    } else {
      // no results at all in this file
      if (first_search) {
        // -L option
        // Print only filenames of files that do not contain matches.
        if (print_only_file_without_matches) {
          fmt::print(fg(fmt::color::cyan), "{}\n", filename);
        }
      }
      break;
    }
  }

  // Done looking through file
  // Print count
  if (print_count) {
    if (count > 0) {
      fmt::print(fg(fmt::color::magenta), "{}\n\n", count);
    }
  }

  return count;
}

void searcher::read_file_and_search(const char* path)
{
  try {
    auto mmap = mio::mmap_source(path);
    if (!mmap.is_open() || !mmap.is_mapped()) {
      return;
    }
    const std::string_view haystack(mmap.data(), mmap.size());

    file_search(path,
                haystack,
                m_query,
                m_print_count,
                m_enforce_max_count,
                m_max_count,
                m_print_only_file_matches,
                m_print_only_file_without_matches);
  } catch (const std::exception& e) {
  }
}

bool searcher::include_file(const std::string_view& str)
{
  bool result = false;
  for (const auto& suffix : m_include_extension) {
    if (str.size() >= suffix.size()
        && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
      result = true;
      break;
    }
  }
  return result;
}

bool searcher::exclude_file(const std::string_view& str)
{
  bool result = false;
  for (const auto& suffix : m_exclude_extension) {
    if (str.size() >= suffix.size()
        && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
      result = true;
      break;
    }
  }
  return result;
}

bool searcher::exclude_file_known_suffixes(const std::string_view& str)
{
  static const std::unordered_set<std::string_view> known_suffixes = {
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

  bool result = false;
  for (const auto& suffix : known_suffixes) {
    if (str.size() >= suffix.size()
        && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
      result = true;
      break;
    }
  }
  return result;
}

bool exclude_directory(const char* path)
{
  static const std::unordered_set<const char*> ignored_dirs = {
      ".git/",         ".github/",       "build/",
      "node_modules/", ".vscode/",       ".DS_Store/",
      "debugPublic/",  "DebugPublic/",   "debug/",
      "Debug/",        "Release/",       "release/",
      "Releases/",     "releases/",      "cmake-build-debug/",
      "__pycache__/",  "Binaries/",      "Doc/",
      "doc/",          "Documentation/", "docs/",
      "Docs/",         "bin/",           "Bin/",
      "patches/",      "tar-install/",   "CMakeFiles/",
      "install/",      "snap/",          "LICENSES/",
      "img/",          "images/",        "imgs/"};

  for (const auto& ignored_dir : ignored_dirs) {
    // if path contains ignored dir, ignore it
    if (strstr(path, ignored_dir) != nullptr) {
      return true;
    }
  }

  return false;
}

#if defined(__unix__)

int handle_posix_directory_entry(const char* filepath,
                                 const struct stat* info,
                                 const int typeflag,
                                 struct FTW* pathinfo)
{
  if (info->st_size > 275 * 1024) {
    return FTW_CONTINUE;
  }

  if (!filepath)
    return 0;

  if (typeflag == FTW_DNR) {
    // directory not readable
    return 0;
  }

  if (typeflag == FTW_D || typeflag == FTW_DP) {
    // directory

    if (exclude_directory(filepath)) {
      return FTW_SKIP_SUBTREE;
    } else {
      return FTW_CONTINUE;
    }
  }

  if (typeflag == FTW_F) {
    auto filepath_size = strlen(filepath);

    if (filepath_size < 1) {
      return FTW_CONTINUE;
    }

    auto filepath_view = std::string_view(filepath, filepath_size);

    // Check if file extension is in `include_extension` list
    // Check if file extension is NOT in `exclude_extension` list
    if ((searcher::m_include_extension.empty()
         || searcher::include_file(filepath_view))
        && (searcher::m_exclude_extension.empty()
            || !searcher::exclude_file(filepath_view)))
    {
      if (exclude_directory(filepath)) {
        return FTW_SKIP_SUBTREE;
      } else if (searcher::m_include_extension.empty()) {
        if (searcher::exclude_file_known_suffixes(filepath_view)) {
          return FTW_CONTINUE;
        }
      }
      // const char *const filename = filepath + pathinfo->base;
      // fmt::print(fg(fmt::color::cyan), "{}\n", filename);
      searcher::read_file_and_search(filepath);
    }
  }

  return FTW_CONTINUE;
}

void directory_search_posix(const char* path)
{
  /* Invalid directory path? */
  if (path == NULL || *path == '\0')
    return;

  nftw(
      path, handle_posix_directory_entry, USE_FDS, FTW_PHYS | FTW_ACTIONRETVAL);
}

#endif

void directory_search_portable(const char* path)
{
  for (auto const& dir_entry : std::filesystem::directory_iterator(
           path, std::filesystem::directory_options::skip_permission_denied))
  {
    try {
      if (std::filesystem::is_regular_file(dir_entry)) {
        const auto& dir_path = dir_entry.path();
        const char* filepath = (const char*)dir_path.c_str();
        auto filepath_view = std::string_view {filepath, strlen(filepath)};

        // Check if file extension is in `include_extension` list
        // Check if file extension is NOT in `exclude_extension` list
        if ((searcher::m_include_extension.empty()
             || searcher::include_file(filepath_view))
            && (searcher::m_exclude_extension.empty()
                || !searcher::exclude_file(filepath_view)))
        {
          if (searcher::m_include_extension.empty()) {
            if (searcher::exclude_file_known_suffixes(filepath_view)) {
              continue;
            }
          }
          // const char *const filename = filepath + pathinfo->base;
          // fmt::print(fg(fmt::color::cyan), "{}\n", filename);
          searcher::read_file_and_search(filepath);
        }
      } else {
        const auto& dir_path = dir_entry.path();
        const char* path_cstr = (const char*)dir_path.c_str();

        if (exclude_directory(path_cstr)) {
          continue;
        } else {
          // recurse
          search::directory_search_portable(path_cstr);
        }
      }
    } catch (std::exception& e) {
      continue;
    }
  }
}

void searcher::directory_search(const char* path)
{
  const boost::regex my_filter("(.*).([c|h])");
  static boost::smatch what;

  for (auto& entry :
       boost::make_iterator_range(bfs::recursive_directory_iterator(path), {})
           | ba::filtered(
               static_cast<bool (*)(const bfs::path&)>(&bfs::is_regular_file))
           | ba::filtered(
               [&](const bfs::path& path) {
                 return boost::regex_match(
                     path.filename().string(), what, my_filter);
               }))
  {
    std::cout << entry.path().c_str() << std::endl;
  }
  return;

#if defined(__unix__)
  directory_search_posix(path);
#else
  directory_search_portable(path);
#endif
}

}  // namespace search
