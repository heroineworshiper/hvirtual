/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */
 
 
// NOT USED

// an attempt to get frame accurate seeking with MKV, which ended up
// showing MKV doesn't have the required information in the 1st place.





#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "file.h"
#include "filemkv.h"
#include <stdlib.h>
#include <string.h>


// bits from ffmpeg
static const uint8_t ff_log2_tab[256] =
{
    0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};


static 
int mkv_log2(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}


#define PALETTE_COUNT 256
#define AVCOL_SPC_UNSPECIFIED 2
#define AVCOL_RANGE_UNSPECIFIED 0
#define AVCOL_TRC_UNSPECIFIED 2
#define AVCOL_PRI_UNSPECIFIED 2
#define AV_NOPTS_VALUE ((int64_t)(0x8000000000000000LL))
#define EBML_MAX_DEPTH 16




/* EBML version supported */
#define EBML_VERSION 1

/* top-level master-IDs */
#define EBML_ID_HEADER             0x1A45DFA3
/* IDs in the HEADER master */
#define EBML_ID_EBMLVERSION        0x4286
#define EBML_ID_EBMLREADVERSION    0x42F7
#define EBML_ID_EBMLMAXIDLENGTH    0x42F2
#define EBML_ID_EBMLMAXSIZELENGTH  0x42F3
#define EBML_ID_DOCTYPE            0x4282
#define EBML_ID_DOCTYPEVERSION     0x4287
#define EBML_ID_DOCTYPEREADVERSION 0x4285


/* general EBML types */
#define EBML_ID_VOID               0xEC
#define EBML_ID_CRC32              0xBF

/*
 * Matroska element IDs, max. 32 bits
 */

/* toplevel segment */
#define MATROSKA_ID_SEGMENT    0x18538067

/* Matroska top-level master IDs */
#define MATROSKA_ID_INFO       0x1549A966
#define MATROSKA_ID_TRACKS     0x1654AE6B
#define MATROSKA_ID_CUES       0x1C53BB6B
#define MATROSKA_ID_TAGS       0x1254C367
#define MATROSKA_ID_SEEKHEAD   0x114D9B74
#define MATROSKA_ID_ATTACHMENTS 0x1941A469
#define MATROSKA_ID_CLUSTER    0x1F43B675
#define MATROSKA_ID_CHAPTERS   0x1043A770

/* IDs in the info master */
#define MATROSKA_ID_TIMECODESCALE 0x2AD7B1
#define MATROSKA_ID_DURATION   0x4489
#define MATROSKA_ID_TITLE      0x7BA9
#define MATROSKA_ID_WRITINGAPP 0x5741
#define MATROSKA_ID_MUXINGAPP  0x4D80
#define MATROSKA_ID_DATEUTC    0x4461
#define MATROSKA_ID_SEGMENTUID 0x73A4

/* ID in the tracks master */
#define MATROSKA_ID_TRACKENTRY 0xAE

/* IDs in the trackentry master */
#define MATROSKA_ID_TRACKNUMBER 0xD7
#define MATROSKA_ID_TRACKUID   0x73C5
#define MATROSKA_ID_TRACKTYPE  0x83
#define MATROSKA_ID_TRACKVIDEO     0xE0
#define MATROSKA_ID_TRACKAUDIO     0xE1
#define MATROSKA_ID_TRACKOPERATION 0xE2
#define MATROSKA_ID_TRACKCOMBINEPLANES 0xE3
#define MATROSKA_ID_TRACKPLANE         0xE4
#define MATROSKA_ID_TRACKPLANEUID      0xE5
#define MATROSKA_ID_TRACKPLANETYPE     0xE6
#define MATROSKA_ID_CODECID    0x86
#define MATROSKA_ID_CODECPRIVATE 0x63A2
#define MATROSKA_ID_CODECNAME  0x258688
#define MATROSKA_ID_CODECINFOURL 0x3B4040
#define MATROSKA_ID_CODECDOWNLOADURL 0x26B240
#define MATROSKA_ID_CODECDECODEALL 0xAA
#define MATROSKA_ID_CODECDELAY 0x56AA
#define MATROSKA_ID_SEEKPREROLL 0x56BB
#define MATROSKA_ID_TRACKNAME  0x536E
#define MATROSKA_ID_TRACKLANGUAGE 0x22B59C
#define MATROSKA_ID_TRACKFLAGENABLED 0xB9
#define MATROSKA_ID_TRACKFLAGDEFAULT 0x88
#define MATROSKA_ID_TRACKFLAGFORCED 0x55AA
#define MATROSKA_ID_TRACKFLAGLACING 0x9C
#define MATROSKA_ID_TRACKMINCACHE 0x6DE7
#define MATROSKA_ID_TRACKMAXCACHE 0x6DF8
#define MATROSKA_ID_TRACKDEFAULTDURATION 0x23E383
#define MATROSKA_ID_TRACKCONTENTENCODINGS 0x6D80
#define MATROSKA_ID_TRACKCONTENTENCODING 0x6240
#define MATROSKA_ID_TRACKTIMECODESCALE 0x23314F
#define MATROSKA_ID_TRACKMAXBLKADDID 0x55EE

/* IDs in the trackvideo master */
#define MATROSKA_ID_VIDEOFRAMERATE 0x2383E3
#define MATROSKA_ID_VIDEODISPLAYWIDTH 0x54B0
#define MATROSKA_ID_VIDEODISPLAYHEIGHT 0x54BA
#define MATROSKA_ID_VIDEOPIXELWIDTH 0xB0
#define MATROSKA_ID_VIDEOPIXELHEIGHT 0xBA
#define MATROSKA_ID_VIDEOPIXELCROPB 0x54AA
#define MATROSKA_ID_VIDEOPIXELCROPT 0x54BB
#define MATROSKA_ID_VIDEOPIXELCROPL 0x54CC
#define MATROSKA_ID_VIDEOPIXELCROPR 0x54DD
#define MATROSKA_ID_VIDEODISPLAYUNIT 0x54B2
#define MATROSKA_ID_VIDEOFLAGINTERLACED 0x9A
#define MATROSKA_ID_VIDEOFIELDORDER 0x9D
#define MATROSKA_ID_VIDEOSTEREOMODE 0x53B8
#define MATROSKA_ID_VIDEOALPHAMODE 0x53C0
#define MATROSKA_ID_VIDEOASPECTRATIO 0x54B3
#define MATROSKA_ID_VIDEOCOLORSPACE 0x2EB524
#define MATROSKA_ID_VIDEOCOLOR 0x55B0

#define MATROSKA_ID_VIDEOCOLORMATRIXCOEFF 0x55B1
#define MATROSKA_ID_VIDEOCOLORBITSPERCHANNEL 0x55B2
#define MATROSKA_ID_VIDEOCOLORCHROMASUBHORZ 0x55B3
#define MATROSKA_ID_VIDEOCOLORCHROMASUBVERT 0x55B4
#define MATROSKA_ID_VIDEOCOLORCBSUBHORZ 0x55B5
#define MATROSKA_ID_VIDEOCOLORCBSUBVERT 0x55B6
#define MATROSKA_ID_VIDEOCOLORCHROMASITINGHORZ 0x55B7
#define MATROSKA_ID_VIDEOCOLORCHROMASITINGVERT 0x55B8
#define MATROSKA_ID_VIDEOCOLORRANGE 0x55B9
#define MATROSKA_ID_VIDEOCOLORTRANSFERCHARACTERISTICS 0x55BA

#define MATROSKA_ID_VIDEOCOLORPRIMARIES 0x55BB
#define MATROSKA_ID_VIDEOCOLORMAXCLL 0x55BC
#define MATROSKA_ID_VIDEOCOLORMAXFALL 0x55BD

#define MATROSKA_ID_VIDEOCOLORMASTERINGMETA 0x55D0
#define MATROSKA_ID_VIDEOCOLOR_RX 0x55D1
#define MATROSKA_ID_VIDEOCOLOR_RY 0x55D2
#define MATROSKA_ID_VIDEOCOLOR_GX 0x55D3
#define MATROSKA_ID_VIDEOCOLOR_GY 0x55D4
#define MATROSKA_ID_VIDEOCOLOR_BX 0x55D5
#define MATROSKA_ID_VIDEOCOLOR_BY 0x55D6
#define MATROSKA_ID_VIDEOCOLOR_WHITEX 0x55D7
#define MATROSKA_ID_VIDEOCOLOR_WHITEY 0x55D8
#define MATROSKA_ID_VIDEOCOLOR_LUMINANCEMAX 0x55D9
#define MATROSKA_ID_VIDEOCOLOR_LUMINANCEMIN 0x55DA

#define MATROSKA_ID_VIDEOPROJECTION 0x7670
#define MATROSKA_ID_VIDEOPROJECTIONTYPE 0x7671
#define MATROSKA_ID_VIDEOPROJECTIONPRIVATE 0x7672
#define MATROSKA_ID_VIDEOPROJECTIONPOSEYAW 0x7673
#define MATROSKA_ID_VIDEOPROJECTIONPOSEPITCH 0x7674
#define MATROSKA_ID_VIDEOPROJECTIONPOSEROLL 0x7675

/* IDs in the trackaudio master */
#define MATROSKA_ID_AUDIOSAMPLINGFREQ 0xB5
#define MATROSKA_ID_AUDIOOUTSAMPLINGFREQ 0x78B5

#define MATROSKA_ID_AUDIOBITDEPTH 0x6264
#define MATROSKA_ID_AUDIOCHANNELS 0x9F

/* IDs in the content encoding master */
#define MATROSKA_ID_ENCODINGORDER 0x5031
#define MATROSKA_ID_ENCODINGSCOPE 0x5032
#define MATROSKA_ID_ENCODINGTYPE 0x5033
#define MATROSKA_ID_ENCODINGCOMPRESSION 0x5034
#define MATROSKA_ID_ENCODINGCOMPALGO 0x4254
#define MATROSKA_ID_ENCODINGCOMPSETTINGS 0x4255

#define MATROSKA_ID_ENCODINGENCRYPTION 0x5035
#define MATROSKA_ID_ENCODINGENCAESSETTINGS 0x47E7
#define MATROSKA_ID_ENCODINGENCALGO 0x47E1
#define MATROSKA_ID_ENCODINGENCKEYID 0x47E2
#define MATROSKA_ID_ENCODINGSIGALGO 0x47E5
#define MATROSKA_ID_ENCODINGSIGHASHALGO 0x47E6
#define MATROSKA_ID_ENCODINGSIGKEYID 0x47E4
#define MATROSKA_ID_ENCODINGSIGNATURE 0x47E3

/* ID in the cues master */
#define MATROSKA_ID_POINTENTRY 0xBB

/* IDs in the pointentry master */
#define MATROSKA_ID_CUETIME    0xB3
#define MATROSKA_ID_CUETRACKPOSITION 0xB7

/* IDs in the cuetrackposition master */
#define MATROSKA_ID_CUETRACK   0xF7
#define MATROSKA_ID_CUECLUSTERPOSITION 0xF1
#define MATROSKA_ID_CUERELATIVEPOSITION 0xF0
#define MATROSKA_ID_CUEDURATION 0xB2
#define MATROSKA_ID_CUEBLOCKNUMBER 0x5378

