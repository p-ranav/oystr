#include <cassert>
#include <chrono>

#include <search.hpp>
namespace fs = std::filesystem;

#include <immintrin.h>

#if __AVX512F__
namespace bits
{
template<typename T>
T clear_leftmost_set(const T value)
{
  assert(value != 0);

  return value & (value - 1);
}

template<typename T>
unsigned get_first_bit_set(const T value)
{
  assert(value != 0);

  return __builtin_ctz(value);
}

template<>
unsigned get_first_bit_set<uint64_t>(const uint64_t value)
{
  assert(value != 0);

  return __builtin_ctzl(value);
}

}  // namespace bits

__mmask16 zero_byte_mask(const __m512i v)
{
  const __m512i v01 = _mm512_set1_epi32(0x01010101u);
  const __m512i v80 = _mm512_set1_epi32(0x80808080u);

  const __m512i v1 = _mm512_sub_epi32(v, v01);
  // tmp1 = (v - 0x01010101) & ~v & 0x80808080
  const __m512i tmp1 = _mm512_ternarylogic_epi32(v1, v, v80, 0x20);

  return _mm512_test_epi32_mask(tmp1, tmp1);
}
/*
    string - pointer to the string
    n      - string length in bytes
    needle - pointer to another string
    n      - needle length in bytes
*/
size_t avx512f_strstr_long(const char* string,
                           size_t n,
                           const char* needle,
                           size_t k)
{
  assert(n > 0);
  assert(k > 4);

  __m512i curr;
  __m512i next;
  __m512i v0, v1, v2, v3;

  char* haystack = const_cast<char*>(string);
  char* last = haystack + n;

  const uint32_t prf = *(uint32_t*)needle;  // the first 4 bytes of needle
  const __m512i prefix = _mm512_set1_epi32(prf);

  next = _mm512_loadu_si512(haystack);

  for (/**/; haystack < last; haystack += 64) {
    curr = next;
    next = _mm512_loadu_si512(haystack + 64);
    const __m512i shft = _mm512_alignr_epi32(next, curr, 1);

    v0 = curr;

    {
      const __m512i t1 = _mm512_srli_epi32(curr, 8);
      const __m512i t2 = _mm512_slli_epi32(shft, 24);
      v1 = _mm512_or_si512(t1, t2);
    }
    {
      const __m512i t1 = _mm512_srli_epi32(curr, 16);
      const __m512i t2 = _mm512_slli_epi32(shft, 16);
      v2 = _mm512_or_si512(t1, t2);
    }
    {
      const __m512i t1 = _mm512_srli_epi32(curr, 24);
      const __m512i t2 = _mm512_slli_epi32(shft, 8);
      v3 = _mm512_or_si512(t1, t2);
    }

    uint16_t m0 = _mm512_cmpeq_epi32_mask(v0, prefix);
    uint16_t m1 = _mm512_cmpeq_epi32_mask(v1, prefix);
    uint16_t m2 = _mm512_cmpeq_epi32_mask(v2, prefix);
    uint16_t m3 = _mm512_cmpeq_epi32_mask(v3, prefix);

    int index = 64;
    while (m0 | m1 | m2 | m3) {
      if (m0) {
        int pos = __builtin_ctz(m0) * 4 + 0;
        m0 = m0 & (m0 - 1);

        if (pos < index && memcmp(haystack + pos + 4, needle + 4, k - 4) == 0) {
          index = pos;
        }
      }

      if (m1) {
        int pos = __builtin_ctz(m1) * 4 + 1;
        m1 = m1 & (m1 - 1);

        if (pos < index && memcmp(haystack + pos + 4, needle + 4, k - 4) == 0) {
          index = pos;
        }
      }

      if (m2) {
        int pos = __builtin_ctz(m2) * 4 + 2;
        m2 = m2 & (m2 - 1);

        if (pos < index && memcmp(haystack + pos + 4, needle + 4, k - 4) == 0) {
          index = pos;
        }
      }

      if (m3) {
        int pos = __builtin_ctz(m3) * 4 + 3;
        m3 = m3 & (m3 - 1);

        if (pos < index && memcmp(haystack + pos + 4, needle + 4, k - 4) == 0) {
          index = pos;
        }
      }
    }

    if (index < 64) {
      return (haystack - string) + index;
    }
  }

  return size_t(-1);
}

