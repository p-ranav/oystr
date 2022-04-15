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
#include <unordered_set>
#include <vector>

#include <avx2_memchr.hpp>
#include <avx512f_strstr.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <immintrin.h>
#include <mio.hpp>

namespace search
{
std::string_view::const_iterator needle_search(
    std::string_view needle,
    std::string_view::const_iterator haystack_begin,
    std::string_view::const_iterator haystack_end);

void directory_search(const char* path,
                      std::string_view query,
                      const std::vector<std::string>& include_extension,
                      const std::vector<std::string>& exclude_extension,
                      bool print_count,
                      bool enforce_max_count,
                      std::size_t max_count,
                      bool print_only_file_matches,
                      bool print_only_file_without_matches);
}  // namespace search
