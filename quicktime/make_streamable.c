#include "quicktime.h"
#include <string.h>

void print_help(char *program_name)
{
	printf("This program moves the headers to the start of the movie.\n");
	printf("It can also tag a movie for spherical playback.\n");
	printf("usage: %s [-360] <in filename> <out filename>\n", program_name);
	printf(" -360  Inject header for spherical playback.\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int i;
	int do_360 = 0;
	char *in_filename = 0;
	char *out_filename = 0;
	
	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-h"))
		{
			print_help(argv[0]);
		}
		else
		if(!strcmp(argv[i], "-360"))
		{
			do_360 = 1;
		}
		else
		if(in_filename == 0)
		{
			in_filename = argv[i];
		}
		else
		if(out_filename == 0)
		{
			out_filename = argv[i];
		}
	}
	
	if(argc < 2 || !in_filename || !out_filename)
	{
		print_help(argv[0]);
	}

	if(quicktime_make_streamable(in_filename, out_filename, do_360))
	{
		exit(1);
	}

	return 0;
}