/* IDs in the tags master */
#define MATROSKA_ID_TAG                 0x7373
#define MATROSKA_ID_SIMPLETAG           0x67C8
#define MATROSKA_ID_TAGNAME             0x45A3
#define MATROSKA_ID_TAGSTRING           0x4487
#define MATROSKA_ID_TAGLANG             0x447A
#define MATROSKA_ID_TAGDEFAULT          0x4484
#define MATROSKA_ID_TAGDEFAULT_BUG      0x44B4
#define MATROSKA_ID_TAGTARGETS          0x63C0
#define MATROSKA_ID_TAGTARGETS_TYPE       0x63CA
#define MATROSKA_ID_TAGTARGETS_TYPEVALUE  0x68CA
#define MATROSKA_ID_TAGTARGETS_TRACKUID   0x63C5
#define MATROSKA_ID_TAGTARGETS_CHAPTERUID 0x63C4
#define MATROSKA_ID_TAGTARGETS_ATTACHUID  0x63C6

/* IDs in the seekhead master */
#define MATROSKA_ID_SEEKENTRY  0x4DBB

/* IDs in the seekpoint master */
#define MATROSKA_ID_SEEKID     0x53AB
#define MATROSKA_ID_SEEKPOSITION 0x53AC

/* IDs in the cluster master */
#define MATROSKA_ID_CLUSTERTIMECODE 0xE7
#define MATROSKA_ID_CLUSTERPOSITION 0xA7
#define MATROSKA_ID_CLUSTERPREVSIZE 0xAB
#define MATROSKA_ID_BLOCKGROUP 0xA0
#define MATROSKA_ID_BLOCKADDITIONS 0x75A1
#define MATROSKA_ID_BLOCKMORE 0xA6
#define MATROSKA_ID_BLOCKADDID 0xEE
#define MATROSKA_ID_BLOCKADDITIONAL 0xA5
#define MATROSKA_ID_SIMPLEBLOCK 0xA3

/* IDs in the blockgroup master */
#define MATROSKA_ID_BLOCK      0xA1
#define MATROSKA_ID_BLOCKDURATION 0x9B
#define MATROSKA_ID_BLOCKREFERENCE 0xFB
#define MATROSKA_ID_CODECSTATE 0xA4
#define MATROSKA_ID_DISCARDPADDING 0x75A2

/* IDs in the attachments master */
#define MATROSKA_ID_ATTACHEDFILE        0x61A7
#define MATROSKA_ID_FILEDESC            0x467E
#define MATROSKA_ID_FILENAME            0x466E
#define MATROSKA_ID_FILEMIMETYPE        0x4660
#define MATROSKA_ID_FILEDATA            0x465C
#define MATROSKA_ID_FILEUID             0x46AE

/* IDs in the chapters master */
#define MATROSKA_ID_EDITIONENTRY        0x45B9
#define MATROSKA_ID_CHAPTERATOM         0xB6
#define MATROSKA_ID_CHAPTERTIMESTART    0x91
#define MATROSKA_ID_CHAPTERTIMEEND      0x92
#define MATROSKA_ID_CHAPTERDISPLAY      0x80
#define MATROSKA_ID_CHAPSTRING          0x85
#define MATROSKA_ID_CHAPLANG            0x437C
#define MATROSKA_ID_CHAPCOUNTRY         0x437E
#define MATROSKA_ID_EDITIONUID          0x45BC
#define MATROSKA_ID_EDITIONFLAGHIDDEN   0x45BD
#define MATROSKA_ID_EDITIONFLAGDEFAULT  0x45DB
#define MATROSKA_ID_EDITIONFLAGORDERED  0x45DD
#define MATROSKA_ID_CHAPTERUID          0x73C4
#define MATROSKA_ID_CHAPTERFLAGHIDDEN   0x98
#define MATROSKA_ID_CHAPTERFLAGENABLED  0x4598
#define MATROSKA_ID_CHAPTERPHYSEQUIV    0x63C3

typedef enum {
  MATROSKA_TRACK_TYPE_NONE     = 0x0,
  MATROSKA_TRACK_TYPE_VIDEO    = 0x1,
  MATROSKA_TRACK_TYPE_AUDIO    = 0x2,
  MATROSKA_TRACK_TYPE_COMPLEX  = 0x3,
  MATROSKA_TRACK_TYPE_LOGO     = 0x10,
  MATROSKA_TRACK_TYPE_SUBTITLE = 0x11,
  MATROSKA_TRACK_TYPE_CONTROL  = 0x20,
  MATROSKA_TRACK_TYPE_METADATA = 0x21,
} MatroskaTrackType;

typedef enum {
  MATROSKA_TRACK_ENCODING_COMP_ZLIB        = 0,
  MATROSKA_TRACK_ENCODING_COMP_BZLIB       = 1,
  MATROSKA_TRACK_ENCODING_COMP_LZO         = 2,
  MATROSKA_TRACK_ENCODING_COMP_HEADERSTRIP = 3,
} MatroskaTrackEncodingCompAlgo;

typedef enum {
    MATROSKA_VIDEO_INTERLACE_FLAG_UNDETERMINED = 0,
    MATROSKA_VIDEO_INTERLACE_FLAG_INTERLACED   = 1,
    MATROSKA_VIDEO_INTERLACE_FLAG_PROGRESSIVE  = 2
} MatroskaVideoInterlaceFlag;

typedef enum {
    MATROSKA_VIDEO_FIELDORDER_PROGRESSIVE  = 0,
    MATROSKA_VIDEO_FIELDORDER_UNDETERMINED = 2,
    MATROSKA_VIDEO_FIELDORDER_TT           = 1,
    MATROSKA_VIDEO_FIELDORDER_BB           = 6,
    MATROSKA_VIDEO_FIELDORDER_TB           = 9,
    MATROSKA_VIDEO_FIELDORDER_BT           = 14,
} MatroskaVideoFieldOrder;

typedef enum {
  MATROSKA_VIDEO_STEREOMODE_TYPE_MONO               = 0,
  MATROSKA_VIDEO_STEREOMODE_TYPE_LEFT_RIGHT         = 1,
  MATROSKA_VIDEO_STEREOMODE_TYPE_BOTTOM_TOP         = 2,
  MATROSKA_VIDEO_STEREOMODE_TYPE_TOP_BOTTOM         = 3,
  MATROSKA_VIDEO_STEREOMODE_TYPE_CHECKERBOARD_RL    = 4,
  MATROSKA_VIDEO_STEREOMODE_TYPE_CHECKERBOARD_LR    = 5,
  MATROSKA_VIDEO_STEREOMODE_TYPE_ROW_INTERLEAVED_RL = 6,
  MATROSKA_VIDEO_STEREOMODE_TYPE_ROW_INTERLEAVED_LR = 7,
  MATROSKA_VIDEO_STEREOMODE_TYPE_COL_INTERLEAVED_RL = 8,
  MATROSKA_VIDEO_STEREOMODE_TYPE_COL_INTERLEAVED_LR = 9,
  MATROSKA_VIDEO_STEREOMODE_TYPE_ANAGLYPH_CYAN_RED  = 10,
  MATROSKA_VIDEO_STEREOMODE_TYPE_RIGHT_LEFT         = 11,
  MATROSKA_VIDEO_STEREOMODE_TYPE_ANAGLYPH_GREEN_MAG = 12,
  MATROSKA_VIDEO_STEREOMODE_TYPE_BOTH_EYES_BLOCK_LR = 13,
  MATROSKA_VIDEO_STEREOMODE_TYPE_BOTH_EYES_BLOCK_RL = 14,
  MATROSKA_VIDEO_STEREOMODE_TYPE_NB,
} MatroskaVideoStereoModeType;

typedef enum {
  MATROSKA_VIDEO_DISPLAYUNIT_PIXELS      = 0,
  MATROSKA_VIDEO_DISPLAYUNIT_CENTIMETERS = 1,
  MATROSKA_VIDEO_DISPLAYUNIT_INCHES      = 2,
  MATROSKA_VIDEO_DISPLAYUNIT_DAR         = 3,
  MATROSKA_VIDEO_DISPLAYUNIT_UNKNOWN     = 4,
} MatroskaVideoDisplayUnit;

typedef enum {
  MATROSKA_COLOUR_CHROMASITINGHORZ_UNDETERMINED     = 0,
  MATROSKA_COLOUR_CHROMASITINGHORZ_LEFT             = 1,
  MATROSKA_COLOUR_CHROMASITINGHORZ_HALF             = 2,
  MATROSKA_COLOUR_CHROMASITINGHORZ_NB
} MatroskaColourChromaSitingHorz;

typedef enum {
  MATROSKA_COLOUR_CHROMASITINGVERT_UNDETERMINED     = 0,
  MATROSKA_COLOUR_CHROMASITINGVERT_TOP              = 1,
  MATROSKA_COLOUR_CHROMASITINGVERT_HALF             = 2,
  MATROSKA_COLOUR_CHROMASITINGVERT_NB
} MatroskaColourChromaSitingVert;

typedef enum {
  MATROSKA_VIDEO_PROJECTION_TYPE_RECTANGULAR        = 0,
  MATROSKA_VIDEO_PROJECTION_TYPE_EQUIRECTANGULAR    = 1,
  MATROSKA_VIDEO_PROJECTION_TYPE_CUBEMAP            = 2,
  MATROSKA_VIDEO_PROJECTION_TYPE_MESH               = 3,
} MatroskaVideoProjectionType;











typedef enum {
    EBML_NONE,
    EBML_UINT,
    EBML_FLOAT,
    EBML_STR,
    EBML_UTF8,
    EBML_BIN,
    EBML_NEST,
    EBML_LEVEL1,
    EBML_PASS,
    EBML_STOP,
    EBML_SINT,
    EBML_TYPE_COUNT
} EbmlType;

typedef const struct EbmlSyntax_
{
    uint32_t id;
    EbmlType type;
    int list_elem_size;
    int data_offset;
    union {
        int64_t     i;
        uint64_t    u;
        double      f;
        const char *s;
        const struct EbmlSyntax_ *n;
    } def;
} EbmlSyntax;


typedef struct EbmlList_ {
    int nb_elem;
    void *elem;
} EbmlList;

typedef struct EbmlBin_ {
    int      size;
    uint8_t *data;
    int64_t  pos;
} EbmlBin;


typedef struct Ebml_ {
    uint64_t version;
    uint64_t max_size;
    uint64_t id_length;
    char    *doctype;
    uint64_t doctype_version;
} Ebml;


typedef struct MatroskaLevel_ {
    uint64_t start;
    uint64_t length;
} MatroskaLevel;

typedef struct MatroskaLevel1Element_ {
    uint64_t id;
    uint64_t pos;
    int parsed;
} MatroskaLevel1Element;

typedef struct MatroskaCluster_ {
    uint64_t timecode;
    EbmlList blocks;
} MatroskaCluster;


typedef struct MatroskaDemuxContext_ {
    /* EBML stuff */
    int num_levels;
    MatroskaLevel levels[EBML_MAX_DEPTH];
    int level_up;
    uint32_t current_id;

    uint64_t time_scale;
    double   duration;
    char    *title;
    char    *muxingapp;
    EbmlBin date_utc;
    EbmlList tracks;
    EbmlList attachments;
    EbmlList chapters;
    EbmlList index;
    EbmlList tags;
    EbmlList seekhead;

    /* byte position of the segment inside the stream */
    int64_t segment_start;

    int done;

    /* What to skip before effectively reading a packet. */
    int skip_to_keyframe;
    uint64_t skip_to_timecode;

    /* File has a CUES element, but we defer parsing until it is needed. */
    int cues_parsing_deferred;

    /* Level1 elements and whether they were read yet */
    MatroskaLevel1Element level1_elems[64];
    int num_level1_elems;

    int current_cluster_num_blocks;
    int64_t current_cluster_pos;
    MatroskaCluster current_cluster;

    /* File has SSA subtitles which prevent incremental cluster parsing. */
    int contains_ssa;

    /* Bandwidth value for WebM DASH Manifest */
    int bandwidth;
} MatroskaDemuxContext;


