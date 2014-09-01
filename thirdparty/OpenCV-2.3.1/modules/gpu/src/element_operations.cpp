/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other GpuMaterials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or bpied warranties, including, but not limited to, the bpied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"

using namespace cv;
using namespace cv::gpu;

#if !defined (HAVE_CUDA)

void cv::gpu::add(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::add(const GpuMat&, const Scalar&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::subtract(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::subtract(const GpuMat&, const Scalar&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::multiply(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::multiply(const GpuMat&, const Scalar&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::divide(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::divide(const GpuMat&, const Scalar&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::absdiff(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::absdiff(const GpuMat&, const Scalar&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::compare(const GpuMat&, const GpuMat&, GpuMat&, int, Stream&) { throw_nogpu(); }
void cv::gpu::bitwise_not(const GpuMat&, GpuMat&, const GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::bitwise_or(const GpuMat&, const GpuMat&, GpuMat&, const GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::bitwise_and(const GpuMat&, const GpuMat&, GpuMat&, const GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::bitwise_xor(const GpuMat&, const GpuMat&, GpuMat&, const GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::min(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::min(const GpuMat&, double, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::max(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_nogpu(); }
void cv::gpu::max(const GpuMat&, double, GpuMat&, Stream&) { throw_nogpu(); }
double cv::gpu::threshold(const GpuMat&, GpuMat&, double, double, int, Stream&) {throw_nogpu(); return 0.0;}

void cv::gpu::pow(const GpuMat&, double, GpuMat&, Stream&)  { throw_nogpu(); }

#else

////////////////////////////////////////////////////////////////////////
// Basic arithmetical operations (add subtract multiply divide)

namespace
{
    typedef NppStatus (*npp_arithm_8u_t)(const Npp8u* pSrc1, int nSrc1Step, const Npp8u* pSrc2, int nSrc2Step, Npp8u* pDst, int nDstStep, NppiSize oSizeROI, int nScaleFactor);
    typedef NppStatus (*npp_arithm_32s_t)(const Npp32s* pSrc1, int nSrc1Step, const Npp32s* pSrc2, int nSrc2Step, Npp32s* pDst, int nDstStep, NppiSize oSizeROI);
    typedef NppStatus (*npp_arithm_32f_t)(const Npp32f* pSrc1, int nSrc1Step, const Npp32f* pSrc2, int nSrc2Step, Npp32f* pDst, int nDstStep, NppiSize oSizeROI);

    void nppArithmCaller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst,
                         npp_arithm_8u_t npp_func_8uc1, npp_arithm_8u_t npp_func_8uc4,
                         npp_arithm_32s_t npp_func_32sc1, npp_arithm_32f_t npp_func_32fc1, cudaStream_t stream)
    {
        CV_DbgAssert(src1.size() == src2.size() && src1.type() == src2.type());
        CV_Assert(src1.type() == CV_8UC1 || src1.type() == CV_8UC4 || src1.type() == CV_32SC1 || src1.type() == CV_32FC1);
        dst.create( src1.size(), src1.type() );

        NppiSize sz;
        sz.width  = src1.cols;
        sz.height = src1.rows;

        NppStreamHandler h(stream);

        switch (src1.type())
        {
        case CV_8UC1:
            nppSafeCall( npp_func_8uc1(src1.ptr<Npp8u>(), static_cast<int>(src1.step), src2.ptr<Npp8u>(), static_cast<int>(src2.step), 
                dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz, 0) );
            break;
        case CV_8UC4:
            nppSafeCall( npp_func_8uc4(src1.ptr<Npp8u>(), static_cast<int>(src1.step), src2.ptr<Npp8u>(), static_cast<int>(src2.step), 
                dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz, 0) );
            break;
        case CV_32SC1:
            nppSafeCall( npp_func_32sc1(src1.ptr<Npp32s>(), static_cast<int>(src1.step), src2.ptr<Npp32s>(), static_cast<int>(src2.step), 
                dst.ptr<Npp32s>(), static_cast<int>(dst.step), sz) );
            break;
        case CV_32FC1:
            nppSafeCall( npp_func_32fc1(src1.ptr<Npp32f>(), static_cast<int>(src1.step), src2.ptr<Npp32f>(), static_cast<int>(src2.step), 
                dst.ptr<Npp32f>(), static_cast<int>(dst.step), sz) );
            break;
        default:
            CV_Assert(!"Unsupported source type");
        }

        if (stream == 0)
            cudaSafeCall( cudaDeviceSynchronize() );
    }

    template<int SCN> struct NppArithmScalarFunc;
    template<> struct NppArithmScalarFunc<1>
    {
        typedef NppStatus (*func_ptr)(const Npp32f *pSrc, int nSrcStep, Npp32f nValue, Npp32f *pDst, int nDstStep, NppiSize oSizeROI);
    };
    template<> struct NppArithmScalarFunc<2>
    {
        typedef NppStatus (*func_ptr)(const Npp32fc *pSrc, int nSrcStep, Npp32fc nValue, Npp32fc *pDst, int nDstStep, NppiSize oSizeROI);
    };

    template<int SCN, typename NppArithmScalarFunc<SCN>::func_ptr func> struct NppArithmScalar;

    template<typename NppArithmScalarFunc<1>::func_ptr func> struct NppArithmScalar<1, func>
    {
        static void calc(const GpuMat& src, const Scalar& sc, GpuMat& dst, cudaStream_t stream)
        {
            dst.create(src.size(), src.type());

            NppiSize sz;
            sz.width  = src.cols;
            sz.height = src.rows;

            NppStreamHandler h(stream);

            nppSafeCall( func(src.ptr<Npp32f>(), static_cast<int>(src.step), (Npp32f)sc[0], dst.ptr<Npp32f>(), static_cast<int>(dst.step), sz) );

            if (stream == 0)
                cudaSafeCall( cudaDeviceSynchronize() );
        }
    };
    template<typename NppArithmScalarFunc<2>::func_ptr func> struct NppArithmScalar<2, func>
    {
        static void calc(const GpuMat& src, const Scalar& sc, GpuMat& dst, cudaStream_t stream)
        {
            dst.create(src.size(), src.type());

            NppiSize sz;
            sz.width  = src.cols;
            sz.height = src.rows;

            Npp32fc nValue;
            nValue.re = (Npp32f)sc[0];
            nValue.im = (Npp32f)sc[1];

            NppStreamHandler h(stream);

            nppSafeCall( func(src.ptr<Npp32fc>(), static_cast<int>(src.step), nValue, dst.ptr<Npp32fc>(), static_cast<int>(dst.step), sz) );

            if (stream == 0)
                cudaSafeCall( cudaDeviceSynchronize() );
        }
    };
}

void cv::gpu::add(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, Stream& stream)
{
    nppArithmCaller(src1, src2, dst, nppiAdd_8u_C1RSfs, nppiAdd_8u_C4RSfs, nppiAdd_32s_C1R, nppiAdd_32f_C1R, StreamAccessor::getStream(stream));
}

namespace cv { namespace gpu { namespace mathfunc
{
    template <typename T>
    void subtractCaller(const DevMem2D src1, const DevMem2D src2, DevMem2D dst, cudaStream_t stream);
}}}

void cv::gpu::subtract(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, Stream& stream)
{
    if (src1.depth() == CV_16S && src2.depth() == CV_16S)
    {
        CV_Assert(src1.size() == src2.size());
        dst.create(src1.size(), src1.type());
        mathfunc::subtractCaller<short>(src1.reshape(1), src2.reshape(1), dst.reshape(1), StreamAccessor::getStream(stream));
    }
    else
        nppArithmCaller(src2, src1, dst, nppiSub_8u_C1RSfs, nppiSub_8u_C4RSfs, nppiSub_32s_C1R, nppiSub_32f_C1R, StreamAccessor::getStream(stream));
}

void cv::gpu::multiply(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, Stream& stream)
{
    nppArithmCaller(src1, src2, dst, nppiMul_8u_C1RSfs, nppiMul_8u_C4RSfs, nppiMul_32s_C1R, nppiMul_32f_C1R, StreamAccessor::getStream(stream));
}

void cv::gpu::divide(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, Stream& stream)
{
    nppArithmCaller(src2, src1, dst, nppiDiv_8u_C1RSfs, nppiDiv_8u_C4RSfs, nppiDiv_32s_C1R, nppiDiv_32f_C1R, StreamAccessor::getStream(stream));
}

void cv::gpu::add(const GpuMat& src, const Scalar& sc, GpuMat& dst, Stream& stream)
{
    typedef void (*caller_t)(const GpuMat& src, const Scalar& sc, GpuMat& dst, cudaStream_t stream);
    static const caller_t callers[] = {0, NppArithmScalar<1, nppiAddC_32f_C1R>::calc, NppArithmScalar<2, nppiAddC_32fc_C1R>::calc};

    CV_Assert(src.type() == CV_32FC1 || src.type() == CV_32FC2);

    callers[src.channels()](src, sc, dst, StreamAccessor::getStream(stream));
}

void cv::gpu::subtract(const GpuMat& src, const Scalar& sc, GpuMat& dst, Stream& stream)
{
    typedef void (*caller_t)(const GpuMat& src, const Scalar& sc, GpuMat& dst, cudaStream_t stream);
    static const caller_t callers[] = {0, NppArithmScalar<1, nppiSubC_32f_C1R>::calc, NppArithmScalar<2, nppiSubC_32fc_C1R>::calc};

    CV_Assert(src.type() == CV_32FC1 || src.type() == CV_32FC2);

    callers[src.channels()](src, sc, dst, StreamAccessor::getStream(stream));
}

void cv::gpu::multiply(const GpuMat& src, const Scalar& sc, GpuMat& dst, Stream& stream)
{
    CV_Assert(src.type() == CV_32FC1);

    dst.create(src.size(), src.type());

    NppiSize sz;
    sz.width  = src.cols;
    sz.height = src.rows;

    cudaStream_t cudaStream = StreamAccessor::getStream(stream);

    NppStreamHandler h(cudaStream);

    nppSafeCall( nppiMulC_32f_C1R(src.ptr<Npp32f>(), static_cast<int>(src.step), (Npp32f)sc[0], dst.ptr<Npp32f>(), static_cast<int>(dst.step), sz) );

    if (cudaStream == 0)
        cudaSafeCall( cudaDeviceSynchronize() );
}

void cv::gpu::divide(const GpuMat& src, const Scalar& sc, GpuMat& dst, Stream& stream)
{
    CV_Assert(src.type() == CV_32FC1);

    dst.create(src.size(), src.type());

    NppiSize sz;
    sz.width  = src.cols;
    sz.height = src.rows;

    cudaStream_t cudaStream = StreamAccessor::getStream(stream);

    NppStreamHandler h(cudaStream);

    nppSafeCall( nppiDivC_32f_C1R(src.ptr<Npp32f>(), static_cast<int>(src.step), (Npp32f)sc[0], dst.ptr<Npp32f>(), static_cast<int>(dst.step), sz) );

    if (cudaStream == 0)
        cudaSafeCall( cudaDeviceSynchronize() );
}


//////////////////////////////////////////////////////////////////////////////
// Absolute difference

void cv::gpu::absdiff(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, Stream& s)
{
    CV_DbgAssert(src1.size() == src2.size() && src1.type() == src2.type());

    CV_Assert(src1.type() == CV_8UC1 || src1.type() == CV_8UC4 || src1.type() == CV_32SC1 || src1.type() == CV_32FC1);

    dst.create( src1.size(), src1.type() );

    NppiSize sz;
    sz.width  = src1.cols;
    sz.height = src1.rows;

    cudaStream_t stream = StreamAccessor::getStream(s);

    NppStreamHandler h(stream);

    switch (src1.type())
    {
    case CV_8UC1:
        nppSafeCall( nppiAbsDiff_8u_C1R(src1.ptr<Npp8u>(), static_cast<int>(src1.step), src2.ptr<Npp8u>(), static_cast<int>(src2.step), 
            dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz) );
        break;
    case CV_8UC4:
        nppSafeCall( nppiAbsDiff_8u_C4R(src1.ptr<Npp8u>(), static_cast<int>(src1.step), src2.ptr<Npp8u>(), static_cast<int>(src2.step), 
            dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz) );
        break;
    case CV_32SC1:
        nppSafeCall( nppiAbsDiff_32s_C1R(src1.ptr<Npp32s>(), static_cast<int>(src1.step), src2.ptr<Npp32s>(), static_cast<int>(src2.step), 
            dst.ptr<Npp32s>(), static_cast<int>(dst.step), sz) );
        break;
    case CV_32FC1:
        nppSafeCall( nppiAbsDiff_32f_C1R(src1.ptr<Npp32f>(), static_cast<int>(src1.step), src2.ptr<Npp32f>(), static_cast<int>(src2.step), 
            dst.ptr<Npp32f>(), static_cast<int>(dst.step), sz) );
        break;
    default:
        CV_Assert(!"Unsupported source type");
    }

    if (stream == 0)
        cudaSafeCall( cudaDeviceSynchronize() );
}

void cv::gpu::absdiff(const GpuMat& src1, const Scalar& src2, GpuMat& dst, Stream& s)
{
    CV_Assert(src1.type() == CV_32FC1);

    dst.create( src1.size(), src1.type() );

    NppiSize sz;
    sz.width  = src1.cols;
    sz.height = src1.rows;

    cudaStream_t stream = StreamAccessor::getStream(s);

    NppStreamHandler h(stream);

    nppSafeCall( nppiAbsDiffC_32f_C1R(src1.ptr<Npp32f>(), static_cast<int>(src1.step), dst.ptr<Npp32f>(), static_cast<int>(dst.step), sz, (Npp32f)src2[0]) );

    if (stream == 0)
        cudaSafeCall( cudaDeviceSynchronize() );
}


//////////////////////////////////////////////////////////////////////////////
// Comparison of two matrixes

namespace cv { namespace gpu { namespace mathfunc
{
    void compare_ne_8uc4(const DevMem2D& src1, const DevMem2D& src2, const DevMem2D& dst, cudaStream_t stream);
    void compare_ne_32f(const DevMem2D& src1, const DevMem2D& src2, const DevMem2D& dst, cudaStream_t stream);
}}}

void cv::gpu::compare(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, int cmpop, Stream& s)
{
    CV_DbgAssert(src1.size() == src2.size() && src1.type() == src2.type());

    CV_Assert(src1.type() == CV_8UC4 || src1.type() == CV_32FC1);

    dst.create( src1.size(), CV_8UC1 );

    static const NppCmpOp nppCmpOp[] = { NPP_CMP_EQ, NPP_CMP_GREATER, NPP_CMP_GREATER_EQ, NPP_CMP_LESS, NPP_CMP_LESS_EQ };

    NppiSize sz;
    sz.width  = src1.cols;
    sz.height = src1.rows;

    cudaStream_t stream = StreamAccessor::getStream(s);

    if (src1.type() == CV_8UC4)
    {
        if (cmpop != CMP_NE)
        {
            NppStreamHandler h(stream);

            nppSafeCall( nppiCompare_8u_C4R(src1.ptr<Npp8u>(), static_cast<int>(src1.step),
                src2.ptr<Npp8u>(), static_cast<int>(src2.step),
                dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz, nppCmpOp[cmpop]) );

            if (stream == 0)
                cudaSafeCall( cudaDeviceSynchronize() );
        }
        else
        {
            mathfunc::compare_ne_8uc4(src1, src2, dst, stream);
        }
    }
    else
    {
        if (cmpop != CMP_NE)
        {
            NppStreamHandler h(stream);

            nppSafeCall( nppiCompare_32f_C1R(src1.ptr<Npp32f>(), static_cast<int>(src1.step),
                src2.ptr<Npp32f>(), static_cast<int>(src2.step),
                dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz, nppCmpOp[cmpop]) );

            if (stream == 0)
                cudaSafeCall( cudaDeviceSynchronize() );
        }
        else
        {
            mathfunc::compare_ne_32f(src1, src2, dst, stream);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// Unary bitwise logical operations

namespace cv { namespace gpu { namespace mathfunc
{
    void bitwiseNotCaller(int rows, int cols, size_t elem_size1, int cn, const PtrStep src, PtrStep dst, cudaStream_t stream);

    template <typename T>
    void bitwiseMaskNotCaller(int rows, int cols, int cn, const PtrStep src, const PtrStep mask, PtrStep dst, cudaStream_t stream);
}}}

namespace
{
    void bitwiseNotCaller(const GpuMat& src, GpuMat& dst, cudaStream_t stream)
    {
        dst.create(src.size(), src.type());

        cv::gpu::mathfunc::bitwiseNotCaller(src.rows, src.cols, src.elemSize1(), 
                                              dst.channels(), src, dst, stream);
    }


    void bitwiseNotCaller(const GpuMat& src, GpuMat& dst, const GpuMat& mask, cudaStream_t stream)
    {
        using namespace cv::gpu;

        typedef void (*Caller)(int, int, int, const PtrStep, const PtrStep, PtrStep, cudaStream_t);
        static Caller callers[] = {mathfunc::bitwiseMaskNotCaller<unsigned char>, mathfunc::bitwiseMaskNotCaller<unsigned char>, 
                                   mathfunc::bitwiseMaskNotCaller<unsigned short>, mathfunc::bitwiseMaskNotCaller<unsigned short>,
                                   mathfunc::bitwiseMaskNotCaller<unsigned int>, mathfunc::bitwiseMaskNotCaller<unsigned int>,
                                   mathfunc::bitwiseMaskNotCaller<unsigned int>};

        CV_Assert(mask.type() == CV_8U && mask.size() == src.size());
        dst.create(src.size(), src.type());

        Caller caller = callers[src.depth()];
        CV_Assert(caller);

        int cn = src.depth() != CV_64F ? src.channels() : src.channels() * (sizeof(double) / sizeof(unsigned int));
        caller(src.rows, src.cols, cn, src, mask, dst, stream);
    }

}


void cv::gpu::bitwise_not(const GpuMat& src, GpuMat& dst, const GpuMat& mask, Stream& stream)
{
    if (mask.empty())
        ::bitwiseNotCaller(src, dst, StreamAccessor::getStream(stream));
    else
        ::bitwiseNotCaller(src, dst, mask, StreamAccessor::getStream(stream));
}


//////////////////////////////////////////////////////////////////////////////
// Binary bitwise logical operations

namespace cv { namespace gpu { namespace mathfunc
{
    void bitwiseOrCaller(int rows, int cols, size_t elem_size1, int cn, const PtrStep src1, const PtrStep src2, PtrStep dst, cudaStream_t stream);

    template <typename T>
    void bitwiseMaskOrCaller(int rows, int cols, int cn, const PtrStep src1, const PtrStep src2, const PtrStep mask, PtrStep dst, cudaStream_t stream);

    void bitwiseAndCaller(int rows, int cols, size_t elem_size1, int cn, const PtrStep src1, const PtrStep src2, PtrStep dst, cudaStream_t stream);

    template <typename T>
    void bitwiseMaskAndCaller(int rows, int cols, int cn, const PtrStep src1, const PtrStep src2, const PtrStep mask, PtrStep dst, cudaStream_t stream);

    void bitwiseXorCaller(int rows, int cols, size_t elem_size1, int cn, const PtrStep src1, const PtrStep src2, PtrStep dst, cudaStream_t stream);

    template <typename T>
    void bitwiseMaskXorCaller(int rows, int cols, int cn, const PtrStep src1, const PtrStep src2, const PtrStep mask, PtrStep dst, cudaStream_t stream);
}}}


namespace
{
    void bitwiseOrCaller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, cudaStream_t stream)
    {
        CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
        dst.create(src1.size(), src1.type());

        cv::gpu::mathfunc::bitwiseOrCaller(dst.rows, dst.cols, dst.elemSize1(), 
                                             dst.channels(), src1, src2, dst, stream);
    }


    void bitwiseOrCaller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, const GpuMat& mask, cudaStream_t stream)
    {
        using namespace cv::gpu;

        typedef void (*Caller)(int, int, int, const PtrStep, const PtrStep, const PtrStep, PtrStep, cudaStream_t);
        static Caller callers[] = {mathfunc::bitwiseMaskOrCaller<unsigned char>, mathfunc::bitwiseMaskOrCaller<unsigned char>, 
                                   mathfunc::bitwiseMaskOrCaller<unsigned short>, mathfunc::bitwiseMaskOrCaller<unsigned short>,
                                   mathfunc::bitwiseMaskOrCaller<unsigned int>, mathfunc::bitwiseMaskOrCaller<unsigned int>,
                                   mathfunc::bitwiseMaskOrCaller<unsigned int>};

        CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
        dst.create(src1.size(), src1.type());

        Caller caller = callers[src1.depth()];
        CV_Assert(caller);

        int cn = dst.depth() != CV_64F ? dst.channels() : dst.channels() * (sizeof(double) / sizeof(unsigned int));
        caller(dst.rows, dst.cols, cn, src1, src2, mask, dst, stream);
    }


    void bitwiseAndCaller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, cudaStream_t stream)
    {
        CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
        dst.create(src1.size(), src1.type());

        cv::gpu::mathfunc::bitwiseAndCaller(dst.rows, dst.cols, dst.elemSize1(), 
                                              dst.channels(), src1, src2, dst, stream);
    }


    void bitwiseAndCaller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, const GpuMat& mask, cudaStream_t stream)
    {
        using namespace cv::gpu;

        typedef void (*Caller)(int, int, int, const PtrStep, const PtrStep, const PtrStep, PtrStep, cudaStream_t);
        static Caller callers[] = {mathfunc::bitwiseMaskAndCaller<unsigned char>, mathfunc::bitwiseMaskAndCaller<unsigned char>, 
                                   mathfunc::bitwiseMaskAndCaller<unsigned short>, mathfunc::bitwiseMaskAndCaller<unsigned short>,
                                   mathfunc::bitwiseMaskAndCaller<unsigned int>, mathfunc::bitwiseMaskAndCaller<unsigned int>,
                                   mathfunc::bitwiseMaskAndCaller<unsigned int>};

        CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
        dst.create(src1.size(), src1.type());

        Caller caller = callers[src1.depth()];
        CV_Assert(caller);

        int cn = dst.depth() != CV_64F ? dst.channels() : dst.channels() * (sizeof(double) / sizeof(unsigned int));
        caller(dst.rows, dst.cols, cn, src1, src2, mask, dst, stream);
    }


    void bitwiseXorCaller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, cudaStream_t stream)
    {
        CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
        dst.create(src1.size(), src1.type());

        cv::gpu::mathfunc::bitwiseXorCaller(dst.rows, dst.cols, dst.elemSize1(), 
                                              dst.channels(), src1, src2, dst, stream);
    }


    void bitwiseXorCaller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, const GpuMat& mask, cudaStream_t stream)
    {
        using namespace cv::gpu;

        typedef void (*Caller)(int, int, int, const PtrStep, const PtrStep, const PtrStep, PtrStep, cudaStream_t);
        static Caller callers[] = {mathfunc::bitwiseMaskXorCaller<unsigned char>, mathfunc::bitwiseMaskXorCaller<unsigned char>, 
                                   mathfunc::bitwiseMaskXorCaller<unsigned short>, mathfunc::bitwiseMaskXorCaller<unsigned short>,
                                   mathfunc::bitwiseMaskXorCaller<unsigned int>, mathfunc::bitwiseMaskXorCaller<unsigned int>,
                                   mathfunc::bitwiseMaskXorCaller<unsigned int>};

        CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
        dst.create(src1.size(), src1.type());

        Caller caller = callers[src1.depth()];
        CV_Assert(caller);

        int cn = dst.depth() != CV_64F ? dst.channels() : dst.channels() * (sizeof(double) / sizeof(unsigned int));
        caller(dst.rows, dst.cols, cn, src1, src2, mask, dst, stream);
    }
}


void cv::gpu::bitwise_or(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, const GpuMat& mask, Stream& stream)
{
    if (mask.empty())
        ::bitwiseOrCaller(src1, src2, dst, StreamAccessor::getStream(stream));
    else
        ::bitwiseOrCaller(src1, src2, dst, mask, StreamAccessor::getStream(stream));
}


void cv::gpu::bitwise_and(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, const GpuMat& mask, Stream& stream)
{
    if (mask.empty())
        ::bitwiseAndCaller(src1, src2, dst, StreamAccessor::getStream(stream));
    else
        ::bitwiseAndCaller(src1, src2, dst, mask, StreamAccessor::getStream(stream));
}


void cv::gpu::bitwise_xor(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, const GpuMat& mask, Stream& stream)
{
    if (mask.empty())
        ::bitwiseXorCaller(src1, src2, dst, StreamAccessor::getStream(stream));
    else
        ::bitwiseXorCaller(src1, src2, dst, mask, StreamAccessor::getStream(stream));
}


//////////////////////////////////////////////////////////////////////////////
// Minimum and maximum operations

namespace cv { namespace gpu { namespace mathfunc
{
    template <typename T>
    void min_gpu(const DevMem2D_<T>& src1, const DevMem2D_<T>& src2, const DevMem2D_<T>& dst, cudaStream_t stream);

    template <typename T>
    void max_gpu(const DevMem2D_<T>& src1, const DevMem2D_<T>& src2, const DevMem2D_<T>& dst, cudaStream_t stream);

    template <typename T>
    void min_gpu(const DevMem2D_<T>& src1, T src2, const DevMem2D_<T>& dst, cudaStream_t stream);

    template <typename T>
    void max_gpu(const DevMem2D_<T>& src1, T src2, const DevMem2D_<T>& dst, cudaStream_t stream);
}}}

namespace
{
    template <typename T>
    void min_caller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, cudaStream_t stream)
    {
        CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
        dst.create(src1.size(), src1.type());
        mathfunc::min_gpu<T>(src1.reshape(1), src2.reshape(1), dst.reshape(1), stream);
    }

    template <typename T>
    void min_caller(const GpuMat& src1, double src2, GpuMat& dst, cudaStream_t stream)
    {
        dst.create(src1.size(), src1.type());
        mathfunc::min_gpu<T>(src1.reshape(1), saturate_cast<T>(src2), dst.reshape(1), stream);
    }
    
    template <typename T>
    void max_caller(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, cudaStream_t stream)
    {
        CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
        dst.create(src1.size(), src1.type());
        mathfunc::max_gpu<T>(src1.reshape(1), src2.reshape(1), dst.reshape(1), stream);
    }

    template <typename T>
    void max_caller(const GpuMat& src1, double src2, GpuMat& dst, cudaStream_t stream)
    {
        dst.create(src1.size(), src1.type());
        mathfunc::max_gpu<T>(src1.reshape(1), saturate_cast<T>(src2), dst.reshape(1), stream);
    }
}

void cv::gpu::min(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, Stream& stream) 
{ 
    CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
    CV_Assert((src1.depth() != CV_64F) || 
        (TargetArchs::builtWith(NATIVE_DOUBLE) && DeviceInfo().supports(NATIVE_DOUBLE)));

    typedef void (*func_t)(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, cudaStream_t stream);
    static const func_t funcs[] = 
    {
        min_caller<uchar>, min_caller<schar>, min_caller<ushort>, min_caller<short>, min_caller<int>, 
        min_caller<float>, min_caller<double>
    };
    funcs[src1.depth()](src1, src2, dst, StreamAccessor::getStream(stream));
}
void cv::gpu::min(const GpuMat& src1, double src2, GpuMat& dst, Stream& stream) 
{
    CV_Assert((src1.depth() != CV_64F) || 
        (TargetArchs::builtWith(NATIVE_DOUBLE) && DeviceInfo().supports(NATIVE_DOUBLE)));

    typedef void (*func_t)(const GpuMat& src1, double src2, GpuMat& dst, cudaStream_t stream);
    static const func_t funcs[] = 
    {
        min_caller<uchar>, min_caller<schar>, min_caller<ushort>, min_caller<short>, min_caller<int>, 
        min_caller<float>, min_caller<double>
    };
    funcs[src1.depth()](src1, src2, dst, StreamAccessor::getStream(stream));
}

void cv::gpu::max(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, Stream& stream) 
{ 
    CV_Assert(src1.size() == src2.size() && src1.type() == src2.type());
    CV_Assert((src1.depth() != CV_64F) || 
        (TargetArchs::builtWith(NATIVE_DOUBLE) && DeviceInfo().supports(NATIVE_DOUBLE)));

    typedef void (*func_t)(const GpuMat& src1, const GpuMat& src2, GpuMat& dst, cudaStream_t stream);
    static const func_t funcs[] = 
    {
        max_caller<uchar>, max_caller<schar>, max_caller<ushort>, max_caller<short>, max_caller<int>, 
        max_caller<float>, max_caller<double>
    };
    funcs[src1.depth()](src1, src2, dst, StreamAccessor::getStream(stream));
}

void cv::gpu::max(const GpuMat& src1, double src2, GpuMat& dst, Stream& stream) 
{
    CV_Assert((src1.depth() != CV_64F) || 
        (TargetArchs::builtWith(NATIVE_DOUBLE) && DeviceInfo().supports(NATIVE_DOUBLE)));

    typedef void (*func_t)(const GpuMat& src1, double src2, GpuMat& dst, cudaStream_t stream);
    static const func_t funcs[] = 
    {
        max_caller<uchar>, max_caller<schar>, max_caller<ushort>, max_caller<short>, max_caller<int>, 
        max_caller<float>, max_caller<double>
    };
    funcs[src1.depth()](src1, src2, dst, StreamAccessor::getStream(stream));
}

////////////////////////////////////////////////////////////////////////
// threshold

namespace cv { namespace gpu { namespace mathfunc
{
    template <typename T>
    void threshold_gpu(const DevMem2D& src, const DevMem2D& dst, T thresh, T maxVal, int type,
        cudaStream_t stream);
}}}

namespace
{
    template <typename T>
    void threshold_caller(const GpuMat& src, GpuMat& dst, double thresh, double maxVal, int type, 
        cudaStream_t stream)
    {
        mathfunc::threshold_gpu<T>(src, dst, saturate_cast<T>(thresh), saturate_cast<T>(maxVal), type, stream);
    }
}

double cv::gpu::threshold(const GpuMat& src, GpuMat& dst, double thresh, double maxVal, int type, Stream& s)
{
    cudaStream_t stream = StreamAccessor::getStream(s);

    if (src.type() == CV_32FC1 && type == THRESH_TRUNC)
    {
        NppStreamHandler h(stream);

        dst.create(src.size(), src.type());

        NppiSize sz;
        sz.width  = src.cols;
        sz.height = src.rows;

        nppSafeCall( nppiThreshold_32f_C1R(src.ptr<Npp32f>(), static_cast<int>(src.step),
            dst.ptr<Npp32f>(), static_cast<int>(dst.step), sz, static_cast<Npp32f>(thresh), NPP_CMP_GREATER) );

        if (stream == 0)
            cudaSafeCall( cudaDeviceSynchronize() );
    }
    else
    {
        CV_Assert((src.depth() != CV_64F) || 
            (TargetArchs::builtWith(NATIVE_DOUBLE) && DeviceInfo().supports(NATIVE_DOUBLE)));

        typedef void (*caller_t)(const GpuMat& src, GpuMat& dst, double thresh, double maxVal, int type, 
            cudaStream_t stream);

        static const caller_t callers[] = 
        {
            threshold_caller<unsigned char>, threshold_caller<signed char>, 
            threshold_caller<unsigned short>, threshold_caller<short>, 
            threshold_caller<int>, threshold_caller<float>, threshold_caller<double>
        };

        CV_Assert(src.channels() == 1 && src.depth() <= CV_64F);
        CV_Assert(type <= THRESH_TOZERO_INV);

        dst.create(src.size(), src.type());

        if (src.depth() != CV_32F)
        {
            thresh = cvFloor(thresh);
            maxVal = cvRound(maxVal);
        }

        callers[src.depth()](src, dst, thresh, maxVal, type, stream);
    }

    return thresh;
}

////////////////////////////////////////////////////////////////////////
// pow

namespace cv
{
    namespace gpu
    {
        namespace mathfunc
        {
            template<typename T>
            void pow_caller(const DevMem2D& src, float power, DevMem2D dst, cudaStream_t stream);
        }
    }
}

void cv::gpu::pow(const GpuMat& src, double power, GpuMat& dst, Stream& stream)
{    
    CV_Assert( src.depth() != CV_64F );
    dst.create(src.size(), src.type());

    typedef void (*caller_t)(const DevMem2D& src, float power, DevMem2D dst, cudaStream_t stream);

    static const caller_t callers[] = 
    {
        mathfunc::pow_caller<unsigned char>,  mathfunc::pow_caller<signed char>, 
        mathfunc::pow_caller<unsigned short>, mathfunc::pow_caller<short>, 
        mathfunc::pow_caller<int>, mathfunc::pow_caller<float>
    };

    callers[src.depth()](src.reshape(1), (float)power, dst.reshape(1), StreamAccessor::getStream(stream));    
}

#endif
