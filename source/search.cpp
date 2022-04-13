#include <cassert>
#include <chrono>

#include <avx2_memchr.hpp>
#include <avx512f_strstr.hpp>
#include <search.hpp>
namespace fs = std::filesystem;

namespace search
{
auto is_binary_file(std::string_view haystack)
{
#if __AVX2__
  return find_avx2_more(haystack.begin(), haystack.end(), '\0')
      != haystack.end();
#else
  return haystack.find('\0') != std::string_view::npos;
#endif
}

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
  fmt::print("  | {}", str.substr(0, pos));
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
                        bool print_only_file_without_matches,
                        bool print_only_matching_parts,
                        bool process_binary_file_as_text)

{
  // Start from the beginning
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();

  auto it = haystack_begin;
  bool first_search = true;
  std::size_t count = 0;
  bool printed_file_name = false;

  bool haystack_is_binary_file = is_binary_file(haystack);

  while (it != haystack_end) {
    // Search for needle

#if __AVX512F__
    // auto start = std::chrono::high_resolution_clock::now();
    auto pos = avx512f_strstr(std::string_view(it, haystack_end - it), needle);
    // auto end = std::chrono::high_resolution_clock::now();
    // fmt::print("{} ms\n",
    // std::chrono::duration_cast<std::chrono::milliseconds>(end -
    // start).count());
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

      // Avoid printing lines from binary files with matches
      if (!process_binary_file_as_text && !print_count
          && haystack_is_binary_file) {
        return count;
      }

      // -l option
      // Print only filenames of files that contain matches.
      if (print_only_file_matches) {
        return count;
      }

      // Found needle in haystack
      auto newline_before = haystack.rfind('\n', it - haystack_begin);
      auto newline_after = std::find(it, haystack_end, '\n');

      if (print_count) {
        it = newline_after + 1;
        first_search = false;
        if (enforce_max_count && count == max_count) {
          break;
        }
        continue;
      }

      if (print_only_matching_parts) {
        fmt::print(
            fg(fmt::color::red),
            "{}\n",
            haystack.substr(std::size_t(it - haystack_begin), needle.size()));
      } else {
        // Get line from newline_before and newline_after
        const auto line_size =
            std::size_t(newline_after - (haystack_begin + newline_before) - 1);
        auto line = haystack.substr(newline_before + 1, line_size);
        print_colored(line, needle);
        fmt::print("\n");
      }

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
      if (strcmp(fext.c_str(), ext.data()) == 0) {
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
                                 bool print_only_file_without_matches,
                                 bool print_only_matching_parts,
                                 bool process_binary_file_as_text)
{
  try {
    const auto path_string = path.c_str();
    const auto absolute_path = fs::absolute(path);
    std::string basename = absolute_path.filename().string();

    auto mmap = mio::mmap_source(path_string);
    if (!mmap.is_open() || !mmap.is_mapped()) {
      return 0;
    }
    const std::string_view haystack(mmap.data(), mmap.size());

    /*std::ifstream is(path_string);
      auto haystack = std::string(std::istreambuf_iterator<char>(is),
      std::istreambuf_iterator<char>());*/

    return file_search(path_string,
                       haystack,
                       needle,
                       print_count,
                       enforce_max_count,
                       max_count,
                       print_only_file_matches,
                       print_only_file_without_matches,
                       print_only_matching_parts,
                       process_binary_file_as_text);
  } catch (const std::exception& e) {
  }
  return 0;
}

}  // namespace search
