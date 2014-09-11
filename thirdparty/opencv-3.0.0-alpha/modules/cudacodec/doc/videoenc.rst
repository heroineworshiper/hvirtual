Video Encoding
==============

.. highlight:: cpp



cudacodec::VideoWriter
----------------------
Video writer interface.

.. ocv:class:: cudacodec::VideoWriter

The implementation uses H264 video codec.

.. note:: Currently only Windows platform is supported.

.. note::

   * An example on how to use the videoWriter class can be found at opencv_source_code/samples/gpu/video_writer.cpp


cudacodec::VideoWriter::write
-----------------------------
Writes the next video frame.

.. ocv:function:: void cudacodec::VideoWriter::write(InputArray frame, bool lastFrame = false) = 0

    :param frame: The written frame.

    :param lastFrame: Indicates that it is end of stream. The parameter can be ignored.

The method write the specified image to video file. The image must have the same size and the same surface format as has been specified when opening the video writer.



cudacodec::createVideoWriter
----------------------------
Creates video writer.

.. ocv:function:: Ptr<cudacodec::VideoWriter> cudacodec::createVideoWriter(const String& fileName, Size frameSize, double fps, SurfaceFormat format = SF_BGR)
.. ocv:function:: Ptr<cudacodec::VideoWriter> cudacodec::createVideoWriter(const String& fileName, Size frameSize, double fps, const EncoderParams& params, SurfaceFormat format = SF_BGR)
.. ocv:function:: Ptr<cudacodec::VideoWriter> cudacodec::createVideoWriter(const Ptr<EncoderCallBack>& encoderCallback, Size frameSize, double fps, SurfaceFormat format = SF_BGR)
.. ocv:function:: Ptr<cudacodec::VideoWriter> cudacodec::createVideoWriter(const Ptr<EncoderCallBack>& encoderCallback, Size frameSize, double fps, const EncoderParams& params, SurfaceFormat format = SF_BGR)

    :param fileName: Name of the output video file. Only AVI file format is supported.

    :param frameSize: Size of the input video frames.

    :param fps: Framerate of the created video stream.

    :param params: Encoder parameters. See :ocv:struct:`cudacodec::EncoderParams` .

    :param format: Surface format of input frames ( ``SF_UYVY`` , ``SF_YUY2`` , ``SF_YV12`` , ``SF_NV12`` , ``SF_IYUV`` , ``SF_BGR`` or ``SF_GRAY``). BGR or gray frames will be converted to YV12 format before encoding, frames with other formats will be used as is.

    :param encoderCallback: Callbacks for video encoder. See :ocv:class:`cudacodec::EncoderCallBack` . Use it if you want to work with raw video stream.

The constructors initialize video writer. FFMPEG is used to write videos. User can implement own multiplexing with :ocv:class:`cudacodec::EncoderCallBack` .



cudacodec::EncoderParams
------------------------
.. ocv:struct:: cudacodec::EncoderParams

Different parameters for CUDA video encoder. ::

    struct EncoderParams
    {
        int       P_Interval;      //    NVVE_P_INTERVAL,
        int       IDR_Period;      //    NVVE_IDR_PERIOD,
        int       DynamicGOP;      //    NVVE_DYNAMIC_GOP,
        int       RCType;          //    NVVE_RC_TYPE,
        int       AvgBitrate;      //    NVVE_AVG_BITRATE,
        int       PeakBitrate;     //    NVVE_PEAK_BITRATE,
        int       QP_Level_Intra;  //    NVVE_QP_LEVEL_INTRA,
        int       QP_Level_InterP; //    NVVE_QP_LEVEL_INTER_P,
        int       QP_Level_InterB; //    NVVE_QP_LEVEL_INTER_B,
        int       DeblockMode;     //    NVVE_DEBLOCK_MODE,
        int       ProfileLevel;    //    NVVE_PROFILE_LEVEL,
        int       ForceIntra;      //    NVVE_FORCE_INTRA,
        int       ForceIDR;        //    NVVE_FORCE_IDR,
        int       ClearStat;       //    NVVE_CLEAR_STAT,
        int       DIMode;          //    NVVE_SET_DEINTERLACE,
        int       Presets;         //    NVVE_PRESETS,
        int       DisableCabac;    //    NVVE_DISABLE_CABAC,
        int       NaluFramingType; //    NVVE_CONFIGURE_NALU_FRAMING_TYPE
        int       DisableSPSPPS;   //    NVVE_DISABLE_SPS_PPS

        EncoderParams();
        explicit EncoderParams(const String& configFile);

        void load(const String& configFile);
        void save(const String& configFile) const;
    };



cudacodec::EncoderParams::EncoderParams
---------------------------------------
Constructors.

.. ocv:function:: cudacodec::EncoderParams::EncoderParams()
.. ocv:function:: cudacodec::EncoderParams::EncoderParams(const String& configFile)

    :param configFile: Config file name.

Creates default parameters or reads parameters from config file.



cudacodec::EncoderParams::load
------------------------------
Reads parameters from config file.

.. ocv:function:: void cudacodec::EncoderParams::load(const String& configFile)

    :param configFile: Config file name.



cudacodec::EncoderParams::save
------------------------------
Saves parameters to config file.

.. ocv:function:: void cudacodec::EncoderParams::save(const String& configFile) const

    :param configFile: Config file name.



cudacodec::EncoderCallBack
--------------------------
.. ocv:class:: cudacodec::EncoderCallBack

Callbacks for CUDA video encoder. ::

    class EncoderCallBack
    {
    public:
        enum PicType
        {
            IFRAME = 1,
            PFRAME = 2,
            BFRAME = 3
        };

        virtual ~EncoderCallBack() {}

        virtual unsigned char* acquireBitStream(int* bufferSize) = 0;
        virtual void releaseBitStream(unsigned char* data, int size) = 0;
        virtual void onBeginFrame(int frameNumber, PicType picType) = 0;
        virtual void onEndFrame(int frameNumber, PicType picType) = 0;
    };



cudacodec::EncoderCallBack::acquireBitStream
--------------------------------------------
Callback function to signal the start of bitstream that is to be encoded.

.. ocv:function:: virtual uchar* cudacodec::EncoderCallBack::acquireBitStream(int* bufferSize) = 0

Callback must allocate buffer for CUDA encoder and return pointer to it and it's size.



cudacodec::EncoderCallBack::releaseBitStream
--------------------------------------------
Callback function to signal that the encoded bitstream is ready to be written to file.

.. ocv:function:: virtual void cudacodec::EncoderCallBack::releaseBitStream(unsigned char* data, int size) = 0



cudacodec::EncoderCallBack::onBeginFrame
----------------------------------------
Callback function to signal that the encoding operation on the frame has started.

.. ocv:function:: virtual void cudacodec::EncoderCallBack::onBeginFrame(int frameNumber, PicType picType) = 0

    :param picType: Specify frame type (I-Frame, P-Frame or B-Frame).



cudacodec::EncoderCallBack::onEndFrame
--------------------------------------
Callback function signals that the encoding operation on the frame has finished.

.. ocv:function:: virtual void cudacodec::EncoderCallBack::onEndFrame(int frameNumber, PicType picType) = 0

    :param picType: Specify frame type (I-Frame, P-Frame or B-Frame).
