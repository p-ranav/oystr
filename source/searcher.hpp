#pragma once
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
#include <thread>
#include <unordered_set>
#include <vector>

#include <avx2_memchr.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <immintrin.h>
#include <sse2_strstr.hpp>
#include <thread_pool.hpp>

namespace search
{
struct searcher
{
  static inline std::unique_ptr<thread_pool> m_ts;
  static inline std::string_view m_query;
  static inline std::string_view m_filter;
  static inline std::vector<std::string> m_include_extension;
  static inline std::vector<std::string> m_exclude_extension;
  static inline bool m_print_count;
  static inline bool m_enforce_max_count;
  static inline std::size_t m_max_count;
  static inline bool m_print_only_file_matches;
  static inline bool m_print_only_file_without_matches;

  static void read_file_and_search(const char* path);
  static bool include_file(const std::string_view& str);
  static bool exclude_file(const std::string_view& str);
  static bool exclude_file_known_suffixes(const std::string_view& str);
  static bool exclude_directory(const char* path);
  static void directory_search(const char* path);
};

}  // namespace search
