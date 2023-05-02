// Utility functions for vbr audio

#include "funcprotos.h"
#include "quicktime.h"
#include <string.h>


// Maximum samples to store in output buffer
#define MAX_VBR_BUFFER 0x200000



void quicktime_init_vbr(quicktime_vbr_t *ptr, int channels)
{
	int i;
	if(ptr->output_buffer && ptr->channels != channels)
	{
		for(i = 0; i < ptr->channels; i++)
		{
			free(ptr->output_buffer[i]);
		}
		free(ptr->output_buffer);
		ptr->output_buffer = 0;
	}
	
	ptr->channels = channels;
	if(!ptr->output_buffer)
	{
		ptr->output_buffer = calloc(channels, sizeof(float*));
		for(i = 0; i < channels; i++)
			ptr->output_buffer[i] = calloc(MAX_VBR_BUFFER, sizeof(float));
	}
}

void quicktime_clear_vbr(quicktime_vbr_t *ptr)
{
	int i;

	if(ptr->output_buffer)
	{
		for(i = 0; i < ptr->channels; i++)
			free(ptr->output_buffer[i]);
		free(ptr->output_buffer);
	}

	if(ptr->input_buffer)
	{
		free(ptr->input_buffer);
	}
}

void quicktime_vbr_set_channels(quicktime_vbr_t *ptr, int channels)
{
	ptr->channels = channels;
}

int64_t quicktime_vbr_end(quicktime_vbr_t *ptr)
{
	return ptr->buffer_end;
}

unsigned char* quicktime_vbr_input(quicktime_vbr_t *ptr)
{
	return ptr->input_buffer;
}

int quicktime_vbr_input_size(quicktime_vbr_t *ptr)
{
	return ptr->input_size;
}

static int limit_samples(int samples)
{
	if(samples > MAX_VBR_BUFFER)
	{
		fprintf(stderr, 
			"quicktime_align_vbr: can't decode more than %d samples at a time.\n",
			MAX_VBR_BUFFER);
		return 1;
	}
	return 0;
}

int quicktime_align_vbr(quicktime_audio_map_t *atrack, 
	int samples)
{
	quicktime_vbr_t *ptr = &atrack->vbr;
	int64_t start_position = atrack->current_position;

	if(limit_samples(samples)) return 1;

// Desired start point is outside existing range.  Reposition buffer pointer
// to start time of nearest frame.
	if(start_position < ptr->buffer_end - ptr->buffer_size ||
		start_position > ptr->buffer_end)
	{
		int64_t start_time = start_position;
		ptr->sample = quicktime_time_to_sample(&atrack->track->mdia.minf.stbl.stts,
			&start_time);
		ptr->buffer_end = start_time;
		ptr->buffer_size = 0;
	}

	return 0;
}

int quicktime_read_vbr(quicktime_t *file,
	quicktime_audio_map_t *atrack)
{
	quicktime_vbr_t *vbr = &atrack->vbr;
	quicktime_trak_t *trak = atrack->track;
	int64_t offset = quicktime_sample_to_offset(file, 
		trak, 
		vbr->sample);

	int size = quicktime_sample_size(trak, vbr->sample);
    if(size < 0) return -1;
    
	int new_allocation = vbr->input_size + size;
	int result = 0;

	if(vbr->input_allocation < new_allocation)
	{
		vbr->input_buffer = realloc(vbr->input_buffer, new_allocation);
		vbr->input_allocation = new_allocation;
	}

//printf("quicktime_read_vbr %d sample offset=%ld sample size=%d\n", __LINE__, offset, size);
	quicktime_set_position(file, offset);
	result = !quicktime_read_data(file, vbr->input_buffer + vbr->input_size, size);
	vbr->input_size += size;
	vbr->sample++;
//printf("quicktime_read_vbr %d sample=%d offset=%d result=%d\n", __LINE__, vbr->sample, offset, result);
	return result;
}

void quicktime_shift_vbr(quicktime_audio_map_t *atrack, int bytes)
{
	quicktime_vbr_t *vbr = &atrack->vbr;
	if(bytes >= vbr->input_size)
	{
		vbr->input_size = 0;
	}
	else
	{
		int i, j;
		for(i = 0, j = bytes; j < vbr->input_size; i++, j++)
			vbr->input_buffer[i] = vbr->input_buffer[j];
		vbr->input_size -= bytes;
	}
}