// ------------------------------------------------------------------------

size_t avx512f_strstr_eq4(const char* string, size_t n, const char* needle)
{
  assert(n > 0);

  __m512i curr;
  __m512i next;
  __m512i v0, v1, v2, v3;

  char* haystack = const_cast<char*>(string);
  char* last = haystack + n;

  const uint32_t prf = *(uint32_t*)needle;  // the first 4 bytes of needle
  const __m512i prefix = _mm512_set1_epi32(prf);

  next = _mm512_loadu_si512(haystack);

  for (/**/; haystack < last; haystack += 64) {
    curr = next;
    next = _mm512_loadu_si512(haystack + 64);
    const __m512i shft = _mm512_alignr_epi32(next, curr, 1);

    v0 = curr;

    {
      const __m512i t1 = _mm512_srli_epi32(curr, 8);
      const __m512i t2 = _mm512_slli_epi32(shft, 24);
      v1 = _mm512_or_si512(t1, t2);
    }
    {
      const __m512i t1 = _mm512_srli_epi32(curr, 16);
      const __m512i t2 = _mm512_slli_epi32(shft, 16);
      v2 = _mm512_or_si512(t1, t2);
    }
    {
      const __m512i t1 = _mm512_srli_epi32(curr, 24);
      const __m512i t2 = _mm512_slli_epi32(shft, 8);
      v3 = _mm512_or_si512(t1, t2);
    }

    uint16_t m0 = _mm512_cmpeq_epi32_mask(v0, prefix);
    uint16_t m1 = _mm512_cmpeq_epi32_mask(v1, prefix);
    uint16_t m2 = _mm512_cmpeq_epi32_mask(v2, prefix);
    uint16_t m3 = _mm512_cmpeq_epi32_mask(v3, prefix);

    int index = 64;
    if (m0) {
      int pos = __builtin_ctz(m0) * 4 + 0;
      if (pos < index) {
        index = pos;
      }
    }

    if (m1) {
      int pos = __builtin_ctz(m1) * 4 + 1;
      if (pos < index) {
        index = pos;
      }
    }

    if (m2) {
      int pos = __builtin_ctz(m2) * 4 + 2;
      if (pos < index) {
        index = pos;
      }
    }

    if (m3) {
      int pos = __builtin_ctz(m3) * 4 + 3;
      if (pos < index) {
        index = pos;
      }
    }

    if (index < 64) {
      return (haystack - string) + index;
    }

    assert(m0 == 0 && m1 == 0 && m2 == 0 && m3 == 0);
  }

  return size_t(-1);
}

// ------------------------------------------------------------------------

size_t avx512f_strstr(const char* s,
                      size_t n,
                      const char* needle,
                      size_t needle_size)
{
  size_t result = std::string::npos;

  if (n < needle_size) {
    return result;
  }

  switch (needle_size) {
    case 0:
      return 0;

    case 1: {
      const char* res = reinterpret_cast<const char*>(strchr(s, needle[0]));

      return (res != nullptr) ? res - s : std::string::npos;
    }
    case 2:
    case 3: {
      const char* res = reinterpret_cast<const char*>(strstr(s, needle));

      return (res != nullptr) ? res - s : std::string::npos;
    }

    case 4:
      result = avx512f_strstr_eq4(s, n, needle);
      break;

    default:
      result = avx512f_strstr_long(s, n, needle, needle_size);
      break;
  }

  if (result <= n - needle_size) {
    return result;
  } else {
    return std::string::npos;
  }
}

// --------------------------------------------------

size_t avx512f_strstr(const std::string_view& s, const std::string_view& needle)
{
  return avx512f_strstr(s.data(), s.size(), needle.data(), needle.size());
}
#endif

