#include "funcprotos.h"
#include "quicktime.h"
#include <string.h>


void quicktime_mjqt_init(quicktime_mjqt_t *mjqt)
{
}

void quicktime_mjqt_delete(quicktime_mjqt_t *mjqt)
{
}

void quicktime_mjqt_dump(quicktime_mjqt_t *mjqt)
{
}


void quicktime_mjht_init(quicktime_mjht_t *mjht)
{
}

void quicktime_mjht_delete(quicktime_mjht_t *mjht)
{
}

void quicktime_mjht_dump(quicktime_mjht_t *mjht)
{
}

// Set esds header to a copy of the argument
void quicktime_set_mpeg4_header(quicktime_stsd_table_t *table,
	unsigned char *data, 
	int size)
{
	if(table->esds.mpeg4_header)
	{
		free(table->esds.mpeg4_header);
	}

	table->esds.mpeg4_header = calloc(1, size);
	memcpy(table->esds.mpeg4_header, data, size);
	table->esds.mpeg4_header_size = size;
}

static void read_wave(quicktime_t *file, 
	quicktime_stsd_table_t *table, 
	quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;
//printf("read_wave 1 start=0x%llx\n", quicktime_position(file));
	while(quicktime_position(file) < parent_atom->end)
	{
		quicktime_atom_read_header(file, &leaf_atom);
		if(quicktime_atom_is(&leaf_atom, "esds"))
		{
// elementary stream data for MP4A
			quicktime_read_esds(file, table, &leaf_atom, &table->esds);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "frma"))
		{
// Extra data for QDM2
			quicktime_read_frma(file, table, parent_atom, &leaf_atom, &table->frma);
		}
		else
			quicktime_atom_skip(file, &leaf_atom);
	}
}

static void read_dac3(quicktime_t *file, 
	quicktime_stsd_table_t *table, 
	quicktime_atom_t *leaf_atom, 
    quicktime_dac3_t *dac3)
{
	dac3->data_size = leaf_atom->size - 8;
	dac3->data = calloc(1, dac3->data_size + 1024);
    quicktime_read_data(file, 
		dac3->data, 
		dac3->data_size);

// from mov_read_dac3
    int ac3info, acmod, lfeon, bsmod;
    ac3info = (table->dac3.data[0] << 16) |
        (table->dac3.data[1] << 8) |
        (table->dac3.data[0]);
    bsmod = (ac3info >> 14) & 0x7;
    acmod = (ac3info >> 11) & 0x7;
    lfeon = (ac3info >> 10) & 0x1;
    table->channels = 0;
    if(lfeon) table->channels++;
// from a52dec-0.7.3/include/a52.h
    const int channel_counts[] = {
        1, 1, 2, 3, 3, 4, 4, 5, 2
    };
    table->channels += channel_counts[acmod];
// printf("quicktime_ffaudio_decode %d channels=%d\n", 
// __LINE__, table->channels);
}


void quicktime_read_stsd_audio(quicktime_t *file, 
	quicktime_stsd_table_t *table, 
	quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;

	table->version = quicktime_read_int16(file);
	table->revision = quicktime_read_int16(file);
	quicktime_read_data(file, table->vendor, 4);
	table->channels = quicktime_read_int16(file);
	table->sample_size = quicktime_read_int16(file);
	table->compression_id = quicktime_read_int16(file);
	table->packet_size = quicktime_read_int16(file);
	table->sample_rate = quicktime_read_fixed32(file);


// printf("quicktime_read_stsd_audio %d sample_rate=%f version=%d\n", 
// __LINE__, 
// table->sample_rate,
// table->version);

	if(table->sample_rate == 0)
	{
		table->sample_rate = 44100;
	}
	else
	// Kluge for fixed32 limitation
	if(table->sample_rate + 65536 == 96000 ||
		table->sample_rate + 65536 == 88200)
	{
		table->sample_rate += 65536;
	}


// Version 1 fields
	if(table->version > 0)
	{
		table->samples_per_packet = quicktime_read_int32(file);
		table->bytes_per_packet = quicktime_read_int32(file);
		table->bytes_per_frame = quicktime_read_int32(file);
		table->bytes_per_sample = quicktime_read_int32(file);

// Skip another 20 bytes
		if(table->version == 2)
		{
			quicktime_set_position(file, quicktime_position(file) + 0x14);
		}

		while(quicktime_position(file) < parent_atom->end)
		{
			quicktime_atom_read_header(file, &leaf_atom);

			if(quicktime_atom_is(&leaf_atom, "wave"))
			{
				read_wave(file, table, &leaf_atom);
			}
			else
			{
// printf("quicktime_read_stsd_audio %d %s\n", 
// __LINE__, 
// leaf_atom.type);
				quicktime_atom_skip(file, &leaf_atom);
			}
		}
	}
    else
    {
		while(quicktime_position(file) < parent_atom->end)
		{
			quicktime_atom_read_header(file, &leaf_atom);

			if(quicktime_atom_is(&leaf_atom, "esds"))
			{
				quicktime_read_esds(file, table, &leaf_atom, &table->esds);
			}
			else
            if(quicktime_atom_is(&leaf_atom, "dac3"))
            {
// AC3 header
                read_dac3(file, table, &leaf_atom, &table->dac3);
            }
            else
			{
// printf("quicktime_read_stsd_audio %d %s\n", 
// __LINE__, 
// leaf_atom.type);
				quicktime_atom_skip(file, &leaf_atom);
			}
		}
    }

}

