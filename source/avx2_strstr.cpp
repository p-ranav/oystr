#include <cassert>
#include <cstring>

#include <ctype.h>
#include <immintrin.h>
#include <sse2_strstr.hpp>

#define FORCE_INLINE inline __attribute__((always_inline))

namespace search
{
namespace
{
bool always_true(const char*, const char*)
{
  return true;
}

bool memcmp1(const char* a, const char* b)
{
  return a[0] == b[0];
}

bool memcmp2(const char* a, const char* b)
{
  const uint16_t A = *reinterpret_cast<const uint16_t*>(a);
  const uint16_t B = *reinterpret_cast<const uint16_t*>(b);
  return A == B;
}

bool memcmp3(const char* a, const char* b)
{
  const uint32_t A = *reinterpret_cast<const uint32_t*>(a);
  const uint32_t B = *reinterpret_cast<const uint32_t*>(b);
  return (A & 0x00ffffff) == (B & 0x00ffffff);
}

bool memcmp4(const char* a, const char* b)
{
  const uint32_t A = *reinterpret_cast<const uint32_t*>(a);
  const uint32_t B = *reinterpret_cast<const uint32_t*>(b);
  return A == B;
}

bool memcmp5(const char* a, const char* b)
{
  const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
  const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
  return ((A ^ B) & 0x000000fffffffffflu) == 0;
}

bool memcmp6(const char* a, const char* b)
{
  const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
  const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
  return ((A ^ B) & 0x0000fffffffffffflu) == 0;
}

bool memcmp7(const char* a, const char* b)
{
  const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
  const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
  return ((A ^ B) & 0x00fffffffffffffflu) == 0;
}

bool memcmp8(const char* a, const char* b)
{
  const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
  const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
  return A == B;
}

bool memcmp9(const char* a, const char* b)
{
  const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
  const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
  return (A == B) & (a[8] == b[8]);
}

bool memcmp10(const char* a, const char* b)
{
  const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
  const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
  const uint16_t Aw = *reinterpret_cast<const uint16_t*>(a + 8);
  const uint16_t Bw = *reinterpret_cast<const uint16_t*>(b + 8);
  return (Aq == Bq) & (Aw == Bw);
}

bool memcmp11(const char* a, const char* b)
{
  const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
  const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
  const uint32_t Ad = *reinterpret_cast<const uint32_t*>(a + 8);
  const uint32_t Bd = *reinterpret_cast<const uint32_t*>(b + 8);
  return (Aq == Bq) & ((Ad & 0x00ffffff) == (Bd & 0x00ffffff));
}

bool memcmp12(const char* a, const char* b)
{
  const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
  const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
  const uint32_t Ad = *reinterpret_cast<const uint32_t*>(a + 8);
  const uint32_t Bd = *reinterpret_cast<const uint32_t*>(b + 8);
  return (Aq == Bq) & (Ad == Bd);
}

}  // namespace

namespace detail
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

}  // namespace detail

size_t FORCE_INLINE avx2_strstr_anysize(const char* s,
                                        size_t n,
                                        const char* needle,
                                        size_t k)
{
  assert(k > 0);
  assert(n > 0);

  const __m256i first = _mm256_set1_epi8(needle[0]);
  const __m256i last = _mm256_set1_epi8(needle[k - 1]);

  for (size_t i = 0; i < n; i += 32) {
    const __m256i block_first =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + i));
    const __m256i block_last =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + i + k - 1));

    const __m256i eq_first = _mm256_cmpeq_epi8(first, block_first);
    const __m256i eq_last = _mm256_cmpeq_epi8(last, block_last);

    uint32_t mask = _mm256_movemask_epi8(_mm256_and_si256(eq_first, eq_last));

    while (mask != 0) {
      const auto bitpos = detail::get_first_bit_set(mask);

      if (memcmp(s + i + bitpos + 1, needle + 1, k - 2) == 0) {
        return i + bitpos;
      }

      mask = detail::clear_leftmost_set(mask);
    }
  }

  return std::string_view::npos;
}

/*
    The following templates implement the loop, where K is a template parameter.
        for (unsigned i=1; i < K; i++) {
            const __m256i substring = _mm256_alignr_epi8(next1, curr, i);
            eq = _mm256_and_si256(eq, _mm256_cmpeq_epi8(substring,
   broadcasted[i]));
        }
    Clang complains that the loop parameter `i` is a variable and it cannot be
    applied as a parameter _mm256_alignr_epi8.  GCC somehow deals with it.
*/

#ifdef __clang__

template<size_t K, int i, bool terminate>
struct inner_loop_aux;

template<size_t K, int i>
struct inner_loop_aux<K, i, false>
{
  void operator()(__m256i& eq,
                  const __m256i& next1,
                  const __m256i& curr,
                  const __m256i (&broadcasted)[K])
  {
    const __m256i substring = _mm256_alignr_epi8(next1, curr, i);
    eq = _mm256_and_si256(eq, _mm256_cmpeq_epi8(substring, broadcasted[i]));
    inner_loop_aux<K, i + 1, i + 1 == K>()(eq, next1, curr, broadcasted);
  }
};

template<size_t K, int i>
struct inner_loop_aux<K, i, true>
{
  void operator()(__m256i&,
                  const __m256i&,
                  const __m256i&,
                  const __m256i (&)[K])
  {
    // nop
  }
};

