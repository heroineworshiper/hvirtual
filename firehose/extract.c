#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BUFFER_SIZE 0x2000000

int64_t get_number(char *text)
{
	int64_t result;
	if(text[0] == '0' && 
		text[1] == 'x')
	{
		sscanf(text, "%llx", &result);
	}
	else
	{
		sscanf(text, "%lld", &result);
	}
	return result;
}

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		fprintf(stderr, "Usage: extract filename <start> <end>\n"
			"Start and end may be decimal or prefixed by 0x for hex.\n");
		exit(1);
	}

	char *filename = argv[1];
	FILE *file = fopen64(filename, "r");
	if(!file)
	{
		perror("fopen");
		exit(1);
	}

	unsigned char *buffer = malloc(BUFFER_SIZE);
	int64_t start = get_number(argv[2]);
	int64_t end = get_number(argv[3]);

	fprintf(stderr, "Extracting %s from %lld (%llx) to %lld (%llx)\n",
		filename,
		start,
		start,
		end,
		end);

	int64_t i;
	fseeko64(file, start, SEEK_SET);
	for(i = start; i < end; i += BUFFER_SIZE)
	{
		int fragment_size = BUFFER_SIZE;
		if(i + fragment_size > end) fragment_size = end - i;
		fprintf(stderr, "Reading from %llx        \r", i);
		fflush(stdout);
		fread(buffer, fragment_size, 1, file);
		fprintf(stderr, "Writing to %llx         \r", i);
		fflush(stdout);
		fwrite(buffer, fragment_size, 1, stdout);
	}
	fclose(file);
}
