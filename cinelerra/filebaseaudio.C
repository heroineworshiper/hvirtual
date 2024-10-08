/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "byteorder.h"
#include "file.h"
#include "filebase.h"


int64_t FileBase::samples_to_raw(uint8_t *out_buffer, 
							double **in_buffer,
							int in_samples, 
							int bits, 
							int channels,
							int byte_order,
							int signed_)
{
// number of bytes in a sample
	int output_advance; 
// channel in input buffer
	double *in_channel; 
	double *in_channel_end;
	int channel;
	double in_sample;
	int out_sample;
	int dither_value;
    const int dither_scale = 255;
	int bytes = in_samples * channels * (file->bytes_per_sample(bits));
	int machine_byte_order = get_byte_order();

	switch(bits)
	{
		case BITSLINEAR8:
		{
			uint8_t *output_ptr, *output_end;
			output_advance = channels;
			for(channel = 0; channel < channels; channel++)
			{
				output_ptr = out_buffer + channel;
				in_channel = in_buffer[channel];
				in_channel_end = in_channel + in_samples;

				if(dither)
				{
					for( ; in_channel < in_channel_end; in_channel++)
					{
						in_sample = *in_channel * 0x7fff;
                        CLAMP(in_sample, -0x8000, 0x7fff);
						out_sample = (int)in_sample;
						if(out_sample > -0x7f00) 
                        { 
                            dither_value = rand() % dither_scale; 
                            out_sample -= dither_value; 
                        }
// shifting bits screws up the signs
						out_sample /= 0x100;  
						*output_ptr = out_sample;
						output_ptr += output_advance;
					}
				}
				else
				{
					for( ; in_channel < in_channel_end; in_channel++)
					{
						in_sample = *in_channel * 0x7f;
                        CLAMP(in_sample, -0x80, 0x7f);
						*output_ptr = (uint8_t)in_sample;
						output_ptr += output_advance;
					}
				}
			}

// fix signed
			if(!signed_)
			{
				output_ptr = out_buffer;
				output_end = out_buffer + in_samples * channels;

				for( ; output_ptr < output_end; output_ptr++)
					*output_ptr ^= 0x80;
			}
			break;
		}

		case BITSLINEAR16:
		{
			uint16_t *output_ptr, *output_end;
			output_advance = channels;
			for(channel = 0; channel < channels; channel++)
			{
				output_ptr = (uint16_t*)out_buffer + channel;
				in_channel = in_buffer[channel];
				in_channel_end = in_channel + in_samples;

				if(dither)
				{
					for( ; in_channel < in_channel_end; in_channel++)
					{
						in_sample = *in_channel * 0x7fffff;
                        CLAMP(in_sample, -0x800000, 0x7fffff);
						out_sample = (int)in_sample;
						if(out_sample > -0x7fff00) 
                        { 
                            dither_value = rand() % dither_scale; 
                            out_sample -= dither_value; 
                        }
						out_sample /= 0x100;
						*output_ptr = out_sample;
						output_ptr += output_advance;
					}
				}
				else
				{
					for( ; in_channel < in_channel_end; in_channel++)
					{
						in_sample = *in_channel * 0x7fff;
                        CLAMP(in_sample, -0x8000, 0x7fff);
						*output_ptr = (int16_t)in_sample;
						output_ptr += output_advance;
					}
				}
			}

// fix signed
			if(!signed_)
			{
				output_ptr = (uint16_t*)out_buffer;
				output_end = (uint16_t*)out_buffer + in_samples * channels;

				for( ; output_ptr < output_end; output_ptr++)
					*output_ptr ^= 0x8000;
			}
			break;
		}

		case BITSLINEAR24:
		{
			uint8_t *output_ptr;
			output_advance = asset->channels * 3 - 2;
			for(channel = 0; channel < channels; channel++)
			{
				output_ptr = out_buffer + channel * 3;
				in_channel = in_buffer[channel];
				in_channel_end = in_channel + in_samples;

// don't dither 24 bits
				for( ; in_channel < in_channel_end; in_channel++)
				{
					in_sample = *in_channel * 0x7fffff;
                    CLAMP(in_sample, -0x800000, 0x7fffff);
					out_sample = (int)in_sample;
// store as little endian
					*output_ptr++ = out_sample & 0xff;
					*output_ptr++ = (out_sample >> 8) & 0xff;
					*output_ptr = (out_sample >> 16) & 0xff;
					output_ptr += output_advance;
				}
			}
			break;
		}

        case BITSFLOAT:
        {
            float *output_ptr;
            output_advance = channels;
            for(channel = 0; channel < channels; channel++)
			{
                output_ptr = (float*)out_buffer + channel;
                in_channel = in_buffer[channel];
                in_channel_end = in_channel + in_samples;
                
                for( ; in_channel < in_channel_end; in_channel++)
                {
                    in_sample = *in_channel;
                    *output_ptr = in_sample;
                    output_ptr += output_advance;
                }
            }
            break;
        }

		case BITSULAW:
		{
			uint8_t *output_ptr;
			output_advance = asset->channels;
//printf("FileBase::samples_to_raw 1\n");
			generate_ulaw_tables();
//printf("FileBase::samples_to_raw 2\n");

			for(channel = 0; channel < channels; channel++)
			{
				output_ptr = out_buffer + channel;
				in_channel = in_buffer[channel];
				in_channel_end = in_channel + in_samples;
				for( ; in_channel < in_channel_end; in_channel++)
				{
					*output_ptr = floattoulaw(*in_channel);
					output_ptr += output_advance;
				}
			}
//printf("FileBase::samples_to_raw 3\n");
			break;
		}
	}

// swap bytes
	if((bits == BITSLINEAR16 && byte_order != machine_byte_order) ||
		(bits == BITSLINEAR24 && byte_order == BYTE_ORDER_HILO))
	{
		swap_bytes(file->bytes_per_sample(bits), 
            (unsigned char*)out_buffer, 
            bytes);
	}

	return bytes;
}


