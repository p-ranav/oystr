#pragma once
#include <cstddef>
#include <string_view>

namespace search
{
size_t avx512f_strstr(const std::string_view& s,
                      const std::string_view& needle);

}
