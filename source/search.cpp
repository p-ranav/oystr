#include <cassert>
#include <chrono>

#include <avx2_memchr.hpp>
#include <avx512f_strstr.hpp>
#include <search.hpp>
namespace fs = std::filesystem;

namespace search
{
auto needle_search(std::string_view needle,
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

// find case insensitive substring
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

#if __AVX512F__
    auto pos = avx512f_strstr(std::string_view(it, haystack_end - it), needle);
    if (pos != std::string_view::npos) {
      it += pos;
    } else {
      it = haystack_end;
      break;
    }
#elif __AVX2__
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
#if __AVX2__
      auto newline_after = find_avx2_more(it, haystack_end, '\n');
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
      fmt::print(fg(fmt::color::magenta), "{:6d} ", current_line_number);

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

bool filename_has_extension_from_list(
    const std::filesystem::path& fext,
    const std::vector<std::string>& extensions)
{
  bool result = false;

  bool all_extensions = extensions.size() == 0;
  if (!all_extensions) {
    for (const auto& ext : extensions) {
      if (strcmp((const char*)fext.c_str(), ext.data()) == 0) {
        result = true;
        break;
      }
    }
  } else {
    result = false;
  }

  return result;
}

std::size_t read_file_and_search(fs::path const& path,
                                 std::string_view needle,
                                 bool print_count,
                                 bool enforce_max_count,
                                 std::size_t max_count,
                                 bool print_only_file_matches,
                                 bool print_only_file_without_matches)
{
  try {
    auto mmap = mio::mmap_source(path.c_str());
    if (!mmap.is_open() || !mmap.is_mapped()) {
      return 0;
    }
    const std::string_view haystack(mmap.data(), mmap.size());

    return file_search((const char*)path.c_str(),
                       haystack,
                       needle,
                       print_count,
                       enforce_max_count,
                       max_count,
                       print_only_file_matches,
                       print_only_file_without_matches);
  } catch (const std::exception& e) {
  }
  return 0;
}

}  // namespace search
