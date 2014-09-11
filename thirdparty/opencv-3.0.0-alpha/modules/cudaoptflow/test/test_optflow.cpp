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

#include "test_precomp.hpp"

#ifdef HAVE_CUDA

using namespace cvtest;

//////////////////////////////////////////////////////
// BroxOpticalFlow

//#define BROX_DUMP

struct BroxOpticalFlow : testing::TestWithParam<cv::cuda::DeviceInfo>
{
    cv::cuda::DeviceInfo devInfo;

    virtual void SetUp()
    {
        devInfo = GetParam();

        cv::cuda::setDevice(devInfo.deviceID());
    }
};

CUDA_TEST_P(BroxOpticalFlow, Regression)
{
    cv::Mat frame0 = readImageType("opticalflow/frame0.png", CV_32FC1);
    ASSERT_FALSE(frame0.empty());

    cv::Mat frame1 = readImageType("opticalflow/frame1.png", CV_32FC1);
    ASSERT_FALSE(frame1.empty());

    cv::cuda::BroxOpticalFlow brox(0.197f /*alpha*/, 50.0f /*gamma*/, 0.8f /*scale_factor*/,
                                  10 /*inner_iterations*/, 77 /*outer_iterations*/, 10 /*solver_iterations*/);

    cv::cuda::GpuMat u;
    cv::cuda::GpuMat v;
    brox(loadMat(frame0), loadMat(frame1), u, v);

    std::string fname(cvtest::TS::ptr()->get_data_path());
    if (devInfo.majorVersion() >= 2)
        fname += "opticalflow/brox_optical_flow_cc20.bin";
    else
        fname += "opticalflow/brox_optical_flow.bin";

#ifndef BROX_DUMP
    std::ifstream f(fname.c_str(), std::ios_base::binary);

    int rows, cols;

    f.read((char*) &rows, sizeof(rows));
    f.read((char*) &cols, sizeof(cols));

    cv::Mat u_gold(rows, cols, CV_32FC1);

    for (int i = 0; i < u_gold.rows; ++i)
        f.read(u_gold.ptr<char>(i), u_gold.cols * sizeof(float));

    cv::Mat v_gold(rows, cols, CV_32FC1);

    for (int i = 0; i < v_gold.rows; ++i)
        f.read(v_gold.ptr<char>(i), v_gold.cols * sizeof(float));

    EXPECT_MAT_SIMILAR(u_gold, u, 1e-3);
    EXPECT_MAT_SIMILAR(v_gold, v, 1e-3);
#else
    std::ofstream f(fname.c_str(), std::ios_base::binary);

    f.write((char*) &u.rows, sizeof(u.rows));
    f.write((char*) &u.cols, sizeof(u.cols));

    cv::Mat h_u(u);
    cv::Mat h_v(v);

    for (int i = 0; i < u.rows; ++i)
        f.write(h_u.ptr<char>(i), u.cols * sizeof(float));

    for (int i = 0; i < v.rows; ++i)
        f.write(h_v.ptr<char>(i), v.cols * sizeof(float));
#endif
}

CUDA_TEST_P(BroxOpticalFlow, OpticalFlowNan)
{
    cv::Mat frame0 = readImageType("opticalflow/frame0.png", CV_32FC1);
    ASSERT_FALSE(frame0.empty());

    cv::Mat frame1 = readImageType("opticalflow/frame1.png", CV_32FC1);
    ASSERT_FALSE(frame1.empty());

    cv::Mat r_frame0, r_frame1;
    cv::resize(frame0, r_frame0, cv::Size(1380,1000));
    cv::resize(frame1, r_frame1, cv::Size(1380,1000));

    cv::cuda::BroxOpticalFlow brox(0.197f /*alpha*/, 50.0f /*gamma*/, 0.8f /*scale_factor*/,
                                  5 /*inner_iterations*/, 150 /*outer_iterations*/, 10 /*solver_iterations*/);

    cv::cuda::GpuMat u;
    cv::cuda::GpuMat v;
    brox(loadMat(r_frame0), loadMat(r_frame1), u, v);

    cv::Mat h_u, h_v;
    u.download(h_u);
    v.download(h_v);

    EXPECT_TRUE(cv::checkRange(h_u));
    EXPECT_TRUE(cv::checkRange(h_v));
};

