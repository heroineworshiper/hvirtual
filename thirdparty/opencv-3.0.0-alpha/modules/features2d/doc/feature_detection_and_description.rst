Feature Detection and Description
=================================

.. highlight:: cpp

.. note::

   * An example explaining keypoint detection and description can be found at opencv_source_code/samples/cpp/descriptor_extractor_matcher.cpp

FAST
----
Detects corners using the FAST algorithm

.. ocv:function:: void FAST( InputArray image, vector<KeyPoint>& keypoints, int threshold, bool nonmaxSuppression=true )
.. ocv:function:: void FAST( InputArray image, vector<KeyPoint>& keypoints, int threshold, bool nonmaxSuppression, int type )

.. ocv:pyfunction:: cv2.FastFeatureDetector([, threshold[, nonmaxSuppression]]) -> <FastFeatureDetector object>
.. ocv:pyfunction:: cv2.FastFeatureDetector(threshold, nonmaxSuppression, type) -> <FastFeatureDetector object>
.. ocv:pyfunction:: cv2.FastFeatureDetector.detect(image[, mask]) -> keypoints


    :param image: grayscale image where keypoints (corners) are detected.

    :param keypoints: keypoints detected on the image.

    :param threshold: threshold on difference between intensity of the central pixel and pixels of a circle around this pixel.

    :param nonmaxSuppression: if true, non-maximum suppression is applied to detected corners (keypoints).

    :param type: one of the three neighborhoods as defined in the paper: ``FastFeatureDetector::TYPE_9_16``, ``FastFeatureDetector::TYPE_7_12``, ``FastFeatureDetector::TYPE_5_8``

Detects corners using the FAST algorithm by [Rosten06]_.

.. note:: In Python API, types are given as ``cv2.FAST_FEATURE_DETECTOR_TYPE_5_8``, ``cv2.FAST_FEATURE_DETECTOR_TYPE_7_12`` and  ``cv2.FAST_FEATURE_DETECTOR_TYPE_9_16``. For corner detection, use ``cv2.FAST.detect()`` method.


.. [Rosten06] E. Rosten. Machine Learning for High-speed Corner Detection, 2006.

MSER
----
.. ocv:class:: MSER : public FeatureDetector

Maximally stable extremal region extractor. ::

    class MSER : public CvMSERParams
    {
    public:
        // default constructor
        MSER();
        // constructor that initializes all the algorithm parameters
        MSER( int _delta, int _min_area, int _max_area,
              float _max_variation, float _min_diversity,
              int _max_evolution, double _area_threshold,
              double _min_margin, int _edge_blur_size );
        // runs the extractor on the specified image; returns the MSERs,
        // each encoded as a contour (vector<Point>, see findContours)
        // the optional mask marks the area where MSERs are searched for
        void operator()( const Mat& image, vector<vector<Point> >& msers, const Mat& mask ) const;
    };

