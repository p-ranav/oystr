#include <searcher.hpp>
namespace fs = std::filesystem;

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

/* We want POSIX.1-2008 + XSI, i.e. SuSv4, features */
#  define _XOPEN_SOURCE 700

/* Added on 2017-06-25:
   If the C library can support 64-bit file sizes
   and offsets, using the standard names,
   these defines tell the C library to do so. */
#  ifndef _LARGEFILE64_SOURCE
#    define _LARGEFILE64_SOURCE
#  endif
#  define _FILE_OFFSET_BITS 64

#  include <errno.h>
#  include <ftw.h>
#  include <stdio.h>
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

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

bool has_one_of_suffixes(const std::string_view& str)
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

int handle_posix_directory_entry(const char* filepath,
                                 const struct stat* info,
                                 const int typeflag,
                                 struct FTW* pathinfo)
{
  if (!filepath)
    return 0;

  auto filepath_size = strlen(filepath);

  if (filepath_size < 1)
    return 0;

  auto filepath_view = std::string_view(filepath, filepath_size);

  if (filepath_size > 0) {
    if (filepath_view[filepath_size - 1] == '/') {
      // directory

      if (filepath_view[0] == '.') {
        // dot directory, ignore it
        return 0;
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
        if (strcmp(filepath, d) == 0) {
          ignore = true;
          break;
        }
      }
      if (ignore) {
        return 0;
      }

    }  // directory
  }  // filepath_size > 0

  // Check if file extension is in `include_extension` list
  // Check if file extension is NOT in `exclude_extension` list
  if ((searcher::m_include_extension.empty()
       || searcher::include_file(filepath_view))
      && (searcher::m_exclude_extension.empty()
          || !searcher::exclude_file(filepath_view)))
  {
    /* const char *const filename = filepath + pathinfo->base; */
    const double bytes = (double)info->st_size; /* Not exact if large! */

    // Ignore large files
    if (bytes > 250 * 1024) {
      // Ignore files larger than 250 kB
      //
      // Only search text files in source code
      //   Unless they are large JSON/CSV files, single files in source code
      //   are unlikely to be this large (although it does happen often)
      //
      // TODO(pranav): Provide a commandline argument to change this amount
      // fmt::print(fg(fmt::color::yellow), "Skipping large file {}\n",
      // path_string);
      return 0;
    }

    // Ignore files without extension, they are unlikely to be source code
    if (has_one_of_suffixes(filepath_view)) {
      // fmt::print(fg(fmt::color::yellow), "Skipping {}\n", path_string);
      return 0;
    }

    /*
struct tm mtime;

localtime_r(&(info->st_mtime), &mtime);

printf("%04d-%02d-%02d %02d:%02d:%02d",
      mtime.tm_year+1900, mtime.tm_mon+1, mtime.tm_mday,
      mtime.tm_hour, mtime.tm_min, mtime.tm_sec);

if (bytes >= 1099511627776.0)
    printf(" %9.3f TiB", bytes / 1099511627776.0);
else
if (bytes >= 1073741824.0)
    printf(" %9.3f GiB", bytes / 1073741824.0);
else
if (bytes >= 1048576.0)
    printf(" %9.3f MiB", bytes / 1048576.0);
else
if (bytes >= 1024.0)
    printf(" %9.3f KiB", bytes / 1024.0);
else
    printf(" %9.0f B  ", bytes);

if (typeflag == FTW_SL) {
    char   *target;
    size_t  maxlen = 1023;
    ssize_t len;

    while (1) {

  target = (char*)malloc(maxlen + 1);
        if (target == NULL)
            return ENOMEM;

        len = readlink(filepath, target, maxlen);
        if (len == (ssize_t)-1) {
            const int saved_errno = errno;
            free(target);
            return saved_errno;
        }
        if (len >= (ssize_t)maxlen) {
            free(target);
            maxlen += 1024;
            continue;
        }

        target[len] = '\0';
        break;
    }

    printf(" %s -> %s\n", filepath, target);
    free(target);
} else
if (typeflag == FTW_SLN)
    printf(" %s (dangling symlink)\n", filepath);
else
if (typeflag == FTW_F)
    printf(" %s\n", filepath);
else
if (typeflag == FTW_D || typeflag == FTW_DP)
    printf(" %s/\n", filepath);
else
if (typeflag == FTW_DNR)
    printf(" %s/ (unreadable)\n", filepath);
else
    printf(" %s (unknown)\n", filepath);
    */

    searcher::read_file_and_search(filepath);
  }

  return 0;
}

void directory_search_posix(const char* path)
{
  /* Invalid directory path? */
  if (path == NULL || *path == '\0')
    return;

  nftw(path, handle_posix_directory_entry, USE_FDS, FTW_PHYS);
}

#endif

void searcher::directory_search(const char* path)
{
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
  directory_search_posix(path);
#endif
}

}  // namespace search