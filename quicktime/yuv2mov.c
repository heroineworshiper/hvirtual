#include "colormodels.h"
#include "quicktime.h"
#include <signal.h>
#include <string.h>


quicktime_t *file = 0;
//#define COMPRESSOR QUICKTIME_JPEG
#define COMPRESSOR QUICKTIME_MP4V
#define CROP_Y1 142
#define CROP_Y2 942
#define INWIDTH 1920
#define INHEIGHT 1080
#define FRAMERATE (24000.0 / 1001.0)
#define QUANTIZATION 5

void signal_handler(int signum)
{
	printf("Got signal %d\n", signum);
	if(file)
	{
		quicktime_close(file);
	}
	abort();
}


int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("Usage: cat yuvstream | yuv2mov movie.mov\n"
			"	Create a movie from a yuv 4:2:0 planar stream.\n");
		exit(1);
	}

	signal(SIGINT, signal_handler);

	int i;
	char filename[1024];
	filename[0] = 0;
	for(i = 1; i < argc; i++)
	{
		if(!filename[0])
		{
			strcpy(filename, argv[i]);
		}
		else
		{
			printf("Unknown option %s\n", argv[i]);
			exit(1);
		}
	}
	
	if(!filename[0])
	{
		printf("No output filename provided.\n");
		exit(1);
	}
	
	if(!(file = quicktime_open(filename, 0, 1)))
	{
		printf("Open '%s' failed\n", filename);
		exit(1);
	}

	quicktime_set_video(file, 
		1, 
		INWIDTH, 
		CROP_Y2 - CROP_Y1, 
		FRAMERATE, 
		COMPRESSOR);

	int value = 0;
	value = 20000000;
	quicktime_set_parameter(file, "ffmpeg_bitrate", &value);
	value = 5000000;
	quicktime_set_parameter(file, "ffmpeg_bitrate_tolerance", &value);
	value = 0;
	quicktime_set_parameter(file, "ffmpeg_interlaced", &value);
	value = QUANTIZATION;
	quicktime_set_parameter(file, "ffmpeg_quantizer", &value);
	value = 45;
	quicktime_set_parameter(file, "ffmpeg_gop_size", &value);
	value = 0;
	quicktime_set_parameter(file, "ffmpeg_fix_bitrate", &value);

	value = 85;
	quicktime_set_parameter(file, "jpeg_quality", &value);

	int ysize = INWIDTH * INHEIGHT;
	int csize = INWIDTH * INHEIGHT / 4;
	unsigned char *buffer = malloc(INWIDTH * INHEIGHT * 3 / 2);
	unsigned char *outrows[3];
	int frame = 0;
	outrows[0] = buffer + CROP_Y1 * INWIDTH;
	outrows[1] = buffer + ysize + CROP_Y1 / 2 * INWIDTH / 2;
	outrows[2] = buffer + ysize + csize + CROP_Y1 / 2 * INWIDTH / 2;

	while(!feof(stdin))
	{
		fread(buffer, INWIDTH * INHEIGHT * 3 / 2, 1, stdin);
		quicktime_set_cmodel(file, BC_YUV420P);
		quicktime_encode_video(file, 
			outrows, 
			0);
		printf("Wrote frame %d\r", frame++);
		fflush(stdout);
	}
	quicktime_close(file);
}

