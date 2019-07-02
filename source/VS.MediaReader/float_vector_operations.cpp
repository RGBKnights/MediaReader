/*
@file		float_vector_operations.cpp
@author		Webstar
@date		2019-07-02 15:34
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/


#include <stdint.h>
#include <algorithm>

#include "float_vector_operations.hpp"

using namespace vs;
using namespace std;

namespace vs
{
	namespace helpers
	{
#define VS_INCREMENT_SRC_DEST         dest += (16 / sizeof (*dest)); src += (16 / sizeof (*dest));
#define VS_INCREMENT_SRC1_SRC2_DEST   dest += (16 / sizeof (*dest)); src1 += (16 / sizeof (*dest)); src2 += (16 / sizeof (*dest));
#define VS_INCREMENT_DEST             dest += (16 / sizeof (*dest));

#define VS_PERFORM_VEC_OP_DEST(normalOp, vecOp, locals, setupOp) \
        for (int i = 0; i < num; ++i) normalOp;

#define VS_PERFORM_VEC_OP_SRC_DEST(normalOp, vecOp, locals, increment, setupOp) \
        for (int i = 0; i < num; ++i) normalOp;

#define VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(normalOp, vecOp, locals, increment, setupOp) \
        for (int i = 0; i < num; ++i) normalOp;

#define VS_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST(normalOp, vecOp, locals, increment, setupOp) \
        for (int i = 0; i < num; ++i) normalOp;

		//==============================================================================
#define VS_VEC_LOOP(vecOp, srcLoad, dstLoad, dstStore, locals, increment) \
        for (int i = 0; i < numLongOps; ++i) \
        { \
            locals (srcLoad, dstLoad); \
            dstStore (dest, vecOp); \
            increment; \
        }

#define VS_VEC_LOOP_TWO_SOURCES(vecOp, src1Load, src2Load, dstStore, locals, increment) \
        for (int i = 0; i < numLongOps; ++i) \
        { \
            locals (src1Load, src2Load); \
            dstStore (dest, vecOp); \
            increment; \
        }

#define VS_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD(vecOp, src1Load, src2Load, dstLoad, dstStore, locals, increment) \
        for (int i = 0; i < numLongOps; ++i) \
        { \
            locals (src1Load, src2Load, dstLoad); \
            dstStore (dest, vecOp); \
            increment; \
        }

#define VS_LOAD_NONE(srcLoad, dstLoad)
#define VS_LOAD_DEST(srcLoad, dstLoad)                        const Mode::ParallelType d = dstLoad (dest);
#define VS_LOAD_SRC(srcLoad, dstLoad)                         const Mode::ParallelType s = srcLoad (src);
#define VS_LOAD_SRC1_SRC2(src1Load, src2Load)                 const Mode::ParallelType s1 = src1Load (src1), s2 = src2Load (src2);
#define VS_LOAD_SRC1_SRC2_DEST(src1Load, src2Load, dstLoad)   const Mode::ParallelType d = dstLoad (dest), s1 = src1Load (src1), s2 = src2Load (src2);
#define VS_LOAD_SRC_DEST(srcLoad, dstLoad)                    const Mode::ParallelType d = dstLoad (dest), s = srcLoad (src);

		union signMask32 { float  f; uint32_t i; };
		union signMask64 { double d; uint64_t i; };
	}
}

//==============================================================================
void  FloatVectorOperations::clear(float* dest, int num) 
{
	zeromem(dest, (size_t)num * sizeof(float));
}

void  FloatVectorOperations::clear(double* dest, int num) 
{
	zeromem(dest, (size_t)num * sizeof(double));
}

void  FloatVectorOperations::fill(float* dest, float valueToFill, int num) 
{
	VS_PERFORM_VEC_OP_DEST(dest[i] = valueToFill, val, VS_LOAD_NONE,
		const Mode::ParallelType val = Mode::load1(valueToFill);)
}

void  FloatVectorOperations::fill(double* dest, double valueToFill, int num) 
{
	VS_PERFORM_VEC_OP_DEST(dest[i] = valueToFill, val, VS_LOAD_NONE,
		const Mode::ParallelType val = Mode::load1(valueToFill);)
}

void  FloatVectorOperations::copy(float* dest, const float* src, int num) 
{
	memcpy(dest, src, (size_t)num * sizeof(float));
}

void  FloatVectorOperations::copy(double* dest, const double* src, int num) 
{
	memcpy(dest, src, (size_t)num * sizeof(double));
}

void  FloatVectorOperations::copyWithMultiply(float* dest, const float* src, float multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = src[i] * multiplier, Mode::mul(mult, s),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::copyWithMultiply(double* dest, const double* src, double multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = src[i] * multiplier, Mode::mul(mult, s),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::add(float* dest, float amount, int num) 
{
	VS_PERFORM_VEC_OP_DEST(dest[i] += amount, Mode::add(d, amountToAdd), VS_LOAD_DEST,
		const Mode::ParallelType amountToAdd = Mode::load1(amount);)
}

void  FloatVectorOperations::add(double* dest, double amount, int num) 
{
	VS_PERFORM_VEC_OP_DEST(dest[i] += amount, Mode::add(d, amountToAdd), VS_LOAD_DEST,
		const Mode::ParallelType amountToAdd = Mode::load1(amount);)
}

void  FloatVectorOperations::add(float* dest, const float* src, float amount, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = src[i] + amount, Mode::add(am, s),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType am = Mode::load1(amount);)
}

void  FloatVectorOperations::add(double* dest, const double* src, double amount, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = src[i] + amount, Mode::add(am, s),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType am = Mode::load1(amount);)
}

void  FloatVectorOperations::add(float* dest, const float* src, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] += src[i], Mode::add(d, s), VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST, )
}

void  FloatVectorOperations::add(double* dest, const double* src, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] += src[i], Mode::add(d, s), VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST, )
}

void  FloatVectorOperations::add(float* dest, const float* src1, const float* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = src1[i] + src2[i], Mode::add(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::add(double* dest, const double* src1, const double* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = src1[i] + src2[i], Mode::add(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::subtract(float* dest, const float* src, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] -= src[i], Mode::sub(d, s), VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST, )
}

void  FloatVectorOperations::subtract(double* dest, const double* src, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] -= src[i], Mode::sub(d, s), VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST, )
}

void  FloatVectorOperations::subtract(float* dest, const float* src1, const float* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = src1[i] - src2[i], Mode::sub(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::subtract(double* dest, const double* src1, const double* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = src1[i] - src2[i], Mode::sub(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::addWithMultiply(float* dest, const float* src, float multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] += src[i] * multiplier, Mode::add(d, Mode::mul(mult, s)),
		VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::addWithMultiply(double* dest, const double* src, double multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] += src[i] * multiplier, Mode::add(d, Mode::mul(mult, s)),
		VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::addWithMultiply(float* dest, const float* src1, const float* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST(dest[i] += src1[i] * src2[i], Mode::add(d, Mode::mul(s1, s2)),
		VS_LOAD_SRC1_SRC2_DEST,
		VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::addWithMultiply(double* dest, const double* src1, const double* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST(dest[i] += src1[i] * src2[i], Mode::add(d, Mode::mul(s1, s2)),
		VS_LOAD_SRC1_SRC2_DEST,
		VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::subtractWithMultiply(float* dest, const float* src, float multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] -= src[i] * multiplier, Mode::sub(d, Mode::mul(mult, s)),
		VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::subtractWithMultiply(double* dest, const double* src, double multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] -= src[i] * multiplier, Mode::sub(d, Mode::mul(mult, s)),
		VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::subtractWithMultiply(float* dest, const float* src1, const float* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST(dest[i] -= src1[i] * src2[i], Mode::sub(d, Mode::mul(s1, s2)),
		VS_LOAD_SRC1_SRC2_DEST,
		VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::subtractWithMultiply(double* dest, const double* src1, const double* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST(dest[i] -= src1[i] * src2[i], Mode::sub(d, Mode::mul(s1, s2)),
		VS_LOAD_SRC1_SRC2_DEST,
		VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::multiply(float* dest, const float* src, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] *= src[i], Mode::mul(d, s), VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST, )
}

void  FloatVectorOperations::multiply(double* dest, const double* src, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] *= src[i], Mode::mul(d, s), VS_LOAD_SRC_DEST, VS_INCREMENT_SRC_DEST, )
}

void  FloatVectorOperations::multiply(float* dest, const float* src1, const float* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = src1[i] * src2[i], Mode::mul(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::multiply(double* dest, const double* src1, const double* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = src1[i] * src2[i], Mode::mul(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::multiply(float* dest, float multiplier, int num) 
{
	VS_PERFORM_VEC_OP_DEST(dest[i] *= multiplier, Mode::mul(d, mult), VS_LOAD_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::multiply(double* dest, double multiplier, int num) 
{
	VS_PERFORM_VEC_OP_DEST(dest[i] *= multiplier, Mode::mul(d, mult), VS_LOAD_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::multiply(float* dest, const float* src, float multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = src[i] * multiplier, Mode::mul(mult, s),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::multiply(double* dest, const double* src, double multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = src[i] * multiplier, Mode::mul(mult, s),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void FloatVectorOperations::negate(float* dest, const float* src, int num) 
{
	copyWithMultiply(dest, src, -1.0f, num);
}

void FloatVectorOperations::negate(double* dest, const double* src, int num) 
{
	copyWithMultiply(dest, src, -1.0f, num);
}

void FloatVectorOperations::abs(float* dest, const float* src, int num) 
{
	helpers::signMask32 signMask;
	signMask.i = 0x7fffffffUL;

	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = fabsf(src[i]), Mode::bit_and(s, mask),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mask = Mode::load1(signMask.f);)
}

void FloatVectorOperations::abs(double* dest, const double* src, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = fabs(src[i]), Mode::bit_and(s, mask),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mask = Mode::load1(signMask.d);)
}

void  FloatVectorOperations::convertFixedToFloat(float* dest, const int* src, float multiplier, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = (float)src[i] * multiplier,
		Mode::mul(mult, _mm_cvtepi32_ps(_mm_loadu_si128((const __m128i*) src))),
		VS_LOAD_NONE, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType mult = Mode::load1(multiplier);)
}

void  FloatVectorOperations::min(float* dest, const float* src, float comp, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = typed_min(src[i], comp), Mode::min(s, cmp),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType cmp = Mode::load1(comp);)
}

void  FloatVectorOperations::min(double* dest, const double* src, double comp, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = typed_min(src[i], comp), Mode::min(s, cmp),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType cmp = Mode::load1(comp);)
}

void  FloatVectorOperations::min(float* dest, const float* src1, const float* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = typed_min(src1[i], src2[i]), Mode::min(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::min(double* dest, const double* src1, const double* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = typed_min(src1[i], src2[i]), Mode::min(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::max(float* dest, const float* src, float comp, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = typed_max(src[i], comp), Mode::max(s, cmp),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType cmp = Mode::load1(comp);)
}

void  FloatVectorOperations::max(double* dest, const double* src, double comp, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = typed_max(src[i], comp), Mode::max(s, cmp),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType cmp = Mode::load1(comp);)
}

void  FloatVectorOperations::max(float* dest, const float* src1, const float* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = typed_max(src1[i], src2[i]), Mode::max(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::max(double* dest, const double* src1, const double* src2, int num) 
{
	VS_PERFORM_VEC_OP_SRC1_SRC2_DEST(dest[i] = typed_max(src1[i], src2[i]), Mode::max(s1, s2), VS_LOAD_SRC1_SRC2, VS_INCREMENT_SRC1_SRC2_DEST, )
}

void  FloatVectorOperations::clip(float* dest, const float* src, float low, float high, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = typed_max(typed_min(src[i], high), low), Mode::max(Mode::min(s, hi), lo),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType lo = Mode::load1(low); const Mode::ParallelType hi = Mode::load1(high);)
}

void  FloatVectorOperations::clip(double* dest, const double* src, double low, double high, int num) 
{
	VS_PERFORM_VEC_OP_SRC_DEST(dest[i] = typed_max(typed_min(src[i], high), low), Mode::max(Mode::min(s, hi), lo),
		VS_LOAD_SRC, VS_INCREMENT_SRC_DEST,
		const Mode::ParallelType lo = Mode::load1(low); const Mode::ParallelType hi = Mode::load1(high);)
}

float  FloatVectorOperations::findMinimum(const float* src, int num) 
{
	return vs::findMinimum(src, num);
}

double  FloatVectorOperations::findMinimum(const double* src, int num) 
{
	return vs::findMinimum(src, num);
}

float  FloatVectorOperations::findMaximum(const float* src, int num) 
{
	return vs::findMaximum(src, num);
}

double  FloatVectorOperations::findMaximum(const double* src, int num) 
{
	return vs::findMaximum(src, num);
}

/*
=============================================================
Copyright Venatio Studios 2019
=============================================================
Revision History

0.0.1 : 2019-07-02 15:34
#vNext
=============================================================
*/
