Common Interfaces of Descriptor Extractors
==========================================

.. highlight:: cpp

Extractors of keypoint descriptors in OpenCV have wrappers with a common interface that enables you to easily switch
between different algorithms solving the same problem. This section is devoted to computing descriptors
represented as vectors in a multidimensional space. All objects that implement the ``vector``
descriptor extractors inherit the
:ocv:class:`DescriptorExtractor` interface.

.. note::

   * An example explaining keypoint extraction can be found at opencv_source_code/samples/cpp/descriptor_extractor_matcher.cpp
   * An example on descriptor evaluation can be found at opencv_source_code/samples/cpp/detector_descriptor_evaluation.cpp

DescriptorExtractor
-------------------
.. ocv:class:: DescriptorExtractor : public Algorithm

Abstract base class for computing descriptors for image keypoints. ::

    class CV_EXPORTS DescriptorExtractor
    {
    public:
        virtual ~DescriptorExtractor();

        void compute( InputArray image, vector<KeyPoint>& keypoints,
                      OutputArray descriptors ) const;
        void compute( InputArrayOfArrays images, vector<vector<KeyPoint> >& keypoints,
                      OutputArrayOfArrays descriptors ) const;

        virtual void read( const FileNode& );
        virtual void write( FileStorage& ) const;

        virtual int descriptorSize() const = 0;
        virtual int descriptorType() const = 0;
        virtual int defaultNorm() const = 0;

        static Ptr<DescriptorExtractor> create( const String& descriptorExtractorType );

    protected:
        ...
    };


In this interface, a keypoint descriptor can be represented as a
dense, fixed-dimension vector of a basic type. Most descriptors
follow this pattern as it simplifies computing
distances between descriptors. Therefore, a collection of
descriptors is represented as
:ocv:class:`Mat` , where each row is a keypoint descriptor.



DescriptorExtractor::compute
--------------------------------
Computes the descriptors for a set of keypoints detected in an image (first variant) or image set (second variant).

.. ocv:function:: void DescriptorExtractor::compute( InputArray image, vector<KeyPoint>& keypoints, OutputArray descriptors ) const

.. ocv:function:: void DescriptorExtractor::compute( InputArrayOfArrays  images, vector<vector<KeyPoint> >& keypoints, OutputArrayOfArrays descriptors ) const

.. ocv:pyfunction:: cv2.DescriptorExtractor_create.compute(image, keypoints[, descriptors]) -> keypoints, descriptors

    :param image: Image.

    :param images: Image set.

    :param keypoints: Input collection of keypoints. Keypoints for which a descriptor cannot be computed are removed. Sometimes new keypoints can be added, for example: ``SIFT`` duplicates keypoint with several dominant orientations (for each orientation).

    :param descriptors: Computed descriptors. In the second variant of the method ``descriptors[i]`` are descriptors computed for a ``keypoints[i]``. Row ``j`` is the ``keypoints`` (or ``keypoints[i]``) is the descriptor for keypoint ``j``-th keypoint.


DescriptorExtractor::create
-------------------------------
Creates a descriptor extractor by name.

.. ocv:function:: Ptr<DescriptorExtractor>  DescriptorExtractor::create( const String& descriptorExtractorType )

.. ocv:pyfunction:: cv2.DescriptorExtractor_create(descriptorExtractorType) -> retval

    :param descriptorExtractorType: Descriptor extractor type.

The current implementation supports the following types of a descriptor extractor:

 * ``"BRISK"`` -- :ocv:class:`BRISK`
 * ``"ORB"`` -- :ocv:class:`ORB`