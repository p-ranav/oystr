#include <search.hpp>
namespace fs = std::filesystem;

namespace search
{
auto is_binary_file(std::string_view haystack)
{
  // If the haystack has NUL characters, it's likely a binary file.
  return (haystack.find('\0') != std::string_view::npos);
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
auto needle_search_case_insensitive(std::string_view str,
                                    std::string_view query)
{
  if (str.size() < query.size())
    return std::string_view::npos;

  auto it = std::search(str.begin(),
                        str.end(),
                        query.begin(),
                        query.end(),
                        [](char c1, char c2)
                        { return std::toupper(c1) == std::toupper(c2); });

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
    std::cout << termcolor::white << termcolor::bold << str << termcolor::reset;
    return;
  }
  std::cout << termcolor::white << termcolor::bold << str.substr(0, pos)
            << termcolor::reset;
  std::cout << termcolor::red << termcolor::bold
            << str.substr(pos, query.size()) << termcolor::reset;
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
                 bool print_only_matching_parts)
{
  // Start from the beginning
  const auto haystack_begin = haystack.cbegin();
  const auto haystack_end = haystack.cend();
  auto it = haystack_begin;
  bool first_search = true;
  std::size_t count = 0;

  while (it != haystack_end) {
    // Search for needle
    it = needle_search(needle, it, haystack_end, ignore_case);

    if (it != haystack_end && !print_only_file_without_matches) {
      count += 1;

      if (enforce_max_count && count > max_count) {
        break;
      }

      // Avoid printing lines from binary files with matches
      if (!print_count && is_binary_file(haystack)) {
        std::cout << termcolor::white << termcolor::bold << "Binary file "
                  << termcolor::cyan << filename << termcolor::white
                  << " matches\n"
                  << termcolor::reset;
        return;
      }

      // -l option
      // Print only filenames of files that contain matches.
      if (print_only_file_matches) {
        std::cout << termcolor::cyan << termcolor::bold << filename << "\n"
                  << termcolor::reset;
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
        std::cout << termcolor::cyan << termcolor::bold << filename << ":"
                  << termcolor::magenta << line_number << termcolor::red << ":"
                  << termcolor::reset;
      } else {
        std::cout << termcolor::cyan << termcolor::bold << filename << ":"
                  << termcolor::reset;
      }

      if (print_only_matching_parts) {
        std::cout << termcolor::red << termcolor::bold
                  << haystack.substr(std::size_t(it - haystack_begin),
                                     needle.size())
                  << termcolor::reset << "\n";
      } else {
        // Get line from newline_before and newline_after
        auto line = haystack.substr(
            newline_before + 1,
            std::size_t(newline_after - (haystack_begin + newline_before) - 1));
        print_colored(line, needle, ignore_case);
        std::cout << "\n";
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
          std::cout << termcolor::cyan << termcolor::bold << filename << "\n"
                    << termcolor::reset;
        }
      }
      break;
    }
  }
  // Done looking through file
  // Print count
  if (print_count) {
    std::cout << termcolor::cyan << termcolor::bold << filename << ":"
              << termcolor::magenta << count << termcolor::reset << "\n";
  }
}

bool filename_has_pattern(std::string_view str, std::string_view pattern)
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

auto include_file(std::string_view filename,
                  const std::vector<std::string>& patterns)
{
  bool result = false;

  bool all_extensions = patterns.size() == 0;
  if (!all_extensions) {
    for (const auto& pattern : patterns) {
      result |= filename_has_pattern(filename, pattern);
    }
  } else {
    result = true;
  }

  return result;
}

// Return true if the filename should be excluded
auto exclude_file(std::string_view filename,
                  const std::vector<std::string>& patterns)
{
  bool result = false;

  bool include_all_extensions = patterns.size() == 0;
  if (!include_all_extensions) {
    for (const auto& pattern : patterns) {
      if (filename_has_pattern(filename, pattern)) {
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
                          bool print_only_matching_parts)
{
  try {
    const auto path_string = path.string();
    const auto absolute_path = fs::absolute(path);
    const auto file_size = fs::file_size(absolute_path);
    if (file_size < needle.size()) {
      return;
    }

    std::string filename = absolute_path.string();

    // Check if file extension is in `include_extension` list
    // Check if file extension is NOT in `exclude_extension` list
    if (include_file(filename, include_extension)
        && !exclude_file(filename, exclude_extension))
    {
      std::ifstream is(filename);
      auto haystack = std::string(std::istreambuf_iterator<char>(is),
                                  std::istreambuf_iterator<char>());
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
                  print_only_matching_parts);
    }
  } catch (const std::exception& e) {
    std::cout << termcolor::red << termcolor::bold << "Error: " << e.what()
              << "\n"
              << termcolor::reset;
  }
}

void directory_search(fs::path const& path,
                      std::string_view query,
                      const std::vector<std::string>& include_extension,
                      const std::vector<std::string>& exclude_extension,
                      bool ignore_case,
                      bool print_count,
                      bool enforce_max_count,
                      std::size_t max_count,
                      bool print_line_numbers,
                      bool print_only_file_matches,
                      bool print_only_file_without_matches,
                      bool print_only_matching_parts)
{
  for (auto const& dir_entry : fs::directory_iterator(
           path, fs::directory_options::skip_permission_denied))
  {
    try {
      if (fs::is_regular_file(dir_entry)) {
        read_file_and_search(dir_entry.path().string(),
                             query,
                             include_extension,
                             exclude_extension,
                             ignore_case,
                             print_count,
                             enforce_max_count,
                             max_count,
                             print_line_numbers,
                             print_only_file_matches,
                             print_only_file_without_matches,
                             print_only_matching_parts);
      }
    } catch (std::exception& e) {
      continue;
    }
  }
}

void recursive_directory_search(
    fs::path const& path,
    std::string_view query,
    const std::vector<std::string>& include_extension,
    const std::vector<std::string>& exclude_extension,
    bool ignore_case,
    bool print_count,
    bool enforce_max_count,
    std::size_t max_count,
    bool print_line_numbers,
    bool print_only_file_matches,
    bool print_only_file_without_matches,
    bool print_only_matching_parts)
{
  for (auto const& dir_entry : fs::recursive_directory_iterator(
           path, fs::directory_options::skip_permission_denied))
  {
    try {
      if (fs::is_regular_file(dir_entry)) {
        read_file_and_search(dir_entry.path().string(),
                             query,
                             include_extension,
                             exclude_extension,
                             ignore_case,
                             print_count,
                             enforce_max_count,
                             max_count,
                             print_line_numbers,
                             print_only_file_matches,
                             print_only_file_without_matches,
                             print_only_matching_parts);
      }
    } catch (std::exception& e) {
      continue;
    }
  }
}

}  // namespace search