INSTANTIATE_TEST_CASE_P(CUDA_OptFlow, BroxOpticalFlow, ALL_DEVICES);

//////////////////////////////////////////////////////
// PyrLKOpticalFlow

namespace
{
    IMPLEMENT_PARAM_CLASS(UseGray, bool)
}

PARAM_TEST_CASE(PyrLKOpticalFlow, cv::cuda::DeviceInfo, UseGray)
{
    cv::cuda::DeviceInfo devInfo;
    bool useGray;

    virtual void SetUp()
    {
        devInfo = GET_PARAM(0);
        useGray = GET_PARAM(1);

        cv::cuda::setDevice(devInfo.deviceID());
    }
};

CUDA_TEST_P(PyrLKOpticalFlow, Sparse)
{
    cv::Mat frame0 = readImage("opticalflow/frame0.png", useGray ? cv::IMREAD_GRAYSCALE : cv::IMREAD_COLOR);
    ASSERT_FALSE(frame0.empty());

    cv::Mat frame1 = readImage("opticalflow/frame1.png", useGray ? cv::IMREAD_GRAYSCALE : cv::IMREAD_COLOR);
    ASSERT_FALSE(frame1.empty());

    cv::Mat gray_frame;
    if (useGray)
        gray_frame = frame0;
    else
        cv::cvtColor(frame0, gray_frame, cv::COLOR_BGR2GRAY);

    std::vector<cv::Point2f> pts;
    cv::goodFeaturesToTrack(gray_frame, pts, 1000, 0.01, 0.0);

    cv::cuda::GpuMat d_pts;
    cv::Mat pts_mat(1, (int) pts.size(), CV_32FC2, (void*) &pts[0]);
    d_pts.upload(pts_mat);

    cv::cuda::PyrLKOpticalFlow pyrLK;

    cv::cuda::GpuMat d_nextPts;
    cv::cuda::GpuMat d_status;
    pyrLK.sparse(loadMat(frame0), loadMat(frame1), d_pts, d_nextPts, d_status);

    std::vector<cv::Point2f> nextPts(d_nextPts.cols);
    cv::Mat nextPts_mat(1, d_nextPts.cols, CV_32FC2, (void*) &nextPts[0]);
    d_nextPts.download(nextPts_mat);

    std::vector<unsigned char> status(d_status.cols);
    cv::Mat status_mat(1, d_status.cols, CV_8UC1, (void*) &status[0]);
    d_status.download(status_mat);

    std::vector<cv::Point2f> nextPts_gold;
    std::vector<unsigned char> status_gold;
    cv::calcOpticalFlowPyrLK(frame0, frame1, pts, nextPts_gold, status_gold, cv::noArray());

    ASSERT_EQ(nextPts_gold.size(), nextPts.size());
    ASSERT_EQ(status_gold.size(), status.size());

    size_t mistmatch = 0;
    for (size_t i = 0; i < nextPts.size(); ++i)
    {
        cv::Point2i a = nextPts[i];
        cv::Point2i b = nextPts_gold[i];

        if (status[i] != status_gold[i])
        {
            ++mistmatch;
            continue;
        }

        if (status[i])
        {
            bool eq = std::abs(a.x - b.x) <= 1 && std::abs(a.y - b.y) <= 1;

            if (!eq)
                ++mistmatch;
        }
    }

    double bad_ratio = static_cast<double>(mistmatch) / nextPts.size();

    ASSERT_LE(bad_ratio, 0.01);
}

INSTANTIATE_TEST_CASE_P(CUDA_OptFlow, PyrLKOpticalFlow, testing::Combine(
    ALL_DEVICES,
    testing::Values(UseGray(true), UseGray(false))));

//////////////////////////////////////////////////////
// FarnebackOpticalFlow

namespace
{
    IMPLEMENT_PARAM_CLASS(PyrScale, double)
    IMPLEMENT_PARAM_CLASS(PolyN, int)
    CV_FLAGS(FarnebackOptFlowFlags, 0, OPTFLOW_FARNEBACK_GAUSSIAN)
    IMPLEMENT_PARAM_CLASS(UseInitFlow, bool)
}

