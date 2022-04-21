#pragma once
#include <cstddef>
#include <string_view>

#if defined(__AVX2__)

namespace search
{
size_t avx2_strstr_v2(const std::string_view& s,
                      const std::string_view& needle);

}

#endif
