#include <fnmatch.h>
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

void print_colored(std::string_view str,
                   std::string_view query,
                   fmt::memory_buffer& out)
{
  auto pos = find_needle_position(str, query);
  if (pos == std::string_view::npos) {
    fmt::format_to(std::back_inserter(out), "{}\n", str);
    return;
  }
  fmt::format_to(std::back_inserter(out), "{}", str.substr(0, pos));
  fmt::format_to(std::back_inserter(out),
                 "\033[1;31m{}\033[0m",
                 str.substr(pos, query.size()));
  print_colored(str.substr(pos + query.size()), query, out);
}

std::size_t searcher::file_search(std::string_view filename,
                                  std::string_view haystack)

{
  auto out = fmt::memory_buffer();

  // Start from the beginning
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();

  auto it = haystack_begin;
  bool first_search = true;
  bool printed_file_name = false;
  std::size_t current_line_number = 1;
  auto last_newline_pos = haystack_begin;
  auto no_file_name = filename.empty();

  while (it != haystack_end) {
#if defined(__SSE2__)
    std::string_view view(it, haystack_end - it);
    if (view.empty()) {
      it = haystack_end;
      break;
    } else {
      auto pos =
          sse2_strstr_v2(std::string_view(it, haystack_end - it), m_query);
      if (pos != std::string::npos) {
        it += pos;
      } else {
        it = haystack_end;
        break;
      }
    }
#else
    it = needle_search(m_query, it, haystack_end);
#endif

    if (it != haystack_end && !m_print_only_file_without_matches) {
      // needle found in haystack

      if (!no_file_name) {
        if (!printed_file_name) {
          if (m_is_stdout) {
            // Print filename once, bold cyan color
            fmt::format_to(
                std::back_inserter(out), "\n\033[1;36m{}\033[0m\n", filename);
          } else {
            // Print filename without newline, without any color
            fmt::format_to(std::back_inserter(out), "{}:", filename);
          }
          printed_file_name = true;
        } else {
          if (!m_is_stdout) {
            // Print filename for every match
            // without any color
            fmt::format_to(std::back_inserter(out), "{}:", filename);
          }
        }
      }

      std::string_view line;

      if (m_is_path_from_terminal) {
        // Only find lines and count line number if
        // this is actually a file
        //
        // If the input haystack is from a pipe or stdin
        // then don't do this

        // Found needle in haystack
        auto newline_before = haystack.rfind('\n', it - haystack_begin);
#if defined(__SSE2__)
        auto newline_after = haystack_end;
        auto pos =
            sse2_strstr_v2(std::string_view(it, haystack_end - it), "\n");
        if (pos != std::string::npos) {
          newline_after = it + pos;
        }
#else
        auto newline_after = std::find(it, haystack_end, '\n');
#endif

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
        if (m_is_stdout) {
          // Print line number in bold magenta
          fmt::format_to(std::back_inserter(out),
                         "\033[1;35m{}\033[0m: ",
                         current_line_number);
        } else {
          fmt::format_to(std::back_inserter(out), "{}: ", current_line_number);
        }

        // Get line [newline_before, newline_after]

        auto line_size =
            std::size_t(newline_after - (haystack_begin + newline_before) - 1);
        line = haystack.substr(newline_before + 1, line_size);

        // Move to next line and continue search
        it = newline_after + 1;
      } else {
        // Input is from pipe or stdin
        // Haystack is one line
        //
        // Since the processing is done
        // Go to end of haystack
        line = haystack;
        it = haystack_end;
      }

      if (m_is_stdout) {
        // Print colored, highlight needle in line
        print_colored(line, m_query, out);
      } else {
        fmt::format_to(std::back_inserter(out), "{}\n", line);
      }

      first_search = false;
    } else {
      // no results at all in this file
      break;
    }
  }

  if (!first_search) {
    fmt::print("{}", fmt::to_string(out));
  }

  return 0;
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
    file_search(path, haystack);
  } catch (const std::exception& e) {
  }
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

  if (typeflag == FTW_DNR) {
    // directory not readable
    return FTW_SKIP_SUBTREE;
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

void searcher::directory_search(const char* path)
{
  /* Invalid directory path? */
  if (path == NULL || *path == '\0')
    return;

  nftw(
      path, handle_posix_directory_entry, USE_FDS, FTW_PHYS | FTW_ACTIONRETVAL);
  searcher::m_ts->wait_for_tasks();
}

}  // namespace search
