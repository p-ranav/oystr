#include <chrono>

#include <search.hpp>
namespace fs = std::filesystem;

#include <immintrin.h>
#include <substr_search.hpp>

const char* find_avx2_more(const char* b, const char* e, char c)
{
  const char* i = b;

  __m256i q = _mm256_set1_epi8(c);

  for (; i + 32 < e; i += 32) {
    __m256i x = _mm256_lddqu_si256(reinterpret_cast<const __m256i*>(i));
    __m256i r = _mm256_cmpeq_epi8(x, q);
    int z = _mm256_movemask_epi8(r);
    if (z)
      return i + __builtin_ffs(z) - 1;
  }
  if (i < e) {
    if (e - b < 32) {
      for (; i < e; ++i)
        if (*i == c)
          return i;
    } else {
      i = e - 32;
      __m256i x = _mm256_lddqu_si256(reinterpret_cast<const __m256i*>(i));
      __m256i r = _mm256_cmpeq_epi8(x, q);
      int z = _mm256_movemask_epi8(r);
      if (z)
        return i + __builtin_ffs(z) - 1;
    }
  }
  return e;
}

namespace search
{
auto is_binary_file(std::string_view haystack)
{
  // If the haystack has NUL characters, it's likely a binary file.
  return (haystack.find('\0') != std::string_view::npos);
}

auto needle_search_avx2(std::string_view needle,
                        std::string_view::const_iterator haystack_begin,
                        std::string_view::const_iterator haystack_end,
                        bool ignore_case)
{
  if (needle.empty()) {
    return haystack_end;
  }
  const char c = needle[0];

  // quickly find c in haystack
  auto it = haystack_begin;
  while (it != haystack_end) {
    const char* ptr = find_avx2_more(it, haystack_end, c);
    bool result = true;

    const char* i = ptr;
    for (auto& n : needle) {
      if (n != *i) {
        result = false;
        break;
      }
      i++;
    }

    if (result) {
      return ptr;
    } else {
      it = i;
    }
  }

  return haystack_end;
}

auto needle_search(std::string_view needle,
                   std::string_view::const_iterator haystack_begin,
                   std::string_view::const_iterator haystack_end,
                   bool ignore_case)
{
  if (haystack_begin != haystack_end) {
    if (ignore_case) {
#ifdef __APPLE__
      return std::search(haystack_begin,
                         haystack_end,
                         needle.begin(),
                         needle.end(),
                         [](char c1, char c2)
                         { return std::toupper(c1) == std::toupper(c2); });
#else
      return std::search(haystack_begin,
                         haystack_end,
                         std::boyer_moore_searcher(
                             needle.begin(),
                             needle.end(),
                             [](char c1, char c2)
                             { return std::toupper(c1) == std::toupper(c2); }));
#endif
    } else {
#ifdef __APPLE__
      return std::search(
          haystack_begin, haystack_end, needle.begin(), needle.end());
#else
      return std::search(
          haystack_begin,
          haystack_end,
          std::boyer_moore_searcher(needle.begin(), needle.end()));
#endif
    }
  } else {
    return haystack_end;
  }
}

// find case insensitive substring
auto needle_search_case_insensitive(std::string_view str,
                                    std::string_view query)
{
  if (str.size() < query.size())
    return std::string_view::npos;

#ifdef __APPLE__
  auto it = std::search(str.begin(),
                        str.end(),
                        query.begin(),
                        query.end(),
                        [](char c1, char c2)
                        { return std::toupper(c1) == std::toupper(c2); });
#else
  auto it = std::search(str.begin(),
                        str.end(),
                        std::boyer_moore_searcher(
                            query.begin(),
                            query.end(),
                            [](char c1, char c2)
                            { return std::toupper(c1) == std::toupper(c2); }));
#endif

  return it != str.end() ? std::size_t(it - str.begin())
                         : std::string_view::npos;
}

void print_colored(std::string_view str,
                   std::string_view query,
                   bool ignore_case)
{
  auto pos = ignore_case ? needle_search_case_insensitive(str, query)
                         : str.find(query);
  if (pos == std::string_view::npos) {
    fmt::print("{}", str);
    return;
  }
  fmt::print("{}", str.substr(0, pos));
  fmt::print(fg(fmt::color::red), "{}", str.substr(pos, query.size()));
  print_colored(str.substr(pos + query.size()), query, ignore_case);
}

auto file_search(std::string_view filename,
                 std::string_view haystack,
                 std::string_view needle,
                 bool ignore_case,
                 bool print_count,
                 bool enforce_max_count,
                 std::size_t max_count,
                 bool print_line_numbers,
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

  while (it != haystack_end) {
    // Search for needle
    it = needle_search_avx2(needle, it, haystack_end, ignore_case);

    if (it != haystack_end && !print_only_file_without_matches) {
      if (!printed_file_name) {
        fmt::print(fg(fmt::color::cyan), "{}", filename);

        if (!print_count) {
          fmt::print("\n");
        }

        printed_file_name = true;
      }

      count += 1;

      if (enforce_max_count && count > max_count) {
        break;
      }

      // Avoid printing lines from binary files with matches
      if (!process_binary_file_as_text && !print_count
          && is_binary_file(haystack)) {
        return;
      }

      // -l option
      // Print only filenames of files that contain matches.
      if (print_only_file_matches) {
        return;
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

      if (print_line_numbers) {
        auto line_number = std::count_if(haystack_begin,
                                         haystack_begin + newline_before + 1,
                                         [](char c) { return c == '\n'; })
            + 1;
        fmt::print(fg(fmt::color::magenta), "{:6d}", line_number);
        fmt::print(" ");
      }

      if (print_only_matching_parts) {
        fmt::print(
            fg(fmt::color::red),
            "{}\n",
            haystack.substr(std::size_t(it - haystack_begin), needle.size()));
      } else {
        // Get line from newline_before and newline_after
        auto line_size =
            std::size_t(newline_after - (haystack_begin + newline_before) - 1);
        if (line_size > 80) {
          line_size = 80;
        }
        auto line = haystack.substr(newline_before + 1, line_size);
        print_colored(line, needle, ignore_case);
        fmt::print("\n");
      }

      // Move to next line and continue search
      it = newline_after + 1;
      first_search = false;
      continue;
    } else {
      // no results at all in this file
      if (first_search) {
        // -L option
        // Print only filenames of files that do not contain matches.
        if (print_only_file_without_matches) {
          fmt::print(fg(fmt::color::cyan), "{}\n", filename);
        }
      } else {
        // at least one result
        fmt::print("\n");
      }
      break;
    }
  }

  // Done looking through file
  // Print count
  if (print_count) {
    fmt::print(fg(fmt::color::magenta), "{}\n", count);
  }
}

bool filename_matches_pattern_impl(std::string_view str,
                                   std::string_view pattern)
{
  auto n = str.size();
  auto m = pattern.size();

  // empty pattern can only match with
  // empty string
  if (m == 0)
    return (n == 0);

  // lookup table for storing results of
  // subproblems
  std::vector<std::vector<bool>> lookup;
  lookup.resize(n + 1, std::vector<bool>(m + 1, false));

  // empty pattern can match with empty string
  lookup[0][0] = true;

  // Only '*' can match with empty string
  for (std::size_t j = 1; j <= m; j++)
    if (pattern[j - 1] == '*')
      lookup[0][j] = lookup[0][j - 1];

  // fill the table in bottom-up fashion
  for (std::size_t i = 1; i <= n; i++) {
    for (std::size_t j = 1; j <= m; j++) {
      // Two cases if we see a '*'
      // a) We ignore ‘*’ character and move
      //    to next  character in the pattern,
      //     i.e., ‘*’ indicates an empty sequence.
      // b) '*' character matches with i_th
      //     character in input
      if (pattern[j - 1] == '*')
        lookup[i][j] = lookup[i][j - 1] || lookup[i - 1][j];

      // Current characters are considered as
      // matching in two cases
      // (a) current character of pattern is '?'
      // (b) characters actually match
      else if (pattern[j - 1] == '?' || str[i - 1] == pattern[j - 1])
        lookup[i][j] = lookup[i - 1][j - 1];

      // If characters don't match
      else
        lookup[i][j] = false;
    }
  }

  return lookup[n][m];
}

auto filename_matches_pattern(std::string_view filename,
                              const std::vector<std::string>& patterns)
{
  bool result = false;

  bool all_extensions = patterns.size() == 0;
  if (!all_extensions) {
    for (const auto& pattern : patterns) {
      if (filename_matches_pattern_impl(filename, pattern)) {
        result = true;
        break;
      }
    }
  } else {
    result = false;
  }

  return result;
}

void read_file_and_search(fs::path const& path,
                          std::string_view needle,
                          const std::vector<std::string>& include_extension,
                          const std::vector<std::string>& exclude_extension,
                          bool ignore_case,
                          bool print_count,
                          bool enforce_max_count,
                          std::size_t max_count,
                          bool print_line_numbers,
                          bool print_only_file_matches,
                          bool print_only_file_without_matches,
                          bool print_only_matching_parts,
                          bool process_binary_file_as_text)
{
  try {
    const auto path_string = path.string();
    const auto absolute_path = fs::absolute(path);
    std::string basename = absolute_path.filename().string();

    // Check if file extension is in `include_extension` list
    // Check if file extension is NOT in `exclude_extension` list
    if ((include_extension.empty()
         || filename_matches_pattern(basename, include_extension))
        && (exclude_extension.empty()
            || !filename_matches_pattern(basename, exclude_extension)))
    {
      auto mmap = mio::mmap_source(path_string);
      if (!mmap.is_open() || !mmap.is_mapped()) {
        return;
      }
      const std::string_view haystack(mmap.data(), mmap.size());

      /*std::ifstream is(path_string);
      auto haystack = std::string(std::istreambuf_iterator<char>(is),
      std::istreambuf_iterator<char>());*/

      file_search(path_string,
                  haystack,
                  needle,
                  ignore_case,
                  print_count,
                  enforce_max_count,
                  max_count,
                  print_line_numbers,
                  print_only_file_matches,
                  print_only_file_without_matches,
                  print_only_matching_parts,
                  process_binary_file_as_text);
    }
  } catch (const std::exception& e) {
  }
}

}  // namespace search
