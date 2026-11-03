#pragma once

#include <cstdint>

#if defined(__AVX2__) || defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

#ifdef __AVX2__

using vec_t     = __m256i;
using halfvec_t = __m128i;

#define vec_loadu  _mm256_loadu_si256

#define vec_storeu _mm256_storeu_si256

#define vec_add_epi32 _mm256_add_epi32
#define vec_add_epi16 _mm256_add_epi16
#define vec_sub_epi16 _mm256_sub_epi16

#define vec_madd_epi16 _mm256_madd_epi16
#define vec_mullo_epi16 _mm256_mullo_epi16

#define vec_max_epi16 _mm256_max_epi16
#define vec_min_epi16 _mm256_min_epi16

#define vec_setzero _mm256_setzero_ps
#define vet_set1_epi16 _mm256_set1_epi16

inline vec_t vec_loadu2(const halfvec_t* hi, const halfvec_t* lo) {
    return _mm256_loadu2_m128i(hi, lo);
}

// https://stackoverflow.com/a/35270026
inline int hsum(__m256i v) {
    __m128i hi128 = _mm256_extracti128_si256(v, 1);
    __m128i lo128 = _mm256_castsi256_si128(v);
    __m128i sum128 = _mm_add_epi32(hi128, lo128);
    __m128i sum64 = _mm_add_epi32(sum128, _mm_unpackhi_epi64(sum128, sum128));
    __m128i sum32 = _mm_add_epi32(sum64, _mm_shufflelo_epi16(sum64, _MM_SHUFFLE(1, 0, 3, 2)));

    return _mm_cvtsi128_si32(sum32);
}

#elif defined(__aarch64__) || defined(__ARM_NEON)

// ARM NEON provides the same 128-bit vector width used by the SSE path.
// Keep the engine's existing vec_t interface so NNUE/search code is unchanged.
using vec_t     = int16x8_t;
using halfvec_t = int64_t;

inline vec_t vec_loadu(const vec_t* p) {
    return vld1q_s16(reinterpret_cast<const int16_t*>(p));
}

inline void vec_storeu(vec_t* p, vec_t value) {
    vst1q_s16(reinterpret_cast<int16_t*>(p), value);
}

inline vec_t vec_add_epi32(vec_t a, vec_t b) {
    return vreinterpretq_s16_s32(vaddq_s32(vreinterpretq_s32_s16(a),
                                           vreinterpretq_s32_s16(b)));
}

#define vec_add_epi16(a, b) vaddq_s16((a), (b))
#define vec_sub_epi16(a, b) vsubq_s16((a), (b))

inline vec_t vec_madd_epi16(vec_t a, vec_t b) {
    const int32x4_t lo = vmull_s16(vget_low_s16(a), vget_low_s16(b));
    const int32x4_t hi = vmull_s16(vget_high_s16(a), vget_high_s16(b));
    const int32x2_t lo_pairs = vpadd_s32(vget_low_s32(lo), vget_high_s32(lo));
    const int32x2_t hi_pairs = vpadd_s32(vget_low_s32(hi), vget_high_s32(hi));
    return vreinterpretq_s16_s32(vcombine_s32(lo_pairs, hi_pairs));
}

#define vec_mullo_epi16(a, b) vmulq_s16((a), (b))
#define vec_max_epi16(a, b) vmaxq_s16((a), (b))
#define vec_min_epi16(a, b) vminq_s16((a), (b))
#define vec_setzero() vdupq_n_s16(0)
#define vet_set1_epi16(value) vdupq_n_s16((value))

inline vec_t vec_loadu2(const halfvec_t* hi, const halfvec_t* lo) {
    // Match _mm_loadu2_m128i/_mm_loadu2 semantics: lo is the lower half.
    return vreinterpretq_s16_s64(vcombine_s64(vld1_s64(lo), vld1_s64(hi)));
}

inline int hsum(vec_t v) {
    const int32x4_t values = vreinterpretq_s32_s16(v);
#if defined(__aarch64__)
    return vaddvq_s32(values);
#else
    const int32x2_t pairs = vpadd_s32(vget_low_s32(values), vget_high_s32(values));
    return vget_lane_s32(vpadd_s32(pairs, pairs), 0);
#endif
}

#else 

using vec_t     = __m128i;
using halfvec_t = int64_t;

#define vec_loadu _mm_loadu_si128

#define vec_storeu _mm_storeu_si128

#define vec_add_epi32 _mm_add_epi32
#define vec_add_epi16 _mm_add_epi16
#define vec_sub_epi16 _mm_sub_epi16

#define vec_madd_epi16 _mm_madd_epi16
#define vec_mullo_epi16 _mm_mullo_epi16

#define vec_max_epi16 _mm_max_epi16
#define vec_min_epi16 _mm_min_epi16

#define vec_setzero _mm_setzero_ps
#define vet_set1_epi16 _mm_set1_epi16

inline vec_t vec_loadu2(const halfvec_t* hi, const halfvec_t* lo) {
    vec_t vec_lo = _mm_loadl_epi64((vec_t*) lo);
    vec_t vec_hi = _mm_loadl_epi64((vec_t*) hi);

    return _mm_unpacklo_epi64(vec_lo, vec_hi);
}

// https://stackoverflow.com/a  /35270026
inline int hsum(vec_t v) {
    __m128i sum64 = _mm_add_epi32(v, _mm_unpackhi_epi64(v, v));
    __m128i sum32 = _mm_add_epi32(sum64, _mm_shufflelo_epi16(sum64, _MM_SHUFFLE(1, 0, 3, 2)));

    return _mm_cvtsi128_si32(sum32);
}

#endif