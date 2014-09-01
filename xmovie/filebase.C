#include "asset.h"
#include "byteorder.h"
#include "file.h"
#include "filebase.h"
#include "quicktime.h"



FileBase::FileBase(Asset *asset, File *file)
{
	this->file = file;
	this->asset = asset;
	this->mwindow = file->mwindow;
	internal_byte_order = get_byte_order();
	reset();
}

FileBase::~FileBase()
{
	if(read_buffer)
		delete read_buffer;
	
	read_buffer = 0;
}

int FileBase::close_file()
{
	close_file_derived();
	reset();
	return 0;
}

int FileBase::reset()
{
	read_buffer = 0;
	read_size = 0;
	return 0;
}


int FileBase::match4(char *in, char *out)
{
	if(in[0] == out[0] &&
		in[1] == out[1] &&
		in[2] == out[2] &&
		in[3] == out[3])
	return 1;
	else
	return 0;
}

int FileBase::get_read_buffer(long len)
{
	if(read_size != len)
	{
		delete read_buffer;
		read_buffer = 0;
	}
	
	if(!read_buffer) read_buffer = new int16_t[len];
	read_size = len;
	return 0;
}
