#ifndef MPEG3PRIVATE_H
#define MPEG3PRIVATE_H

#include <pthread.h>

#include <stdint.h>

#include <stdio.h>




/* Constants */

#define MPEG3_MAJOR   1
#define MPEG3_MINOR   8
#define MPEG3_RELEASE 0

#define RENDERFARM_FS_PREFIX "vfs://"


#define MPEG3_FLOAT32 float

#define MPEG3_TOC_PREFIX                 0x544f4320
// This decreases with every new version
#define MPEG3_TOC_VERSION                0x000000fa
#define MPEG3_ID3_PREFIX                 0x494433
#define MPEG3_IFO_PREFIX                 0x44564456
// First byte to read when opening a file
#define MPEG3_START_BYTE                 0x0
#define MPEG3_IO_SIZE                    0x100000     /* Bytes read by mpeg3io at a time */
//#define MPEG3_IO_SIZE                    0x800          /* Bytes read by mpeg3io at a time */
#define MPEG3_RIFF_CODE                  0x52494646
#define MPEG3_PROC_CPUINFO               "/proc/cpuinfo"
#define MPEG3_RAW_SIZE                   0x100000     /* Largest possible packet */
#define MPEG3_BD_PACKET_SIZE             192
#define MPEG3_TS_PACKET_SIZE             188
#define MPEG3_DVD_PACKET_SIZE            0x800
#define MPEG3_SYNC_BYTE                  0x47
#define MPEG3_PACK_START_CODE            0x000001ba
#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_SEQUENCE_END_CODE          0x000001b7
#define MPEG3_SYSTEM_START_CODE          0x000001bb
#define MPEG3_STRLEN                     1024
#define MPEG3_PIDMAX                     256             /* Maximum number of PIDs in one stream */
#define MPEG3_PROGRAM_ASSOCIATION_TABLE  0x00
#define MPEG3_CONDITIONAL_ACCESS_TABLE   0x01
#define MPEG3_PACKET_START_CODE_PREFIX   0x000001
#define MPEG3_PRIVATE_STREAM_2           0xbf
#define MPEG3_PADDING_STREAM             0xbe
#define MPEG3_GOP_START_CODE             0x000001b8
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_EXT_START_CODE             0x000001b5
#define MPEG3_USER_START_CODE            0x000001b2
#define MPEG3_SLICE_MIN_START            0x00000101
#define MPEG3_SLICE_MAX_START            0x000001af
#define MPEG3_AC3_START_CODE             0x0b77
#define MPEG3_PCM_START_CODE             0x7f7f807f
#define MPEG3_MAX_CPUS                   256
#define MPEG3_MAX_STREAMS                0x10000
#define MPEG3_MAX_PACKSIZE               262144
/* Maximum number of complete subtitles to buffer in a subtitle track */
/* or number of incomplete subtitles to buffer in demuxer. */
#define MPEG3_MAX_SUBTITLES              256
/* Positive difference before declaring timecodes discontinuous */
#define MPEG3_CONTIGUOUS_THRESHOLD       10  
/* Minimum number of seconds before interleaving programs */
#define MPEG3_PROGRAM_THRESHOLD          5   
/* Number of frames difference before absolute seeking */
#define MPEG3_SEEK_THRESHOLD             16  
/* Size of chunk of audio in table of contents */
#define MPEG3_AUDIO_CHUNKSIZE            0x10000 
/* Minimum amount of data required to read an audio packet in streaming mode. */
#define MPEG3_AUDIO_STREAM_SIZE          0x1000 
/* Minimum amount of data required to read a video header in streaming mode. */
#define MPEG3_VIDEO_STREAM_SIZE          0x1000 
#define MPEG3_LITTLE_ENDIAN              ((*(uint32_t*)"x\0\0\0") & 0x000000ff)
/* Number of samples in audio history */
#define MPEG3_AUDIO_HISTORY              0x100000 
/* Range to scan for pts after byte seek */
#define MPEG3_PTS_RANGE                  0x100000 

