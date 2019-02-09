#include "nan.h"
#include "intrin.h"
#include "stdint.h"

// Name: from_to

NAN_METHOD(Float32Array_Float64Array) {
	v8::Local<v8::Float64Array> dstTA = info[0].As<v8::Float64Array>();
	v8::Local<v8::Float32Array> srcTA = info[1].As<v8::Float32Array>();
	Nan::TypedArrayContents<double> dst(dstTA);
	Nan::TypedArrayContents<float> src(srcTA);

	for (size_t i = 0; i < dstTA->Length(); i += 16) {
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
	v8::Local<v8::Float32Array> dstTA = info[0].As<v8::Float32Array>();
	v8::Local<v8::Float64Array> srcTA = info[1].As<v8::Float64Array>();
	Nan::TypedArrayContents<float> dst(dstTA);
	Nan::TypedArrayContents<double> src(srcTA);

	for (size_t i = 0; i < dstTA->Length(); i += 16) {
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
	v8::Local<v8::Int32Array> dstTA = info[0].As<v8::Int32Array>();
	v8::Local<v8::Float32Array> srcTA = info[1].As<v8::Float32Array>();
	Nan::TypedArrayContents<int32_t> dst(dstTA);
	Nan::TypedArrayContents<float> src(srcTA);

	for (size_t i = 0; i < dstTA->Length(); i += 8*4) {
		auto srcvA = _mm256_loadu_ps(&(*src)[i + 0]);
		auto srcvB = _mm256_loadu_ps(&(*src)[i + 8]);
		auto srcvC = _mm256_loadu_ps(&(*src)[i + 16]);
		auto srcvD = _mm256_loadu_ps(&(*src)[i + 24]);

		auto dstvA = _mm256_cvttps_epi32(srcvA);
		auto dstvB = _mm256_cvttps_epi32(srcvB);
		auto dstvC = _mm256_cvttps_epi32(srcvC);
		auto dstvD = _mm256_cvttps_epi32(srcvD);

		_mm256_store_si256((__m256i*)&(*dst)[i + 0], dstvA);
		_mm256_store_si256((__m256i*)&(*dst)[i + 8], dstvB);
		_mm256_store_si256((__m256i*)&(*dst)[i + 16], dstvC);
		_mm256_store_si256((__m256i*)&(*dst)[i + 24], dstvD);
	}
}

NAN_METHOD(Int32Array_Float32Array) {
	v8::Local<v8::Float32Array> dstTA = info[0].As<v8::Float32Array>();
	v8::Local<v8::Int32Array> srcTA = info[1].As<v8::Int32Array>();
	Nan::TypedArrayContents<float> dst(dstTA);
	Nan::TypedArrayContents<int32_t> src(srcTA);

	for (size_t i = 0; i < dstTA->Length(); i += 8*4) {
		auto srcvA = _mm256_lddqu_si256((__m256i*)&(*src)[i + 0]);
		auto srcvB = _mm256_lddqu_si256((__m256i*)&(*src)[i + 8]);
		auto srcvC = _mm256_lddqu_si256((__m256i*)&(*src)[i + 16]);
		auto srcvD = _mm256_lddqu_si256((__m256i*)&(*src)[i + 24]);

		auto dstvA = _mm256_cvtepi32_ps(srcvA);
		auto dstvB = _mm256_cvtepi32_ps(srcvB);
		auto dstvC = _mm256_cvtepi32_ps(srcvC);
		auto dstvD = _mm256_cvtepi32_ps(srcvD);

		_mm256_storeu_ps(&(*dst)[i + 0], dstvA);
		_mm256_storeu_ps(&(*dst)[i + 8], dstvB);
		_mm256_storeu_ps(&(*dst)[i + 16], dstvC);
		_mm256_storeu_ps(&(*dst)[i + 24], dstvD);
	}
}

NAN_METHOD(Int8Array_Int32Array) {
	v8::Local<v8::Int32Array> dstTA = info[0].As<v8::Int32Array>();
	v8::Local<v8::Int8Array> srcTA = info[1].As<v8::Int8Array>();
	Nan::TypedArrayContents<int32_t> dst(dstTA);
	Nan::TypedArrayContents<int8_t> src(srcTA);

	for (size_t i = 0; i < dstTA->Length(); i += 8*4) {
		auto srcvA = _mm_lddqu_si128((__m128i*)&(*src)[i + 0]);
		auto srcvB = _mm_lddqu_si128((__m128i*)&(*src)[i + 8]);
		auto srcvC = _mm_lddqu_si128((__m128i*)&(*src)[i + 16]);
		auto srcvD = _mm_lddqu_si128((__m128i*)&(*src)[i + 24]);

		auto dstvA = _mm256_cvtepi8_epi32(srcvA);
		auto dstvB = _mm256_cvtepi8_epi32(srcvB);
		auto dstvC = _mm256_cvtepi8_epi32(srcvC);
		auto dstvD = _mm256_cvtepi8_epi32(srcvD);

		_mm256_storeu_si256((__m256i*)&(*dst)[i + 0], dstvA);
		_mm256_storeu_si256((__m256i*)&(*dst)[i + 8], dstvB);
		_mm256_storeu_si256((__m256i*)&(*dst)[i + 16], dstvC);
		_mm256_storeu_si256((__m256i*)&(*dst)[i + 24], dstvD);
	}
}

NAN_METHOD(Uint8Array_Uint32Array) {
	v8::Local<v8::Uint32Array> dstTA = info[0].As<v8::Uint32Array>();
	v8::Local<v8::Uint8Array> srcTA = info[1].As<v8::Uint8Array>();
	Nan::TypedArrayContents<uint32_t> dst(dstTA);
	Nan::TypedArrayContents<uint8_t> src(srcTA);

	for (size_t i = 0; i < dstTA->Length(); i += 8*4) {
		auto srcvA = _mm_lddqu_si128((__m128i*)&(*src)[i + 0]);
		auto srcvB = _mm_lddqu_si128((__m128i*)&(*src)[i + 8]);
		auto srcvC = _mm_lddqu_si128((__m128i*)&(*src)[i + 16]);
		auto srcvD = _mm_lddqu_si128((__m128i*)&(*src)[i + 24]);

		auto dstvA = _mm256_cvtepu8_epi32(srcvA);
		auto dstvB = _mm256_cvtepu8_epi32(srcvB);
		auto dstvC = _mm256_cvtepu8_epi32(srcvC);
		auto dstvD = _mm256_cvtepu8_epi32(srcvD);

		_mm256_storeu_si256((__m256i*)&(*dst)[i + 0], dstvA);
		_mm256_storeu_si256((__m256i*)&(*dst)[i + 8], dstvB);
		_mm256_storeu_si256((__m256i*)&(*dst)[i + 16], dstvC);
		_mm256_storeu_si256((__m256i*)&(*dst)[i + 24], dstvD);
	}
}

NAN_MODULE_INIT(Init) {
	NAN_EXPORT(target, Float32Array_Float64Array);
	NAN_EXPORT(target, Float64Array_Float32Array);
	NAN_EXPORT(target, Int32Array_Float32Array);
	NAN_EXPORT(target, Int8Array_Int32Array);
	NAN_EXPORT(target, Uint8Array_Uint32Array);
	NAN_EXPORT(target, Float32Array_Int32Array);
}

NODE_MODULE(tacvt, Init);
