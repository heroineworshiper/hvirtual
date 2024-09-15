// Failed attempt to extract subtitle images from a .sub file & idx file.
// The format is actually much different from the DVD format.

// the .sub & .idx files come from mkvextract
// It encodes the subtitle images in a movie file with 1 color as the alpha
// The video editor then has to composite the subtitle movie with a chroma key
// preferrably with the subtitles not covering the video.


#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// rgb chroma key
#define CHROMA_R 0
#define CHROMA_G 0
#define CHROMA_B 255

int main(int argc, char *argv[])
{
    char *subpath = 0;
    char *idxpath = 0;
    char *outpath = 0;
    mpeg3_t *subs_in;
    FILE *idx_in;
    int i, j;
    int error;
    int w, h;

	if(argc < 4)
	{
		fprintf(stderr, "Subtitle renderer\n"
			"Usage: mpeg3subs <sub path> <idx path> <output path>\n"
			"Example: mpeg3subs english.sub english.idx subs.mp4\n");
		exit(1);
    }

	for(i = 1; i < argc; i++)
	{
        if(!subpath) 
            subpath = argv[i];
        else
        if(!idxpath)
            idxpath = argv[i];
        else
        if(!outpath)
            outpath = argv[i];
    }

// need this 1st
    if(!(subs_in = mpeg3_open(subpath, &error)))
    {
        fprintf(stderr, "Couldn't open %s\n", subpath);
        exit(1);
    }
    subs_in->seekable = 0;

    if(!(idx_in = fopen(idxpath, "r")))
    {
        fprintf(stderr, "Couldn't open %s\n", idxpath);
        exit(1);
    }
    
    char string[MPEG3_STRLEN];
    char string2[MPEG3_STRLEN];
    while(!feof(idx_in))
    {
        char *s = fgets(string, MPEG3_STRLEN, idx_in);
        if(!s) break;
        
#define PALETTE_TAG "palette:"
#define SIZE_TAG "size:"
        if(!strncasecmp(s, PALETTE_TAG, strlen(PALETTE_TAG)))
        {
//printf("main %d: %s", __LINE__, s);
            char *ptr = s + strlen(PALETTE_TAG);

            for(i = 0; i < 16 && *ptr != 0; i++)
            {
// skip whitespace
                while(*ptr == ' ' && *ptr != 0) ptr++;

// copy until ,
                char *ptr2 = string2;
                while(*ptr != 0 && *ptr != ',') *ptr2++ = *ptr++;
                *ptr2++ = 0;
// skip ,
                if(*ptr == ',') ptr++;
                uint32_t value;
                sscanf(string2, "%x", &value);
//                printf("main %d: palette %d=0x%x\n", __LINE__, i, value);
                int r, g, b;
                r = (value >> 16) & 0xff;
                g = (value >> 8) & 0xff;
                b = (value) & 0xff;
                subs_in->palette[i * 4] = r;
                subs_in->palette[i * 4 + 1] = g;
                subs_in->palette[i * 4 + 2] = b;
            }
            subs_in->have_palette = 1;
        }
        else
        if(!strncasecmp(s, SIZE_TAG, strlen(SIZE_TAG)))
        {
            char *ptr = s + strlen(SIZE_TAG);
// skip whitespace
            while(*ptr == ' ' && *ptr != 0) ptr++;
// get w
            char *ptr2 = string2;
            while(*ptr != 0 && *ptr != 'x') *ptr2++ = *ptr++;
            *ptr2++ = 0;
            if(*ptr == 'x') ptr++;
            w = atoi(string2);
// get h
            ptr2 = string2;
            while(*ptr != 0) *ptr2++ = *ptr++;
            *ptr2++ = 0;
            h = atoi(string2);
            printf("main %d: size=%dx%d\n", __LINE__,  w, h);
        }

//        printf("main %d: %s", __LINE__, s);
    }
    fclose(idx_in);
    

    printf("main %d: got %d subtitle tracks\n", __LINE__, mpeg3_subtitle_tracks(subs_in));
    if(mpeg3_subtitle_tracks(subs_in) < 1)
    {
        fprintf(stderr, "No subtitle tracks found\n");
        exit(1);
    }

// select the subtitle track
    mpeg3_show_subtitle(subs_in, 0);
    mpeg3_strack_t *strack = mpeg3_get_strack(subs_in, subs_in->subtitle_track);;

// the output video encoding
    uint8_t *output = malloc(w * h * 3);
    sprintf(string, 
        "ffmpeg -y -f rawvideo -y -pix_fmt rgb24 -r 10 -s:v %dx%d -i - -c:v mpeg4 -qscale:v 5 -pix_fmt yuvj420p -an %s",
        w,
        h,
        outpath);
    printf("main %d: running %s\n", __LINE__, string);
    FILE *ffmpeg_writer = popen(string, "w");
// DEBUG
//    FILE *ffmpeg_writer = 0;

// read through the file again
    mpeg3demux_seek_byte(subs_in->demuxer, 0);
    mpeg3_pop_all_subtitles(strack);
    while(!mpeg3_read_next_packet(subs_in->demuxer))
    {
//         printf("main %d: offset=%x total_subtitles=%d\n", 
//             __LINE__, 
//             (int)mpeg3demux_tell_byte(subs_in->demuxer),
//             strack->total_subtitles);
        while(strack->total_subtitles > 0)
        {
            mpeg3_subtitle_t *subtitle = strack->subtitles[0];
            mpeg3_decompress_subtitle(subs_in, subtitle);
// overlay the subtitle
            for(i = 0; i < h * w; i++)
            {
                output[i * 3 + 0] = CHROMA_R;
                output[i * 3 + 1] = CHROMA_G;
                output[i * 3 + 2] = CHROMA_B;
            }
	        for(i = subtitle->y1; 
		        i < subtitle->y2 && i < h; 
		        i++)
            {
		        unsigned char *output_row = output + 
			        (i * w + subtitle->x1) * 3;
		        unsigned char *input_r = subtitle->image_y + (i - subtitle->y1) * subtitle->w;
		        unsigned char *input_g = subtitle->image_u + (i - subtitle->y1) * subtitle->w;
		        unsigned char *input_b = subtitle->image_v + (i - subtitle->y1) * subtitle->w;
		        unsigned char *input_a = subtitle->image_a + (i - subtitle->y1) * subtitle->w;
                
		        for(j = subtitle->x1; 
			        j < subtitle->x2 && j < w; 
			        j++)
                {
                    if(*input_a > 0)
                    {
                        output_row[0] = *input_r;
                        output_row[1] = *input_g;
                        output_row[2] = *input_b;
                    }
                    output_row += 3;
                    input_r++;
                    input_g++;
                    input_b++;
                    input_a++;
                }
            }

            if(ffmpeg_writer) fwrite(output, w * h * 3, 1, ffmpeg_writer);
            mpeg3_pop_subtitle(strack, 0, 1);
        }
    }

    printf("main %d: reached end of file\n", __LINE__);
}






