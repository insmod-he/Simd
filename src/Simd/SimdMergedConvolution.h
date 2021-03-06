/*
* Simd Library (http://ermig1979.github.io/Simd).
*
* Copyright (c) 2011-2019 Yermalayeu Ihar.
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
#ifndef __SimdMergedConvolution_h__
#define __SimdMergedConvolution_h__

#include "Simd/SimdArray.h"
#include "Simd/SimdPerformance.h"
#include "Simd/SimdRuntime.h"

#ifdef _N
#undef _N
#endif

namespace Simd
{
    const size_t MC_MAX_COUNT = 3;
    struct MergConvParam
    {
        SimdBool trans, add;
        size_t batch, count;
        SimdConvolutionParameters conv[MC_MAX_COUNT];

        MergConvParam(SimdBool trans, size_t batch, const SimdConvolutionParameters * convs, size_t count, SimdBool add)
        {
            assert(count <= MC_MAX_COUNT);
            this->trans = trans;
            this->add = add;
            this->batch = batch;
            this->count = count;
            for (size_t i = 0; i < count; ++i)
                this->conv[i] = convs[i];
        }

        bool Valid()
        {
            if (trans != SimdTrue)
                return false;
            if (count != MC_MAX_COUNT)
                return false;
            for (size_t i = 0; i < count; ++i)
            {
                const SimdConvolutionParameters & c = conv[i];
                if (c.dstH != (c.srcH + c.padY + c.padH - (c.dilationY * (c.kernelY - 1) + 1)) / c.strideY + 1 || c.dstH == 0)
                    return false;
                if (c.dstW != (c.srcW + c.padX + c.padW - (c.dilationY * (c.kernelX - 1) + 1)) / c.strideX + 1 || c.dstW == 0)
                    return false;
                if (c.kernelY != c.kernelX || !(c.kernelY == 1 || c.kernelY == 3))
                    return false;
                if (c.strideY != c.strideX || !(c.strideY == 1 || c.strideY == 2))
                    return false;
                if (c.dilationY != 1 || c.dilationX != 1)
                    return false;
            }
            if (conv[0].group != 1)
                return false;
            if (conv[1].group != conv[1].srcC || conv[1].group != conv[1].dstC || conv[1].kernelY != 3)
                return false;
            if (conv[2].group != 1 || conv[2].kernelY != 1 || conv[2].strideY != 1)
                return false;
            return true;
        }

        SIMD_INLINE bool IsPad(size_t index, size_t value) const
        {
            return conv[index].padY == value && conv[index].padX == value && conv[index].padH == value && conv[index].padW == value;
        }

#ifdef SIMD_PERFORMANCE_STATISTIC
        String Info() const
        {
            std::stringstream ss;
            ss << batch << "x" << conv[0].srcC << "x" << conv[0].srcH << "x" << conv[0].srcW;
            ss << "-" << conv[0].dstC << "x" << conv[0].kernelY << "x" << conv[0].strideY;
            ss << "-" << conv[1].kernelY << "x" << conv[1].strideY << "-" << conv[2].dstC;
            return ss.str();
        }
#endif
    };

    class MergedConvolution : public Deletable
    {
    public:
        MergedConvolution(const MergConvParam & p) 
            : _param(p)
            , _0(0.0f)
            , _1(1.0f)
        {
        }

        virtual size_t ExternalBufferSize() const
        {
            return 1;
        }

        virtual size_t InternalBufferSize() const
        {
            return 1;
        }

        virtual void SetParams(const float * const * weight, SimdBool * internal, const float * const * bias, const float * const * params)
        {
            for (size_t i = 0; i < _param.count; ++i)
            {
                _weight[i] = weight[i];
                if (internal)
                    internal[i] = SimdFalse;
                _bias[i] = bias[i];
                _params[i] = params[i];
            }
        }

        virtual void Forward(const float * src, float * buf, float * dst) = 0;

        float * Buffer(float * buffer)
        {
            if (buffer)
                return buffer;
            else
            {
                _buffer.Resize(ExternalBufferSize());
                return _buffer.data;
            }
        }

    protected:
        MergConvParam _param;
        Array32f _buffer;
        float _0, _1;
        const float * _weight[MC_MAX_COUNT], * _bias[MC_MAX_COUNT], * _params[MC_MAX_COUNT];
    };

    namespace Base
    {
        class MergedConvolution : public Simd::MergedConvolution
        {
        public:
            MergedConvolution(const MergConvParam & p);

            virtual size_t ExternalBufferSize() const;
            virtual size_t InternalBufferSize() const;
            virtual void SetParams(const float * const * weight, SimdBool * internal, const float * const * bias, const float * const * params);
            virtual void Forward(const float * src, float * buf, float * dst);

        protected:
            typedef void(*ConvolutionPtr)(const float * src, const SimdConvolutionParameters & p, size_t yBeg, size_t yEnd, 
                const float * weight, const float * bias, const float * params, float * dst);

            size_t _sizeS[MC_MAX_COUNT], _sizeD[MC_MAX_COUNT];
            ConvolutionPtr _convolution[MC_MAX_COUNT];
        };

        void * MergedConvolutionInit(SimdBool trans, size_t batch, const SimdConvolutionParameters * convs, size_t count, SimdBool add);
    }
#if 0
#ifdef SIMD_SSE_ENABLE    
    namespace Sse
    {
        class MergedConvolution : public Base::MergedConvolution
        {
        public:
            MergedConvolution(const MergConvParam & p);
        };

        void * MergedConvolutionInit(SimdBool trans, size_t batch, const SimdConvolutionParameters * convs, size_t count, SimdBool add);
    }
#endif//SIMD_SSE_ENABLE

#ifdef SIMD_AVX_ENABLE    
    namespace Avx
    {
        class MergedConvolution : public Sse::MergedConvolution
        {
        public:
            MergedConvolution(const MergConvParam & p);
        };

        void * MergedConvolutionInit(SimdBool trans, size_t batch, const SimdConvolutionParameters * convs, size_t count, SimdBool add);
    }
#endif//SIMD_AVX_ENABLE

#ifdef SIMD_AVX2_ENABLE    
    namespace Avx2
    {
        class MergedConvolution : public Avx::MergedConvolution
        {
        public:
            MergedConvolution(const MergConvParam & p);
        };

        void * MergedConvolutionInit(SimdBool trans, size_t batch, const SimdConvolutionParameters * convs, size_t count, SimdBool add);
    }
#endif//SIMD_AVX2_ENABLE

#ifdef SIMD_AVX512F_ENABLE    
    namespace Avx512f
    {
        class MergedConvolution : public Avx2::MergedConvolution
        {
        public:
            MergedConvolution(const MergConvParam & p);
        };

        void * MergedConvolutionInit(SimdBool trans, size_t batch, const SimdConvolutionParameters * convs, size_t count, SimdBool add);
    }
#endif//SIMD_AVX512F_ENABLE

#ifdef SIMD_NEON_ENABLE    
    namespace Neon
    {
        class MergedConvolution : public Base::MergedConvolution
        {
        public:
            MergedConvolution(const MergConvParam & p);
        };

        void * MergedConvolutionInit(SimdBool trans, size_t batch, const SimdConvolutionParameters * convs, size_t count, SimdBool add);
    }
#endif//SIMD_NEON_ENABLE
#endif
}
#endif//__SimMergedConvolution_h__
