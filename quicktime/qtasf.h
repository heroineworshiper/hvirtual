#ifndef QTASF_H
#define QTASF_H

#include "quicktime.h"
#include <stdint.h>

/* We have lifted sections of ASF decoding from ffmpeg */
/* to add direct copy support and seeking to it */

#define MAX_ASFSTREAMS 256

typedef struct 
{
    uint32_t v1;
    uint16_t v2;
    uint16_t v3;
    uint8_t v4[8];
} quicktime_guid_t;

typedef struct
{
	int number;
	int count;
} quicktime_asfpacket_t;

typedef struct
{
	int is_audio;
	int is_video;
	int is_ext_audio;
	int total_size;
	int type_specific_size;
	int start_time;
	int duration;
	int id;
	int need_parsing;
	int bits_per_sample;
	int extradata_size;
	unsigned char *extradata;
	unsigned char fourcc[5];

/* Audio */
	int ds_span;
	int ds_packet_size;
	int ds_chunk_size;
	int ds_data_size;
	int ds_silence_data;
	int frame_size;
/* WAV header */
	int codec_tag;
	int channels;
	int samplerate;
	int bitrate;
	int block_align;
	int signed_;
	int littleendian;

/* Video */
	int width;
	int height;
} quicktime_asfstream_t;

typedef struct
{
	quicktime_guid_t quid;
	int64_t file_size;
	int64_t create_time;
	int64_t total_packets;
	int64_t send_time;
	int64_t play_time;
	int preroll;
	int ignore;
	int flags;
	int min_packet;
	int max_packet;
	int max_bitrate;
	int packet_size;
} quicktime_asfheader_t;


typedef struct
{
	quicktime_asfheader_t header;
	quicktime_asfstream_t *streams[MAX_ASFSTREAMS];
	int total_streams;
	quicktime_asfpacket_t *index;
	int index_size;
	int index_allocated;
} quicktime_asf_t;



int quicktime_read_asf_header(quicktime_t *file);
void quicktime_delete_asf(quicktime_asf_t *asf);
void quicktime_dump_asf(quicktime_asf_t *asf);
int quicktime_read_guid(quicktime_t *file, quicktime_guid_t *guid);



#endif
