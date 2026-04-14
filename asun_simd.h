#pragma once
#include <cstdint>
#include <cstddef>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define ASUN_SSE2 1
#if defined(__AVX2__)
#define ASUN_AVX2 1
#endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#define ASUN_NEON 1
#endif

namespace asun_simd {

static constexpr int LANES = 16;

#if defined(ASUN_NEON)
inline uint16_t movemask(uint8x16_t v) {
    uint16x8_t high_bits = vreinterpretq_u16_u8(vshrq_n_u8(v, 7));
    uint32x4_t paired16  = vreinterpretq_u32_u16(vsraq_n_u16(high_bits, high_bits, 7));
    uint64x2_t paired32  = vreinterpretq_u64_u32(vsraq_n_u32(paired16, paired16, 14));
    uint8x16_t paired64  = vreinterpretq_u8_u64(vsraq_n_u64(paired32, paired32, 28));
    uint16_t lo = vgetq_lane_u8(paired64, 0);
    uint16_t hi = vgetq_lane_u8(paired64, 8);
    return lo | (hi << 8);
}
inline size_t find_quote_or_special(const uint8_t* ptr, size_t len) {
    size_t i = 0;
    uint8x16_t vq = vdupq_n_u8('"'), vb = vdupq_n_u8('\\'), vc = vdupq_n_u8(0x1F);
    for (; i + LANES <= len; i += LANES) {
        uint8x16_t ch = vld1q_u8(ptr + i);
        uint8x16_t r = vorrq_u8(vorrq_u8(vceqq_u8(ch, vq), vceqq_u8(ch, vb)), vcleq_u8(ch, vc));
        uint16_t m = movemask(r);
        if (m) return i + __builtin_ctz(m);
    }
    for (; i < len; i++) { uint8_t b = ptr[i]; if (b == '"' || b == '\\' || b < 0x20) return i; }
    return len;
}
inline bool has_special_chars(const uint8_t* ptr, size_t len) {
    size_t i = 0;
    uint8x16_t vc = vdupq_n_u8(0x1F), v1 = vdupq_n_u8(','), va = vdupq_n_u8('@'), v2 = vdupq_n_u8('('), v3 = vdupq_n_u8(')');
    uint8x16_t v4 = vdupq_n_u8('['), v5 = vdupq_n_u8(']'), v6 = vdupq_n_u8('"'), v7 = vdupq_n_u8('\\');
    for (; i + LANES <= len; i += LANES) {
        uint8x16_t ch = vld1q_u8(ptr + i);
        uint8x16_t r = vcleq_u8(ch, vc);
        r = vorrq_u8(r, vceqq_u8(ch, v1)); r = vorrq_u8(r, vceqq_u8(ch, va)); r = vorrq_u8(r, vceqq_u8(ch, v2));
        r = vorrq_u8(r, vceqq_u8(ch, v3)); r = vorrq_u8(r, vceqq_u8(ch, v4));
        r = vorrq_u8(r, vceqq_u8(ch, v5)); r = vorrq_u8(r, vceqq_u8(ch, v6));
        r = vorrq_u8(r, vceqq_u8(ch, v7));
        if (movemask(r)) return true;
    }
    for (; i < len; i++) {
        uint8_t b = ptr[i];
        if (b < 0x20 || b == ',' || b == '@' || b == '(' || b == ')' || b == '[' || b == ']' || b == '"' || b == '\\') return true;
    }
    return false;
}
#elif defined(ASUN_SSE2)
inline size_t find_quote_or_special(const uint8_t* ptr, size_t len) {
    size_t i = 0;
    __m128i vq = _mm_set1_epi8('"'), vb = _mm_set1_epi8('\\'), vc = _mm_set1_epi8(0x1F);
    for (; i + LANES <= len; i += LANES) {
        __m128i ch = _mm_loadu_si128((const __m128i*)(ptr + i));
        __m128i eq_q = _mm_cmpeq_epi8(ch, vq), eq_b = _mm_cmpeq_epi8(ch, vb);
        __m128i le_c = _mm_cmpeq_epi8(_mm_max_epu8(ch, vc), vc);
        int m = _mm_movemask_epi8(_mm_or_si128(_mm_or_si128(eq_q, eq_b), le_c));
        if (m) return i + __builtin_ctz(m);
    }
    for (; i < len; i++) { uint8_t b = ptr[i]; if (b == '"' || b == '\\' || b < 0x20) return i; }
    return len;
}
inline bool has_special_chars(const uint8_t* ptr, size_t len) {
    size_t i = 0;
    __m128i vc = _mm_set1_epi8(0x1F), v1 = _mm_set1_epi8(','), va = _mm_set1_epi8('@'), v2 = _mm_set1_epi8('('), v3 = _mm_set1_epi8(')');
    __m128i v4 = _mm_set1_epi8('['), v5 = _mm_set1_epi8(']'), v6 = _mm_set1_epi8('"'), v7 = _mm_set1_epi8('\\');
    for (; i + LANES <= len; i += LANES) {
        __m128i ch = _mm_loadu_si128((const __m128i*)(ptr + i));
        __m128i r = _mm_cmpeq_epi8(_mm_max_epu8(ch, vc), vc);
        r = _mm_or_si128(r, _mm_cmpeq_epi8(ch, v1)); r = _mm_or_si128(r, _mm_cmpeq_epi8(ch, va)); r = _mm_or_si128(r, _mm_cmpeq_epi8(ch, v2));
        r = _mm_or_si128(r, _mm_cmpeq_epi8(ch, v3)); r = _mm_or_si128(r, _mm_cmpeq_epi8(ch, v4));
        r = _mm_or_si128(r, _mm_cmpeq_epi8(ch, v5)); r = _mm_or_si128(r, _mm_cmpeq_epi8(ch, v6));
        r = _mm_or_si128(r, _mm_cmpeq_epi8(ch, v7));
        if (_mm_movemask_epi8(r)) return true;
    }
    for (; i < len; i++) {
        uint8_t b = ptr[i];
        if (b < 0x20 || b == ',' || b == '@' || b == '(' || b == ')' || b == '[' || b == ']' || b == '"' || b == '\\') return true;
    }
    return false;
}
#else
inline size_t find_quote_or_special(const uint8_t* ptr, size_t len) {
    for (size_t i = 0; i < len; i++) { uint8_t b = ptr[i]; if (b == '"' || b == '\\' || b < 0x20) return i; }
    return len;
}
inline bool has_special_chars(const uint8_t* ptr, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint8_t b = ptr[i];
        if (b < 0x20 || b == ',' || b == '@' || b == '(' || b == ')' || b == '[' || b == ']' || b == '"' || b == '\\') return true;
    }
    return false;
}
#endif

} // namespace asun_simd
