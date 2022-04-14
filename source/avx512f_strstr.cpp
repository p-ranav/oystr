#include <cassert>
#include <cstring>

#include <avx512f_strstr.hpp>
#include <ctype.h>
#include <immintrin.h>

#if __AVX512F__

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

size_t avx512f_strstr_anysize(const char* string,
                              size_t n,
                              const char* needle,
                              size_t k)
{
  assert(n > 0);
  assert(k > 0);

  const __m512i first = _mm512_set1_epi8(needle[0]);
  const __m512i last = _mm512_set1_epi8(needle[k - 1]);

  char* haystack = const_cast<char*>(string);
  char* end = haystack + n;

  for (/**/; haystack < end; haystack += 64) {
    const __m512i block_first = _mm512_loadu_si512(haystack + 0);
    const __m512i block_last = _mm512_loadu_si512(haystack + k - 1);

    const __m512i first_zeros = _mm512_xor_si512(block_first, first);
    /*
        first_zeros | block_last | last |  first_zeros | (block_last ^ last)
        ------------+------------+------+------------------------------------
             0      |      0     |   0  |      0
             0      |      0     |   1  |      1
             0      |      1     |   0  |      1
             0      |      1     |   1  |      0
             1      |      0     |   0  |      1
             1      |      0     |   1  |      1
             1      |      1     |   0  |      1
             1      |      1     |   1  |      1
    */
    const __m512i zeros =
        _mm512_ternarylogic_epi32(first_zeros, block_last, last, 0xf6);

    uint32_t mask = zero_byte_mask(zeros);
    while (mask) {
      const uint64_t p = __builtin_ctz(mask);

      if (memcmp(haystack + 4 * p + 0, needle, k) == 0) {
        return (haystack - string) + 4 * p + 0;
      }

      if (memcmp(haystack + 4 * p + 1, needle, k) == 0) {
        return (haystack - string) + 4 * p + 1;
      }

      if (memcmp(haystack + 4 * p + 2, needle, k) == 0) {
        return (haystack - string) + 4 * p + 2;
      }

      if (memcmp(haystack + 4 * p + 3, needle, k) == 0) {
        return (haystack - string) + 4 * p + 3;
      }

      mask = bits::clear_leftmost_set(mask);
    }
  }

  return size_t(-1);
}

template<size_t k, typename MEMCMP>
size_t avx512f_strstr_memcmp(const char* string,
                             size_t n,
                             const char* needle,
                             MEMCMP memeq_fun)
{
  assert(n > 0);
  assert(k > 0);

  const __m512i first = _mm512_set1_epi8(needle[0]);
  const __m512i last = _mm512_set1_epi8(needle[k - 1]);

  char* haystack = const_cast<char*>(string);
  char* end = haystack + n;

  for (/**/; haystack < end; haystack += 64) {
    const __m512i block_first = _mm512_loadu_si512(haystack + 0);
    const __m512i block_last = _mm512_loadu_si512(haystack + k - 1);

    const __m512i first_zeros = _mm512_xor_si512(block_first, first);
    const __m512i zeros =
        _mm512_ternarylogic_epi32(first_zeros, block_last, last, 0xf6);

    uint32_t mask = zero_byte_mask(zeros);
    while (mask) {
      const uint64_t p = __builtin_ctz(mask);

      if (memeq_fun(haystack + 4 * p + 0, needle)) {
        return (haystack - string) + 4 * p + 0;
      }

      if (memeq_fun(haystack + 4 * p + 1, needle)) {
        return (haystack - string) + 4 * p + 1;
      }

      if (memeq_fun(haystack + 4 * p + 2, needle)) {
        return (haystack - string) + 4 * p + 2;
      }

      if (memeq_fun(haystack + 4 * p + 3, needle)) {
        return (haystack - string) + 4 * p + 3;
      }

      mask = bits::clear_leftmost_set(mask);
    }
  }

  return size_t(-1);
}

// ------------------------------------------------------------------------

size_t avx512f_strstr(const char* s, size_t n, const char* needle, size_t k)
{
  size_t result = std::string_view::npos;

  if (n < k) {
    return result;
  }

  result = avx512f_strstr_anysize(s, n, needle, k);

  switch (k) {
    case 0:
      return 0;

    case 1: {
      const char* res = reinterpret_cast<const char*>(strchr(s, needle[0]));

      return res != nullptr ? res - s : std::string_view::npos;
    }

    case 2:
      result = avx512f_strstr_memcmp<2>(s, n, needle, memcmp2);
      break;

    case 3:
      result = avx512f_strstr_memcmp<3>(s, n, needle, memcmp3);
      break;

    case 4:
      result = avx512f_strstr_memcmp<4>(s, n, needle, memcmp4);
      break;

    case 5:
      result = avx512f_strstr_memcmp<5>(s, n, needle, memcmp5);
      break;

    case 6:
      result = avx512f_strstr_memcmp<6>(s, n, needle, memcmp6);
      break;

    case 7:
      result = avx512f_strstr_memcmp<7>(s, n, needle, memcmp7);
      break;

    case 8:
      result = avx512f_strstr_memcmp<8>(s, n, needle, memcmp8);
      break;

    case 9:
      result = avx512f_strstr_memcmp<9>(s, n, needle, memcmp9);
      break;

    case 10:
      result = avx512f_strstr_memcmp<10>(s, n, needle, memcmp10);
      break;

    case 11:
      result = avx512f_strstr_memcmp<11>(s, n, needle, memcmp11);
      break;

    case 12:
      result = avx512f_strstr_memcmp<12>(s, n, needle, memcmp12);
      break;

    default:
      result = avx512f_strstr_anysize(s, n, needle, k);
      break;
  }

  if (result <= n - k) {
    return result;
  } else {
    return std::string_view::npos;
  }
}

// --------------------------------------------------

size_t avx512f_strstr(const std::string_view& s, const std::string_view& needle)
{
  return avx512f_strstr(s.data(), s.size(), needle.data(), needle.size());
}

}  // namespace search
#endif
