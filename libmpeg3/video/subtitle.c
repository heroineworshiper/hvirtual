#include "mpeg3private.h"
#include "mpeg3protos.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define OVERRIDE_ALPHA
#define USE_INTERLACE
#define SWAP_FIELDS
#define FIELD_OFFSET1 3
//#define FIELD_OFFSET1 1
#define MONOCHROME
#define NORMALIZE
#define USE_RGB

static unsigned char get_nibble(unsigned char **ptr, int *nibble)
{
	if(*nibble)
	{
		*nibble = !*nibble;
		return (*(*ptr)++) & 0xf;
	}
	else
	{
		*nibble = !*nibble;
		return (*(*ptr)) >> 4;
	}
}

/* Returns 1 if failure */
/* This is largely from spudec, ffmpeg */
int mpeg3_decompress_subtitle(mpeg3_t *file, mpeg3_subtitle_t *subtitle)
{
	int i, j, pass;
	unsigned char *ptr = subtitle->data;
	unsigned char *end = subtitle->data + subtitle->size;
	int even_offset = 0;
	int odd_offset = 0;

/* packet size */
	ptr += 2;

/* data packet size */
	if(ptr + 2 > end) return 1;

	int data_size = (*ptr++) << 8;
	data_size |= *ptr++;

	unsigned char *data_start = ptr;

// printf("decompress_subtitle %d 0x%02x%02x size=%d data_size=%d\n", 
// __LINE__, 
// subtitle->data[0], 
// subtitle->data[1], 
// subtitle->size,
// data_size);


//	if(ptr + data_size - 2 > end) return 1;

/* Advance to control sequences */
	ptr += data_size - 2;

	subtitle->palette[0] = 0x00;
	subtitle->palette[1] = 0x01;
	subtitle->palette[2] = 0x02;
	subtitle->palette[3] = 0x03;

	subtitle->alpha[0] = 0xff;
	subtitle->alpha[1] = 0xff;
	subtitle->alpha[2] = 0xff;
	subtitle->alpha[3] = 0xff;

	subtitle->x1 = 0;
	subtitle->x2 = 720;
	subtitle->y1 = 2;
	subtitle->y2 = 575;
	subtitle->w = 720;
	subtitle->h = 575;
	subtitle->start_time = 1897;
	subtitle->stop_time = 175;


/* Control sequence */
// from spudec_process_control
	unsigned char *control_start = 0;
	unsigned char *next_control_start = ptr;
	int got_alpha = 0;
    int done = 0;
	while(!done && ptr < end && control_start != next_control_start)
	{
		control_start = next_control_start;

/* Date */
		if(ptr + 2 > end) break;
		int date = (*ptr++) << 8;
		date |= *ptr++;

/* Offset of next control sequence */
		if(ptr + 2 > end) break;
		int next = (*ptr++) << 8;
		next |= *ptr++;

		next_control_start = subtitle->data + next;

		while(ptr < end && !done)
		{
			int type = *ptr++;
//printf("decompress_subtitle %d: offset=%d control=%x\n", 
//__LINE__, (int)(ptr - data_start), type);

			switch(type)
			{
				case 0x00:
					subtitle->force = 1;
					break;

				case 0x01:
					subtitle->start_time = date;
//printf("decompress_subtitle %d\n", subtitle->start_time);
					break;

				case 0x02:
					subtitle->stop_time = date;
					break;

				case 0x03:
/* Entry in palette of each color */
					if(ptr + 2 > end) return 1;
//printf("decompress_subtitle %d\n", __LINE__);
					subtitle->palette[0] = (*ptr) >> 4;
					subtitle->palette[1] = (*ptr++) & 0xf;
					subtitle->palette[2] = (*ptr) >> 4;
					subtitle->palette[3] = (*ptr++) & 0xf;
//printf("subtitle palette %d %d %d %d\n", subtitle->palette[0], subtitle->palette[1], subtitle->palette[2], subtitle->palette[3]);
					break;

				case 0x04:
/* Alpha corresponding to each color */
					if(ptr + 2 > end) return 1;
//printf("decompress_subtitle %d\n", __LINE__);
					subtitle->alpha[3] = ((*ptr) >> 4) * 255 / 15;
					subtitle->alpha[2] = ((*ptr++) & 0xf) * 255 / 15;
					subtitle->alpha[1] = ((*ptr) >> 4) * 255 / 15;
					subtitle->alpha[0] = ((*ptr++) & 0xf) * 255 / 15;
					got_alpha = 1;
//printf("subtitle alphas %d %d %d %d\n", subtitle->alpha[0], subtitle->alpha[1], subtitle->alpha[2], subtitle->alpha[3]);
#ifdef OVERRIDE_ALPHA
					subtitle->alpha[3] = 0xff;
					subtitle->alpha[2] = 0x80;
					subtitle->alpha[1] = 0x40;
					subtitle->alpha[0] = 0x00;
#endif
					break;

				case 0x05:
/* Extent of image on screen */
					if(ptr + 6 > end)
                    {
                        printf("decompress_subtitle %d image size overflow\n", __LINE__);
                        return 1;
                    }
//printf("decompress_subtitle %d\n", __LINE__);
/*
 * printf("decompress_subtitle 10 %02x %02x %02x %02x %02x %02x\n",
 * ptr[0],
 * ptr[1],
 * ptr[2],
 * ptr[3],
 * ptr[4],
 * ptr[5]);
 */
					subtitle->x1 = (*ptr++) << 4;
					subtitle->x1 |= (*ptr) >> 4;
					subtitle->x2 = ((*ptr++) & 0xf) << 8;
					subtitle->x2 |= *ptr++;
					subtitle->y1 = (*ptr++) << 4;
					subtitle->y1 |= (*ptr) >> 4;
					subtitle->y2 = ((*ptr++) & 0xf) << 8;
					subtitle->y2 |= *ptr++;
					subtitle->x2++;
					subtitle->y2++;
					subtitle->w = subtitle->x2 - subtitle->x1;
					subtitle->h = subtitle->y2 - subtitle->y1 + 2;

printf("decompress_subtitle %d x1=%d x2=%d y1=%d y2=%d\n", 
__LINE__,
subtitle->x1, 
subtitle->x2, 
subtitle->y1, 
subtitle->y2);

					CLAMP(subtitle->w, 1, 2048);
					CLAMP(subtitle->h, 1, 2048);
					CLAMP(subtitle->x1, 0, 2048);
					CLAMP(subtitle->x2, 0, 2048);
					CLAMP(subtitle->y1, 0, 2048);
					CLAMP(subtitle->y2, 0, 2048);
					break;

				case 0x06:
/* offsets of even and odd field in compressed data */
					if(ptr + 4 > end) return 1;
//printf("decompress_subtitle %d\n", __LINE__);
					even_offset = (ptr[0] << 8) | (ptr[1]);
					odd_offset = (ptr[2] << 8) | (ptr[3]);
//printf("decompress_subtitle 30 even=0x%x odd=0x%x\n", even_offset, odd_offset);
					ptr += 4;
					break;

				case 0xff:
					done = 1;
//printf("decompress_subtitle %d done=%d\n", __LINE__, done);
					break;

				default:
					printf("decompress_subtitle %d: unknown type 0x%02x\n", 
                        __LINE__, type);
					break;
			}
		}
	}

//printf("decompress_subtitle %d\n", __LINE__);


/* Allocate image buffer */
	subtitle->image_y = (unsigned char*)calloc(1, subtitle->w * subtitle->h + subtitle->w);
	subtitle->image_u = (unsigned char*)calloc(1, subtitle->w * subtitle->h + subtitle->w);
	subtitle->image_v = (unsigned char*)calloc(1, subtitle->w * subtitle->h + subtitle->w);
	subtitle->image_a = (unsigned char*)calloc(1, subtitle->w * subtitle->h + subtitle->w);

/* Decode image */
// from spudec_process_data
	int current_nibble = 0;
	int x = 0, y = 0, field = 0;
	ptr = data_start;
	int first_pixel = 1;

	while(ptr < end && y < subtitle->h + 1 && x < subtitle->w)
	{

// Start new field based on offset, not total lines
#ifdef USE_INTERLACE
		if(ptr - data_start >= odd_offset - 4 && 
			field == 0)
		{
// Only decode even field because too many bugs
			field = 1;
#ifndef SWAP_FIELDS
			y = FIELD_OFFSET1;
#endif
			x = 0;

			if(current_nibble)
			{
				ptr++;
				current_nibble = 0;
			}
		}
#endif // USE_INTERLACE

		unsigned int code = get_nibble(&ptr, &current_nibble);
		if(code < 0x4 && ptr < end)
		{
			code = (code << 4) | get_nibble(&ptr, &current_nibble);
			if(code < 0x10 && ptr < end)
			{
				code = (code << 4) | get_nibble(&ptr, &current_nibble);
				if(code < 0x40 && ptr < end)
				{
					code = (code << 4) | get_nibble(&ptr, &current_nibble);
/* carriage return */
					if(code < 0x4 && ptr < end)
						code |= (subtitle->w - x) << 2;
				}
			}
		}

		int color = (code & 0x3);
		int len = code >> 2;
//if(len == 0 || len >= subtitle->w - x) printf("%d\n", len);
		if(len > subtitle->w - x)
			len = subtitle->w - x;

		int y_color = file->palette[subtitle->palette[color] * 4 + 0];
		int u_color = file->palette[subtitle->palette[color] * 4 + 1];
		int v_color = file->palette[subtitle->palette[color] * 4 + 2];
		int a_color = subtitle->alpha[color];

// The alpha seems to be arbitrary.  Assume the top left pixel is always 
// transparent.
		if(first_pixel)
		{
			subtitle->alpha[color] = 0x0;
			a_color = 0x0;
			first_pixel = 0;
		}

/*
 * printf("0x%02x 0x%02x 0x%02x\n", 
 * y_color,
 * u_color,
 * v_color);
 */
		if(y < subtitle->h - 1)
		{
			for(i = 0; i < len; i++)
			{
				subtitle->image_y[y * subtitle->w + x] = y_color;
				subtitle->image_u[y * subtitle->w + x] = u_color;
				subtitle->image_v[y * subtitle->w + x] = v_color;
				subtitle->image_a[y * subtitle->w + x] = a_color;
#ifndef USE_INTERLACE
// line doubling
				subtitle->image_y[(y + 1) * subtitle->w + x] = y_color;
				subtitle->image_u[(y + 1) * subtitle->w + x] = u_color;
				subtitle->image_v[(y + 1) * subtitle->w + x] = v_color;
				subtitle->image_a[(y + 1) * subtitle->w + x] = a_color;
#endif
				x++;
			}
		}

		if(x >= subtitle->w)
		{
			x = 0;
// lined doubling
			y += 2;

/* Byte alignment */
			if(current_nibble)
			{
				ptr++;
				current_nibble = 0;
			}

// Clamp y
			if(y >= subtitle->h)
			{
#ifdef SWAP_FIELDS
				y++;
#endif
				while(y >= subtitle->h) y -= subtitle->h;
			}
		}
	}

#ifdef NORMALIZE
// Normalize image colors
	float min_h = 360;
	float max_h = 0;
	float threshold;
#define HISTOGRAM_SIZE 1000
// Decompression coefficients straight out of jpeglib
#define V_TO_R    1.40200
#define V_TO_G    -0.71414

#define U_TO_G    -0.34414
#define U_TO_B    1.77200
	unsigned char histogram[HISTOGRAM_SIZE];
	bzero(histogram, HISTOGRAM_SIZE);
	for(pass = 0; pass < 2; pass++)
	{
		for(i = 0; i < subtitle->h; i++)
		{
			for(j = 0; j < subtitle->w; j++)
			{
				if(subtitle->image_a[i * subtitle->w + j])
				{
					unsigned char *y_color = subtitle->image_y + i * subtitle->w + j;
					unsigned char *u_color = subtitle->image_u + i * subtitle->w + j;
					unsigned char *v_color = subtitle->image_v + i * subtitle->w + j;
					unsigned char *a_color = subtitle->image_a + i * subtitle->w + j;

// Convert to RGB
#ifndef USE_RGB
					float r = (*y_color + *v_color * V_TO_R);
					float g = (*y_color + *u_color * U_TO_G + *v_color * V_TO_G);
					float b = (*y_color + *u_color * U_TO_B);
#else
                    float r = *y_color;
                    float g = *u_color;
                    float b = *v_color;
#endif

// Multiply alpha
/*
 * 					r = r * *a_color / 0xff;
 * 					g = g * *a_color / 0xff;
 * 					b = b * *a_color / 0xff;
 */

// Convert to HSV
					float h, s, v;
					float min, max, delta;
					float f, p, q, t;
					min = ((r < g) ? r : g) < b ? ((r < g) ? r : g) : b;
					max = ((r > g) ? r : g) > b ? ((r > g) ? r : g) : b;
					v = max; 

					delta = max - min;

					if(max != 0 && delta != 0)
    				{
	    				s = delta / max;               // s

						if(r == max)
        					h = (g - b) / delta;         // between yellow & magenta
						else 
						if(g == max)
        					h = 2 + (b - r) / delta;     // between cyan & yellow
						else
        					h = 4 + (r - g) / delta;     // between magenta & cyan

						h *= 60;                               // degrees
						if(h < 0)
        					h += 360;
					}
					else 
					{
        				// r = g = b = 0                // s = 0, v is undefined
        				s = 0;
        				h = -1;
					}


/*
 * 					int magnitude = (int)(*y_color * *y_color + 
 * 						*u_color * *u_color + 
 * 						*v_color * *v_color);
 */

// Multiply alpha
					h = h * *a_color / 0xff;

					if(pass == 0)
					{
						histogram[(int)h]++;
						if(h < min_h) min_h = h;
						if(h > max_h) max_h = h;
					}
					else
					{
// Set new color in a 2x2 pixel block
#ifndef USE_RGB
						if(h > threshold)
						{
							*y_color = 0xff;
						}
						else
						{
							*y_color = 0;
						}

						*u_color = 0x80;
						*v_color = 0x80;

#else // !USE_RGB
						if(h > threshold)
						{
							*y_color = 0xff;
							*u_color = 0xff;
							*v_color = 0xff;
						}
						else
						{
							*y_color = 0;
							*u_color = 0;
							*v_color = 0;
						}
#endif // USE_RGB
						*a_color = 0xff;
					}
				}
			}
		}

		if(pass == 0)
		{
/*
 * 			int hist_total = 0;
 * 			for(i = 0; i < HISTOGRAM_SIZE; i++)
 * 			{
 * 				hist_total += histogram[i];
 * 			}
 * 
 * 			int hist_count = 0;
 * 			for(i = 0; i < HISTOGRAM_SIZE; i++)
 * 			{
 * 				hist_count += histogram[i];
 * 				if(hist_count > hist_total * 1 / 3)
 * 				{
 * 					threshold = i;
 * 					break;
 * 				}
 * 			}
 */
			threshold = (min_h + max_h) / 2;
//			threshold = 324;
//printf("min_h=%f max_h=%f threshold=%f\n", min_h, max_h, threshold);
		}
	}
#endif // NORMALIZE



/*
 * printf("decompress_subtitle coords: %d,%d - %d,%d size: %d,%d start_time=%d end_time=%d\n", 
 * subtitle->x1, 
 * subtitle->y1, 
 * subtitle->x2, 
 * subtitle->y2,
 * subtitle->w,
 * subtitle->h,
 * subtitle->start_time,
 * subtitle->stop_time);
 */
	return 0;
}




