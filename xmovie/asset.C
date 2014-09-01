#include "asset.h"
#include "quicktime.h"
#include <string.h>

Asset::Asset()
{
	init_values();
}


Asset::Asset(const char *path)
{
	init_values();
	strcpy(this->path, path);
}

int Asset::init_values()
{
	audio_data = video_data = 0;
	format = 0;
	channels = 0;
// Needed for synchronization
	rate = 48000;
	bits = 0;
	byte_order = 0;
	signed_ = 0;
	header = 0;
	
	frame_rate = 0;
	width = 0;
	height = 0;
	strcpy(compression, QUICKTIME_YUV2);
	quality = 100;
	audio_streams = 0;
	video_streams = 0;
	return 0;
}


Asset& Asset::operator=(Asset &asset)
{
	strcpy(this->path, asset.path);

	audio_data = asset.audio_data;
	format = asset.format;
	channels = asset.channels;
	rate = asset.rate;
	bits = asset.bits;
	byte_order = asset.byte_order;
	signed_ = asset.signed_;
	header = asset.header;
	audio_streams = asset.audio_streams;

	video_data = asset.video_data;
	frame_rate = asset.frame_rate;
	width = asset.width;
	height = asset.height;
	quality = asset.quality;
	strcpy(compression, asset.compression);
	video_streams = asset.video_streams;
	return *this;
}

int Asset::operator==(Asset &asset)
{
	if(
	format == asset.format && 
	channels == asset.channels && 
	rate == asset.rate && 
	bits == asset.bits && 
	byte_order == asset.byte_order && 
	signed_ == asset.signed_ && 
	header == asset.header && 
	audio_streams == asset.audio_streams &&

	frame_rate == asset.frame_rate &&
	quality == asset.quality &&
	width == asset.width &&
	height == asset.height &&
	!strcmp(compression, asset.compression) &&
	video_streams == asset.video_streams
	) return 1;
	else
	return 0;
}

int Asset::operator!=(Asset &asset)
{
	return !(*this == asset);
}

Asset::~Asset()
{
}


int Asset::dump()
{
	printf("asset::dump\n");
	printf("	audio_data %d format %d channels %d samplerate %d bits %d byte_order %d signed %d header %d\n",
		audio_data, format, channels, rate, bits, byte_order, signed_, header);
	printf("	video_data %d framerate %f width %d height %d compression %c%c%c%c\n",
		video_data, frame_rate, width, height, compression[0], compression[1], compression[2], compression[3]);
	return 0;
}