void quicktime_write_stsd_audio(quicktime_t *file, quicktime_stsd_table_t *table)
{
	quicktime_write_int16(file, table->version);
	quicktime_write_int16(file, table->revision);
	quicktime_write_data(file, table->vendor, 4);
	quicktime_write_int16(file, table->channels);
	quicktime_write_int16(file, table->sample_size);

	quicktime_write_int16(file, table->compression_id);
	quicktime_write_int16(file, table->packet_size);
	quicktime_write_fixed32(file, table->sample_rate);

// Write header for mp4a
	if(table->esds.mpeg4_header_size && table->esds.mpeg4_header)
	{
// Android requires leaving out the WAVE header
#if 0
// Version 1 info
		quicktime_write_int32(file, 0);
		quicktime_write_int32(file, 0);
		quicktime_write_int32(file, 0);
		quicktime_write_int32(file, 0);

		quicktime_atom_t wave_atom;
		quicktime_atom_t frma_atom;
		quicktime_atom_t mp4a_atom;
		quicktime_atom_write_header(file, &wave_atom, "wave");

		quicktime_atom_write_header(file, &frma_atom, "frma");
		quicktime_write_data(file, "mp4a", 4);
		quicktime_atom_write_footer(file, &frma_atom);

		quicktime_atom_write_header(file, &mp4a_atom, "mp4a");
		quicktime_write_int32(file, 0x0);
		quicktime_atom_write_footer(file, &mp4a_atom);
#endif // 0

		quicktime_write_esds(file, &table->esds, 0, 1);
//		quicktime_atom_write_footer(file, &wave_atom);
	}
}

void quicktime_read_stsd_video(quicktime_t *file, 
	quicktime_stsd_table_t *table, 
	quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;
	int len;
	
	table->version = quicktime_read_int16(file);
	table->revision = quicktime_read_int16(file);
	quicktime_read_data(file, table->vendor, 4);
	table->temporal_quality = quicktime_read_int32(file);
	table->spatial_quality = quicktime_read_int32(file);
	table->width = quicktime_read_int16(file);
	table->height = quicktime_read_int16(file);
	table->dpi_horizontal = quicktime_read_fixed32(file);
	table->dpi_vertical = quicktime_read_fixed32(file);
	table->data_size = quicktime_read_int32(file);
	table->frames_per_sample = quicktime_read_int16(file);
	len = quicktime_read_char(file);
	quicktime_read_data(file, table->compressor_name, 31);
	table->depth = quicktime_read_int16(file);
	table->ctab_id = quicktime_read_int16(file);
	
	while(quicktime_position(file) < parent_atom->end)
	{
		quicktime_atom_read_header(file, &leaf_atom);

/*
 * printf("quicktime_read_stsd_video 1 %lx %lx %lx %s\n", 
 * leaf_atom.start, leaf_atom.end, quicktime_position(file),
 * leaf_atom.type);
 */



		if(quicktime_atom_is(&leaf_atom, "esds"))
		{
			quicktime_read_esds(file, table, &leaf_atom, &table->esds);
		}
		else
// TODO: consolidate these into a common mpeg4 header
		if(quicktime_atom_is(&leaf_atom, "avcC"))
		{
			quicktime_read_avcc(file, &leaf_atom, &table->avcc, 0);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "hvcC"))
		{
			quicktime_read_avcc(file, &leaf_atom, &table->avcc, 1);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "colr"))
		{
//printf("quicktime_read_stsd_video %d\n", __LINE__);
			quicktime_atom_skip(file, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "ctab"))
		{
			quicktime_read_ctab(file, &(table->ctab));
		}
		else
		if(quicktime_atom_is(&leaf_atom, "gama"))
		{
			table->gamma = quicktime_read_fixed32(file);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "fiel"))
		{
			table->fields = quicktime_read_char(file);
			table->field_dominance = quicktime_read_char(file);
		}
		else
        {
			quicktime_atom_skip(file, &leaf_atom);
        }


