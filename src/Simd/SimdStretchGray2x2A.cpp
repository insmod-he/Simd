/*
* Simd Library.
*
* Copyright (c) 2011-2013 Yermalayeu Ihar.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy 
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
* copies of the Software, and to permit persons to whom the Software is 
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in 
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "Simd/SimdEnable.h"
#include "Simd/SimdMemory.h"
#include "Simd/SimdConst.h"
#include "Simd/SimdStretchGray2x2.h"

namespace Simd
{
#ifdef SIMD_AVX2_ENABLE    
    namespace Avx2
    {
		template<bool align> SIMD_INLINE void StoreUnpacked(__m256i value, uchar * dst)
		{
			Store<align>((__m256i*)(dst + 0), _mm256_unpacklo_epi8(value, value));
			Store<align>((__m256i*)(dst + A), _mm256_unpackhi_epi8(value, value));
		}

		template <bool align> void StretchGray2x2(
			const uchar *src, size_t srcWidth, size_t srcHeight, size_t srcStride, 
			uchar *dst, size_t dstWidth, size_t dstHeight, size_t dstStride)
		{
			assert(srcWidth*2 == dstWidth && srcHeight*2 == dstHeight && srcWidth >= A);
			if(align)
			{
				assert(Aligned(src) && Aligned(srcStride));
				assert(Aligned(dst) && Aligned(dstStride));
			}

			size_t alignedWidth = AlignLo(srcWidth, A);
			for(size_t row = 0; row < srcHeight; ++row)
			{
				uchar * dstEven = dst;
				uchar * dstOdd = dst + dstStride;
				for(size_t srcCol = 0, dstCol = 0; srcCol < alignedWidth; srcCol += A, dstCol += DA)
				{
					__m256i value = _mm256_permute4x64_epi64(Load<align>((__m256i*)(src + srcCol)), 0xD8);
					StoreUnpacked<align>(value, dstEven + dstCol);
					StoreUnpacked<align>(value, dstOdd + dstCol);
				}
				if(alignedWidth != srcWidth)
				{
					__m256i value = _mm256_permute4x64_epi64(Load<false>((__m256i*)(src + srcWidth - A)), 0xD8);
					StoreUnpacked<false>(value, dstEven + dstWidth - 2*A);
					StoreUnpacked<false>(value, dstOdd + dstWidth - 2*A);
				}
				src += srcStride;
				dst += 2*dstStride;
			}
		}

		void StretchGray2x2(const uchar *src, size_t srcWidth, size_t srcHeight, size_t srcStride, 
			uchar *dst, size_t dstWidth, size_t dstHeight, size_t dstStride)
		{
			if(Aligned(src) && Aligned(srcStride) && Aligned(dst) && Aligned(dstStride))
				StretchGray2x2<true>(src, srcWidth, srcHeight, srcStride, dst, dstWidth, dstHeight, dstStride);
			else
				StretchGray2x2<false>(src, srcWidth, srcHeight, srcStride, dst, dstWidth, dstHeight, dstStride);
		}
    }
#endif// SIMD_AVX2_ENABLE
}