/* Values for audio format */
#define AUDIO_UNKNOWN 0
#define AUDIO_MPEG 1
#define AUDIO_AC3  2
#define AUDIO_PCM  3
#define AUDIO_AAC  4
#define AUDIO_JESUS  5


/* Table of contents sections */
#define FILE_TYPE_PROGRAM 0x00000000
#define FILE_TYPE_TRANSPORT 0x00000001
#define FILE_TYPE_AUDIO 0x00000002
#define FILE_TYPE_VIDEO 0x00000003

#define STREAM_AUDIO 0x00000004
#define STREAM_VIDEO 0x00000005
#define STREAM_SUBTITLE 0x00000006

#define OFFSETS_AUDIO 0x00000007
#define OFFSETS_VIDEO 0x00000008

#define ATRACK_COUNT 0x9
#define VTRACK_COUNT 0xa
#define STRACK_COUNT 0xb

#define TITLE_PATH 0xc
#define IFO_PALETTE 0xd
#define FILE_INFO 0xe

// Combine the pid and the stream id into one unit
#define CUSTOM_ID(pid, stream_id) (((pid << 8) | stream_id) & 0xffff)
#define CUSTOM_ID_PID(id) (id >> 8)
#define CUSTOM_ID_STREAMID(id) (id & 0xff)

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


// CSS







struct mpeg3_block 
{
	unsigned char b[5];
};

struct mpeg3_playkey 
{
	int offset;
	unsigned char key[5];
};

typedef struct
{
	int encrypted;
	char device_path[MPEG3_STRLEN];    /* Device the file is located on */
	unsigned char disk_key[MPEG3_DVD_PACKET_SIZE];
	unsigned char title_key[5];
	char challenge[10];
	struct mpeg3_block key1;
	struct mpeg3_block key2;
	struct mpeg3_block keycheck;
	int varient;
	int fd;
	char path[MPEG3_STRLEN];
} mpeg3_css_t;










// I/O











/* Filesystem structure */
/* We buffer in MPEG3_IO_SIZE buffers.  Stream IO would require back */
/* buffering a buffer since the stream must be rewound for packet headers, */
/* sequence start codes, format parsing, decryption, and mpeg3cat. */



typedef struct
{
	FILE *fd;
	mpeg3_css_t *css;          /* Encryption object */
	char path[MPEG3_STRLEN];
	unsigned char *buffer;   /* Readahead buffer */
	int64_t buffer_offset;      /* Current buffer position */
	int64_t buffer_size;        /* Bytes in buffer */
	int64_t buffer_position;    /* Byte in file of start of buffer */

/* Hypothetical position of file pointer */
	int64_t current_byte;
	int64_t total_bytes;
} mpeg3_fs_t;








// Table of contents








// May get rid of time values and rename to a cell offset table.
// May also get rid of end byte.
typedef struct
{
/* Starting byte of cell in the title (start_byte) */
	int64_t title_start;
/* Ending byte of cell in the title (end_byte) */
	int64_t title_end;
/* Starting byte of the cell in the program */
	int64_t program_start;
/* Ending byte of the cell in the program */
	int64_t program_end;
/* Program the cell belongs to */
	int program;
} mpeg3_cell_t;

typedef struct
{
	void *file;
	mpeg3_fs_t *fs;
/* Total bytes in title file.  Critical for seeking and length. */
	int64_t total_bytes;
/* Absolute starting byte of the title in the stream */
	int64_t start_byte;
/* Absolute ending byte of the title in the stream + 1 */
	int64_t end_byte;
/* Timecode table */
	mpeg3_cell_t *cell_table;
	int cell_table_size;    /* Number of entries */
	int cell_table_allocation;    /* Number of available slots */
} mpeg3_title_t;








/* Subtitle object. */
/* Created by the demuxer to store subtitles. */