PARAM_TEST_CASE(FarnebackOpticalFlow, cv::cuda::DeviceInfo, PyrScale, PolyN, FarnebackOptFlowFlags, UseInitFlow)
{
    cv::cuda::DeviceInfo devInfo;
    double pyrScale;
    int polyN;
    int flags;
    bool useInitFlow;

    virtual void SetUp()
    {
        devInfo = GET_PARAM(0);
        pyrScale = GET_PARAM(1);
        polyN = GET_PARAM(2);
        flags = GET_PARAM(3);
        useInitFlow = GET_PARAM(4);

        cv::cuda::setDevice(devInfo.deviceID());
    }
};

CUDA_TEST_P(FarnebackOpticalFlow, Accuracy)
{
    cv::Mat frame0 = readImage("opticalflow/rubberwhale1.png", cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(frame0.empty());

    cv::Mat frame1 = readImage("opticalflow/rubberwhale2.png", cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(frame1.empty());

    double polySigma = polyN <= 5 ? 1.1 : 1.5;

    cv::cuda::FarnebackOpticalFlow farn;
    farn.pyrScale = pyrScale;
    farn.polyN = polyN;
    farn.polySigma = polySigma;
    farn.flags = flags;

    cv::cuda::GpuMat d_flowx, d_flowy;
    farn(loadMat(frame0), loadMat(frame1), d_flowx, d_flowy);

    cv::Mat flow;
    if (useInitFlow)
    {
        cv::Mat flowxy[] = {cv::Mat(d_flowx), cv::Mat(d_flowy)};
        cv::merge(flowxy, 2, flow);

        farn.flags |= cv::OPTFLOW_USE_INITIAL_FLOW;
        farn(loadMat(frame0), loadMat(frame1), d_flowx, d_flowy);
    }

    cv::calcOpticalFlowFarneback(
        frame0, frame1, flow, farn.pyrScale, farn.numLevels, farn.winSize,
        farn.numIters, farn.polyN, farn.polySigma, farn.flags);

    std::vector<cv::Mat> flowxy;
    cv::split(flow, flowxy);

    EXPECT_MAT_SIMILAR(flowxy[0], d_flowx, 0.1);
    EXPECT_MAT_SIMILAR(flowxy[1], d_flowy, 0.1);
}

INSTANTIATE_TEST_CASE_P(CUDA_OptFlow, FarnebackOpticalFlow, testing::Combine(
    ALL_DEVICES,
    testing::Values(PyrScale(0.3), PyrScale(0.5), PyrScale(0.8)),
    testing::Values(PolyN(5), PolyN(7)),
    testing::Values(FarnebackOptFlowFlags(0), FarnebackOptFlowFlags(cv::OPTFLOW_FARNEBACK_GAUSSIAN)),
    testing::Values(UseInitFlow(false), UseInitFlow(true))));

//////////////////////////////////////////////////////
// OpticalFlowDual_TVL1

PARAM_TEST_CASE(OpticalFlowDual_TVL1, cv::cuda::DeviceInfo, UseRoi)
{
    cv::cuda::DeviceInfo devInfo;
    bool useRoi;

    virtual void SetUp()
    {
        devInfo = GET_PARAM(0);
        useRoi = GET_PARAM(1);

        cv::cuda::setDevice(devInfo.deviceID());
    }
};

CUDA_TEST_P(OpticalFlowDual_TVL1, Accuracy)
{
    cv::Mat frame0 = readImage("opticalflow/rubberwhale1.png", cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(frame0.empty());

    cv::Mat frame1 = readImage("opticalflow/rubberwhale2.png", cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(frame1.empty());

    cv::cuda::OpticalFlowDual_TVL1_CUDA d_alg;
    cv::cuda::GpuMat d_flowx = createMat(frame0.size(), CV_32FC1, useRoi);
    cv::cuda::GpuMat d_flowy = createMat(frame0.size(), CV_32FC1, useRoi);
    d_alg(loadMat(frame0, useRoi), loadMat(frame1, useRoi), d_flowx, d_flowy);

    cv::Ptr<cv::DenseOpticalFlow> alg = cv::createOptFlow_DualTVL1();
    alg->set("medianFiltering", 1);
    alg->set("innerIterations", 1);
    alg->set("outerIterations", d_alg.iterations);
    cv::Mat flow;
    alg->calc(frame0, frame1, flow);
    cv::Mat gold[2];
    cv::split(flow, gold);

    EXPECT_MAT_SIMILAR(gold[0], d_flowx, 4e-3);
    EXPECT_MAT_SIMILAR(gold[1], d_flowy, 4e-3);
}

INSTANTIATE_TEST_CASE_P(CUDA_OptFlow, OpticalFlowDual_TVL1, testing::Combine(
    ALL_DEVICES,
    WHOLE_SUBMAT));

//////////////////////////////////////////////////////
// FastOpticalFlowBM

namespace
{
    void FastOpticalFlowBM_gold(const cv::Mat_<uchar>& I0, const cv::Mat_<uchar>& I1, cv::Mat_<float>& velx, cv::Mat_<float>& vely, int search_window, int block_window)
    {
        velx.create(I0.size());
        vely.create(I0.size());

        int search_radius = search_window / 2;
        int block_radius = block_window / 2;

        for (int y = 0; y < I0.rows; ++y)
        {
            for (int x = 0; x < I0.cols; ++x)
            {
                int bestDist = std::numeric_limits<int>::max();
                int bestDx = 0;
                int bestDy = 0;

                for (int dy = -search_radius; dy <= search_radius; ++dy)
                {
                    for (int dx = -search_radius; dx <= search_radius; ++dx)
                    {
                        int dist = 0;

                        for (int by = -block_radius; by <= block_radius; ++by)
                        {
                            for (int bx = -block_radius; bx <= block_radius; ++bx)
                            {
                                int I0_val = I0(cv::borderInterpolate(y + by, I0.rows, cv::BORDER_DEFAULT), cv::borderInterpolate(x + bx, I0.cols, cv::BORDER_DEFAULT));
                                int I1_val = I1(cv::borderInterpolate(y + dy + by, I0.rows, cv::BORDER_DEFAULT), cv::borderInterpolate(x + dx + bx, I0.cols, cv::BORDER_DEFAULT));

                                dist += std::abs(I0_val - I1_val);
                            }
                        }

                        if (dist < bestDist)
                        {
                            bestDist = dist;
                            bestDx = dx;
                            bestDy = dy;
                        }
                    }
                }

                velx(y, x) = (float) bestDx;
                vely(y, x) = (float) bestDy;
            }
        }
    }

    double calc_rmse(const cv::Mat_<float>& flow1, const cv::Mat_<float>& flow2)
    {
        double sum = 0.0;

        for (int y = 0; y < flow1.rows; ++y)
        {
            for (int x = 0; x < flow1.cols; ++x)
            {
                double diff = flow1(y, x) - flow2(y, x);
                sum += diff * diff;
            }
        }

        return std::sqrt(sum / flow1.size().area());
    }
}

struct FastOpticalFlowBM : testing::TestWithParam<cv::cuda::DeviceInfo>
{
};

CUDA_TEST_P(FastOpticalFlowBM, Accuracy)
{
    const double MAX_RMSE = 0.6;

    int search_window = 15;
    int block_window = 5;

    cv::cuda::DeviceInfo devInfo = GetParam();
    cv::cuda::setDevice(devInfo.deviceID());

    cv::Mat frame0 = readImage("opticalflow/rubberwhale1.png", cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(frame0.empty());

    cv::Mat frame1 = readImage("opticalflow/rubberwhale2.png", cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(frame1.empty());

    cv::Size smallSize(320, 240);
    cv::Mat frame0_small;
    cv::Mat frame1_small;

    cv::resize(frame0, frame0_small, smallSize);
    cv::resize(frame1, frame1_small, smallSize);

    cv::cuda::GpuMat d_flowx;
    cv::cuda::GpuMat d_flowy;
    cv::cuda::FastOpticalFlowBM fastBM;

    fastBM(loadMat(frame0_small), loadMat(frame1_small), d_flowx, d_flowy, search_window, block_window);

    cv::Mat_<float> flowx;
    cv::Mat_<float> flowy;
    FastOpticalFlowBM_gold(frame0_small, frame1_small, flowx, flowy, search_window, block_window);

    double err;

    err = calc_rmse(flowx, cv::Mat(d_flowx));
    EXPECT_LE(err, MAX_RMSE);

    err = calc_rmse(flowy, cv::Mat(d_flowy));
    EXPECT_LE(err, MAX_RMSE);
}

INSTANTIATE_TEST_CASE_P(CUDA_OptFlow, FastOpticalFlowBM, ALL_DEVICES);

#endif // HAVE_CUDA