typedef struct MatroskaTrackCompression_ {
    uint64_t algo;
    EbmlBin  settings;
} MatroskaTrackCompression;

typedef struct MatroskaTrackEncryption_ {
    uint64_t algo;
    EbmlBin  key_id;
} MatroskaTrackEncryption;

typedef struct MatroskaTrackEncoding_ {
    uint64_t scope;
    uint64_t type;
    MatroskaTrackCompression compression;
    MatroskaTrackEncryption encryption;
} MatroskaTrackEncoding;

typedef struct MatroskaMasteringMeta_ {
    double r_x;
    double r_y;
    double g_x;
    double g_y;
    double b_x;
    double b_y;
    double white_x;
    double white_y;
    double max_luminance;
    double min_luminance;
} MatroskaMasteringMeta;

typedef struct MatroskaTrackVideoColor_ {
    uint64_t matrix_coefficients;
    uint64_t bits_per_channel;
    uint64_t chroma_sub_horz;
    uint64_t chroma_sub_vert;
    uint64_t cb_sub_horz;
    uint64_t cb_sub_vert;
    uint64_t chroma_siting_horz;
    uint64_t chroma_siting_vert;
    uint64_t range;
    uint64_t transfer_characteristics;
    uint64_t primaries;
    uint64_t max_cll;
    uint64_t max_fall;
    MatroskaMasteringMeta mastering_meta;
} MatroskaTrackVideoColor;

typedef struct MatroskaTrackVideoProjection_ {
    uint64_t type;
    EbmlBin priv; // was private
    double yaw;
    double pitch;
    double roll;
} MatroskaTrackVideoProjection;

typedef struct MatroskaTrackVideo_ {
    double   frame_rate;
    uint64_t display_width;
    uint64_t display_height;
    uint64_t pixel_width;
    uint64_t pixel_height;
    EbmlBin color_space;
    uint64_t display_unit;
    uint64_t interlaced;
    uint64_t field_order;
    uint64_t stereo_mode;
    uint64_t alpha_mode;
    EbmlList color;
    MatroskaTrackVideoProjection projection;
} MatroskaTrackVideo;

typedef struct MatroskaTrackAudio_ {
    double   samplerate;
    double   out_samplerate;
    uint64_t bitdepth;
    uint64_t channels;

    /* real audio header (extracted from extradata) */
    int      coded_framesize;
    int      sub_packet_h;
    int      frame_size;
    int      sub_packet_size;
    int      sub_packet_cnt;
    int      pkt_cnt;
    uint64_t buf_timecode;
    uint8_t *buf;
} MatroskaTrackAudio;

typedef struct MatroskaTrackPlane_ {
    uint64_t uid;
    uint64_t type;
} MatroskaTrackPlane;

typedef struct MatroskaTrackOperation_ {
    EbmlList combine_planes;
} MatroskaTrackOperation;

typedef struct MatroskaTrack_ {
    uint64_t num;
    uint64_t uid;
    uint64_t type;
    char    *name;
    char    *codec_id;
    EbmlBin  codec_priv;
    char    *language;
    double time_scale;
    uint64_t default_duration;
    uint64_t flag_default;
    uint64_t flag_forced;
    uint64_t seek_preroll;
    MatroskaTrackVideo video;
    MatroskaTrackAudio audio;
    MatroskaTrackOperation operation;
    EbmlList encodings;
    uint64_t codec_delay;
    uint64_t codec_delay_in_track_tb;

//    AVStream *stream;
    int64_t end_timecode;
    int ms_compat;
    uint64_t max_block_additional_id;

    uint32_t palette[PALETTE_COUNT];
    int has_palette;
} MatroskaTrack;

typedef struct MatroskaAttachment_ {
    uint64_t uid;
    char *filename;
    char *mime;
    EbmlBin bin;

//    AVStream *stream;
} MatroskaAttachment;

typedef struct MatroskaChapter_ {
    uint64_t start;
    uint64_t end;
    uint64_t uid;
    char    *title;

//    AVChapter *chapter;
} MatroskaChapter;

typedef struct MatroskaIndexPos_ {
    uint64_t track;
    uint64_t pos;
} MatroskaIndexPos;

typedef struct MatroskaIndex_ {
    uint64_t time;
    EbmlList pos;
} MatroskaIndex;

typedef struct MatroskaTag_ {
    char *name;
    char *string;
    char *lang;
    uint64_t def;
    EbmlList sub;
} MatroskaTag;

typedef struct MatroskaTagTarget_ {
    char    *type;
    uint64_t typevalue;
    uint64_t trackuid;
    uint64_t chapteruid;
    uint64_t attachuid;
} MatroskaTagTarget;

typedef struct MatroskaTags_ {
    MatroskaTagTarget target;
    EbmlList tag;
} MatroskaTags;

typedef struct MatroskaSeekhead_ {
    uint64_t id;
    uint64_t pos;
} MatroskaSeekhead;

typedef struct MatroskaBlock_ {
    uint64_t duration;
    int64_t  reference;
    uint64_t non_simple;
    EbmlBin  bin;
    uint64_t additional_id;
    EbmlBin  additional;
    int64_t discard_padding;
} MatroskaBlock;

static const EbmlSyntax ebml_header[] = {
    { EBML_ID_EBMLREADVERSION,    EBML_UINT, 0, offsetof(Ebml, version),         { .u = EBML_VERSION } },
    { EBML_ID_EBMLMAXSIZELENGTH,  EBML_UINT, 0, offsetof(Ebml, max_size),        { .u = 8 } },
    { EBML_ID_EBMLMAXIDLENGTH,    EBML_UINT, 0, offsetof(Ebml, id_length),       { .u = 4 } },
    { EBML_ID_DOCTYPE,            EBML_STR,  0, offsetof(Ebml, doctype),         { .s = "(none)" } },
    { EBML_ID_DOCTYPEREADVERSION, EBML_UINT, 0, offsetof(Ebml, doctype_version), { .u = 1 } },
    { EBML_ID_EBMLVERSION,        EBML_NONE },
    { EBML_ID_DOCTYPEVERSION,     EBML_NONE },
    { 0 }
};

static const EbmlSyntax ebml_syntax[] = {
    { EBML_ID_HEADER, EBML_NEST, 0, 0, { .n = ebml_header } },
    { 0 }
};


static const EbmlSyntax matroska_info[] = {
    { MATROSKA_ID_TIMECODESCALE, EBML_UINT,  0, offsetof(MatroskaDemuxContext, time_scale), { .u = 1000000 } },
    { MATROSKA_ID_DURATION,      EBML_FLOAT, 0, offsetof(MatroskaDemuxContext, duration) },
    { MATROSKA_ID_TITLE,         EBML_UTF8,  0, offsetof(MatroskaDemuxContext, title) },
    { MATROSKA_ID_WRITINGAPP,    EBML_NONE },
    { MATROSKA_ID_MUXINGAPP,     EBML_UTF8, 0, offsetof(MatroskaDemuxContext, muxingapp) },
    { MATROSKA_ID_DATEUTC,       EBML_BIN,  0, offsetof(MatroskaDemuxContext, date_utc) },
    { MATROSKA_ID_SEGMENTUID,    EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_mastering_meta[] = {
    { MATROSKA_ID_VIDEOCOLOR_RX, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, r_x), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_RY, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, r_y), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_GX, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, g_x), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_GY, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, g_y), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_BX, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, b_x), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_BY, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, b_y), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_WHITEX, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, white_x), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_WHITEY, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, white_y), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_LUMINANCEMIN, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, min_luminance), { .f=-1 } },
    { MATROSKA_ID_VIDEOCOLOR_LUMINANCEMAX, EBML_FLOAT, 0, offsetof(MatroskaMasteringMeta, max_luminance), { .f=-1 } },
    { 0 }
};

static const EbmlSyntax matroska_track_video_color[] = {
    { MATROSKA_ID_VIDEOCOLORMATRIXCOEFF,      EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, matrix_coefficients), { .u = AVCOL_SPC_UNSPECIFIED } },
    { MATROSKA_ID_VIDEOCOLORBITSPERCHANNEL,   EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, bits_per_channel), { .u=0 } },
    { MATROSKA_ID_VIDEOCOLORCHROMASUBHORZ,    EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, chroma_sub_horz), { .u=0 } },
    { MATROSKA_ID_VIDEOCOLORCHROMASUBVERT,    EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, chroma_sub_vert), { .u=0 } },
    { MATROSKA_ID_VIDEOCOLORCBSUBHORZ,        EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, cb_sub_horz), { .u=0 } },
    { MATROSKA_ID_VIDEOCOLORCBSUBVERT,        EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, cb_sub_vert), { .u=0 } },
    { MATROSKA_ID_VIDEOCOLORCHROMASITINGHORZ, EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, chroma_siting_horz), { .u = MATROSKA_COLOUR_CHROMASITINGHORZ_UNDETERMINED } },
    { MATROSKA_ID_VIDEOCOLORCHROMASITINGVERT, EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, chroma_siting_vert), { .u = MATROSKA_COLOUR_CHROMASITINGVERT_UNDETERMINED } },
    { MATROSKA_ID_VIDEOCOLORRANGE,            EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, range), { .u = AVCOL_RANGE_UNSPECIFIED } },
    { MATROSKA_ID_VIDEOCOLORTRANSFERCHARACTERISTICS, EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, transfer_characteristics), { .u = AVCOL_TRC_UNSPECIFIED } },
    { MATROSKA_ID_VIDEOCOLORPRIMARIES,        EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, primaries), { .u = AVCOL_PRI_UNSPECIFIED } },
    { MATROSKA_ID_VIDEOCOLORMAXCLL,           EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, max_cll), { .u=0 } },
    { MATROSKA_ID_VIDEOCOLORMAXFALL,          EBML_UINT, 0, offsetof(MatroskaTrackVideoColor, max_fall), { .u=0 } },
    { MATROSKA_ID_VIDEOCOLORMASTERINGMETA,    EBML_NEST, 0, offsetof(MatroskaTrackVideoColor, mastering_meta), { .n = matroska_mastering_meta } },
    { 0 }
};

static const EbmlSyntax matroska_track_video_projection[] = {
    { MATROSKA_ID_VIDEOPROJECTIONTYPE,        EBML_UINT,  0, offsetof(MatroskaTrackVideoProjection, type), { .u = MATROSKA_VIDEO_PROJECTION_TYPE_RECTANGULAR } },
    { MATROSKA_ID_VIDEOPROJECTIONPRIVATE,     EBML_BIN,   0, offsetof(MatroskaTrackVideoProjection, priv) },
    { MATROSKA_ID_VIDEOPROJECTIONPOSEYAW,     EBML_FLOAT, 0, offsetof(MatroskaTrackVideoProjection, yaw), { .f=0.0 } },
    { MATROSKA_ID_VIDEOPROJECTIONPOSEPITCH,   EBML_FLOAT, 0, offsetof(MatroskaTrackVideoProjection, pitch), { .f=0.0 } },
    { MATROSKA_ID_VIDEOPROJECTIONPOSEROLL,    EBML_FLOAT, 0, offsetof(MatroskaTrackVideoProjection, roll), { .f=0.0 } },
    { 0 }
};