typedef struct
{
/* Raw data of subtitle */
	unsigned char *data;
/* number of bytes of data */
	int size;
/* number of bytes of data read so far */
	int bytes_read;
/* Number of stream starting at 0x20 */
	int id;
	int done;
/* Program offset of start of subtitle */
	int64_t offset;

/* image in YUV 4:2:0 */
	unsigned char *image_y;
	unsigned char *image_u;
	unsigned char *image_v;
	unsigned char *image_a;
	int x1;
	int x2;
	int y1;
	int y2;
	int w;
	int h;
/* Force display */
	int force;
/* Time after detection of subtitle to display it in 1/100sec */
	int start_time;
/* Time after detection of subtitle to hide it in 1/100sec */
	int stop_time;
/* Indexes in the main palette */
	int palette[4];
	int alpha[4];
/* The subtitle is being drawn */
	int active;
} mpeg3_subtitle_t;








// Demuxer










typedef struct
{
/* mpeg3_t */
	void* file;
/* One unparsed packet.  MPEG3_RAW_SIZE allocated since we don't know the packet size */
	unsigned char *raw_data;
/* Offset in raw_data of read pointer */
	int raw_offset;
/* Amount loaded in last raw_data */
	int raw_size;


/* Elementary stream data when only one stream is to be read. */
/* Erased in every call to read a packet. */
	unsigned char *data_buffer;
/* Allocation of data_buffer */
	int data_allocated;
/* Position in data_buffer of write pointer */
	int data_size;
/* Position in data_buffer of read pointer */
	int data_position;
/* Start of the next pes packet in the data buffer for reading.  */
/* Used for decoding PCM. */
	int data_start;

/* Elementary stream data when all streams are to be read.  There is no */
/* read pointer since data is expected to be copied directly to a track. */
/* Some packets contain audio and video.  Further division into */
/* stream ID may be needed. */
	unsigned char *audio_buffer;
	int audio_allocated;
	int audio_size;
	int audio_start;
	unsigned char *video_buffer;
	int video_allocated;
	int video_size;
	int video_start;
	unsigned char *subtitle_buffer;
	int subtitle_size;
	int subtitle_allocated;

/* Subtitle objects */
	mpeg3_subtitle_t **subtitles;
	int total_subtitles;





/* What type of data to read. */
	int do_audio;
	int do_video;
	int read_all;

/* Direction of reads */
	int reverse;
/* Set to 1 when eof or attempt to read before beginning */
	int error_flag;
/* Temp variables for returning */
	unsigned char next_char;
/* Info for mpeg3cat */
	int64_t last_packet_start;
	int64_t last_packet_end;
	int64_t last_packet_decryption;

/* Titles */
	mpeg3_title_t *titles[MPEG3_MAX_STREAMS];
	int total_titles;
/* Title currently being used */
	int current_title;
	

/* Tables of every stream ID encountered */
	int astream_table[MPEG3_MAX_STREAMS];  /* macro of audio format if audio  */
	int vstream_table[MPEG3_MAX_STREAMS];  /* 1 if video */

/* Programs */
	int total_programs;
	int current_program;

/* Cell in the current title currently used */
	int title_cell;

/* Byte position in current program. */
	int64_t program_byte;
/* Total bytes in all titles */
	int64_t total_bytes;
/* The end of the current stream in the current program */
	int64_t stream_end;

	int transport_error_indicator;
	int payload_unit_start_indicator;
/* PID of last packet */
	int pid;
/* Stream ID of last packet */
	unsigned int stream_id;
/* Custom ID of last packet */
	unsigned int custom_id;
	int transport_scrambling_control;
	int adaptation_field_control;
	int continuity_counter;
	int is_padding;
	int pid_table[MPEG3_PIDMAX];
	int continuity_counters[MPEG3_PIDMAX];
	int total_pids;
	int adaptation_fields;
	double time;           /* Time in seconds */
	int audio_pid;
	int video_pid;
	int got_audio;
	int got_video;
/* if subtitle object was created in last packet */
	int got_subtitle;
/* When only one stream is to be read, these store the stream IDs */
/* Video stream ID being decoded.  -1 = select first ID in stream */
	int astream;     
/* Audio stream ID being decoded.  -1 = select first ID in stream */
	int vstream;
/* Multiplexed streams have the audio type */
/* Format of the audio derived from multiplexing codes */
	int aformat;      
	int program_association_tables;
	int table_id;
	int section_length;
	int transport_stream_id;
	int pes_packets;
	double pes_audio_time;  /* Presentation Time stamps */
	double pes_video_time;  /* Presentation Time stamps */
/* Cause the stream parameters to be dumped in human readable format */
	int dump;
} mpeg3_demuxer_t;








