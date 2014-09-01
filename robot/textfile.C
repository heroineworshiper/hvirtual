#include "textfile.h"


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))

TextFile::TextFile(char *path, int rd, int wr)
{
	buffer_start = 0;
	this->wr = wr;
	this->rd = rd;
	type = 0;
	size = 0;
	data = 0;
	strcpy(this->path, path);

	if(wr)
	{
		buffer_start = new char[BCTEXTLEN];
		buffer_end = buffer_start + BCTEXTLEN;
		buffer_ptr = buffer_start;
	}
}

TextFile::~TextFile()
{

	if(buffer_start) delete [] buffer_start;
}

int TextFile::read_db()
{
	FILE *fd;
	if(rd)
	{
		fd = fopen(path, "r");
		if(fd)
		{
			struct stat64 stat_result;
			stat64(path, &stat_result);
			buffer_start = new char[stat_result.st_size];
			buffer_end = buffer_start + stat_result.st_size;
			buffer_ptr = buffer_start;
			int result = fread(buffer_start, 1, stat_result.st_size, fd);
			fclose(fd);

			if(result < stat_result.st_size)
			{
				fprintf(stderr, "TextFile::TextFile fread %s: %s\n",
					path,
					strerror(errno));
				return 1;
			}
		}
		else
		{
			fprintf(stderr, "TextFile::TextFile fopen %s for read: %s\n",
				path,
				strerror(errno));
			return 1;
		}
	}
	return 0;
}

int TextFile::write_db()
{
	FILE *fd;
	if(wr)
	{
		fd = fopen(path, "w");


		if(fd)
		{
			int result = fwrite(buffer_start, 1, buffer_ptr - buffer_start, fd);
			fclose(fd);
			if(result < buffer_ptr - buffer_start)
			{
				fprintf(stderr, "TextFile::~TextFile fwrite %s: %s\n",
					path,
					strerror(errno));
				return 1;
			}
		}
		else
		{
			fprintf(stderr, "TextFile::~TextFile fopen %s for write: %s\n",
				path,
				strerror(errno));
			return 1;
		}
	}
	return 0;
}


int TextFile::read_record()
{
	type = 0;
	size = 0;
	data = 0;
	subrecords = 0;


	if(!buffer_start) return 1;

	if(buffer_ptr > buffer_end - 9 - 5 - 9) return 1;

// subrecords
// SScanf is really slow when given the entire buffer.  Use a temporary instead.
	char temp[9];
	memcpy(temp, buffer_ptr, 8);
	temp[8] = 0;
	buffer_ptr += 9;
	sscanf(temp, "%x", &subrecords);

// type
	type = buffer_ptr;

// size
	buffer_ptr += 5;
	memcpy(temp, buffer_ptr, 8);
	temp[8] = 0;
	buffer_ptr += 9;
	sscanf(temp, "%x", &size);

// data
	data = buffer_ptr;

	buffer_ptr += size + 1;
	return 0;
}

int TextFile::write_record(int subrecords, char *type, int size, char *data)
{
	int record_size = 9 + 5 + 9 + size + 1;
	if(buffer_end - buffer_ptr < record_size)
	{
		int new_size = MAX(buffer_end - buffer_start + record_size, 
			(buffer_end - buffer_start) * 2);
		char *new_buffer = new char[new_size];
		memcpy(new_buffer, buffer_start, buffer_ptr - buffer_start);
		delete [] buffer_start;
		buffer_ptr = new_buffer + (buffer_ptr - buffer_start);
		buffer_end = new_buffer + new_size;
		buffer_start = new_buffer;
	}

// subrecords
	sprintf(buffer_ptr, "%08x ", subrecords);
	buffer_ptr += 9;

// type
	sprintf(buffer_ptr, "%s ", type);
	buffer_ptr += 5;

// size
	sprintf(buffer_ptr, "%08x ", size);
	buffer_ptr += 9;

// data
	memcpy(buffer_ptr, data, size);
	buffer_ptr += size;

// newline
	*buffer_ptr++ = 0xa;

	return 0;
}

int TextFile::type_is(char *value)
{
	if(!type) return 0;
	return !memcmp(value, type, 4);
}


int TextFile::get_size()
{
	if(wr)
		return buffer_ptr - buffer_start;
	else
		return buffer_end - buffer_start;
}


int TextFile::get_position()
{
	return buffer_ptr - buffer_start;
}