static const EbmlSyntax matroska_track_video[] = {
    { MATROSKA_ID_VIDEOFRAMERATE,      EBML_FLOAT, 0, offsetof(MatroskaTrackVideo, frame_rate) },
    { MATROSKA_ID_VIDEODISPLAYWIDTH,   EBML_UINT,  0, offsetof(MatroskaTrackVideo, display_width), { .u=-1 } },
    { MATROSKA_ID_VIDEODISPLAYHEIGHT,  EBML_UINT,  0, offsetof(MatroskaTrackVideo, display_height), { .u=-1 } },
    { MATROSKA_ID_VIDEOPIXELWIDTH,     EBML_UINT,  0, offsetof(MatroskaTrackVideo, pixel_width) },
    { MATROSKA_ID_VIDEOPIXELHEIGHT,    EBML_UINT,  0, offsetof(MatroskaTrackVideo, pixel_height) },
    { MATROSKA_ID_VIDEOCOLORSPACE,     EBML_BIN,   0, offsetof(MatroskaTrackVideo, color_space) },
    { MATROSKA_ID_VIDEOALPHAMODE,      EBML_UINT,  0, offsetof(MatroskaTrackVideo, alpha_mode) },
    { MATROSKA_ID_VIDEOCOLOR,          EBML_NEST,  sizeof(MatroskaTrackVideoColor), offsetof(MatroskaTrackVideo, color), { .n = matroska_track_video_color } },
    { MATROSKA_ID_VIDEOPROJECTION,     EBML_NEST,  0, offsetof(MatroskaTrackVideo, projection), { .n = matroska_track_video_projection } },
    { MATROSKA_ID_VIDEOPIXELCROPB,     EBML_NONE },
    { MATROSKA_ID_VIDEOPIXELCROPT,     EBML_NONE },
    { MATROSKA_ID_VIDEOPIXELCROPL,     EBML_NONE },
    { MATROSKA_ID_VIDEOPIXELCROPR,     EBML_NONE },
    { MATROSKA_ID_VIDEODISPLAYUNIT,    EBML_UINT,  0, offsetof(MatroskaTrackVideo, display_unit), { .u= MATROSKA_VIDEO_DISPLAYUNIT_PIXELS } },
    { MATROSKA_ID_VIDEOFLAGINTERLACED, EBML_UINT,  0, offsetof(MatroskaTrackVideo, interlaced),  { .u = MATROSKA_VIDEO_INTERLACE_FLAG_UNDETERMINED } },
    { MATROSKA_ID_VIDEOFIELDORDER,     EBML_UINT,  0, offsetof(MatroskaTrackVideo, field_order), { .u = MATROSKA_VIDEO_FIELDORDER_UNDETERMINED } },
    { MATROSKA_ID_VIDEOSTEREOMODE,     EBML_UINT,  0, offsetof(MatroskaTrackVideo, stereo_mode), { .u = MATROSKA_VIDEO_STEREOMODE_TYPE_NB } },
    { MATROSKA_ID_VIDEOASPECTRATIO,    EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_track_audio[] = {
    { MATROSKA_ID_AUDIOSAMPLINGFREQ,    EBML_FLOAT, 0, offsetof(MatroskaTrackAudio, samplerate), { .f = 8000.0 } },
    { MATROSKA_ID_AUDIOOUTSAMPLINGFREQ, EBML_FLOAT, 0, offsetof(MatroskaTrackAudio, out_samplerate) },
    { MATROSKA_ID_AUDIOBITDEPTH,        EBML_UINT,  0, offsetof(MatroskaTrackAudio, bitdepth) },
    { MATROSKA_ID_AUDIOCHANNELS,        EBML_UINT,  0, offsetof(MatroskaTrackAudio, channels),   { .u = 1 } },
    { 0 }
};

static const EbmlSyntax matroska_track_encoding_compression[] = {
    { MATROSKA_ID_ENCODINGCOMPALGO,     EBML_UINT, 0, offsetof(MatroskaTrackCompression, algo), { .u = 0 } },
    { MATROSKA_ID_ENCODINGCOMPSETTINGS, EBML_BIN,  0, offsetof(MatroskaTrackCompression, settings) },
    { 0 }
};

static const EbmlSyntax matroska_track_encoding_encryption[] = {
    { MATROSKA_ID_ENCODINGENCALGO,        EBML_UINT, 0, offsetof(MatroskaTrackEncryption,algo), {.u = 0} },
    { MATROSKA_ID_ENCODINGENCKEYID,       EBML_BIN, 0, offsetof(MatroskaTrackEncryption,key_id) },
    { MATROSKA_ID_ENCODINGENCAESSETTINGS, EBML_NONE },
    { MATROSKA_ID_ENCODINGSIGALGO,        EBML_NONE },
    { MATROSKA_ID_ENCODINGSIGHASHALGO,    EBML_NONE },
    { MATROSKA_ID_ENCODINGSIGKEYID,       EBML_NONE },
    { MATROSKA_ID_ENCODINGSIGNATURE,      EBML_NONE },
    { 0 }
};
static const EbmlSyntax matroska_track_encoding[] = {
    { MATROSKA_ID_ENCODINGSCOPE,       EBML_UINT, 0, offsetof(MatroskaTrackEncoding, scope),       { .u = 1 } },
    { MATROSKA_ID_ENCODINGTYPE,        EBML_UINT, 0, offsetof(MatroskaTrackEncoding, type),        { .u = 0 } },
    { MATROSKA_ID_ENCODINGCOMPRESSION, EBML_NEST, 0, offsetof(MatroskaTrackEncoding, compression), { .n = matroska_track_encoding_compression } },
    { MATROSKA_ID_ENCODINGENCRYPTION,  EBML_NEST, 0, offsetof(MatroskaTrackEncoding, encryption),  { .n = matroska_track_encoding_encryption } },
    { MATROSKA_ID_ENCODINGORDER,       EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_track_encodings[] = {
    { MATROSKA_ID_TRACKCONTENTENCODING, EBML_NEST, sizeof(MatroskaTrackEncoding), offsetof(MatroskaTrack, encodings), { .n = matroska_track_encoding } },
    { 0 }
};

static const EbmlSyntax matroska_track_plane[] = {
    { MATROSKA_ID_TRACKPLANEUID,  EBML_UINT, 0, offsetof(MatroskaTrackPlane,uid) },
    { MATROSKA_ID_TRACKPLANETYPE, EBML_UINT, 0, offsetof(MatroskaTrackPlane,type) },
    { 0 }
};

static const EbmlSyntax matroska_track_combine_planes[] = {
    { MATROSKA_ID_TRACKPLANE, EBML_NEST, sizeof(MatroskaTrackPlane), offsetof(MatroskaTrackOperation,combine_planes), {.n = matroska_track_plane} },
    { 0 }
};

static const EbmlSyntax matroska_track_operation[] = {
    { MATROSKA_ID_TRACKCOMBINEPLANES, EBML_NEST, 0, 0, {.n = matroska_track_combine_planes} },
    { 0 }
};

static const EbmlSyntax matroska_track[] = {
    { MATROSKA_ID_TRACKNUMBER,           EBML_UINT,  0, offsetof(MatroskaTrack, num) },
    { MATROSKA_ID_TRACKNAME,             EBML_UTF8,  0, offsetof(MatroskaTrack, name) },
    { MATROSKA_ID_TRACKUID,              EBML_UINT,  0, offsetof(MatroskaTrack, uid) },
    { MATROSKA_ID_TRACKTYPE,             EBML_UINT,  0, offsetof(MatroskaTrack, type) },
    { MATROSKA_ID_CODECID,               EBML_STR,   0, offsetof(MatroskaTrack, codec_id) },
    { MATROSKA_ID_CODECPRIVATE,          EBML_BIN,   0, offsetof(MatroskaTrack, codec_priv) },
    { MATROSKA_ID_CODECDELAY,            EBML_UINT,  0, offsetof(MatroskaTrack, codec_delay) },
    { MATROSKA_ID_TRACKLANGUAGE,         EBML_UTF8,  0, offsetof(MatroskaTrack, language),     { .s = "eng" } },
    { MATROSKA_ID_TRACKDEFAULTDURATION,  EBML_UINT,  0, offsetof(MatroskaTrack, default_duration) },
    { MATROSKA_ID_TRACKTIMECODESCALE,    EBML_FLOAT, 0, offsetof(MatroskaTrack, time_scale),   { .f = 1.0 } },
    { MATROSKA_ID_TRACKFLAGDEFAULT,      EBML_UINT,  0, offsetof(MatroskaTrack, flag_default), { .u = 1 } },
    { MATROSKA_ID_TRACKFLAGFORCED,       EBML_UINT,  0, offsetof(MatroskaTrack, flag_forced),  { .u = 0 } },
    { MATROSKA_ID_TRACKVIDEO,            EBML_NEST,  0, offsetof(MatroskaTrack, video),        { .n = matroska_track_video } },
    { MATROSKA_ID_TRACKAUDIO,            EBML_NEST,  0, offsetof(MatroskaTrack, audio),        { .n = matroska_track_audio } },
    { MATROSKA_ID_TRACKOPERATION,        EBML_NEST,  0, offsetof(MatroskaTrack, operation),    { .n = matroska_track_operation } },
    { MATROSKA_ID_TRACKCONTENTENCODINGS, EBML_NEST,  0, 0,                                     { .n = matroska_track_encodings } },
    { MATROSKA_ID_TRACKMAXBLKADDID,      EBML_UINT,  0, offsetof(MatroskaTrack, max_block_additional_id) },
    { MATROSKA_ID_SEEKPREROLL,           EBML_UINT,  0, offsetof(MatroskaTrack, seek_preroll) },
    { MATROSKA_ID_TRACKFLAGENABLED,      EBML_NONE },
    { MATROSKA_ID_TRACKFLAGLACING,       EBML_NONE },
    { MATROSKA_ID_CODECNAME,             EBML_NONE },
    { MATROSKA_ID_CODECDECODEALL,        EBML_NONE },
    { MATROSKA_ID_CODECINFOURL,          EBML_NONE },
    { MATROSKA_ID_CODECDOWNLOADURL,      EBML_NONE },
    { MATROSKA_ID_TRACKMINCACHE,         EBML_NONE },
    { MATROSKA_ID_TRACKMAXCACHE,         EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_tracks[] = {
    { MATROSKA_ID_TRACKENTRY, EBML_NEST, sizeof(MatroskaTrack), offsetof(MatroskaDemuxContext, tracks), { .n = matroska_track } },
    { 0 }
};

static const EbmlSyntax matroska_attachment[] = {
    { MATROSKA_ID_FILEUID,      EBML_UINT, 0, offsetof(MatroskaAttachment, uid) },
    { MATROSKA_ID_FILENAME,     EBML_UTF8, 0, offsetof(MatroskaAttachment, filename) },
    { MATROSKA_ID_FILEMIMETYPE, EBML_STR,  0, offsetof(MatroskaAttachment, mime) },
    { MATROSKA_ID_FILEDATA,     EBML_BIN,  0, offsetof(MatroskaAttachment, bin) },
    { MATROSKA_ID_FILEDESC,     EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_attachments[] = {
    { MATROSKA_ID_ATTACHEDFILE, EBML_NEST, sizeof(MatroskaAttachment), offsetof(MatroskaDemuxContext, attachments), { .n = matroska_attachment } },
    { 0 }
};

static const EbmlSyntax matroska_chapter_display[] = {
    { MATROSKA_ID_CHAPSTRING,  EBML_UTF8, 0, offsetof(MatroskaChapter, title) },
    { MATROSKA_ID_CHAPLANG,    EBML_NONE },
    { MATROSKA_ID_CHAPCOUNTRY, EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_chapter_entry[] = {
    { MATROSKA_ID_CHAPTERTIMESTART,   EBML_UINT, 0, offsetof(MatroskaChapter, start), { .u = AV_NOPTS_VALUE } },
    { MATROSKA_ID_CHAPTERTIMEEND,     EBML_UINT, 0, offsetof(MatroskaChapter, end),   { .u = AV_NOPTS_VALUE } },
    { MATROSKA_ID_CHAPTERUID,         EBML_UINT, 0, offsetof(MatroskaChapter, uid) },
    { MATROSKA_ID_CHAPTERDISPLAY,     EBML_NEST, 0,                        0,         { .n = matroska_chapter_display } },
    { MATROSKA_ID_CHAPTERFLAGHIDDEN,  EBML_NONE },
    { MATROSKA_ID_CHAPTERFLAGENABLED, EBML_NONE },
    { MATROSKA_ID_CHAPTERPHYSEQUIV,   EBML_NONE },
    { MATROSKA_ID_CHAPTERATOM,        EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_chapter[] = {
    { MATROSKA_ID_CHAPTERATOM,        EBML_NEST, sizeof(MatroskaChapter), offsetof(MatroskaDemuxContext, chapters), { .n = matroska_chapter_entry } },
    { MATROSKA_ID_EDITIONUID,         EBML_NONE },
    { MATROSKA_ID_EDITIONFLAGHIDDEN,  EBML_NONE },
    { MATROSKA_ID_EDITIONFLAGDEFAULT, EBML_NONE },
    { MATROSKA_ID_EDITIONFLAGORDERED, EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_chapters[] = {
    { MATROSKA_ID_EDITIONENTRY, EBML_NEST, 0, 0, { .n = matroska_chapter } },
    { 0 }
};

static const EbmlSyntax matroska_index_pos[] = {
    { MATROSKA_ID_CUETRACK,           EBML_UINT, 0, offsetof(MatroskaIndexPos, track) },
    { MATROSKA_ID_CUECLUSTERPOSITION, EBML_UINT, 0, offsetof(MatroskaIndexPos, pos) },
    { MATROSKA_ID_CUERELATIVEPOSITION,EBML_NONE },
    { MATROSKA_ID_CUEDURATION,        EBML_NONE },
    { MATROSKA_ID_CUEBLOCKNUMBER,     EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_index_entry[] = {
    { MATROSKA_ID_CUETIME,          EBML_UINT, 0,                        offsetof(MatroskaIndex, time) },
    { MATROSKA_ID_CUETRACKPOSITION, EBML_NEST, sizeof(MatroskaIndexPos), offsetof(MatroskaIndex, pos), { .n = matroska_index_pos } },
    { 0 }
};

static const EbmlSyntax matroska_index[] = {
    { MATROSKA_ID_POINTENTRY, EBML_NEST, sizeof(MatroskaIndex), offsetof(MatroskaDemuxContext, index), { .n = matroska_index_entry } },
    { 0 }
};

static const EbmlSyntax matroska_simpletag[] = {
    { MATROSKA_ID_TAGNAME,        EBML_UTF8, 0,                   offsetof(MatroskaTag, name) },
    { MATROSKA_ID_TAGSTRING,      EBML_UTF8, 0,                   offsetof(MatroskaTag, string) },
    { MATROSKA_ID_TAGLANG,        EBML_STR,  0,                   offsetof(MatroskaTag, lang), { .s = "und" } },
    { MATROSKA_ID_TAGDEFAULT,     EBML_UINT, 0,                   offsetof(MatroskaTag, def) },
    { MATROSKA_ID_TAGDEFAULT_BUG, EBML_UINT, 0,                   offsetof(MatroskaTag, def) },
    { MATROSKA_ID_SIMPLETAG,      EBML_NEST, sizeof(MatroskaTag), offsetof(MatroskaTag, sub),  { .n = matroska_simpletag } },
    { 0 }
};

static const EbmlSyntax matroska_tagtargets[] = {
    { MATROSKA_ID_TAGTARGETS_TYPE,       EBML_STR,  0, offsetof(MatroskaTagTarget, type) },
    { MATROSKA_ID_TAGTARGETS_TYPEVALUE,  EBML_UINT, 0, offsetof(MatroskaTagTarget, typevalue), { .u = 50 } },
    { MATROSKA_ID_TAGTARGETS_TRACKUID,   EBML_UINT, 0, offsetof(MatroskaTagTarget, trackuid) },
    { MATROSKA_ID_TAGTARGETS_CHAPTERUID, EBML_UINT, 0, offsetof(MatroskaTagTarget, chapteruid) },
    { MATROSKA_ID_TAGTARGETS_ATTACHUID,  EBML_UINT, 0, offsetof(MatroskaTagTarget, attachuid) },
    { 0 }
};

static const EbmlSyntax matroska_tag[] = {
    { MATROSKA_ID_SIMPLETAG,  EBML_NEST, sizeof(MatroskaTag), offsetof(MatroskaTags, tag),    { .n = matroska_simpletag } },
    { MATROSKA_ID_TAGTARGETS, EBML_NEST, 0,                   offsetof(MatroskaTags, target), { .n = matroska_tagtargets } },
    { 0 }
};

static const EbmlSyntax matroska_tags[] = {
    { MATROSKA_ID_TAG, EBML_NEST, sizeof(MatroskaTags), offsetof(MatroskaDemuxContext, tags), { .n = matroska_tag } },
    { 0 }
};

static const EbmlSyntax matroska_seekhead_entry[] = {
    { MATROSKA_ID_SEEKID,       EBML_UINT, 0, offsetof(MatroskaSeekhead, id) },
    { MATROSKA_ID_SEEKPOSITION, EBML_UINT, 0, offsetof(MatroskaSeekhead, pos), { .u = -1 } },
    { 0 }
};

static const EbmlSyntax matroska_seekhead[] = {
    { MATROSKA_ID_SEEKENTRY, EBML_NEST, sizeof(MatroskaSeekhead), offsetof(MatroskaDemuxContext, seekhead), { .n = matroska_seekhead_entry } },
    { 0 }
};

static const EbmlSyntax matroska_segment[] = {
    { MATROSKA_ID_INFO,        EBML_LEVEL1, 0, 0, { .n = matroska_info } },
    { MATROSKA_ID_TRACKS,      EBML_LEVEL1, 0, 0, { .n = matroska_tracks } },
    { MATROSKA_ID_ATTACHMENTS, EBML_LEVEL1, 0, 0, { .n = matroska_attachments } },
    { MATROSKA_ID_CHAPTERS,    EBML_LEVEL1, 0, 0, { .n = matroska_chapters } },
    { MATROSKA_ID_CUES,        EBML_LEVEL1, 0, 0, { .n = matroska_index } },
    { MATROSKA_ID_TAGS,        EBML_LEVEL1, 0, 0, { .n = matroska_tags } },
    { MATROSKA_ID_SEEKHEAD,    EBML_LEVEL1, 0, 0, { .n = matroska_seekhead } },
    { MATROSKA_ID_CLUSTER,     EBML_STOP },
    { 0 }
};

static const EbmlSyntax matroska_segments[] = {
    { MATROSKA_ID_SEGMENT, EBML_NEST, 0, 0, { .n = matroska_segment } },
    { 0 }
};

static const EbmlSyntax matroska_blockmore[] = {
    { MATROSKA_ID_BLOCKADDID,      EBML_UINT, 0, offsetof(MatroskaBlock,additional_id) },
    { MATROSKA_ID_BLOCKADDITIONAL, EBML_BIN,  0, offsetof(MatroskaBlock,additional) },
    { 0 }
};

static const EbmlSyntax matroska_blockadditions[] = {
    { MATROSKA_ID_BLOCKMORE, EBML_NEST, 0, 0, {.n = matroska_blockmore} },
    { 0 }
};

static const EbmlSyntax matroska_blockgroup[] = {
    { MATROSKA_ID_BLOCK,          EBML_BIN,  0, offsetof(MatroskaBlock, bin) },
    { MATROSKA_ID_BLOCKADDITIONS, EBML_NEST, 0, 0, { .n = matroska_blockadditions} },
    { MATROSKA_ID_SIMPLEBLOCK,    EBML_BIN,  0, offsetof(MatroskaBlock, bin) },
    { MATROSKA_ID_BLOCKDURATION,  EBML_UINT, 0, offsetof(MatroskaBlock, duration) },
    { MATROSKA_ID_DISCARDPADDING, EBML_SINT, 0, offsetof(MatroskaBlock, discard_padding) },
    { MATROSKA_ID_BLOCKREFERENCE, EBML_SINT, 0, offsetof(MatroskaBlock, reference), { .i = INT64_MIN } },
    { MATROSKA_ID_CODECSTATE,     EBML_NONE },
    {                          1, EBML_UINT, 0, offsetof(MatroskaBlock, non_simple), { .u = 1 } },
    { 0 }
};

static const EbmlSyntax matroska_cluster[] = {
    { MATROSKA_ID_CLUSTERTIMECODE, EBML_UINT, 0,                     offsetof(MatroskaCluster, timecode) },
    { MATROSKA_ID_BLOCKGROUP,      EBML_NEST, sizeof(MatroskaBlock), offsetof(MatroskaCluster, blocks), { .n = matroska_blockgroup } },
    { MATROSKA_ID_SIMPLEBLOCK,     EBML_PASS, sizeof(MatroskaBlock), offsetof(MatroskaCluster, blocks), { .n = matroska_blockgroup } },
    { MATROSKA_ID_CLUSTERPOSITION, EBML_NONE },
    { MATROSKA_ID_CLUSTERPREVSIZE, EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_clusters[] = {
    { MATROSKA_ID_CLUSTER,  EBML_NEST, 0, 0, { .n = matroska_cluster } },
    { MATROSKA_ID_INFO,     EBML_NONE },
    { MATROSKA_ID_CUES,     EBML_NONE },
    { MATROSKA_ID_TAGS,     EBML_NONE },
    { MATROSKA_ID_SEEKHEAD, EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_cluster_incremental_parsing[] = {
    { MATROSKA_ID_CLUSTERTIMECODE, EBML_UINT, 0,                     offsetof(MatroskaCluster, timecode) },
    { MATROSKA_ID_BLOCKGROUP,      EBML_NEST, sizeof(MatroskaBlock), offsetof(MatroskaCluster, blocks), { .n = matroska_blockgroup } },
    { MATROSKA_ID_SIMPLEBLOCK,     EBML_PASS, sizeof(MatroskaBlock), offsetof(MatroskaCluster, blocks), { .n = matroska_blockgroup } },
    { MATROSKA_ID_CLUSTERPOSITION, EBML_NONE },
    { MATROSKA_ID_CLUSTERPREVSIZE, EBML_NONE },
    { MATROSKA_ID_INFO,            EBML_NONE },
    { MATROSKA_ID_CUES,            EBML_NONE },
    { MATROSKA_ID_TAGS,            EBML_NONE },
    { MATROSKA_ID_SEEKHEAD,        EBML_NONE },
    { MATROSKA_ID_CLUSTER,         EBML_STOP },
    { 0 }
};

static const EbmlSyntax matroska_cluster_incremental[] = {
    { MATROSKA_ID_CLUSTERTIMECODE, EBML_UINT, 0, offsetof(MatroskaCluster, timecode) },
    { MATROSKA_ID_BLOCKGROUP,      EBML_STOP },
    { MATROSKA_ID_SIMPLEBLOCK,     EBML_STOP },
    { MATROSKA_ID_CLUSTERPOSITION, EBML_NONE },
    { MATROSKA_ID_CLUSTERPREVSIZE, EBML_NONE },
    { 0 }
};

static const EbmlSyntax matroska_clusters_incremental[] = {
    { MATROSKA_ID_CLUSTER,  EBML_NEST, 0, 0, { .n = matroska_cluster_incremental } },
    { MATROSKA_ID_INFO,     EBML_NONE },
    { MATROSKA_ID_CUES,     EBML_NONE },
    { MATROSKA_ID_TAGS,     EBML_NONE },
    { MATROSKA_ID_SEEKHEAD, EBML_NONE },
    { 0 }
};



static int sign_extend(int val, unsigned bits)
{
    unsigned shift = 8 * sizeof(int) - bits;
    union { unsigned u; int s; } v = { (unsigned) val << shift };
    return v.s >> shift;
}

// big endian readers
static uint32_t rb32(FILE *fd)
{
    uint32_t result;
    result = ((uint32_t)fgetc(fd)) << 24;
    result |= ((uint32_t)fgetc(fd)) << 16;
    result |= ((uint32_t)fgetc(fd)) << 8;
    result |= ((uint32_t)fgetc(fd));
    return result;
}

static uint64_t rb64(FILE *fd)
{
    uint64_t result;
    result = ((uint64_t)fgetc(fd)) << 56;
    result |= ((uint64_t)fgetc(fd)) << 48;
    result |= ((uint64_t)fgetc(fd)) << 40;
    result |= ((uint64_t)fgetc(fd)) << 32;
    result |= ((uint64_t)fgetc(fd)) << 24;
    result |= ((uint64_t)fgetc(fd)) << 16;
    result |= ((uint64_t)fgetc(fd)) << 8;
    result |= ((uint64_t)fgetc(fd));
    return result;
}

union intfloat32_ {
    uint32_t i;
    float    f;
};

static float int2float(uint32_t i)
{
    union intfloat32_ v;
    v.i = i;
    return v.f;
}


union intfloat64_ {
    uint64_t i;
    double   f;
};

static double int2double(uint64_t i)
{
    union intfloat64_ v;
    v.i = i;
    return v.f;
}

// free the pointer pointed to by arg
static void freep(void *arg)
{
    void *val;

    memcpy(&val, arg, sizeof(val));
    memset(arg, 0, sizeof(val));
    
    free(val);
}


class FileMKVPriv
{
public:
    FileMKVPriv();
    ~FileMKVPriv();
    
    int open(Asset *asset);
    int ebml_parse_elem(EbmlSyntax *syntax, void *data);
    int ebml_read_num(int max_size, uint64_t *number);
    int ebml_read_length(uint64_t *number);
    int ebml_read_uint(int size, uint64_t *num);
    int ebml_read_sint(int size, int64_t *num);
    int ebml_read_float(int size, double *num);
    int ebml_read_ascii(int size, char **str);
    int ebml_read_binary(int length, EbmlBin *bin);
    int ebml_read_master(uint64_t length);
    int matroska_ebmlnum_sint(uint8_t *data, uint32_t size, int64_t *num);
    int matroska_ebmlnum_uint(uint8_t *data, uint32_t size, uint64_t *num);
    MatroskaLevel1Element* matroska_find_level1_elem(uint32_t id);
    int ebml_parse_nest(EbmlSyntax *syntax, void *data);
    int ebml_level_end();
    int matroska_resync(int64_t last_pos);
    int matroska_parse_seekhead_entry(uint64_t pos);
    void matroska_execute_seekhead();
    int matroska_parse_tracks();
    int matroska_decode_buffer(uint8_t **buf, 
        int *buf_size,
        MatroskaTrack *track);
    MatroskaTrack* matroska_find_track_by_num(int num);
    int matroska_add_index_entries();
    int matroska_parse_cues();


    int ebml_parse_id(EbmlSyntax *syntax, uint32_t id, void *data);
    int ebml_parse(EbmlSyntax *syntax, void *data);
    int read_header();
    void ebml_free(EbmlSyntax *syntax, void *data);
    
    
    FILE *fd;
    Asset *asset;
    MatroskaDemuxContext context;
};

FileMKVPriv::FileMKVPriv()
{
    fd = 0;
    bzero(&context, sizeof(MatroskaDemuxContext));
}

FileMKVPriv::~FileMKVPriv()
{
    if(fd)
    {
        fclose(fd);
    }
}

int FileMKVPriv::open(Asset *asset)
{
    this->asset = asset;
    fd = fopen(asset->path, "r");
    if(!fd)
    {
        printf("FileMKVPriv::open %d: %s failed\n", __LINE__, asset->path);
        return 1;
    }

    if(read_header())
    {
        return 1;
    }
    return 0;
}


static int is_ebml_id_valid(uint32_t id)
{
    // Due to endian nonsense in Matroska, the highest byte with any bits set
    // will contain the leading length bit. This bit in turn identifies the
    // total byte length of the element by its position within the byte.
    unsigned int bits = mkv_log2(id);
    return id && (bits + 7) / 8 ==  (8 - bits % 8);
}

/*
 * Allocate and return the entry for the level1 element with the given ID. If
 * an entry already exists, return the existing entry.
 */
MatroskaLevel1Element* FileMKVPriv::matroska_find_level1_elem(uint32_t id)
{
    int i;
    MatroskaLevel1Element *elem;

    if (!is_ebml_id_valid(id))
        return NULL;

    // Some files link to all clusters; useless.
    if (id == MATROSKA_ID_CLUSTER)
        return NULL;

    // There can be multiple seekheads.
    if (id != MATROSKA_ID_SEEKHEAD) {
        for (i = 0; i < context.num_level1_elems; i++) {
            if (context.level1_elems[i].id == id)
                return &context.level1_elems[i];
        }
    }

    // Only a completely broken file would have more elements.
    // It also provides a low-effort way to escape from circular seekheads
    // (every iteration will add a level1 entry).
    if (context.num_level1_elems >= sizeof(context.level1_elems) /  
        sizeof(MatroskaLevel1Element)) 
    {
        printf("FileMKVPriv::matroska_find_level1_elem %d: Too many level1 elements or circular seekheads.\n",
            __LINE__);
        return NULL;
    }

    elem = &context.level1_elems[context.num_level1_elems++];
    *elem = (MatroskaLevel1Element){.id = id};

    return elem;
}

int FileMKVPriv::ebml_parse_elem(EbmlSyntax *syntax, void *data)
{
    static uint64_t max_lengths[EBML_TYPE_COUNT];
    
    max_lengths[EBML_UINT]  = 8;
    max_lengths[EBML_FLOAT] = 8;
// max. 16 MB for strings
    max_lengths[EBML_STR]   = 0x1000000;
    max_lengths[EBML_UTF8]  = 0x1000000;
// max. 256 MB for binary data
    max_lengths[EBML_BIN]   = 0x10000000;
// no limits for anything else
    
    uint32_t id = syntax->id;
    uint64_t length;
    int res;
    void *newelem;
    MatroskaLevel1Element *level1_elem;

    data = (char *) data + syntax->data_offset;

    if (syntax->list_elem_size) 
    {
        EbmlList *list = (EbmlList*)data;
        newelem = realloc(list->elem, 
            (list->nb_elem + 1) * syntax->list_elem_size);
 
        if (!newelem)
            return -1;
        list->elem = newelem;
        data = (char *) list->elem + list->nb_elem * syntax->list_elem_size;
        memset(data, 0, syntax->list_elem_size);
        list->nb_elem++;
    }

    if (syntax->type != EBML_PASS && syntax->type != EBML_STOP) {
        context.current_id = 0;
        if ((res = ebml_read_length(&length)) < 0)
            return res;
        if (max_lengths[syntax->type] && length > max_lengths[syntax->type]) {
            printf("FileMKVPriv::ebml_parse_elem %d: Invalid length %ld > %ld for syntax element %i\n",
                   __LINE__,
                   length, 
                   max_lengths[syntax->type], 
                   syntax->type);
            return -1;
        }
    }

    switch (syntax->type) {
    case EBML_UINT:
        res = ebml_read_uint(length, (uint64_t*)data);
        break;
    case EBML_SINT:
        res = ebml_read_sint(length, (int64_t*)data);
        break;
    case EBML_FLOAT:
        res = ebml_read_float(length, (double*)data);
        break;
    case EBML_STR:
    case EBML_UTF8:
        res = ebml_read_ascii(length, (char**)data);
        break;
    case EBML_BIN:
        res = ebml_read_binary(length, (EbmlBin*)data);
        break;
    case EBML_LEVEL1:
    case EBML_NEST:
        if ((res = ebml_read_master(length)) < 0)
            return res;
        if (id == MATROSKA_ID_SEGMENT)
            context.segment_start = ftell(fd);
        if (id == MATROSKA_ID_CUES)
            context.cues_parsing_deferred = 0;
        if (syntax->type == EBML_LEVEL1 &&
            (level1_elem = matroska_find_level1_elem(syntax->id))) {
            if (level1_elem->parsed)
                printf("FileMKVPriv::ebml_parse_elem %d: Duplicate element\n", __LINE__);
            level1_elem->parsed = 1;
        }
        return ebml_parse_nest(syntax->def.n, data);
    case EBML_PASS:
        return ebml_parse_id(syntax->def.n, id, data);
    case EBML_STOP:
        return 1;
    default:
        return fseek(fd, length, SEEK_CUR);
    }
    
    
    if (res)
    {
        printf("FileMKVPriv::ebml_parse_elem %d: Read error\n", __LINE__);
    }
    return res;
}


int FileMKVPriv::matroska_parse_seekhead_entry(uint64_t pos)
{
    uint32_t level_up       = context.level_up;
    uint32_t saved_id       = context.current_id;
    int64_t before_pos = ftell(fd);
    MatroskaLevel level;
    int64_t offset;
    int ret = 0;

    /* seek */
    offset = pos + context.segment_start;
    if (!fseek(fd, offset, SEEK_SET)) 
    {
        /* We don't want to lose our seekhead level, so we add
         * a dummy. This is a crude hack. */
        if (context.num_levels == EBML_MAX_DEPTH) 
        {
            printf("FileMKVPriv::matroska_parse_seekhead_entry %d: Max EBML element depth (%d) reached\n", 
                __LINE__, 
                EBML_MAX_DEPTH);
            ret = -1;
        } 
        else 
        {
            level.start  = 0;
            level.length = (uint64_t) -1;
            context.levels[context.num_levels] = level;
            context.num_levels++;
            context.current_id = 0;

            ret = ebml_parse(matroska_segment, &context);

            /* remove dummy level */
            while (context.num_levels) 
            {
                uint64_t length = context.levels[--context.num_levels].length;
                if (length == (uint64_t) -1)
                    break;
            }
        }
    }

    /* seek back */
    fseek(fd, before_pos, SEEK_SET);
    context.level_up   = level_up;
    context.current_id = saved_id;

    return ret;
}

void FileMKVPriv::matroska_execute_seekhead()
{
    EbmlList *seekhead_list = &context.seekhead;
    int i;

    for (i = 0; i < seekhead_list->nb_elem; i++) 
    {
        MatroskaSeekhead *seekheads = (MatroskaSeekhead*)seekhead_list->elem;
        uint32_t id  = seekheads[i].id;
        uint64_t pos = seekheads[i].pos;

        MatroskaLevel1Element *elem = matroska_find_level1_elem(id);
        if (!elem || elem->parsed)
            continue;

        elem->pos = pos;

        // defer cues parsing until we actually need cue data.
        if (id == MATROSKA_ID_CUES)
            continue;

        if (matroska_parse_seekhead_entry(pos) < 0) 
        {
            // mark index as broken
            context.cues_parsing_deferred = -1;
            break;
        }

        elem->parsed = 1;
    }
}

int FileMKVPriv::matroska_resync(int64_t last_pos)
{
    int64_t ret;
    uint32_t id;
    context.current_id = 0;
    context.num_levels = 0;

    /* seek to next position to resync from */
    if ((ret = fseek(fd, last_pos + 1, SEEK_SET)) < 0) 
    {
        context.done = 1;
        return ret;
    }

    id = rb32(fd);

    // try to find a toplevel element
    while (!feof(fd)) 
    {
        if (id == MATROSKA_ID_INFO     || id == MATROSKA_ID_TRACKS      ||
            id == MATROSKA_ID_CUES     || id == MATROSKA_ID_TAGS        ||
            id == MATROSKA_ID_SEEKHEAD || id == MATROSKA_ID_ATTACHMENTS ||
            id == MATROSKA_ID_CLUSTER  || id == MATROSKA_ID_CHAPTERS) 
        {
            context.current_id = id;
            return 0;
        }
        id = (id << 8) | fgetc(fd);
    }

    context.done = 1;
    return -1;
}

/*
 * Return: Whether we reached the end of a level in the hierarchy or not.
 */
int FileMKVPriv::ebml_level_end()
{
    int64_t pos = ftell(fd);

    if (context.num_levels > 0) {
        MatroskaLevel *level = &context.levels[context.num_levels - 1];
        if (pos - level->start >= level->length || context.current_id) {
            context.num_levels--;
            return 1;
        }
    }
    return 0;
}

int FileMKVPriv::ebml_parse_nest(EbmlSyntax *syntax, void *data)
{
    int i, res = 0;

    for (i = 0; syntax[i].id; i++)
        switch (syntax[i].type) {
        case EBML_SINT:
            *(int64_t *) ((char *) data + syntax[i].data_offset) = syntax[i].def.i;
            break;
        case EBML_UINT:
            *(uint64_t *) ((char *) data + syntax[i].data_offset) = syntax[i].def.u;
            break;
        case EBML_FLOAT:
            *(double *) ((char *) data + syntax[i].data_offset) = syntax[i].def.f;
            break;
        case EBML_STR:
        case EBML_UTF8:
            // the default may be NULL
            if (syntax[i].def.s) 
            {
                uint8_t **dst = (uint8_t **) ((uint8_t *) data + syntax[i].data_offset);
  
                    
                *dst = (uint8_t*)strdup(syntax[i].def.s);
                
                

                if (!*dst)
                {
                    return -1;
                }
            }
            break;
        }

    while (!res && !ebml_level_end())
        res = ebml_parse(syntax, data);

    return res;
}



int FileMKVPriv::ebml_parse_id(EbmlSyntax *syntax, uint32_t id, void *data)
{
    int i;
    for (i = 0; syntax[i].id; i++)
        if (id == syntax[i].id)
            break;
    
    if(!syntax[i].id && id == MATROSKA_ID_CLUSTER &&
        context.num_levels > 0                   &&
        context.levels[context.num_levels - 1].length == 0xffffffffffffff)
    {
        return 0;  // we reached the end of an unknown size cluster
    }
//    printf("FileMKVPriv::ebml_parse_id %d i=%d\n", __LINE__, i);
    return ebml_parse_elem(&syntax[i], data);
}

// returns the number of bytes read or -1 on error
int FileMKVPriv::ebml_read_num(int max_size, uint64_t *number)
{
    int read = 1, n = 1;
    uint64_t total = 0;

/* The first byte tells us the length in bytes - avio_r8() can normally
 * return 0, but since that's not a valid first ebmlID byte, we can
 * use it safely here to catch EOF. */
    if (!(total = fgetc(fd))) 
    {
        return -1;
    }

    /* get the length of the EBML number */
    read = 8 - ff_log2_tab[total];
    if (read > max_size) 
    {
        return -1;
    }

    /* read out length */
    total ^= 1 << ff_log2_tab[total];
    while (n++ < read)
        total = (total << 8) | fgetc(fd);

    *number = total;

//printf("FileMKVPriv::ebml_read_num %d number=%ld\n", __LINE__, *number);
    return read;
}



/**
 * Read a EBML length value.
 * This needs special handling for the "unknown length" case which has multiple
 * encodings.
 */
int FileMKVPriv::ebml_read_length(uint64_t *number)
{
    int res = ebml_read_num(8, number);
    if (res > 0 && *number + 1 == 1ULL << (7 * res))
        *number = 0xffffffffffffffULL;
    return res;
}



/*
 * Read the next element as an unsigned int.
 * 0 is success, < 0 is failure.
 */
int FileMKVPriv::ebml_read_uint(int size, uint64_t *num)
{
    int n = 0;

    if (size > 8)
        return -1;

    /* big-endian ordering; build up number */
    *num = 0;
    while (n++ < size)
        *num = (*num << 8) | fgetc(fd);

    return 0;
}

/*
 * Read the next element as a signed int.
 * 0 is success, < 0 is failure.
 */
int FileMKVPriv::ebml_read_sint(int size, int64_t *num)
{
    int n = 1;

    if (size > 8)
        return -1;

    if (size == 0) {
        *num = 0;
    } else {
        *num = sign_extend(fgetc(fd), 8);

        /* big-endian ordering; build up number */
        while (n++ < size)
            *num = ((uint64_t)*num << 8) | fgetc(fd);
    }

    return 0;
}

/*
 * Read the next element as a float.
 * 0 is success, < 0 is failure.
 */
int FileMKVPriv::ebml_read_float(int size, double *num)
{
    if (size == 0)
        *num = 0;
    else if (size == 4)
        *num = int2float(rb32(fd));
    else if (size == 8)
        *num = int2double(rb64(fd));
    else
        return -1;

    return 0;
}

/*
 * Read the next element as an ASCII string.
 * 0 is success, < 0 is failure.
 */
int FileMKVPriv::ebml_read_ascii(int size, char **str)
{
    char *res = 0;

/* EBML strings are usually not 0-terminated, so we allocate one
 * byte more, read the string and NULL-terminate it ourselves. */
    res = (char*)malloc(size + 1);
    if(fread(res, 1, size, fd) != size) 
    {
        free(res);
        return -1;
    }
    res[size] = 0;
    if(*str)
    {
        free(*str);
    }
    *str = res;

    return 0;
}

/*
 * Read the next element as binary data.
 * 0 is success, < 0 is failure.
 */
#define INPUT_BUFFER_PADDING_SIZE 64
int FileMKVPriv::ebml_read_binary(int length, EbmlBin *bin)
{
    bin->data = (uint8_t*)realloc(bin->data, 
        length + INPUT_BUFFER_PADDING_SIZE);
    memset(bin->data + length, 0, INPUT_BUFFER_PADDING_SIZE);

    bin->size = length;
    bin->pos  = ftell(fd);
    if (fread(bin->data, 1, length, fd) != length) 
    {
        free(bin->data);
        bin->data = NULL;
        bin->size = 0;
        return -1;
    }

    return 0;
}

/*
 * Read the next element, but only the header. The contents
 * are supposed to be sub-elements which can be read separately.
 * 0 is success, < 0 is failure.
 */
int FileMKVPriv::ebml_read_master(uint64_t length)
{
    MatroskaLevel *level;

    if (context.num_levels >= EBML_MAX_DEPTH) {
        printf("FileMKVPriv::ebml_read_master %d: File is beyond max. allowed depth (%d)\n", 
            __LINE__,
            EBML_MAX_DEPTH);
        return -1;
    }

    level         = &context.levels[context.num_levels++];
    level->start  = ftell(fd);
    level->length = length;

    return 0;
}

/*
 * Read signed/unsigned "EBML" numbers.
 * Return: number of bytes processed, < 0 on error
 */
int FileMKVPriv::matroska_ebmlnum_uint(uint8_t *data, 
    uint32_t size, 
    uint64_t *num)
{
    return ebml_read_num(MIN(size, 8), num);
}

/*
 * Same as above, but signed.
 */
int FileMKVPriv::matroska_ebmlnum_sint(uint8_t *data, uint32_t size, int64_t *num)
{
    uint64_t unum;
    int res;

    /* read as unsigned number first */
    if ((res = matroska_ebmlnum_uint(data, size, &unum)) < 0)
        return res;

    /* make signed (weird way) */
    *num = unum - ((1LL << (7 * res - 1)) - 1);

    return res;
}


int FileMKVPriv::ebml_parse(EbmlSyntax *syntax, void *data)
{
    if(!context.current_id)
    {
        uint64_t id;
        int result = ebml_read_num(4, &id);

        if(result < 0)
        {
            return 1;
        }
        context.current_id = id | 1 << 7 * result;

//printf("FileMKVPriv::ebml_parse %d: current_id=0x%x\n", __LINE__, context.current_id);
    }

    return ebml_parse_id(syntax, context.current_id, data);
}


void FileMKVPriv::ebml_free(EbmlSyntax *syntax, void *data)
{
    int i, j;

    for (i = 0; syntax[i].id; i++) 
    {
        void *data_off = (char *) data + syntax[i].data_offset;
//        printf("FileMKVPriv::ebml_free %d: type=%d\n", __LINE__, syntax[i].type);


        switch (syntax[i].type) 
        {
        case EBML_STR:
        case EBML_UTF8:
            freep(data_off);
            break;
        case EBML_BIN:
            free(&((EbmlBin *) data_off)->data);
            break;
        case EBML_LEVEL1:
        case EBML_NEST:
//            printf("FileMKVPriv::ebml_free %d\n", __LINE__);
            if (syntax[i].list_elem_size) 
            {
                EbmlList *list = (EbmlList*)data_off;
                char *ptr = (char*)list->elem;
                for (j = 0; j < list->nb_elem;
                     j++, ptr += syntax[i].list_elem_size)
                {
                    ebml_free(syntax[i].def.n, ptr);
                }
                freep(&list->elem);
                list->nb_elem = 0;
            } 
            else
            {
                ebml_free(syntax[i].def.n, data_off);
            }
        default:
            break;
        }
//        printf("FileMKVPriv::ebml_free %d\n", __LINE__);
    }
}


int FileMKVPriv::matroska_decode_buffer(uint8_t **buf, 
    int *buf_size,
    MatroskaTrack *track)
{
    MatroskaTrackEncoding *encodings = (MatroskaTrackEncoding*)track->encodings.elem;
    uint8_t *data = *buf;
    int isize = *buf_size;
    uint8_t *pkt_data = NULL;
    int pkt_size = isize;
    int result = 0;
    int olen;

    if (pkt_size >= 10000000U)
        return -1;

    switch (encodings[0].compression.algo) 
    {
        case MATROSKA_TRACK_ENCODING_COMP_HEADERSTRIP:
        {
            int header_size = encodings[0].compression.settings.size;
            uint8_t *header = encodings[0].compression.settings.data;

            if (header_size && !header) 
            {
                return -1;
            }

            if (!header_size)
            {
                return 0;
            }

            pkt_size = isize + header_size;

printf("FileMKVPriv::matroska_decode_buffer %d\n", __LINE__);
            pkt_data = (uint8_t*)malloc(pkt_size);
printf("FileMKVPriv::matroska_decode_buffer %d\n", __LINE__);


            memcpy(pkt_data, header, header_size);
            memcpy(pkt_data + header_size, data, isize);
            break;
        }
        
        default:
            return -1;
    }

    *buf      = pkt_data;
    *buf_size = pkt_size;
    return 0;
}



int FileMKVPriv::matroska_parse_tracks()
{
    MatroskaTrack *tracks = (MatroskaTrack*)context.tracks.elem;
    int i, j, k, ret;

    asset->video_data = 0;
    asset->layers = 0;

    asset->audio_data = 0;
    asset->channels = 0;
    
    
    for (i = 0; i < context.tracks.nb_elem; i++) 
    {
        MatroskaTrack *track = &tracks[i];
        int codec_id = -1;
        EbmlList *encodings_list = &track->encodings;
        MatroskaTrackEncoding *encodings = (MatroskaTrackEncoding*)encodings_list->elem;
        uint8_t *extradata = NULL;
        int extradata_size = 0;
        int extradata_offset = 0;
        uint32_t fourcc = 0;
        char* key_id_base64 = NULL;
        int bit_depth = -1;

        if (track->type != MATROSKA_TRACK_TYPE_VIDEO &&
            track->type != MATROSKA_TRACK_TYPE_AUDIO) 
        {
            continue;
        }
        
        if (!track->codec_id)
            continue;

        if (track->audio.samplerate < 0 || 
            track->audio.samplerate > 0x7FFFFFFFL ||
            isnan(track->audio.samplerate)) 
        {
            track->audio.samplerate = 8000;
        }

        if (track->type == MATROSKA_TRACK_TYPE_VIDEO) 
        {
            if (!track->default_duration && track->video.frame_rate > 0) 
            {
                double default_duration = 1000000000 / track->video.frame_rate;
                if (default_duration > UINT64_MAX || default_duration < 0) 
                {
                } 
                else 
                {
                    track->default_duration = default_duration;
                }
            }
            if (track->video.display_width == -1)
                track->video.display_width = track->video.pixel_width;
            if (track->video.display_height == -1)
                track->video.display_height = track->video.pixel_height;
            if (track->video.color_space.size == 4)
            {
                printf("FileMKVPriv::matroska_parse_tracks %d: color_space.data=0x%x\n",
                    __LINE__,
                    *(uint32_t*)track->video.color_space.data);
            }
        } 
        else 
        if (track->type == MATROSKA_TRACK_TYPE_AUDIO) 
        {
            if (!track->audio.out_samplerate)
                track->audio.out_samplerate = track->audio.samplerate;
        }
        
        
        if (encodings_list->nb_elem > 1) 
        {
            printf("FileMKVPriv::matroska_parse_tracks %d: encodings_list->nb_elem=%d\n", 
                __LINE__,
                encodings_list->nb_elem);
        } 
        else 
        if (encodings_list->nb_elem == 1) 
        {
            printf("FileMKVPriv::matroska_parse_tracks %d: track=%d encoding type=%ld encryption size=%d algo=%ld codec_priv size=%d\n", 
                __LINE__,
                i,
                encodings[0].type,
                encodings[0].encryption.key_id.size,
                encodings[0].compression.algo,
                track->codec_priv.size);
                
            if (encodings[0].type) 
            {
                if (encodings[0].encryption.key_id.size > 0) 
                {
                    printf("FileMKVPriv::matroska_parse_tracks %d: encodings[0].encryption.key_id.size=%d\n",
                    __LINE__,
                    encodings[0].encryption.key_id.size);
                } 
                else 
                {
                    encodings[0].scope = 0;
                }
            } 
            else 
            if (encodings[0].compression.algo != MATROSKA_TRACK_ENCODING_COMP_HEADERSTRIP) 
            {
                encodings[0].scope = 0;
            } 
            else 
            if (track->codec_priv.size && encodings[0].scope & 2) 
            {
                int ret = matroska_decode_buffer(&track->codec_priv.data,
                                                 &track->codec_priv.size,
                                                 track);
                if (ret < 0) 
                {
                    track->codec_priv.data = NULL;
                    track->codec_priv.size = 0;
                }
            }
        }

        printf("FileMKVPriv::matroska_parse_tracks %d: codec_id=%s\n",
            __LINE__,
            track->codec_id);


// extradata is now parsed here, in a different way for each codec
        
        track->codec_priv.size -= extradata_offset;


        if (track->time_scale < 0.01)
            track->time_scale = 1.0;




        if (track->type == MATROSKA_TRACK_TYPE_VIDEO) 
        {
            asset->video_data = 1;
            asset->layers++;
        
            MatroskaTrackPlane *planes = (MatroskaTrackPlane*)track->operation.combine_planes.elem;
            int display_width_mul  = 1;
            int display_height_mul = 1;

            asset->width = track->video.pixel_width;
            asset->height = track->video.pixel_height;


        }
        else 
        if (track->type == MATROSKA_TRACK_TYPE_AUDIO) 
        {
            asset->audio_data = 1;
            asset->channels += track->audio.channels;
            asset->sample_rate = track->audio.out_samplerate;;
        }
    }

    return 0;
}


MatroskaTrack* FileMKVPriv::matroska_find_track_by_num(int num)
{
    MatroskaTrack *tracks =  (MatroskaTrack*)context.tracks.elem;
    int i;

    for (i = 0; i < context.tracks.nb_elem; i++)
        if (tracks[i].num == num)
            return &tracks[i];
    return NULL;
}

int FileMKVPriv::matroska_add_index_entries()
{
    EbmlList *index_list;
    MatroskaIndex *index;
    uint64_t index_scale = 1;
    int i, j;
    
    index_list = &context.index;
    index      = (MatroskaIndex*)index_list->elem;

printf("FileMKVPriv::matroska_add_index_entries %d: index_list->nb_elem=%d\n", __LINE__, index_list->nb_elem);
    if (index_list->nb_elem < 2)
        return 0;
    
    for (i = 0; i < index_list->nb_elem; i++) 
    {
        EbmlList *pos_list    = &index[i].pos;
        MatroskaIndexPos *pos = (MatroskaIndexPos*)pos_list->elem;
        for (j = 0; j < pos_list->nb_elem; j++) 
        {
            MatroskaTrack *track = matroska_find_track_by_num(pos[j].track);
            if (track)
            {
                printf("FileMKVPriv::matroska_add_index_entries %d track=%ld pos=%ld time=%ld\n",
                    __LINE__,
                    pos[j].track,
                    pos[j].pos + context.segment_start,
                    index[i].time / index_scale);
            }
        }
    }


    return 0;
}

int FileMKVPriv::matroska_parse_cues()
{
    int i;

    for (i = 0; i < context.num_level1_elems; i++) 
    {
        MatroskaLevel1Element *elem = (MatroskaLevel1Element*)&context.level1_elems[i];
        if (elem->id == MATROSKA_ID_CUES && !elem->parsed) 
        {
printf("FileMKVPriv::matroska_parse_cues %d elem->pos=%ld\n", __LINE__, elem->pos);
            if (matroska_parse_seekhead_entry(elem->pos) < 0)
            {
                context.cues_parsing_deferred = -1;
            }
            
            elem->parsed = 1;
//            break;
        }
    }


    matroska_add_index_entries();

    return 0;
}



int FileMKVPriv::read_header()
{
    Ebml ebml;
    bzero(&ebml, sizeof(Ebml));
    ebml_parse(ebml_syntax, &ebml);
    printf("FileMKVPriv::read_header %d: version=%ld max_size=%ld id_length=%ld doctype_version=%ld\n",
        __LINE__,
        ebml.version,
        ebml.max_size,
        ebml.id_length,
        ebml.doctype_version);
    
    
    ebml_free(ebml_syntax, &ebml);

/* The next thing is a segment. */
    int64_t pos = ftell(fd);
    int res = ebml_parse(matroska_segments, &context);

// try resyncing until we find a EBML_STOP type element.
    while (res != 1) 
    {
        res = matroska_resync(pos);
        if (res < 0)
        {
            fclose(fd);
            fd = 0;
            return -1;
        }
        pos = ftell(fd);
        res = ebml_parse(matroska_segment, &context);
    }
    
    matroska_execute_seekhead();
    
    if (!context.time_scale)
        context.time_scale = 1000000;

    res = matroska_parse_tracks();
    if (res < 0)
    {
        fclose(fd);
        fd = 0;
        return -1;
    }
    printf("FileMKVPriv::read_header %d\n", __LINE__);

    matroska_parse_cues();


    return 0;
}




FileMKV::FileMKV(Asset *asset, File *file)
 : FileBase(asset, file)
{
    reset_parameters_derived();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_MKV;
}

FileMKV::~FileMKV()
{
	close_file();
}

int FileMKV::close_file()
{
    if(priv)
    {
        FileMKVPriv *ptr = (FileMKVPriv*)priv;
        delete ptr;
    }


	FileBase::close_file();
    return 0;
}


int FileMKV::reset_parameters_derived()
{
	priv = 0;

    return 0;
}

int FileMKV::check_sig(Asset *asset)
{
// file extension
    char *ptr = strrchr(asset->path, '.');
    if(!ptr)
    {
        return 0;
    }

    if(strcasecmp(ptr, ".webm") &&
        strcasecmp(ptr, ".mkv"))
    {
        return 0;
    }

// start code
    FILE *fd = fopen(asset->path, "r");
    if(!fd)
    {
        return 0;
    }

    uint8_t buffer[4];
    int bytes_read = fread(buffer, 1, 4, fd);
    fclose(fd);

    if(bytes_read != 4 ||
        buffer[0] != 0x1a ||
        buffer[1] != 0x45 ||
        buffer[2] != 0xdf ||
        buffer[3] != 0xa3)
    {
        return 0;
    }




    return 1;
}

int FileMKV::open_file(int rd, int wr)
{
    if(rd)
    {
        if(!priv)
        {
            FileMKVPriv *ptr = new FileMKVPriv;
            priv = ptr;
            
            return ptr->open(asset);
        }
        
        
    }
    return 1;
}




int FileMKV::set_video_position(int64_t x)
{
    return 0;
}

int FileMKV::set_audio_position(int64_t x)
{
    return 0;
}


int64_t FileMKV::get_memory_usage()
{
    return 0;
}


int FileMKV::read_frame(VFrame *frame)
{
    return 0;
}


int FileMKV::read_samples(double *buffer, int64_t len)
{
    return 0;
}