// Bitstream










//                                    next bit in forward direction
//                                  next bit in reverse direction |
//                                                              v v
// | | | | | | | | | | | | | | | | | | | | | | | | | | |1|1|1|1|1|1| */
//                                                     ^         ^
//                                                     |         bit_number = 1
//                                                     bfr_size = 6

typedef struct
{
	uint32_t bfr;  /* bfr = buffer for bits */
	int bit_number;   /* position of pointer in bfr */
	int bfr_size;    /* number of bits in bfr.  Should always be a multiple of 8 */
	void *file;    /* Mpeg2 file */
	mpeg3_demuxer_t *demuxer;   /* Mpeg2 demuxer */
/* If the input ptr is true, data is read from it instead of the demuxer. */
	unsigned char *input_ptr;
} mpeg3_bits_t;










// Audio







#define AC3_N 512

#define MAXFRAMESIZE 4096
#define MAXFRAMESAMPLES 65536
#define HDRCMPMASK 0xfffffd00
#define SBLIMIT 32
#define SSLIMIT 18
#define SCALE_BLOCK 12
#define MPEG3AUDIO_PADDING 1024

/* Values for mode */
#define MPG_MD_STEREO			0
#define MPG_MD_JOINT_STEREO 	1
#define MPG_MD_DUAL_CHANNEL 	2
#define MPG_MD_MONO 			3


#define MAX_AC3_FRAMESIZE 1920 * 2 + 512

extern int mpeg3_ac3_samplerates[3];

/* Exponent strategy constants */
#define MPEG3_EXP_REUSE (0)
#define MPEG3_EXP_D15   (1)
#define MPEG3_EXP_D25   (2)
#define MPEG3_EXP_D45   (3)

/* Delta bit allocation constants */
#define DELTA_BIT_REUSE (0)
#define DELTA_BIT_NEW (1)
#define DELTA_BIT_NONE (2)
#define DELTA_BIT_RESERVED (3)





// Layered decoder

typedef struct
{
	mpeg3_bits_t *stream;


// Layer 3
	unsigned char *bsbuf, *bsbufold;
	unsigned char bsspace[2][MAXFRAMESIZE + 512]; /* MAXFRAMESIZE */
	int bsnum;
	long framesize;           /* For mp3 current framesize without header.  For AC3 current framesize with header. */
	long prev_framesize;
	int channels;
	int samplerate;
	int single;
	int sampling_frequency_code;
	int error_protection;
	int mode;
	int mode_ext;
	int lsf;
	long ssize;
	int mpeg35;
	int padding;
	int layer;
	int extension;
	int copyright;
	int original;
	int emphasis;
	int bitrate;
/* Static variable in synthesizer */
	int bo;                      
/* Ignore first frame after a seek */
	int first_frame;
	float synth_stereo_buffs[2][2][0x110];
	float synth_mono_buff[64];
	float mp3_block[2][2][SBLIMIT * SSLIMIT];
	int mp3_blc[2];

/* State of ID3 parsing */
	int id3_state;
/* No ID3 tag found */
#define MPEG3_ID3_IDLE 0
/* Reading header */
#define MPEG3_ID3_HEADER 1
/* Skipping ID3 tag */
#define MPEG3_ID3_SKIP 2
	int id3_current_byte;
	int id3_size;


// Layer 2
	int bitrate_index;
    struct al_table *alloc;
    int jsbound;
    int II_sblimit;
	unsigned int layer2_scfsi_buf[64];
} mpeg3_layer_t;





