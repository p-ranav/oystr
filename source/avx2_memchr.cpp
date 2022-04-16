#include <cstdio>
#include <string_view>

#include <avx2_memchr.hpp>
#include <ctype.h>
#include <immintrin.h>

#if defined(__AVX2__)
// https://gms.tf/stdfind-and-memchr-optimizations.html
//

namespace search
{
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
      for (; i < e; ++i) {
        if (*i == c) {
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

std::string_view::const_iterator needle_search_avx2(
    std::string_view needle,
    std::string_view::const_iterator haystack_begin,
    std::string_view::const_iterator haystack_end)
{
  if (needle.empty()) {
    return haystack_end;
  }
  const char c = needle[0];

  auto it = haystack_begin;
  while (it < haystack_end) {
    const char* ptr = find_avx2_more(it, haystack_end, c);

    if (!ptr) {
      break;
    }

    bool result = true;

    const char* i = ptr;
    for (auto& n : needle) {
      if (i == haystack_end || n != *i) {
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

}  // namespace search

#endif