template<size_t K>
struct inner_loop
{
  void operator()(__m256i& eq,
                  const __m256i& next1,
                  const __m256i& curr,
                  const __m256i (&broadcasted)[K])
  {
    static_assert(K > 0, "wrong value");
    inner_loop_aux<K, 0, false>()(eq, next1, curr, broadcasted);
  }
};

#endif

template<size_t K>
size_t FORCE_INLINE avx2_strstr_eq(const char* s, size_t n, const char* needle)
{
  static_assert(K > 0 && K < 16, "K must be in range [1..15]");
  assert(n > 0);

  __m256i broadcasted[K];
  for (unsigned i = 0; i < K; i++) {
    broadcasted[i] = _mm256_set1_epi8(needle[i]);
  }

  __m256i curr = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s));

  for (size_t i = 0; i < n; i += 32) {
    const __m256i next =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + i + 32));

    __m256i eq = _mm256_cmpeq_epi8(curr, broadcasted[0]);

    // AVX2 palignr works on 128-bit lanes, thus some extra work is needed
    //
    // curr = [a, b] (2 x 128 bit)
    // next = [c, d]
    // substring = [palignr(b, a, i), palignr(c, b, i)]
    __m256i next1;
    next1 = _mm256_inserti128_si256(
        next1, _mm256_extracti128_si256(curr, 1), 0);  // b
    next1 = _mm256_inserti128_si256(
        next1, _mm256_extracti128_si256(next, 0), 1);  // c

#ifndef __clang__
    for (unsigned i = 1; i < K; i++) {
      const __m256i substring = _mm256_alignr_epi8(next1, curr, i);
      eq = _mm256_and_si256(eq, _mm256_cmpeq_epi8(substring, broadcasted[i]));
    }
#else
    inner_loop<K>()(eq, next1, curr, broadcasted);
#endif

    curr = next;

    const uint32_t mask = _mm256_movemask_epi8(eq);
    if (mask != 0) {
      return i + detail::get_first_bit_set(mask);
    }
  }

  return std::string_view::npos;
}

template<size_t k, typename MEMCMP>
size_t FORCE_INLINE avx2_strstr_memcmp(const char* s,
                                       size_t n,
                                       const char* needle,
                                       MEMCMP memcmp_fun)
{
  assert(k > 0);
  assert(n > 0);

  const __m256i first = _mm256_set1_epi8(needle[0]);
  const __m256i last = _mm256_set1_epi8(needle[k - 1]);

  for (size_t i = 0; i < n; i += 32) {
    const __m256i block_first =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + i));
    const __m256i block_last =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + i + k - 1));

    const __m256i eq_first = _mm256_cmpeq_epi8(first, block_first);
    const __m256i eq_last = _mm256_cmpeq_epi8(last, block_last);

    uint32_t mask = _mm256_movemask_epi8(_mm256_and_si256(eq_first, eq_last));

    while (mask != 0) {
      const auto bitpos = detail::get_first_bit_set(mask);

      if (memcmp_fun(s + i + bitpos + 1, needle + 1)) {
        return i + bitpos;
      }

      mask = detail::clear_leftmost_set(mask);
    }
  }

  return std::string_view::npos;
}

// ------------------------------------------------------------------------

size_t avx2_strstr_v2(const char* s, size_t n, const char* needle, size_t k)
{
  size_t result = std::string_view::npos;

  if (n < k) {
    return result;
  }

  switch (k) {
    case 0:
      return 0;

    case 1: {
      const char* res = reinterpret_cast<const char*>(strchr(s, needle[0]));

      return (res != nullptr) ? res - s : std::string_view::npos;
    }

    case 2:
      result = avx2_strstr_eq<2>(s, n, needle);
      break;

    case 3:
      result = avx2_strstr_memcmp<3>(s, n, needle, memcmp1);
      break;

    case 4:
      result = avx2_strstr_memcmp<4>(s, n, needle, memcmp2);
      break;

    case 5:
      // Note: use memcmp4 rather memcmp3, as the last character
      //       of needle is already proven to be equal
      result = avx2_strstr_memcmp<5>(s, n, needle, memcmp4);
      break;

    case 6:
      result = avx2_strstr_memcmp<6>(s, n, needle, memcmp4);
      break;

    case 7:
      result = avx2_strstr_memcmp<7>(s, n, needle, memcmp5);
      break;

    case 8:
      result = avx2_strstr_memcmp<8>(s, n, needle, memcmp6);
      break;

    case 9:
      // Note: use memcmp8 rather memcmp7 for the same reason as above.
      result = avx2_strstr_memcmp<9>(s, n, needle, memcmp8);
      break;

    case 10:
      result = avx2_strstr_memcmp<10>(s, n, needle, memcmp8);
      break;

    case 11:
      result = avx2_strstr_memcmp<11>(s, n, needle, memcmp9);
      break;

    case 12:
      result = avx2_strstr_memcmp<12>(s, n, needle, memcmp10);
      break;

    default:
      result = avx2_strstr_anysize(s, n, needle, k);
      break;
  }

  if (result <= n - k) {
    return result;
  } else {
    return std::string_view::npos;
  }
}

// ------------------------------------------------------------------------

size_t avx2_strstr_v2(const std::string_view& s, const std::string_view& needle)
{
  return avx2_strstr_v2(s.data(), s.size(), needle.data(), needle.size());
}

}  // namespace search
