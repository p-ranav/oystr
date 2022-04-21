#include <fnmatch.h>
#include <glob.h>
#include <searcher.hpp>
namespace fs = std::filesystem;

/* We want POSIX.1-2008 + XSI, i.e. SuSv4, features */
#ifndef _XOPEN_SOURCE
#  define _XOPEN_SOURCE 700
#endif

/* If the C library can support 64-bit file sizes
   and offsets, using the standard names,
   these defines tell the C library to do so. */
#ifndef _LARGEFILE64_SOURCE
#  define _LARGEFILE64_SOURCE
#endif

#ifndef _FILE_OFFSET_BITS
#  define _FILE_OFFSET_BITS 64
#endif

#include <errno.h>

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <ftw.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
#ifndef USE_FDS
#  define USE_FDS 15
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
  auto out = fmt::memory_buffer();

  // Start from the beginning
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();

  auto it = haystack_begin;
  bool first_search = true;
  std::size_t count = 0;
  std::size_t current_line_number = 1;
  auto last_newline_pos = haystack_begin;

  while (it != haystack_end) {
#if defined(__SSE2__)
    auto pos = sse2_strstr_v2(std::string_view(it, haystack_end - it), needle);
    if (pos != std::string::npos) {
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
      // needle found in haystack

      fmt::format_to(std::back_inserter(out), "{}:", filename);

      count += 1;

      if (enforce_max_count && count > max_count) {
        break;
      }

      // -l option
      // Print only filenames of files that contain matches.
      if (print_only_file_matches) {
        return count;
      }

      // Found needle in haystack
      auto newline_before = haystack.rfind('\n', it - haystack_begin);
#if defined(__SSE2__)
      auto newline_after = haystack_end;
      auto pos = sse2_strstr_v2(std::string_view(it, haystack_end - it), "\n");
      if (pos != std::string::npos) {
        newline_after = it + pos;
      }
#elif defined(__AVX2__)
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
        fmt::format_to(std::back_inserter(out), "{}:", current_line_number);
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
      fmt::format_to(std::back_inserter(out), "{}\n", line);

      // Move to next line and continue search
      it = newline_after + 1;
      first_search = false;
    } else {
      // no results at all in this file
      if (first_search) {
        // -L option
        // Print only filenames of files that do not contain matches.
        if (print_only_file_without_matches) {
          fmt::format_to(std::back_inserter(out), "{}\n", filename);
        }
      }
      break;
    }
  }

  // Done looking through file
  // Print count
  if (print_count) {
    if (count > 0) {
      fmt::format_to(std::back_inserter(out), "{}\n", count);
    }
  }

  if (!first_search) {
    fmt::print("{}", fmt::to_string(out));
  }

  return count;
}

std::string get_file_contents(const char* filename)
{
  std::FILE* fp = std::fopen(filename, "rb");
  if (fp) {
    std::string contents;
    std::fseek(fp, 0, SEEK_END);
    contents.resize(std::ftell(fp));
    std::rewind(fp);
    const auto size = std::fread(&contents[0], 1, contents.size(), fp);
    std::fclose(fp);
    return (contents);
  }
  throw(errno);
}

void searcher::read_file_and_search(const char* path)
{
  try {
    const std::string haystack = get_file_contents(path);
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

int handle_posix_directory_entry(const char* filepath,
                                 const struct stat* info,
                                 const int typeflag,
                                 struct FTW* pathinfo)
{
  static const bool skip_fnmatch =
      searcher::m_filter == std::string_view {"*.*"};
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
    if (skip_fnmatch || fnmatch(searcher::m_filter.data(), filepath, 0) == 0) {
      searcher::m_ts->push_task(
          [pathstring = std::string {filepath}]()
          { searcher::read_file_and_search(pathstring.data()); });
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
  searcher::m_ts->wait_for_tasks();
}

void searcher::directory_search(const char* path)
{
  directory_search_posix(path);
}

}  // namespace search
