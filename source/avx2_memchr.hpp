#pragma once
#include <string_view>

#if defined(__AVX2__)

namespace search
{
const char* find_avx2_more(const char* b, const char* e, char c);

std::string_view::const_iterator needle_search_avx2(
    std::string_view needle,
    std::string_view::const_iterator haystack_begin,
    std::string_view::const_iterator haystack_end);
}  // namespace search

#endif