The class encapsulates all the parameters of the MSER extraction algorithm (see
http://en.wikipedia.org/wiki/Maximally_stable_extremal_regions). Also see http://code.opencv.org/projects/opencv/wiki/MSER for useful comments and parameters description.

.. note::

   * (Python) A complete example showing the use of the MSER detector can be found at opencv_source_code/samples/python2/mser.py


ORB
---
.. ocv:class:: ORB : public Feature2D

Class implementing the ORB (*oriented BRIEF*) keypoint detector and descriptor extractor, described in [RRKB11]_. The algorithm uses FAST in pyramids to detect stable keypoints, selects the strongest features using FAST or Harris response, finds their orientation using first-order moments and computes the descriptors using BRIEF (where the coordinates of random point pairs (or k-tuples) are rotated according to the measured orientation).

.. [RRKB11] Ethan Rublee, Vincent Rabaud, Kurt Konolige, Gary R. Bradski: ORB: An efficient alternative to SIFT or SURF. ICCV 2011: 2564-2571.

ORB::ORB
--------
The ORB constructor

.. ocv:function:: ORB::ORB(int nfeatures = 500, float scaleFactor = 1.2f, int nlevels = 8, int edgeThreshold = 31, int firstLevel = 0, int WTA_K=2, int scoreType=ORB::HARRIS_SCORE, int patchSize=31)

.. ocv:pyfunction:: cv2.ORB([, nfeatures[, scaleFactor[, nlevels[, edgeThreshold[, firstLevel[, WTA_K[, scoreType[, patchSize]]]]]]]]) -> <ORB object>


    :param nfeatures: The maximum number of features to retain.

    :param scaleFactor: Pyramid decimation ratio, greater than 1. ``scaleFactor==2`` means the classical pyramid, where each next level has 4x less pixels than the previous, but such a big scale factor will degrade feature matching scores dramatically. On the other hand, too close to 1 scale factor will mean that to cover certain scale range you will need more pyramid levels and so the speed will suffer.

    :param nlevels: The number of pyramid levels. The smallest level will have linear size equal to ``input_image_linear_size/pow(scaleFactor, nlevels)``.

    :param edgeThreshold: This is size of the border where the features are not detected. It should roughly match the ``patchSize`` parameter.

    :param firstLevel: It should be 0 in the current implementation.

    :param WTA_K: The number of points that produce each element of the oriented BRIEF descriptor. The default value 2 means the BRIEF where we take a random point pair and compare their brightnesses, so we get 0/1 response. Other possible values are 3 and 4. For example, 3 means that we take 3 random points (of course, those point coordinates are random, but they are generated from the pre-defined seed, so each element of BRIEF descriptor is computed deterministically from the pixel rectangle), find point of maximum brightness and output index of the winner (0, 1 or 2). Such output will occupy 2 bits, and therefore it will need a special variant of Hamming distance, denoted as ``NORM_HAMMING2`` (2 bits per bin).  When ``WTA_K=4``, we take 4 random points to compute each bin (that will also occupy 2 bits with possible values 0, 1, 2 or 3).

    :param scoreType: The default HARRIS_SCORE means that Harris algorithm is used to rank features (the score is written to ``KeyPoint::score`` and is used to retain best ``nfeatures`` features); FAST_SCORE is alternative value of the parameter that produces slightly less stable keypoints, but it is a little faster to compute.

    :param patchSize: size of the patch used by the oriented BRIEF descriptor. Of course, on smaller pyramid layers the perceived image area covered by a feature will be larger.

ORB::operator()
---------------
Finds keypoints in an image and computes their descriptors

.. ocv:function:: void ORB::operator()(InputArray image, InputArray mask, vector<KeyPoint>& keypoints, OutputArray descriptors, bool useProvidedKeypoints=false ) const

.. ocv:pyfunction:: cv2.ORB.detect(image[, mask]) -> keypoints
.. ocv:pyfunction:: cv2.ORB.compute(image, keypoints[, descriptors]) -> keypoints, descriptors
.. ocv:pyfunction:: cv2.ORB.detectAndCompute(image, mask[, descriptors[, useProvidedKeypoints]]) -> keypoints, descriptors


    :param image: The input 8-bit grayscale image.

    :param mask: The operation mask.

    :param keypoints: The output vector of keypoints.

    :param descriptors: The output descriptors. Pass ``cv::noArray()`` if you do not need it.

    :param useProvidedKeypoints: If it is true, then the method will use the provided vector of keypoints instead of detecting them.


BRISK
-----
.. ocv:class:: BRISK : public Feature2D

Class implementing the BRISK keypoint detector and descriptor extractor, described in [LCS11]_.

.. [LCS11] Stefan Leutenegger, Margarita Chli and Roland Siegwart: BRISK: Binary Robust Invariant Scalable Keypoints. ICCV 2011: 2548-2555.

BRISK::BRISK
------------
The BRISK constructor

.. ocv:function:: BRISK::BRISK(int thresh=30, int octaves=3, float patternScale=1.0f)

.. ocv:pyfunction:: cv2.BRISK([, thresh[, octaves[, patternScale]]]) -> <BRISK object>

    :param thresh: FAST/AGAST detection threshold score.

    :param octaves: detection octaves. Use 0 to do single scale.

    :param patternScale: apply this scale to the pattern used for sampling the neighbourhood of a keypoint.

BRISK::BRISK
------------
The BRISK constructor for a custom pattern

.. ocv:function:: BRISK::BRISK(std::vector<float> &radiusList, std::vector<int> &numberList, float dMax=5.85f, float dMin=8.2f, std::vector<int> indexChange=std::vector<int>())

.. ocv:pyfunction:: cv2.BRISK(radiusList, numberList[, dMax[, dMin[, indexChange]]]) -> <BRISK object>

    :param radiusList: defines the radii (in pixels) where the samples around a keypoint are taken (for keypoint scale 1).

    :param numberList: defines the number of sampling points on the sampling circle. Must be the same size as radiusList..

    :param dMax: threshold for the short pairings used for descriptor formation (in pixels for keypoint scale 1).

    :param dMin: threshold for the long pairings used for orientation determination (in pixels for keypoint scale 1).

    :param indexChanges: index remapping of the bits.

BRISK::operator()
-----------------
Finds keypoints in an image and computes their descriptors

.. ocv:function:: void BRISK::operator()(InputArray image, InputArray mask, vector<KeyPoint>& keypoints, OutputArray descriptors, bool useProvidedKeypoints=false ) const

.. ocv:pyfunction:: cv2.BRISK.detect(image[, mask]) -> keypoints
.. ocv:pyfunction:: cv2.BRISK.compute(image, keypoints[, descriptors]) -> keypoints, descriptors
.. ocv:pyfunction:: cv2.BRISK.detectAndCompute(image, mask[, descriptors[, useProvidedKeypoints]]) -> keypoints, descriptors

    :param image: The input 8-bit grayscale image.

    :param mask: The operation mask.

    :param keypoints: The output vector of keypoints.

    :param descriptors: The output descriptors. Pass ``cv::noArray()`` if you do not need it.

    :param useProvidedKeypoints: If it is true, then the method will use the provided vector of keypoints instead of detecting them.

KAZE
----
.. ocv:class:: KAZE : public Feature2D

Class implementing the KAZE keypoint detector and descriptor extractor, described in [ABD12]_. ::

    class CV_EXPORTS_W KAZE : public Feature2D
    {
    public:
        CV_WRAP KAZE();
        CV_WRAP explicit KAZE(bool extended, bool upright, float threshold = 0.001f,
                              int octaves = 4, int sublevels = 4, int diffusivity = DIFF_PM_G2);
    };

.. note:: AKAZE descriptor can only be used with KAZE or AKAZE keypoints

.. [ABD12] KAZE Features. Pablo F. Alcantarilla, Adrien Bartoli and Andrew J. Davison. In European Conference on Computer Vision (ECCV), Fiorenze, Italy, October 2012.

KAZE::KAZE
----------
The KAZE constructor

.. ocv:function:: KAZE::KAZE(bool extended, bool upright, float threshold, int octaves, int sublevels, int diffusivity)

    :param extended: Set to enable extraction of extended (128-byte) descriptor.
    :param upright: Set to enable use of upright descriptors (non rotation-invariant).
    :param threshold: Detector response threshold to accept point
    :param octaves: Maximum octave evolution of the image
    :param sublevels: Default number of sublevels per scale level
    :param diffusivity: Diffusivity type. DIFF_PM_G1, DIFF_PM_G2, DIFF_WEICKERT or DIFF_CHARBONNIER

AKAZE
-----
.. ocv:class:: AKAZE : public Feature2D

Class implementing the AKAZE keypoint detector and descriptor extractor, described in [ANB13]_. ::

    class CV_EXPORTS_W AKAZE : public Feature2D
    {
    public:
        CV_WRAP AKAZE();
        CV_WRAP explicit AKAZE(int descriptor_type, int descriptor_size = 0, int descriptor_channels = 3,
                               float threshold = 0.001f, int octaves = 4, int sublevels = 4, int diffusivity = DIFF_PM_G2);
    };

.. note:: AKAZE descriptor can only be used with KAZE or AKAZE keypoints

.. [ANB13] Fast Explicit Diffusion for Accelerated Features in Nonlinear Scale Spaces. Pablo F. Alcantarilla, Jesús Nuevo and Adrien Bartoli. In British Machine Vision Conference (BMVC), Bristol, UK, September 2013.

AKAZE::AKAZE
------------
The AKAZE constructor

.. ocv:function:: AKAZE::AKAZE(int descriptor_type, int descriptor_size, int descriptor_channels, float threshold, int octaves, int sublevels, int diffusivity)

    :param descriptor_type: Type of the extracted descriptor: DESCRIPTOR_KAZE, DESCRIPTOR_KAZE_UPRIGHT, DESCRIPTOR_MLDB or DESCRIPTOR_MLDB_UPRIGHT.
    :param descriptor_size: Size of the descriptor in bits. 0 -> Full size
    :param descriptor_channels: Number of channels in the descriptor (1, 2, 3)
    :param threshold: Detector response threshold to accept point
    :param octaves: Maximum octave evolution of the image
    :param sublevels: Default number of sublevels per scale level
    :param diffusivity: Diffusivity type. DIFF_PM_G1, DIFF_PM_G2, DIFF_WEICKERT or DIFF_CHARBONNIER

SIFT
----

.. ocv:class:: SIFT : public Feature2D

The SIFT algorithm has been moved to opencv_contrib/xfeatures2d module.