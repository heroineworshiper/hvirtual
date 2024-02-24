#include "funcprotos.h"
#include "quicktime.h"


static int decode_length(quicktime_t *file)
{
	int bytes = 0;
	int result = 0;
	int byte;
	do
	{
		byte = quicktime_read_char(file);
		result = (result << 7) + (byte & 0x7f);
		bytes++;
	}while((byte & 0x80) && bytes < 4);
	return result;
}

void quicktime_delete_esds(quicktime_esds_t *esds)
{
	if(esds->mpeg4_header) free(esds->mpeg4_header);
}

static uint32_t get_bits(uint64_t *data, int *remane, int count)
{
    uint32_t result = 0;
    int i;
    for(i = 0; i < count; i++)
    {
        result <<= 1;
        if((*data & (((uint64_t)1) << 63)))
            result |= 1;
        *data <<= 1;
    }
    *remane -= count;
    return result;
}

static uint32_t show_bits(uint64_t data, int count)
{
    uint32_t result = 0;
    int i;
    for(i = 0; i < count; i++)
    {
        result <<= 1;
        if((data & (((uint64_t)1) << 63)))
            result |= 1;
        data <<= 1;
    }
    return result;
}


#define AOT_ESCAPE 28
#define AOT_SBR 5
#define AOT_PS 26
#define AOT_ER_BSAC 19
#define AOT_ALS 33

static int get_object_type(uint64_t *bits, int *remane)
{
    int object_type = get_bits(bits, remane, 5);
    if (object_type == AOT_ESCAPE)
        object_type = 32 + get_bits(bits, remane, 6);
    return object_type;
}

static int samplerate_table[] = 
{
     96000, 88200, 64000, 48000, 44100, 32000, 
     24000, 22050, 16000, 12000, 11025, 8000, 
     7350, 0, 0, 0
};
static int get_sample_rate(uint64_t *bits, int *remane, int *index)
{
    *index = get_bits(bits, remane, 4);
    return *index == 0x0f ? get_bits(bits, remane, 24) :
        samplerate_table[*index];
}

static void decode_mp4a_config(quicktime_stsd_table_t *table, 
	quicktime_esds_t *esds)
{
	if(esds->mpeg4_header_size > 1 &&
		quicktime_match_32(table->format, QUICKTIME_MP4A))
	{
// Straight out of ff_mpeg4audio_get_config_gb
// some bits have been left out
		unsigned char *ptr = esds->mpeg4_header;
        uint64_t bits = (((uint64_t)ptr[0]) << 56) |
            (((uint64_t)ptr[1]) << 48) |
            (((uint64_t)ptr[2]) << 40) |
            (((uint64_t)ptr[3]) << 32) |
            (((uint64_t)ptr[4]) << 24) |
            (((uint64_t)ptr[5]) << 16) |
            (((uint64_t)ptr[6]) << 8) |
            (((uint64_t)ptr[7]));
        int remane = esds->mpeg4_header_size * 8;
        int object_type = get_object_type(&bits, &remane);
        int samplerate_index;
        esds->sample_rate = get_sample_rate(&bits, &remane, &samplerate_index);
        esds->channels = get_bits(&bits, &remane, 4);
        int ext_object_type = 0;
        int sbr = -1;

// check for W6132 Annex YYYY draft MP3onMP4
        if(object_type == AOT_SBR || (object_type == AOT_PS &&
            !(show_bits(bits, 3) & 0x03 && !(show_bits(bits, 9) & 0x3F)))) 
        {
            ext_object_type = AOT_SBR;
            esds->sample_rate = get_sample_rate(&bits, &remane, &samplerate_index);
            object_type = get_object_type(&bits, &remane);
            if (object_type == AOT_ER_BSAC)
                esds->channels = get_bits(&bits, &remane, 4);
        }
        
        if(object_type == AOT_ALS) {
            printf("decode_mp4a_config %d AOT_ALS not implemented\n", __LINE__);
        }

// sync extension
        if (ext_object_type != AOT_SBR) 
        {
            while (remane > 15) 
            {
                if (show_bits(bits, 11) == 0x2b7) 
                {
                    get_bits(&bits, &remane, 11);
                    ext_object_type = get_object_type(&bits, &remane);
                    if (ext_object_type == AOT_SBR && (sbr = get_bits(&bits, &remane, 1)) == 1) 
                    {
                        esds->sample_rate = get_sample_rate(&bits, &remane, &samplerate_index);
                    }
                    break;
                } else
                    get_bits(&bits, &remane, 1); // skip 1 bit
            }
        }

        esds->got_esds_rate = 1;
// printf("quicktime_esds_samplerate %d samplerate_index=%d sample_rate=%d channels=%d\n", 
// __LINE__, 
// samplerate_index,
// esds->sample_rate,
// esds->channels);

// override the stsd with the esds values
        table->sample_rate = table->esds.sample_rate;
        table->channels = table->esds.channels;
	}
}