/* 		if(quicktime_atom_is(&leaf_atom, "mjqt")) */
/* 		{ */
/* 			quicktime_read_mjqt(file, &(table->mjqt)); */
/* 		} */
/* 		else */
/* 		if(quicktime_atom_is(&leaf_atom, "mjht")) */
/* 		{ */
/* 			quicktime_read_mjht(file, &(table->mjht)); */
/* 		} */
/* 		else */
	}
//printf("quicktime_read_stsd_video 2\n");
}

void quicktime_write_stsd_video(quicktime_t *file, quicktime_stsd_table_t *table)
{
	quicktime_write_int16(file, table->version);
	quicktime_write_int16(file, table->revision);
	quicktime_write_data(file, table->vendor, 4);
	quicktime_write_int32(file, table->temporal_quality);
	quicktime_write_int32(file, table->spatial_quality);
	quicktime_write_int16(file, table->width);
	quicktime_write_int16(file, table->height);
	quicktime_write_fixed32(file, table->dpi_horizontal);
	quicktime_write_fixed32(file, table->dpi_vertical);
	quicktime_write_int32(file, table->data_size);
	quicktime_write_int16(file, table->frames_per_sample);
	quicktime_write_char(file, strlen(table->compressor_name));
	quicktime_write_data(file, table->compressor_name, 31);
	quicktime_write_int16(file, table->depth);
	quicktime_write_int16(file, table->ctab_id);


// Write field order for mjpa
	if(table->fields)
	{
		quicktime_atom_t atom;

		quicktime_atom_write_header(file, &atom, "fiel");
		quicktime_write_char(file, table->fields);
		quicktime_write_char(file, table->field_dominance);
		quicktime_atom_write_footer(file, &atom);
	}

// Write header for mp4v
	if(table->esds.mpeg4_header_size && table->esds.mpeg4_header)
	{
		quicktime_write_esds(file, &table->esds, 1, 0);
	}

	if(table->avcc.data_size)
	{
		quicktime_write_avcc(file, &table->avcc);
	}
    

// Write another 32 bits
	if(table->version == 1)
		quicktime_write_int32(file, 0x0);
}

void quicktime_read_stsd_table(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_table_t *table)
{
	quicktime_atom_t leaf_atom;

	quicktime_atom_read_header(file, &leaf_atom);

	table->format[0] = leaf_atom.type[0];
	table->format[1] = leaf_atom.type[1];
	table->format[2] = leaf_atom.type[2];
	table->format[3] = leaf_atom.type[3];
	quicktime_read_data(file, table->reserved, 6);
	table->data_reference = quicktime_read_int16(file);

// printf("quicktime_read_stsd_table %d %d %d %c%c%c%c\n", 
// __LINE__, 
// minf->is_audio, 
// minf->is_video,
// table->format[0],
// table->format[1],
// table->format[2],
// table->format[3]);

// if minf comes after stsd, we're screwed.  Just base it on certain codecs for now.
	if(!minf->is_audio && !minf->is_video)
	{
		if(quicktime_match_32(table->format, QUICKTIME_H264))
		{
			minf->is_video = 1;
		}
		else
		if(quicktime_match_32(table->format, QUICKTIME_MP4A))
		{
			minf->is_audio = 1;
		}
        else
        {
            
        }
	}

	if(minf->is_audio) quicktime_read_stsd_audio(file, table, &leaf_atom);
	if(minf->is_video) quicktime_read_stsd_video(file, table, &leaf_atom);
}

void quicktime_stsd_table_init(quicktime_stsd_table_t *table)
{
	int i;
	table->format[0] = 'y';
	table->format[1] = 'u';
	table->format[2] = 'v';
	table->format[3] = '2';
	for(i = 0; i < 6; i++) table->reserved[i] = 0;
	table->data_reference = 1;

	table->version = 0;
	table->revision = 0;
 	table->vendor[0] = 'l';
 	table->vendor[1] = 'n';
 	table->vendor[2] = 'u';
 	table->vendor[3] = 'x';

	table->temporal_quality = 100;
	table->spatial_quality = 258;
	table->width = 0;
	table->height = 0;
	table->dpi_horizontal = 72;
	table->dpi_vertical = 72;
	table->data_size = 0;
	table->frames_per_sample = 1;
	for(i = 0; i < 32; i++) table->compressor_name[i] = 0;
	sprintf(table->compressor_name, "Quicktime for Linux");
	table->depth = 24;
	table->ctab_id = 65535;
	quicktime_ctab_init(&(table->ctab));
	table->gamma = 0;
	table->fields = 0;
	table->field_dominance = 1;
	quicktime_mjqt_init(&(table->mjqt));
	quicktime_mjht_init(&(table->mjht));
	
	table->channels = 0;
	table->sample_size = 0;
	table->compression_id = 0;
	table->packet_size = 0;
	table->sample_rate = 0;
}