// AC3 decoder

typedef struct
{
	mpeg3_bits_t *stream;
	int samplerate;
	int bitrate;
	int flags;
	int channels;
	void *state;  /* a52_state_t */
	void *output; /* sample_t */
	int framesize;
} mpeg3_ac3_t;




// PCM decoder

#define PCM_HEADERSIZE 20
typedef struct
{
	int samplerate;
	int bits;
	int channels;
	int framesize;
} mpeg3_pcm_t;






/* IMDCT variables */
typedef struct
{
	float real;
	float imag;
} mpeg3_complex_t;

struct al_table 
{
	short bits;
	short d;
};


typedef struct
{
	void* file;
	void* track;

	mpeg3_ac3_t *ac3_decoder;
	mpeg3_layer_t *layer_decoder;
	mpeg3_pcm_t *pcm_decoder;

/* In order of importance */
	long outscale;
/* Number of current frame being decoded */
	int framenum;
	
/* Size of frame including header */
	int framesize;
/* First byte of audio data in the file */
	int64_t start_byte;
/* Output from synthesizer in linear floats */
	float **output;           
/* Number of pcm samples in the buffer */
	int output_size;         
/* Allocated number of samples in output */
	int output_allocated;    
/* Sample position in file of start of output buffer */
	int output_position;     

/* Perform a seek to the sample */
	int sample_seek;
/* Perform a seek to the absolute byte */
	int64_t byte_seek;
/* +/- number of samples of difference between audio and video */
	int seek_correction;
/* Buffer containing current packet */
	unsigned char packet_buffer[MAXFRAMESIZE];
/* Position in packet buffer of next byte to read */
	int packet_position;
} mpeg3audio_t;





typedef struct
{
/* Buffer of frames for index.  A frame is a high/low pair. */
	float **index_data;
/* Number of frames allocated in each index channel. */
	int index_allocated;
/* Number of index channels allocated */
	int index_channels;
/* Number of high/low pairs in index channel */
	int index_size;
/* Downsampling of index buffers when constructing index */
	int index_zoom;
} mpeg3_index_t;



typedef struct
{
	int channels;
	int sample_rate;
	mpeg3_demuxer_t *demuxer;
	mpeg3audio_t *audio;
	int current_position;
	int64_t total_samples;
	int format;               /* format of audio */
	unsigned int pid;
/* If we got the header information yet.  Used in streaming mode. */
	int got_header;



/* Pointer to master table of contents when the TOC is read. */
/* Pointer to private table when the TOC is being created */
/* Stores the absolute byte of each audio chunk */
	int64_t *sample_offsets;
	int total_sample_offsets;
	int sample_offsets_allocated;
/* If this sample offset table must be deleted by the track */
	int private_offsets;
/* End of stream in table of contents construction */
	int64_t audio_eof;




/* Starting byte of previous packet for making TOC */
	int64_t prev_offset;

} mpeg3_atrack_t;










// Video










/* zig-zag scan */
extern unsigned char mpeg3_zig_zag_scan_nommx[64];

/* alternate scan */
extern unsigned char mpeg3_alternate_scan_nommx[64];

/* default intra quantization matrix */
extern unsigned char mpeg3_default_intra_quantizer_matrix[64];

/* Frame rate table must agree with the one in the encoder */
extern double mpeg3_frame_rate_table[16];

/* non-linear quantization coefficient table */
extern unsigned char mpeg3_non_linear_mquant_table[32];

