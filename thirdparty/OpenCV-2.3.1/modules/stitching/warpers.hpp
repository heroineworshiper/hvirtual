 /*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
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
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
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
#ifndef __OPENCV_WARPERS_HPP__
#define __OPENCV_WARPERS_HPP__

#include "precomp.hpp"

class Warper
{
public:
    enum { PLANE, CYLINDRICAL, SPHERICAL };
    static cv::Ptr<Warper> createByCameraFocal(float focal, int type, bool try_gpu = false);

    virtual ~Warper() {}
    virtual cv::Point warp(const cv::Mat &src, float focal, const cv::Mat& R, cv::Mat &dst,
                           int interp_mode = cv::INTER_LINEAR, int border_mode = cv::BORDER_REFLECT) = 0;
    virtual cv::Rect warpRoi(const cv::Size &sz, float focal, const cv::Mat &R) = 0;
};


struct ProjectorBase
{
    void setTransformation(const cv::Mat& R);

    cv::Size size;
    float focal;
    float r[9];
    float rinv[9];
    float scale;
};


template <class P>
class WarperBase : public Warper
{   
public:
    virtual cv::Point warp(const cv::Mat &src, float focal, const cv::Mat &R, cv::Mat &dst,
                           int interp_mode, int border_mode);

    virtual cv::Rect warpRoi(const cv::Size &sz, float focal, const cv::Mat &R);

protected:
    // Detects ROI of the destination image. It's correct for any projection.
    virtual void detectResultRoi(cv::Point &dst_tl, cv::Point &dst_br);

    // Detects ROI of the destination image by walking over image border.
    // Correctness for any projection isn't guaranteed.
    void detectResultRoiByBorder(cv::Point &dst_tl, cv::Point &dst_br);

    cv::Size src_size_;
    P projector_;
};


struct PlaneProjector : ProjectorBase
{
    void mapForward(float x, float y, float &u, float &v);
    void mapBackward(float u, float v, float &x, float &y);
    float plane_dist;
};


// Projects image onto z = plane_dist plane
class PlaneWarper : public WarperBase<PlaneProjector>
{
public:
    PlaneWarper(float plane_dist = 1.f, float scale = 1.f)
    {
        projector_.plane_dist = plane_dist;
        projector_.scale = scale;
    }

protected:
    void detectResultRoi(cv::Point &dst_tl, cv::Point &dst_br);
};


class PlaneWarperGpu : public PlaneWarper
{
public:
    PlaneWarperGpu(float plane_dist = 1.f, float scale = 1.f) : PlaneWarper(plane_dist, scale) {}
    cv::Point warp(const cv::Mat &src, float focal, const cv::Mat &R, cv::Mat &dst,
                   int interp_mode, int border_mode);

private:
    cv::gpu::GpuMat d_xmap_, d_ymap_, d_dst_;
};


struct SphericalProjector : ProjectorBase
{
    void mapForward(float x, float y, float &u, float &v);
    void mapBackward(float u, float v, float &x, float &y);
};


// Projects image onto unit sphere with origin at (0, 0, 0).
// Poles are located at (0, -1, 0) and (0, 1, 0) points.
class SphericalWarper : public WarperBase<SphericalProjector>
{
public:
    SphericalWarper(float scale = 300.f) { projector_.scale = scale; }

protected:
    void detectResultRoi(cv::Point &dst_tl, cv::Point &dst_br);
};


class SphericalWarperGpu : public SphericalWarper
{
public:
    SphericalWarperGpu(float scale = 300.f) : SphericalWarper(scale) {}
    cv::Point warp(const cv::Mat &src, float focal, const cv::Mat &R, cv::Mat &dst,
                   int interp_mode, int border_mode);

private:
    cv::gpu::GpuMat d_xmap_, d_ymap_, d_dst_;
};


struct CylindricalProjector : ProjectorBase
{
    void mapForward(float x, float y, float &u, float &v);
    void mapBackward(float u, float v, float &x, float &y);
};


// Projects image onto x * x + z * z = 1 cylinder
class CylindricalWarper : public WarperBase<CylindricalProjector>
{
public:
    CylindricalWarper(float scale = 300.f) { projector_.scale = scale; }

protected:
    void detectResultRoi(cv::Point &dst_tl, cv::Point &dst_br)
    {
        WarperBase<CylindricalProjector>::detectResultRoiByBorder(dst_tl, dst_br);
    }
};


class CylindricalWarperGpu : public CylindricalWarper
{
public:
    CylindricalWarperGpu(float scale = 300.f) : CylindricalWarper(scale) {}
    cv::Point warp(const cv::Mat &src, float focal, const cv::Mat &R, cv::Mat &dst,
                   int interp_mode, int border_mode);

private:
    cv::gpu::GpuMat d_xmap_, d_ymap_, d_dst_;
};

#include "warpers_inl.hpp"

#endif // __OPENCV_WARPERS_HPP__