void overlay_subtitle(mpeg3video_t *video, mpeg3_subtitle_t *subtitle)
{
	int x, y;
	if(!subtitle->image_y ||
		!subtitle->image_u ||
		!subtitle->image_v ||
		!subtitle->image_a) return;

	for(y = subtitle->y1; 
		y < subtitle->y2 && y < video->coded_picture_height; 
		y++)
	{
		unsigned char *output_y = video->subtitle_frame[0] + 
			y * video->coded_picture_width +
			subtitle->x1;
		unsigned char *output_u = video->subtitle_frame[1] + 
			y / 2 * video->chrom_width +
			subtitle->x1 / 2;
		unsigned char *output_v = video->subtitle_frame[2] + 
			y / 2 * video->chrom_width +
			subtitle->x1 / 2;
		unsigned char *input_y = subtitle->image_y + (y - subtitle->y1) * subtitle->w;
		unsigned char *input_u = subtitle->image_u + (y - subtitle->y1) * subtitle->w;
		unsigned char *input_v = subtitle->image_v + (y - subtitle->y1) * subtitle->w;
		unsigned char *input_a = subtitle->image_a + (y - subtitle->y1) * subtitle->w;

		for(x = subtitle->x1; 
			x < subtitle->x2 && x < video->coded_picture_width; 
			x++)
		{
			unsigned int opacity = *input_a;
			unsigned int transparency = 0xff - opacity;
			*output_y = (*input_y * opacity + *output_y * transparency) / 0xff;

#ifdef MONOCHROME
			if(!(y % 2) && !(x % 2))
#endif
			{
				*output_u = (*input_u * opacity + *output_u * transparency) / 0xff;
				*output_v = (*input_v * opacity + *output_v * transparency) / 0xff;

#ifndef MONOCHROME
				if(!(y % 2) && !(x % 2))
				{
#endif
					output_u++;
					output_v++;
#ifndef MONOCHROME
				}
#endif

			}

			output_y++;
			input_y++;
			input_a++;
			input_u++;
			input_v++;
		}
	}
}