#define CHROMA420     1     /* chroma_format */
#define CHROMA422     2
#define CHROMA444     3

#define TOP_FIELD     1     /* picture structure */
#define BOTTOM_FIELD  2
#define FRAME_PICTURE 3

#define SEQ_ID        1     /* extension start code IDs */
#define DISP_ID       2
#define QUANT_ID      3
#define SEQSCAL_ID    5
#define PANSCAN_ID    7
#define CODING_ID     8
#define SPATSCAL_ID   9
#define TEMPSCAL_ID  10

#define ERROR (-1)

#define SC_NONE       0   /* scalable_mode */
#define SC_DP         1
#define SC_SPAT       2
#define SC_SNR        3
#define SC_TEMP       4

#define I_TYPE        1     /* picture coding type */
#define P_TYPE        2
#define B_TYPE        3
#define D_TYPE        4

#define MB_INTRA      1 	/* macroblock type */
#define MB_PATTERN    2
#define MB_BACKWARD   4
#define MB_FORWARD    8
#define MB_QUANT      16
#define MB_WEIGHT     32
#define MB_CLASS4     64

#define MC_FIELD      1     /* motion_type */
#define MC_FRAME      2
#define MC_16X8       2
#define MC_DMV        3

#define MV_FIELD      0     /* mv_format */
#define MV_FRAME      1











/* Array of these feeds the slice decoders */
typedef struct
{
	unsigned char *data;   /* Buffer for holding the slice data */
	int buffer_size;         /* Size of buffer */
	int buffer_allocation;   /* Space allocated for buffer  */
	int current_position;    /* Position in buffer */
	uint32_t bits;
	int bits_size;
	pthread_mutex_t completion_lock; /* Lock slice until completion */
	int done;           /* Signal for slice decoder to skip */
} mpeg3_slice_buffer_t;

/* Each slice decoder */
typedef struct
{
	void *video;     /* mpeg3video_t */
	mpeg3_slice_buffer_t *slice_buffer;

	int thread_number;      /* Number of this thread */
	int current_buffer;     /* Buffer this slice decoder is on */
	int buffer_step;        /* Number of buffers to skip */
	int last_buffer;        /* Last buffer this decoder should process */
	int fault;
	int done;
	int quant_scale;
	int pri_brk;                  /* slice/macroblock */
	short block[12][64];
	int sparse[12];
	pthread_t tid;   /* ID of thread */
	pthread_mutex_t input_lock, output_lock, completion_lock;
} mpeg3_slice_t;

typedef struct 
{
	int hour;
	int minute;
	int second;
	int frame;
} mpeg3_timecode_t;



typedef struct
{
	unsigned char *y, *u, *v;
	int y_size;
	int u_size;
	int v_size;
	int64_t frame_number;
} mpeg3_cacheframe_t;

typedef struct
{
	mpeg3_cacheframe_t *frames;
	int total;
	int allocation;
} mpeg3_cache_t;


