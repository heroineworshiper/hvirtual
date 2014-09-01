#ifndef ASSET_H
#define ASSET_H

class Asset
{
public:
	Asset();
	Asset(const char *path);
	~Asset();

	int dump();

	Asset& operator=(Asset &asset);
	int operator==(Asset &asset);
	int operator!=(Asset &asset);

	char path[1024];
	int format;      // format of file
	int audio_data;     // contains audio data
	int channels, rate, bits, byte_order, signed_, header;
	int audio_streams;

	int video_data;     // contains video data
	float frame_rate;
	int width, height;
	int silence;
	char compression[5];
	int quality;     // for jpeg compression
	int video_streams;
private:
	int init_values();
};




#endif
