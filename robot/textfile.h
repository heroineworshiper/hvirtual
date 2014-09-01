#ifndef TEXTFILE_H
#define TEXTFILE_H

#include "bcwindowbase.inc"
#include <stdio.h>

class TextFile
{
public:
	TextFile(char *path, int rd, int wr);
	~TextFile();

	int read_db();
	int write_db();
	int read_record();
	int write_record(int subrecords, char *type, int size, char *data);
	int type_is(char *value);
	int get_size();
	int get_position();

// Returns for read_record
	int subrecords;
	char *type;
	int size;
	char *data;

private:
	char *buffer_ptr;
	char *buffer_end;
	char *buffer_start;
	int rd;
	int wr;
	char path[BCTEXTLEN];
};


#endif