typedef struct
{
	void* file;
	void* track;

/* ================================= Seeking variables ========================= */
	mpeg3_bits_t *vstream;
	int decoder_initted;
	unsigned char **output_rows;     /* Output frame buffer supplied by user */
	int in_x, in_y, in_w, in_h, out_w, out_h; /* Output dimensions */
	int row_span;
	int *x_table, *y_table;          /* Location of every output pixel in the input */
	int color_model;
	int want_yvu;                    /* Want to return a YUV frame */
	char *y_output, *u_output, *v_output; /* Output pointers for a YUV frame */

	mpeg3_slice_t slice_decoders[MPEG3_MAX_CPUS];  /* One slice decoder for every CPU */
	int total_slice_decoders;                       /* Total slice decoders in use */
	mpeg3_slice_buffer_t slice_buffers[MPEG3_MAX_CPUS];   /* Buffers for holding the slice data */
	int total_slice_buffers;         /* Total buffers in the array to be decompressed */
	int slice_buffers_initialized;     /* Total buffers initialized in the array */
	pthread_mutex_t slice_lock;      /* Lock slice array while getting the next buffer */
	pthread_mutex_t test_lock;

	int blockreadsize;
	int maxframe;         /* Max value of frame num to read */
	int64_t byte_seek;   /* Perform absolute byte seek before the next frame is read */
	int frame_seek;        /* Perform a frame seek before the next frame is read */
	int framenum;         /* Number of the next frame to be decoded */
	int last_number;       /* Last framenum rendered */
	int found_seqhdr;
	int bitrate;
	mpeg3_timecode_t gop_timecode;     /* Timecode for the last GOP header read. */
	int has_gops; /* Some streams have no GOPs so try sequence start codes instead */
	int is_h264;

/* These are only available from elementary streams. */
	int frames_per_gop;       /* Frames per GOP after the first GOP. */
	int first_gop_frames;     /* Frames in the first GOP. */
	int first_frame;     /* Number of first frame stored in timecode */
	int last_frame;      /* Last frame in file */

/* ================================= Compression variables ===================== */
/* Malloced frame buffers.  2 refframes are swapped in and out. */
/* while only 1 auxframe is used. */
	unsigned char *yuv_buffer[5];  /* Make YVU buffers contiguous for all frames */
	unsigned char *oldrefframe[3], *refframe[3], *auxframe[3];
	unsigned char *llframe0[3], *llframe1[3];
	unsigned char *mpeg3_zigzag_scan_table;
	unsigned char *mpeg3_alternate_scan_table;
// Source for the next frame presentation
	unsigned char *output_src[3];
/* Pointers to frame buffers. */
	unsigned char *newframe[3];
	int horizontal_size, vertical_size, mb_width, mb_height;
	int coded_picture_width,  coded_picture_height;
	int chroma_format, chrom_width, chrom_height, blk_cnt;
	int pict_type;
	int field_sequence;
	int forw_r_size, back_r_size, full_forw, full_back;
	int prog_seq, prog_frame;
	int h_forw_r_size, v_forw_r_size, h_back_r_size, v_back_r_size;
	int dc_prec, pict_struct, topfirst, frame_pred_dct, conceal_mv;
	int intravlc;
	int repeatfirst;
/* Number of times to repeat the current frame * 100 since floating point is impossible in MMX */
	int repeat_count;
/* Number of times the current frame has been repeated * 100 */
	int current_repeat;
	int secondfield;
	int skip_bframes;
	int stwc_table_index, llw, llh, hm, hn, vm, vn;
	int lltempref, llx0, lly0, llprog_frame, llfieldsel;
	int matrix_coefficients;
	int framerate_code;
	double frame_rate;
	int *cr_to_r, *cr_to_g, *cb_to_g, *cb_to_b;
	int *cr_to_r_ptr, *cr_to_g_ptr, *cb_to_g_ptr, *cb_to_b_ptr;
	int intra_quantizer_matrix[64], non_intra_quantizer_matrix[64];
	int chroma_intra_quantizer_matrix[64], chroma_non_intra_quantizer_matrix[64];
	int mpeg2;
	int qscale_type, altscan;      /* picture coding extension */
	int pict_scal;                /* picture spatial scalable extension */
	int scalable_mode;            /* sequence scalable extension */

/* Subtitling frame */
	unsigned char *subtitle_frame[3];
} mpeg3video_t;















