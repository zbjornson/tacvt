#include "nan.h"
#include "stdint.h"
#include <cmath>
#if defined(_MSC_VER)
  #include <intrin.h>
#elif defined(__GNUC__)
  #include <x86intrin.h>
#endif

// Name: from_to

NAN_METHOD(Float32Array_Float64Array) {
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();
	Nan::TypedArrayContents<double> dst(dstTA);
	Nan::TypedArrayContents<float> src(srcTA);

	for (size_t i = 0; i < dst.length(); i += 16) {
		auto srcvA = _mm_loadu_ps(&(*src)[i + 0]);
		auto srcvB = _mm_loadu_ps(&(*src)[i + 4]);
		auto srcvC = _mm_loadu_ps(&(*src)[i + 8]);
		auto srcvD = _mm_loadu_ps(&(*src)[i + 12]);

		auto dstvA = _mm256_cvtps_pd(srcvA);
		auto dstvB = _mm256_cvtps_pd(srcvB);
		auto dstvC = _mm256_cvtps_pd(srcvC);
		auto dstvD = _mm256_cvtps_pd(srcvD);

		_mm256_storeu_pd(&(*dst)[i + 0], dstvA);
		_mm256_storeu_pd(&(*dst)[i + 4], dstvB);
		_mm256_storeu_pd(&(*dst)[i + 8], dstvC);
		_mm256_storeu_pd(&(*dst)[i + 12], dstvD);
	}
}

NAN_METHOD(Float64Array_Float32Array) {
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();
	Nan::TypedArrayContents<float> dst(dstTA);
	Nan::TypedArrayContents<double> src(srcTA);

	for (size_t i = 0; i < dst.length(); i += 16) {
		auto srcvA = _mm256_loadu_pd(&(*src)[i + 0]);
		auto srcvB = _mm256_loadu_pd(&(*src)[i + 4]);
		auto srcvC = _mm256_loadu_pd(&(*src)[i + 8]);
		auto srcvD = _mm256_loadu_pd(&(*src)[i + 12]);

		auto dstvA = _mm256_cvtpd_ps(srcvA);
		auto dstvB = _mm256_cvtpd_ps(srcvB);
		auto dstvC = _mm256_cvtpd_ps(srcvC);
		auto dstvD = _mm256_cvtpd_ps(srcvD);

		_mm_storeu_ps(&(*dst)[i + 0], dstvA);
		_mm_storeu_ps(&(*dst)[i + 4], dstvB);
		_mm_storeu_ps(&(*dst)[i + 8], dstvC);
		_mm_storeu_ps(&(*dst)[i + 12], dstvD);
	}
}