void quicktime_store_vbr_float(quicktime_audio_map_t *atrack,
	float *samples,
	int sample_count)
{
	int i, j;
	quicktime_vbr_t *vbr = &atrack->vbr;
	for(i = 0; i < sample_count; i++)
	{
		for(j = 0; j < vbr->channels; j++)
		{
			vbr->output_buffer[j][vbr->buffer_ptr] = 
				samples[i * vbr->channels + j];
		}
		vbr->buffer_ptr++;
		if(vbr->buffer_ptr >= MAX_VBR_BUFFER)
			vbr->buffer_ptr = 0;
	}
	vbr->buffer_end += sample_count;
	vbr->buffer_size += sample_count;

//printf("quicktime_store_vbr_float %d buffer_size=%d\n", __LINE__, vbr->buffer_size);
	if(vbr->buffer_size > MAX_VBR_BUFFER) vbr->buffer_size = MAX_VBR_BUFFER;
}

void quicktime_store_vbr_int16(quicktime_audio_map_t *atrack,
	int16_t *samples,
	int sample_count)
{
	int i, j;
	quicktime_vbr_t *vbr = &atrack->vbr;
	for(i = 0; i < sample_count; i++)
	{
		for(j = 0; j < vbr->channels; j++)
		{
			vbr->output_buffer[j][vbr->buffer_ptr] = 
				(float)samples[i * vbr->channels + j] / 32768.0;
		}
		vbr->buffer_ptr++;
		if(vbr->buffer_ptr >= MAX_VBR_BUFFER)
			vbr->buffer_ptr = 0;
	}
	vbr->buffer_end += sample_count;
	vbr->buffer_size += sample_count;
	if(vbr->buffer_size > MAX_VBR_BUFFER) vbr->buffer_size = MAX_VBR_BUFFER;
}

void quicktime_copy_vbr_float(quicktime_vbr_t *vbr,
	int64_t start_position, 
	int samples,
	float *output, 
	int channel)
{
	int i, j;
	if(channel >= vbr->channels)
	{
		channel = vbr->channels - 1;
	}
	int input_ptr = vbr->buffer_ptr - 
		(vbr->buffer_end - start_position);
	while(input_ptr < 0) input_ptr += MAX_VBR_BUFFER;

// printf("quicktime_copy_vbr_float %d available=%d samples=%d\n", 
// __LINE__, 
// vbr->buffer_end - start_position,
// samples);

// truncate to available samples
	int samples_copied = samples;
	if(samples_copied > vbr->buffer_end - start_position)
	{
		samples_copied = vbr->buffer_end - start_position;
	}

	for(i = 0; i < samples_copied; i++)
	{
		output[i] = vbr->output_buffer[channel][input_ptr++];
		if(input_ptr >= MAX_VBR_BUFFER)
			input_ptr = 0;
	}


// printf("quicktime_copy_vbr_float %d samples=%d buffer_size=%d\n", 
// __LINE__, 
// samples, 
// vbr->buffer_size);

	if(samples_copied < samples)
	{
		if(samples_copied < 0) samples_copied = 0;
		bzero(output + samples_copied, (samples - samples_copied) * sizeof(float));
	}
}


void quicktime_copy_vbr_int16(quicktime_vbr_t *vbr,
	int64_t start_position, 
	int samples,
	int16_t *output, 
	int channel)
{
	int i, j;
	if(channel >= vbr->channels)
	{
		channel = vbr->channels - 1;
	}
	int input_ptr = vbr->buffer_ptr - 
		(vbr->buffer_end - start_position);
	while(input_ptr < 0) input_ptr += MAX_VBR_BUFFER;

// truncate to available samples
	int samples_copied = samples;
	if(samples_copied > vbr->buffer_end - start_position)
	{
		samples_copied = vbr->buffer_end - start_position;
	}

	for(i = 0; i < samples_copied; i++)
	{
		output[i] = (int)(vbr->output_buffer[channel][input_ptr++] * 32767);
		
		if(input_ptr >= MAX_VBR_BUFFER)
			input_ptr = 0;
	}

	if(samples_copied < samples)
	{
		if(samples_copied < 0) samples_copied = 0;
		bzero(output + samples_copied, (samples - samples_copied) * sizeof(int16_t));
	}
}