void mpeg3_decode_subtitle(mpeg3video_t *video)
{
/* Test demuxer for subtitle */
	mpeg3_vtrack_t *vtrack  = (mpeg3_vtrack_t*)video->track;
	mpeg3_t *file = (mpeg3_t*)video->file;

/* Clear subtitles from inactive subtitle tracks */
	int i;
	for(i = 0; i < mpeg3_subtitle_tracks(file); i++)
	{
		if(i != file->subtitle_track)
			mpeg3_pop_all_subtitles(mpeg3_get_strack(file, i));
	}

	if(file->subtitle_track >= 0 &&
		file->subtitle_track < mpeg3_subtitle_tracks(file))
	{
		mpeg3_strack_t *strack = mpeg3_get_strack(file, file->subtitle_track);
		int total = 0;
		if(strack)
		{
			for(i = 0; i < strack->total_subtitles; i++)
			{
				mpeg3_subtitle_t *subtitle = strack->subtitles[i];
				if(!subtitle->active)
				{
/* Exclude object from future activation */
					subtitle->active = 1;

/*
 * printf("mpeg3_decode_subtitle id=0x%x size=%d done=%d\n", 
 * subtitle->id, 
 * subtitle->size, 
 * subtitle->done);
 */
/* Decompress subtitle */
					if(mpeg3_decompress_subtitle(file, subtitle))
					{
/* Remove subtitle if failed */
						mpeg3_pop_subtitle(strack, i, 1);
						i--;
						continue;
					}
				}



/* Test start and end time of subtitle */
//printf("mpeg3_decode_subtitle subtitle->stop_time=%d\n", subtitle->stop_time);
				if(subtitle->stop_time > 0)
				{
// Copy video to temporary
					if(!total)
					{
						if(!video->subtitle_frame[0])
						{
							video->subtitle_frame[0] = malloc(
								video->coded_picture_width * 
								video->coded_picture_height + 8);
							video->subtitle_frame[1] = malloc(
								video->chrom_width * 
								video->chrom_height + 8);
							video->subtitle_frame[2] = malloc(
								video->chrom_width * 
								video->chrom_height + 8);
						}

						memcpy(video->subtitle_frame[0],
							video->output_src[0],
							video->coded_picture_width * video->coded_picture_height);
						memcpy(video->subtitle_frame[1],
							video->output_src[1],
							video->chrom_width * video->chrom_height);
						memcpy(video->subtitle_frame[2],
							video->output_src[2],
							video->chrom_width * video->chrom_height);

						video->output_src[0] = video->subtitle_frame[0];
						video->output_src[1] = video->subtitle_frame[1];
						video->output_src[2] = video->subtitle_frame[2];
					}
					total++;


// Overlay subtitle on video
					overlay_subtitle(video, subtitle);
					subtitle->stop_time -= (int)(100.0 / video->frame_rate);
				}

				if(subtitle->stop_time <= 0)
				{
					mpeg3_pop_subtitle(strack, i, 1);
					i--;
				}
			}
		}
	}




}