namespace search
{
#if __AVX2__
// https://gms.tf/stdfind-and-memchr-optimizations.html
//
const char* find_avx2_more(const char* b,
                           const char* e,
                           char c,
                           bool ignore_case)
{
  const char* i = b;

  __m256i q = ignore_case ? _mm256_set1_epi8(toupper(c)) : _mm256_set1_epi8(c);

  for (; i + 32 < e; i += 32) {
    __m256i x = _mm256_lddqu_si256(reinterpret_cast<const __m256i*>(i));
    __m256i r = _mm256_cmpeq_epi8(x, q);
    int z = _mm256_movemask_epi8(r);
    if (z)
      return i + __builtin_ffs(z) - 1;
  }
  if (i < e) {
    if (e - b < 32) {
      for (; i < e; ++i) {
        if ((!ignore_case && *i == c)
            || (ignore_case && toupper(*i) == toupper(c))) {
          return i;
        }
      }
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

auto needle_search_avx2(std::string_view needle,
                        std::string_view::const_iterator haystack_begin,
                        std::string_view::const_iterator haystack_end,
                        bool ignore_case)
{
  if (needle.empty()) {
    return haystack_end;
  }
  const char c = needle[0];

  auto it = haystack_begin;
  while (it < haystack_end) {
    const char* ptr = find_avx2_more(it, haystack_end, c, ignore_case);

    if (!ptr) {
      break;
    }

    bool result = true;

    const char* i = ptr;
    for (auto& n : needle) {
      if ((!ignore_case && n != *i)
          || (ignore_case && toupper(n) != toupper(*i))) {
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
#endif

auto is_binary_file(std::string_view haystack)
{
#if __AVX2__
  return find_avx2_more(haystack.begin(), haystack.end(), '\0', false)
      != haystack.end();
#else
  return haystack.find('\0') != std::string_view::npos;
#endif
}

auto needle_search(std::string_view needle,
                   std::string_view::const_iterator haystack_begin,
                   std::string_view::const_iterator haystack_end,
                   bool ignore_case)
{
  if (haystack_begin != haystack_end) {
    if (ignore_case) {
      return std::search(haystack_begin,
                         haystack_end,
                         needle.begin(),
                         needle.end(),
                         [](char c1, char c2)
                         { return std::toupper(c1) == std::toupper(c2); });
    } else {
      return std::search(
          haystack_begin, haystack_end, needle.begin(), needle.end());
    }
  } else {
    return haystack_end;
  }
}

// find case insensitive substring
auto find_needle_position(std::string_view str,
                          std::string_view query,
                          bool ignore_case)
{
/*
#if __AVX512F__
  auto it = str.begin();
  auto pos = avx512f_strstr(str, query);
  if (pos != std::string_view::npos) {
    it += pos;
  }
#el
*/
#if __AVX2__
  auto it = needle_search_avx2(query, str.begin(), str.end(), ignore_case);
#else
  auto it = needle_search(query, str.begin(), str.end(), ignore_case);
#endif

  return it != str.end() ? std::size_t(it - str.begin())
                         : std::string_view::npos;
}

void print_colored(std::string_view str,
                   std::string_view query,
                   bool ignore_case)
{
  auto pos = find_needle_position(str, query, ignore_case);
  if (pos == std::string_view::npos) {
    fmt::print("{}", str);
    return;
  }
  fmt::print("{}", str.substr(0, pos));
  fmt::print(fg(fmt::color::red), "{}", str.substr(pos, query.size()));
  print_colored(str.substr(pos + query.size()), query, ignore_case);
}

std::size_t file_search(std::string_view filename,
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

#if __AVX512F__
    auto pos = avx512f_strstr(std::string_view(it, haystack_end - it), needle);
    if (pos != std::string_view::npos) {
      it += pos;
    } else {
      it = haystack_end;
      break;
    }
#elif __AVX2__
    it = needle_search_avx2(needle, it, haystack_end, ignore_case);
#else
    it = needle_search(needle, it, haystack_end, ignore_case);
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
          && is_binary_file(haystack)) {
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
        const auto line_size =
            std::size_t(newline_after - (haystack_begin + newline_before) - 1);
        auto line = haystack.substr(newline_before + 1, line_size);
        print_colored(line, needle, ignore_case);
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

std::size_t read_file_and_search(
    fs::path const& path,
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
        return 0;
      }
      const std::string_view haystack(mmap.data(), mmap.size());

      /*std::ifstream is(path_string);
      auto haystack = std::string(std::istreambuf_iterator<char>(is),
      std::istreambuf_iterator<char>());*/

      return file_search(path_string,
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
  return 0;
}

}  // namespace search