typedef struct
{
	int width;
	int height;
	double frame_rate;
	float aspect_ratio;
	mpeg3_demuxer_t *demuxer;
/* Video decoding object */
	mpeg3video_t *video;
/* Table of current subtitles being overlayed */
	mpeg3_subtitle_t **subtitles;
	int total_subtitles;
	int current_position;  /* Number of next frame to be played */
	int total_frames;     /* Total frames in the file */
	unsigned int pid;

/* Pointer to master table of contents when the TOC is read. */
/* Pointer to private table when the TOC is being created */
/* Stores the absolute byte of each frame */
	int64_t *frame_offsets;
	int total_frame_offsets;
	int frame_offsets_allocated;
	int64_t *keyframe_numbers;
	int total_keyframe_numbers;
	int keyframe_numbers_allocated;
/* Starting byte of previous packet for making TOC */
	int64_t prev_offset;
/* Starting byte of previous packet when the start code was found. */
/* Used for headers which require multiple packets. */
	int64_t prev_frame_offset;
/* End of stream in table of contents construction */
	int64_t video_eof;
	int got_top;
	int got_keyframe;

	mpeg3_cache_t *frame_cache;



/* If these tables must be deleted by the track */
	int private_offsets;
} mpeg3_vtrack_t;







/* Subtitle track */
/* Stores the program offsets of subtitle images. */
/* Only used for seeking off of table of contents for editing. */
/* Doesn't have its own demuxer but hangs off the video demuxer. */

typedef struct
{
/* 0x2X */
	int id;
/* Offsets in program of subtitle packets */
	int64_t *offsets;
	int total_offsets;
	int allocated_offsets;

/* Last subtitle objects found in stream. */
	mpeg3_subtitle_t **subtitles;
	int total_subtitles;
	int allocated_subtitles;
} mpeg3_strack_t;








// Whole thing
typedef struct
{
/* Store entry path here */
	mpeg3_fs_t *fs;
/* Master title tables copied to all tracks */
	mpeg3_demuxer_t *demuxer;        

/* Media specific */
	int total_astreams;
	mpeg3_atrack_t *atrack[MPEG3_MAX_STREAMS];
	int total_vstreams;
	mpeg3_vtrack_t *vtrack[MPEG3_MAX_STREAMS];
	int total_sstreams;
	mpeg3_strack_t *strack[MPEG3_MAX_STREAMS];

/* Table of contents storage */
	int64_t **frame_offsets;
	int64_t **sample_offsets;
	int64_t **keyframe_numbers;
	int64_t *video_eof;
	int64_t *audio_eof;
	int *total_frame_offsets;
	int *total_sample_offsets;
	int64_t *total_samples;
	int *total_keyframe_numbers;
/* Handles changes in channel count after the start of a stream */
	int *channel_counts;
/* Indexes for audio tracks */
	mpeg3_index_t **indexes;
	int total_indexes;


/* Number of bytes to devote to the index of a single track in the index */
/* building process. */
	int64_t index_bytes;

/* Only one of these is set to 1 to specify what kind of stream we have. */
	int is_transport_stream;
	int is_program_stream;
	int is_ifo_file;
	int is_audio_stream;         /* Elemental stream */
	int is_video_stream;         /* Elemental stream */
// Special kind of transport stream for BD or AVC-HD
	int is_bd;
/* > 0 if known otherwise determine empirically for every packet */
	int packet_size;
/* Type and stream for getting current absolute byte */
	int last_type_read;  /* 1 - audio   2 - video */
	int last_stream_read;

/* Subtitle track to composite if >= 0 */
	int subtitle_track;

/* Number of program to play */
	int program;
	int cpus;

/* Filesystem is seekable.  Also means the file isn't a stream. */
	int seekable;

/* For building TOC, the output file. */
	FILE *toc_fd;

/*
 * After byte seeking is called, this is set to -1.
 * The first operation to seek needs to set it to the pts of the byte seek.
 * Then the next operation to seek needs to match its pts to this value.
 */
	int64_t byte_pts;

/*
 * The first color palette in the IFO file.  Used for subtitles.
 * Byte order: YUVX * 16
 */
	int have_palette;
	unsigned char palette[16 * 4];

/* Date of source file index was created from. */
/* Used to compare DVD source file to table of contents source. */
	int64_t source_date;
} mpeg3_t;




#endif