#define READ_8_MACRO \
				sample = *inbuffer_8;                   \
				sample /= 0x7f; \
				inbuffer_8 += input_frame;

#define READ_16_MACRO \
				sample = *inbuffer_16;                   \
				sample /= 0x7fff; \
				inbuffer_16 += input_frame;

#define READ_24_MACRO \
				sample = (unsigned char)*inbuffer_24++;  \
				sample_24 = (unsigned char)*inbuffer_24++; \
				sample_24 <<= 8;                           \
				sample += sample_24;                       \
				sample_24 = *inbuffer_24;                  \
				sample_24 <<= 16;                          \
				sample += sample_24;                       \
				sample /= 0x7fffff; \
				inbuffer_24 += input_frame; \

#define READ_ULAW_MACRO \
				sample = ulawtofloat(*inbuffer_8);                   \
				inbuffer_8 += input_frame;

#define LFEATHER_MACRO1 \
				for(feather_current = 0; feather_current < lfeather_len; \
					output_current++, feather_current++) \
				{

#define LFEATHER_MACRO2 \
					current_gain = lfeather_gain + lfeather_slope * feather_current; \
					out_buffer[output_current] = out_buffer[output_current] * (1 - current_gain) + sample * current_gain; \
				}

#define CENTER_MACRO1 \
				for(; output_current < samples; \
					output_current++) \
				{

#define CENTER_MACRO2 \
					out_buffer[output_current] += sample; \
				}

