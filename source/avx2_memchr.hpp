#pragma once
#include <string_view>

namespace search
{
const char* find_avx2_more(const char* b,
                           const char* e,
                           char c,
                           bool ignore_case);

std::string_view::const_iterator needle_search_avx2(
    std::string_view needle,
    std::string_view::const_iterator haystack_begin,
    std::string_view::const_iterator haystack_end,
    bool ignore_case);
}  // namespace search