NAN_METHOD(Float32Array_Int32Array) {
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();
	Nan::TypedArrayContents<int32_t> dst(dstTA);
	Nan::TypedArrayContents<float> src(srcTA);

	// TODO possibly unroll
	for (size_t i = 0; i < dst.length(); i += 8) {
		auto srcvA = _mm256_loadu_ps(&(*src)[i]);
		// Floor for the exact conversion test several lines down
		srcvA = _mm256_round_ps(srcvA, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
		// Fast path: no Infinity, -Infinity, NaN or magnitude >2**32
		// WARNING! There seems to be a bug in MSVC 2015 but not 2017 where this emits vcvtps2dq
		auto dstvA = _mm256_cvttps_epi32(srcvA); // could be cvtps since there has to be the round() above
		auto backA = _mm256_cvtepi32_ps(dstvA);
		auto exact = _mm256_cmpeq_epi32(_mm256_castps_si256(srcvA), _mm256_castps_si256(backA));
		// _mm256_cmpeq_epi32 has 3 cycles less latency than _mm256_cmp_ps, at the cost of bypass delay
		// Also, idk if JS has multiple NaNs or -0, in which case this would give false-positives

		_mm256_storeu_si256(reinterpret_cast<__m256i*>(&(*dst)[i]), dstvA);

		// auto allexact = !_mm256_testc_si256(exact, _mm256_set1_epi8(0xFF));
		auto exactMask = _mm256_movemask_ps(_mm256_castsi256_ps(exact));
		if (exactMask != 0xFF) { // Slow path: fixup.
			// It might be faster to port v8's conversions-inl.h:DoubleToInt32 to SIMD,
			// but once all of the conditional code paths are turned into predicated
			// operations, it might not be fast.
			for (size_t j = 0; j < 8; j++) {
				if (exactMask & (1 << j)) continue;
				auto value = (*src)[i + j];
				if (std::isnan(value)) {
					value = 0;
				} else if (std::isinf(value)) {
					value = 0;
				} else { // overflow -- maybe faster to use std::isfinite at the top of the if/else since this is probably most common
					value = fmod(value, 0xFFFFFFFF);
				}
				(*dst)[i + j] = value;
			}
		}
	}
}

// Narrowing
NAN_METHOD(Int32Array_Int16Array) {
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();
	Nan::TypedArrayContents<int64_t> dst(dstTA);
	Nan::TypedArrayContents<int32_t> src(srcTA);
	__m128i mask32_to_16 = _mm_setr_epi8(
		0, 1, 4, 5, 8, 9, 12, 13,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	);

	// Other possibilities:
	// vcompressw with AVX512
	// vpshufb into Ax|xB, vpermq into AB, store xmm
	// vpshufb into Ax|xB, store lower xmm, extract upper, store xmm

	for (size_t i = 0, j = 0; i < src.length(); i += 8, j += 2) {
		auto srcvA = _mm_lddqu_si128((__m128i*)&(*src)[i]); // load 128
		auto srcvB = _mm_lddqu_si128((__m128i*)&(*src)[i + 4]);
		auto dstvA = _mm_shuffle_epi8(srcvA, mask32_to_16); // compress
		auto dstvB = _mm_shuffle_epi8(srcvB, mask32_to_16);
		(*dst)[j] = _mm_cvtsi128_si64(dstvA); // store 64
		(*dst)[j + 1] = _mm_cvtsi128_si64(dstvB);
	}
}

NAN_METHOD(Int32Array_Int8Array) {
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();
	Nan::TypedArrayContents<int32_t> dst(dstTA);
	Nan::TypedArrayContents<int32_t> src(srcTA);
	__m128i mask32_to_8 = _mm_setr_epi8(
		0, 4, 8, 12, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	);

	for (size_t i = 0, j = 0; i < src.length(); i += 8, j += 2) {
		auto srcvA = _mm_lddqu_si128((__m128i*)&(*src)[i]); // load 128
		auto srcvB = _mm_lddqu_si128((__m128i*)&(*src)[i + 4]);
		auto dstvA = _mm_shuffle_epi8(srcvA, mask32_to_8); // compress
		auto dstvB = _mm_shuffle_epi8(srcvB, mask32_to_8);
		(*dst)[j] = _mm_cvtsi128_si32(dstvA); // store 64
		(*dst)[j + 1] = _mm_cvtsi128_si32(dstvB);
	}
}

NAN_METHOD(Int16Array_Int8Array) {
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();
	Nan::TypedArrayContents<int64_t> dst(dstTA);
	Nan::TypedArrayContents<int16_t> src(srcTA);
	__m128i mask16_to_8 = _mm_setr_epi8(
		0, 2, 4, 6, 8, 10, 12, 14,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	);

	for (size_t i = 0, j = 0; i < src.length(); i += 8, j += 1) {
		auto srcvA = _mm_lddqu_si128((__m128i*)&(*src)[i]); // load 128
		auto dstvA = _mm_shuffle_epi8(srcvA, mask16_to_8); // compress
		(*dst)[j] = _mm_cvtsi128_si64(dstvA); // store 64
	}
}

// Widening
#define IntToIntConvert(Name, dsttype, srctype, cvtop)							\
NAN_METHOD(Name) {																\
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();	\
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();	\
	Nan::TypedArrayContents<dsttype> dst(dstTA);								\
	Nan::TypedArrayContents<srctype> src(srcTA);								\
	size_t nel = 256 / (sizeof(dsttype) * 8);									\
	for (size_t i = 0; i < dst.length(); i += nel * 4) {						\
		auto srcvA = _mm_lddqu_si128((__m128i*)&(*src)[i]);						\
		auto srcvB = _mm_lddqu_si128((__m128i*)&(*src)[i + nel]);				\
		auto srcvC = _mm_lddqu_si128((__m128i*)&(*src)[i + 2 * nel]);			\
		auto srcvD = _mm_lddqu_si128((__m128i*)&(*src)[i + 3 * nel]);			\
																				\
		auto dstvA = cvtop(srcvA);												\
		auto dstvB = cvtop(srcvB);												\
		auto dstvC = cvtop(srcvC);												\
		auto dstvD = cvtop(srcvD);												\
																				\
		_mm256_storeu_si256((__m256i*)&(*dst)[i], dstvA);						\
		_mm256_storeu_si256((__m256i*)&(*dst)[i + nel], dstvB);					\
		_mm256_storeu_si256((__m256i*)&(*dst)[i + 2 * nel], dstvC);				\
		_mm256_storeu_si256((__m256i*)&(*dst)[i + 3 * nel], dstvD);				\
	}																			\
}

IntToIntConvert(Int16Array_Int32Array, int32_t, int16_t, _mm256_cvtepi16_epi32)
IntToIntConvert(Uint16Array_Uint32Array, uint32_t, uint16_t, _mm256_cvtepu16_epi32)
IntToIntConvert(Int8Array_Int32Array, int32_t, int8_t, _mm256_cvtepi8_epi32)
IntToIntConvert(Uint8Array_Uint32Array, uint32_t, uint8_t, _mm256_cvtepu8_epi32)
IntToIntConvert(Int8Array_Int16Array, int16_t, int8_t, _mm256_cvtepi8_epi16)
IntToIntConvert(Uint8Array_Uint16Array, uint16_t, uint8_t, _mm256_cvtepu8_epi16)
#undef IntToIntConvert

#define IntToFloatSameSizeConvert(Name, dsttype, srctype, cvtop)				\
NAN_METHOD(Name) {											\
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();	\
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();	\
	Nan::TypedArrayContents<dsttype> dst(dstTA);								\
	Nan::TypedArrayContents<srctype> src(srcTA);								\
																				\
	for (size_t i = 0; i < dst.length(); i += 8*4) {							\
		auto srcvA = _mm256_lddqu_si256((__m256i*)&(*src)[i + 0]);				\
		auto srcvB = _mm256_lddqu_si256((__m256i*)&(*src)[i + 8]);				\
		auto srcvC = _mm256_lddqu_si256((__m256i*)&(*src)[i + 16]);				\
		auto srcvD = _mm256_lddqu_si256((__m256i*)&(*src)[i + 24]);				\
																				\
		auto dstvA = cvtop(srcvA);												\
		auto dstvB = cvtop(srcvB);												\
		auto dstvC = cvtop(srcvC);												\
		auto dstvD = cvtop(srcvD);												\
																				\
		_mm256_storeu_ps(&(*dst)[i + 0], dstvA);								\
		_mm256_storeu_ps(&(*dst)[i + 8], dstvB);								\
		_mm256_storeu_ps(&(*dst)[i + 16], dstvC);								\
		_mm256_storeu_ps(&(*dst)[i + 24], dstvD);								\
	}																			\
}

IntToFloatSameSizeConvert(Int32Array_Float32Array, float, int32_t, _mm256_cvtepi32_ps)
#undef IntToFloatSameSizeConvert

#define IntToFloatConvert(Name, srctype, cvtop)									\
NAN_METHOD(Name) {																\
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();	\
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();	\
	Nan::TypedArrayContents<float> dst(dstTA);									\
	Nan::TypedArrayContents<srctype> src(srcTA);								\
	size_t nel = 256 / (sizeof(float) * 8);										\
	for (size_t i = 0; i < dst.length(); i += nel * 4) {						\
		auto srcvA = _mm_lddqu_si128((__m128i*)&(*src)[i]);						\
		auto srcvB = _mm_lddqu_si128((__m128i*)&(*src)[i + nel]);				\
		auto srcvC = _mm_lddqu_si128((__m128i*)&(*src)[i + 2 * nel]);			\
		auto srcvD = _mm_lddqu_si128((__m128i*)&(*src)[i + 3 * nel]);			\
																				\
		auto dstvA = _mm256_cvtepi32_ps(cvtop(srcvA));							\
		auto dstvB = _mm256_cvtepi32_ps(cvtop(srcvB));							\
		auto dstvC = _mm256_cvtepi32_ps(cvtop(srcvC));							\
		auto dstvD = _mm256_cvtepi32_ps(cvtop(srcvD));							\
																				\
		_mm256_storeu_ps(&(*dst)[i], dstvA);									\
		_mm256_storeu_ps(&(*dst)[i + nel], dstvB);								\
		_mm256_storeu_ps(&(*dst)[i + 2 * nel], dstvC);							\
		_mm256_storeu_ps(&(*dst)[i + 3 * nel], dstvD);							\
	}																			\
}

IntToFloatConvert(Int16Array_Float32Array, int16_t, _mm256_cvtepi16_epi32)
IntToFloatConvert(Uint16Array_Float32Array, uint16_t, _mm256_cvtepu16_epi32)
IntToFloatConvert(Int8Array_Float32Array, int8_t, _mm256_cvtepi8_epi32)
IntToFloatConvert(Uint8Array_Float32Array, uint8_t, _mm256_cvtepu8_epi32)
#undef IntToFloatConvert

#define IntToDoubleConvert(Name, srctype, cvtop)								\
NAN_METHOD(Name) {																\
	v8::Local<v8::ArrayBufferView> dstTA = info[0].As<v8::ArrayBufferView>();	\
	v8::Local<v8::ArrayBufferView> srcTA = info[1].As<v8::ArrayBufferView>();	\
	Nan::TypedArrayContents<double> dst(dstTA);									\
	Nan::TypedArrayContents<srctype> src(srcTA);								\
	size_t nel = 256 / (sizeof(double) * 8);									\
	for (size_t i = 0; i < dst.length(); i += nel * 4) {						\
		auto srcvA = _mm_lddqu_si128((__m128i*)&(*src)[i]);						\
		auto srcvB = _mm_lddqu_si128((__m128i*)&(*src)[i + nel]);				\
		auto srcvC = _mm_lddqu_si128((__m128i*)&(*src)[i + 2 * nel]);			\
		auto srcvD = _mm_lddqu_si128((__m128i*)&(*src)[i + 3 * nel]);			\
																				\
		auto dstvA = _mm256_cvtepi32_pd(cvtop(srcvA));							\
		auto dstvB = _mm256_cvtepi32_pd(cvtop(srcvB));							\
		auto dstvC = _mm256_cvtepi32_pd(cvtop(srcvC));							\
		auto dstvD = _mm256_cvtepi32_pd(cvtop(srcvD));							\
																				\
		_mm256_storeu_pd(&(*dst)[i], dstvA);									\
		_mm256_storeu_pd(&(*dst)[i + nel], dstvB);								\
		_mm256_storeu_pd(&(*dst)[i + 2 * nel], dstvC);							\
		_mm256_storeu_pd(&(*dst)[i + 3 * nel], dstvD);							\
	}																			\
}

IntToDoubleConvert(Int32Array_Float64Array, int32_t, )
IntToDoubleConvert(Int16Array_Float64Array, int16_t, _mm_cvtepi16_epi32)
IntToDoubleConvert(Uint16Array_Float64Array, uint16_t, _mm_cvtepu16_epi32)
IntToDoubleConvert(Int8Array_Float64Array, int8_t, _mm_cvtepi8_epi32)
IntToDoubleConvert(Uint8Array_Float64Array, uint8_t, _mm_cvtepu8_epi32)
#undef IntToDoubleConvert

NAN_MODULE_INIT(Init) {
	NAN_EXPORT(target, Float64Array_Float32Array);
	NAN_EXPORT(target, Float32Array_Float64Array);

	NAN_EXPORT(target, Float32Array_Int32Array);

	NAN_EXPORT(target, Int32Array_Int16Array);
	NAN_EXPORT(target, Int32Array_Int8Array);
	NAN_EXPORT(target, Int16Array_Int8Array);

	NAN_EXPORT(target, Int16Array_Int32Array);
	NAN_EXPORT(target, Uint16Array_Uint32Array);
	NAN_EXPORT(target, Int8Array_Int32Array);
	NAN_EXPORT(target, Int8Array_Int16Array);
	NAN_EXPORT(target, Uint8Array_Uint32Array);
	NAN_EXPORT(target, Uint8Array_Uint16Array);

	NAN_EXPORT(target, Int32Array_Float32Array);
	NAN_EXPORT(target, Int16Array_Float32Array);
	NAN_EXPORT(target, Uint16Array_Float32Array);
	NAN_EXPORT(target, Int8Array_Float32Array);
	NAN_EXPORT(target, Uint8Array_Float32Array);

	NAN_EXPORT(target, Int32Array_Float64Array);
	NAN_EXPORT(target, Int16Array_Float64Array);
	NAN_EXPORT(target, Uint16Array_Float64Array);
	NAN_EXPORT(target, Int8Array_Float64Array);
	NAN_EXPORT(target, Uint8Array_Float64Array);
}

NODE_MODULE(tacvt, Init);