void quicktime_read_esds(quicktime_t *file, 
    quicktime_stsd_table_t *table, 
	quicktime_atom_t *parent_atom, 
	quicktime_esds_t *esds)
{

// version
	quicktime_read_char(file);
// flags
	quicktime_read_int24(file);
// elementary stream descriptor tag
// printf("quicktime_read_esds %d format=%s size=%d\n", 
// __LINE__, 
// table->format,
// (int)parent_atom->size);

	if(quicktime_read_char(file) == 0x3)
	{
		int len = decode_length(file);
// elementary stream id
		quicktime_read_int16(file);
// stream priority
		quicktime_read_char(file);
// decoder configuration descripton tag
		if(quicktime_read_char(file) == 0x4)
		{
			int len2 = decode_length(file);
// object type id
			quicktime_read_char(file);
// stream type
			quicktime_read_char(file);
// buffer size
			quicktime_read_int24(file);
// max bitrate
			quicktime_read_int32(file);
// average bitrate
			quicktime_read_int32(file);

//printf("quicktime_read_esds %d\n", __LINE__);
// decoder specific description tag
			if(quicktime_read_char(file) == 0x5)
			{
//printf("quicktime_read_esds %d\n", __LINE__);
				esds->mpeg4_header_size = decode_length(file);
				if(!esds->mpeg4_header_size) return;

// Need padding for FFMPEG
				esds->mpeg4_header = calloc(1, 
					esds->mpeg4_header_size + 1024);
// Get extra data for decoder
				quicktime_read_data(file, 
					esds->mpeg4_header, 
					esds->mpeg4_header_size);
                decode_mp4a_config(table, esds);
    
// skip rest
				quicktime_atom_skip(file, parent_atom);
				return;
			}
			else
			{
// error
				quicktime_atom_skip(file, parent_atom);
				return;
			}
		}
		else
		{
// error
			quicktime_atom_skip(file, parent_atom);
			return;
		}
	}
	else
	{
// error
		quicktime_atom_skip(file, parent_atom);
		return;
	}
}



void quicktime_write_esds(quicktime_t *file, 
	quicktime_esds_t *esds,
	int do_video,
	int do_audio)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "esds");
// version
	quicktime_write_char(file, 0);
// flags
	quicktime_write_int24(file, 0);

// elementary stream descriptor tag
	quicktime_write_char(file, 0x3);

// length placeholder
	int64_t length1 = quicktime_position(file);
//	quicktime_write_int32(file, 0x80808080);
	quicktime_write_char(file, 0x00);

// elementary stream id
	quicktime_write_int16(file, 0x1);

// stream priority
/*
 * 	if(do_video)
 * 		quicktime_write_char(file, 0x1f);
 * 	else
 */
		quicktime_write_char(file, 0);


// decoder configuration description tab
	quicktime_write_char(file, 0x4);
// length placeholder
	int64_t length2 = quicktime_position(file);
//	quicktime_write_int32(file, 0x80808080);
	quicktime_write_char(file, 0x00);

// video
	if(do_video)
	{
// object type id
		quicktime_write_char(file, 0x20);
// stream type
		quicktime_write_char(file, 0x11);
// buffer size
//		quicktime_write_int24(file, 0x007b00);
		quicktime_write_int24(file, 0x000000);
// max bitrate
//		quicktime_write_int32(file, 0x00014800);
		quicktime_write_int32(file, 0x000030d40);
// average bitrate
//		quicktime_write_int32(file, 0x00014800);
		quicktime_write_int32(file, 0x00000000);
	}
	else
	{
// object type id
		quicktime_write_char(file, 0x40);
// stream type
		quicktime_write_char(file, 0x15);
// buffer size
		quicktime_write_int24(file, 0x001800);
// max bitrate
		quicktime_write_int32(file, 0x00004e20);
// average bitrate
		quicktime_write_int32(file, 0x00003e80);
	}

// decoder specific description tag
	quicktime_write_char(file, 0x05);
// length placeholder
	int64_t length3 = quicktime_position(file);
//	quicktime_write_int32(file, 0x80808080);
	quicktime_write_char(file, 0x00);

// mpeg4 sequence header
	quicktime_write_data(file, esds->mpeg4_header, esds->mpeg4_header_size);


	int64_t current_length2 = quicktime_position(file) - length2 - 1;
	int64_t current_length3 = quicktime_position(file) - length3 - 1;

// unknown tag, length and data
	quicktime_write_char(file, 0x06);
//	quicktime_write_int32(file, 0x80808001);
	quicktime_write_char(file, 0x01);
	quicktime_write_char(file, 0x02);


// write lengths
	int64_t current_length1 = quicktime_position(file) - length1 - 1;
	quicktime_atom_write_footer(file, &atom);
	int64_t current_position = quicktime_position(file);
//	quicktime_set_position(file, length1 + 3);
	quicktime_set_position(file, length1);
	quicktime_write_char(file, current_length1);
//	quicktime_set_position(file, length2 + 3);
	quicktime_set_position(file, length2);
	quicktime_write_char(file, current_length2);
//	quicktime_set_position(file, length3 + 3);
	quicktime_set_position(file, length3);
	quicktime_write_char(file, current_length3);
	quicktime_set_position(file, current_position);
}

void quicktime_esds_dump(quicktime_esds_t *esds)
{
	if(esds->mpeg4_header_size)
	{
		printf("       elementary stream description\n");
		printf("        mpeg4_header_size=0x%x\n", esds->mpeg4_header_size);
		printf("        mpeg4_header=");
		int i;
		for(i = 0; i < esds->mpeg4_header_size; i++)
			printf("%02x ", (unsigned char)esds->mpeg4_header[i]);
		printf("\n");
	}
}