void quicktime_stsd_table_delete(quicktime_stsd_table_t *table)
{
	quicktime_ctab_delete(&(table->ctab));
	quicktime_mjqt_delete(&(table->mjqt));
	quicktime_mjht_delete(&(table->mjht));
	quicktime_delete_avcc(&(table->avcc));
	quicktime_delete_esds(&(table->esds));
	quicktime_delete_frma(&(table->frma));
    if(table->dac3.data)
        free(table->dac3.data);
	
}

void quicktime_stsd_video_dump(quicktime_stsd_table_t *table)
{
	printf("       version %d\n", table->version);
	printf("       revision %d\n", table->revision);
	printf("       vendor %c%c%c%c\n", table->vendor[0], table->vendor[1], table->vendor[2], table->vendor[3]);
	printf("       temporal_quality %ld\n", table->temporal_quality);
	printf("       spatial_quality %ld\n", table->spatial_quality);
	printf("       width %d\n", table->width);
	printf("       height %d\n", table->height);
	printf("       dpi_horizontal %f\n", table->dpi_horizontal);
	printf("       dpi_vertical %f\n", table->dpi_vertical);
	printf("       data_size %ld\n", table->data_size);
	printf("       frames_per_sample %d\n", table->frames_per_sample);
	printf("       compressor_name %s\n", table->compressor_name);
	printf("       depth %d\n", table->depth);
	printf("       ctab_id %d\n", table->ctab_id);
	printf("       gamma %f\n", table->gamma);
	if(table->fields)
	{
		printf("       fields %d\n", table->fields);
		printf("       field dominance %d\n", table->field_dominance);
	}
	if(!table->ctab_id) quicktime_ctab_dump(&(table->ctab));
	quicktime_mjqt_dump(&(table->mjqt));
	quicktime_mjht_dump(&(table->mjht));
	quicktime_esds_dump(&table->esds);
	quicktime_avcc_dump(&table->avcc);
	quicktime_frma_dump(&table->frma);
}

void quicktime_stsd_audio_dump(quicktime_stsd_table_t *table)
{
	printf("       version %d\n", table->version);
	printf("       revision %d\n", table->revision);
	printf("       vendor %c%c%c%c\n", table->vendor[0], table->vendor[1], table->vendor[2], table->vendor[3]);
	printf("       channels %d\n", table->channels);
	printf("       sample_size %d\n", table->sample_size);
	printf("       compression_id 0x%x\n", table->compression_id);
	printf("       packet_size %d\n", table->packet_size);
	printf("       sample_rate %f\n", table->sample_rate);
	if(table->version > 0)
	{
		printf("       samples_per_packet %d\n", table->samples_per_packet);
		printf("       bytes_per_packet %d\n", table->bytes_per_packet);
		printf("       bytes_per_frame %d\n", table->bytes_per_frame);
		printf("       bytes_per_sample %d\n", table->bytes_per_sample);
	}
	quicktime_esds_dump(&table->esds);
	quicktime_avcc_dump(&table->avcc);
	quicktime_frma_dump(&table->frma);
    if(table->dac3.data)
    {
        printf("       DAC3 data=");
        int i;
        for(i = 0; i < table->dac3.data_size; i++)
			printf("%02x ", (unsigned char)table->dac3.data[i]);
		printf("\n");
    }
}

void quicktime_stsd_table_dump(void *minf_ptr, quicktime_stsd_table_t *table)
{
	quicktime_minf_t *minf = minf_ptr;
	printf("       format %c%c%c%c\n", table->format[0], table->format[1], table->format[2], table->format[3]);
	quicktime_print_chars("       reserved ", table->reserved, 6);
	printf("       data_reference %d\n", table->data_reference);

	if(minf->is_audio) quicktime_stsd_audio_dump(table);
	if(minf->is_video) quicktime_stsd_video_dump(table);
}

void quicktime_write_stsd_table(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_table_t *table)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, table->format);
/*printf("quicktime_write_stsd_table %c%c%c%c\n", table->format[0], table->format[1], table->format[2], table->format[3]); */
	quicktime_write_data(file, table->reserved, 6);
	quicktime_write_int16(file, table->data_reference);
	
	if(minf->is_audio) quicktime_write_stsd_audio(file, table);
	if(minf->is_video) quicktime_write_stsd_video(file, table);

	quicktime_atom_write_footer(file, &atom);
}