int FileBase::raw_to_samples(float *out_buffer, char *in_buffer, 
		int64_t samples, int bits, int channels, int channel, int feather, 
		float lfeather_len, float lfeather_gain, float lfeather_slope)
{
	int64_t output_current = 0;  // position in output buffer
	int64_t input_len = samples;     // length of input buffer
// The following are floats because they are multiplied by the slope to get the gain.
	float feather_current;     // input position for feather

	float sample; 
	char *inbuffer_8;               // point to actual byte being read
	int16_t *inbuffer_16;
	char *inbuffer_24;
	int sample_24;                                         
	float current_gain;
	int input_frame;                   // amount to advance the input buffer pointer

// set up the parameters
	switch(bits)
	{
		case BITSLINEAR8:  
			inbuffer_8 = in_buffer + channel;
			input_frame = channels;
			break;
			
		case BITSLINEAR16: 
			inbuffer_16 = (int16_t *)in_buffer + channel;          
			input_frame = channels;
			break;
			 
		case BITSLINEAR24: 
			inbuffer_24 = in_buffer + channel * 3;
			input_frame = channels * file->bytes_per_sample(bits) - 2; 
			break;
		
		case BITSULAW:
			generate_ulaw_tables();
			inbuffer_8 = in_buffer + channel;
			input_frame = channels;
			break;
	}

// read the data
// ================== calculate feathering and add to buffer ================
	if(feather)
	{
// left feather
		switch(bits)
		{
			case BITSLINEAR8:
				LFEATHER_MACRO1;                                             
				READ_8_MACRO; 
				LFEATHER_MACRO2;
				break;

			case BITSLINEAR16:
				LFEATHER_MACRO1;                                             
				READ_16_MACRO; 
				LFEATHER_MACRO2;
				break;

			case BITSLINEAR24:                                               
				LFEATHER_MACRO1;                                             
				READ_24_MACRO; 
				LFEATHER_MACRO2;
				break;
			
			case BITSULAW:
				LFEATHER_MACRO1;
				READ_ULAW_MACRO;
				LFEATHER_MACRO2;
				break;
		}
	

// central region
		switch(bits)
		{
			case BITSLINEAR8:                                                  
				CENTER_MACRO1;
				READ_8_MACRO; 
				CENTER_MACRO2;
				break;

			case BITSLINEAR16:
				CENTER_MACRO1;
				READ_16_MACRO;
				CENTER_MACRO2;
				break;

			case BITSLINEAR24:
				CENTER_MACRO1;
				READ_24_MACRO;
				CENTER_MACRO2;
				break;
			
			case BITSULAW:
				CENTER_MACRO1;
				READ_ULAW_MACRO;
				CENTER_MACRO2;
				break;
		}
	}
	else
// ====================== don't feather and overwrite buffer =================
	{
		switch(bits)
		{
			case BITSLINEAR8:
				for(; output_current < input_len; 
					output_current++) 
				{ READ_8_MACRO; out_buffer[output_current] = sample; }
				break;

			case BITSLINEAR16:
				for(; output_current < input_len; 
					output_current++) 
				{ READ_16_MACRO; out_buffer[output_current] = sample; }
				break;

			case BITSLINEAR24:
				for(; output_current < input_len; 
					output_current++) 
				{ READ_24_MACRO; out_buffer[output_current] = sample; }
				break;
			
			case BITSULAW:
				for(; output_current < input_len; 
					output_current++) 
				{ READ_ULAW_MACRO; out_buffer[output_current] = sample; }
				break;
		}
	}

	return 0;
}

int FileBase::overlay_float_buffer(float *out_buffer, float *in_buffer, 
		int64_t samples, 
		float lfeather_len, float lfeather_gain, float lfeather_slope)
{
	int64_t output_current = 0;
	float sample, current_gain;
	float feather_current;     // input position for feather

	LFEATHER_MACRO1
		sample = in_buffer[output_current];
	LFEATHER_MACRO2

	CENTER_MACRO1
		sample = in_buffer[output_current];
	CENTER_MACRO2

	return 0;
}


int FileBase::get_audio_buffer(char **buffer, int64_t len, int64_t bits, int64_t channels)
{
	int64_t bytes = len * channels * (file->bytes_per_sample(bits));
	if(*buffer && bytes > prev_bytes) 
	{ 
		delete [] *buffer; 
		*buffer = 0; 
	}
	prev_bytes = bytes;

	if(!*buffer) *buffer = new char[bytes];
    return 0;
}

int FileBase::get_float_buffer(float **buffer, int64_t len)
{
	if(*buffer && len > prev_len) 
	{ 
		delete [] *buffer; 
		*buffer = 0; 
	}
	prev_len = len;

	if(!*buffer) *buffer = new float[len];
    return 0;
}
