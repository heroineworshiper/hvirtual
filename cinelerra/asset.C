/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */


#include "asset.h"
#include "assets.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexstate.h"
#include "quicktime.h"

#include <stdio.h>
#include <string.h>


Asset::Asset()
 : ListItem<Asset>(), Indexable(1)
{
	init_values();
}

Asset::Asset(Asset &asset)
 : ListItem<Asset>(), Indexable(1)
{
	init_values();
	this->copy_from(&asset, 1);
}

Asset::Asset(const char *path)
 : ListItem<Asset>(), Indexable(1)
{
	init_values();
	strcpy(this->path, path);
}

Asset::Asset(const int plugin_type, const char *plugin_title)
 : ListItem<Asset>(), Indexable(1)
{
	init_values();
}

Asset::~Asset()
{
}


int Asset::init_values()
{
	path[0] = 0;
//	format = FILE_MOV;
// Has to be unknown for file probing to succeed
	format = FILE_UNKNOWN;
	channels = 0;
	sample_rate = 0;
	bits = 0;
	byte_order = 0;
	signed_ = 0;
	header = 0;
	dither = 0;
	audio_data = 0;
	video_data = 0;
	audio_length = 0;
	video_length = 0;

	layers = 0;
	frame_rate = 0;
	width = 0;
	height = 0;
	strcpy(vcodec, QUICKTIME_YUV2);
	strcpy(acodec, QUICKTIME_TWOS);
	jpeg_quality = 80;
	aspect_ratio = -1;

	ampeg_bitrate = 256;
	ampeg_derivative = 3;

	vorbis_vbr = 0;
	vorbis_min_bitrate = -1;
	vorbis_bitrate = 128000;
	vorbis_max_bitrate = -1;

	theora_fix_bitrate = 1;
	theora_bitrate = 860000;
	theora_quality = 16;
	theora_sharpness = 2;
	theora_keyframe_frequency = 64;
	theora_keyframe_force_frequency = 64;

	mp3_bitrate = 256000;


	mp4a_bitrate = 192000;
	mp4a_quantqual = 100;



// mpeg parameters
	vmpeg_iframe_distance = 45;
	vmpeg_pframe_distance = 0;
	vmpeg_progressive = 0;
	vmpeg_denoise = 1;
	vmpeg_bitrate = 1000000;
	vmpeg_derivative = 1;
	vmpeg_quantization = 15;
	vmpeg_cmodel = 0;
	vmpeg_fix_bitrate = 0;
	vmpeg_seq_codes = 0;
	vmpeg_preset = 0;
	vmpeg_field_order = 0;

// Divx parameters.  BC_Hash from encore2
	divx_bitrate = 2000000;
	divx_rc_period = 50;
	divx_rc_reaction_ratio = 45;
	divx_rc_reaction_period = 10;
	divx_max_key_interval = 250;
	divx_max_quantizer = 31;
	divx_min_quantizer = 1;
	divx_quantizer = 5;
	divx_quality = 5;
	divx_fix_bitrate = 1;
	divx_use_deblocking = 1;

	h264_bitrate = 2000000;
	h264_quantizer = 28;
	h264_fix_bitrate = 0;

	ms_bitrate = 1000000;
	ms_bitrate_tolerance = 500000;
	ms_quantization = 10;
	ms_interlaced = 0;
	ms_gop_size = 45;
	ms_fix_bitrate = 1;

	ac3_bitrate = 128;

	png_use_alpha = 0;
	exr_use_alpha = 0;
	exr_compression = 0;

	tiff_cmodel = 0;
	tiff_compression = 0;
	is_sphere = 0;

	use_header = 1;


	reset_index();
	id = EDL::next_id();
	return 0;
}

void Asset::boundaries()
{
//printf("Asset::boundaries %d %d %f\n", __LINE__, sample_rate, frame_rate);
// sample_rate & frame_rate are user defined
// 	CLAMP(sample_rate, 1, 1000000);
// 	CLAMP(frame_rate, 0.001, 1000000);
 	CLAMP(channels, 1, 1000000);
 	CLAMP(width, 1, 1000000);
 	CLAMP(height, 1, 1000000);
//printf("Asset::boundaries %d %d %f\n", __LINE__, sample_rate, frame_rate);
}

int Asset::reset_index()
{
	index_state->reset();
}

void Asset::copy_from(Asset *asset, int do_index)
{
	copy_location(asset);
	copy_format(asset, do_index);
}

void Asset::copy_location(Asset *asset)
{
	strcpy(this->path, asset->path);
	strcpy(this->folder, asset->folder);
}

void Asset::copy_format(Asset *asset, int do_index)
{
	if(do_index) update_index(asset);

	audio_data = asset->audio_data;
	format = asset->format;
	channels = asset->channels;
	sample_rate = asset->sample_rate;
	bits = asset->bits;
	byte_order = asset->byte_order;
	signed_ = asset->signed_;
	header = asset->header;
	dither = asset->dither;
	mp3_bitrate = asset->mp3_bitrate;
	mp4a_bitrate = asset->mp4a_bitrate;
	mp4a_quantqual = asset->mp4a_quantqual;
	use_header = asset->use_header;
	aspect_ratio = asset->aspect_ratio;

	video_data = asset->video_data;
	layers = asset->layers;
	frame_rate = asset->frame_rate;
	width = asset->width;
	height = asset->height;
	strcpy(vcodec, asset->vcodec);
	strcpy(acodec, asset->acodec);

	this->audio_length = asset->audio_length;
	this->video_length = asset->video_length;


	ampeg_bitrate = asset->ampeg_bitrate;
	ampeg_derivative = asset->ampeg_derivative;


	vorbis_vbr = asset->vorbis_vbr;
	vorbis_min_bitrate = asset->vorbis_min_bitrate;
	vorbis_bitrate = asset->vorbis_bitrate;
	vorbis_max_bitrate = asset->vorbis_max_bitrate;

	
	theora_fix_bitrate = asset->theora_fix_bitrate;
	theora_bitrate = asset->theora_bitrate;
	theora_quality = asset->theora_quality;
	theora_sharpness = asset->theora_sharpness;
	theora_keyframe_frequency = asset->theora_keyframe_frequency;
	theora_keyframe_force_frequency = asset->theora_keyframe_frequency;


	jpeg_quality = asset->jpeg_quality;

// mpeg parameters
	vmpeg_iframe_distance = asset->vmpeg_iframe_distance;
	vmpeg_pframe_distance = asset->vmpeg_pframe_distance;
	vmpeg_progressive = asset->vmpeg_progressive;
	vmpeg_denoise = asset->vmpeg_denoise;
	vmpeg_bitrate = asset->vmpeg_bitrate;
	vmpeg_derivative = asset->vmpeg_derivative;
	vmpeg_quantization = asset->vmpeg_quantization;
	vmpeg_cmodel = asset->vmpeg_cmodel;
	vmpeg_fix_bitrate = asset->vmpeg_fix_bitrate;
	vmpeg_seq_codes = asset->vmpeg_seq_codes;
	vmpeg_preset = asset->vmpeg_preset;
	vmpeg_field_order = asset->vmpeg_field_order;


	divx_bitrate = asset->divx_bitrate;
	divx_rc_period = asset->divx_rc_period;
	divx_rc_reaction_ratio = asset->divx_rc_reaction_ratio;
	divx_rc_reaction_period = asset->divx_rc_reaction_period;
	divx_max_key_interval = asset->divx_max_key_interval;
	divx_max_quantizer = asset->divx_max_quantizer;
	divx_min_quantizer = asset->divx_min_quantizer;
	divx_quantizer = asset->divx_quantizer;
	divx_quality = asset->divx_quality;
	divx_fix_bitrate = asset->divx_fix_bitrate;
	divx_use_deblocking = asset->divx_use_deblocking;

	h264_bitrate = asset->h264_bitrate;
	h264_quantizer = asset->h264_quantizer;
	h264_fix_bitrate = asset->h264_fix_bitrate;


	ms_bitrate = asset->ms_bitrate;
	ms_bitrate_tolerance = asset->ms_bitrate_tolerance;
	ms_interlaced = asset->ms_interlaced;
	ms_quantization = asset->ms_quantization;
	ms_gop_size = asset->ms_gop_size;
	ms_fix_bitrate = asset->ms_fix_bitrate;

	
	ac3_bitrate = asset->ac3_bitrate;
	
	png_use_alpha = asset->png_use_alpha;
	exr_use_alpha = asset->exr_use_alpha;
	exr_compression = asset->exr_compression;

	tiff_cmodel = asset->tiff_cmodel;
	tiff_compression = asset->tiff_compression;
	
	
	is_sphere = asset->is_sphere;
}

int64_t Asset::get_index_offset(int channel)
{
	return index_state->get_index_offset(channel);
}

int64_t Asset::get_index_size(int channel)
{
	return index_state->get_index_size(channel);
}


char* Asset::get_compression_text(int audio, int video)
{
	if(audio)
	{
		switch(format)
		{
			case FILE_MOV:
			case FILE_AVI:
				if(acodec[0])
					return quicktime_acodec_title(acodec);
				else
					return 0;
				break;
		}
	}
	else
	if(video)
	{
		switch(format)
		{
			case FILE_MOV:
			case FILE_AVI:
				if(vcodec[0])
					return quicktime_vcodec_title(vcodec);
				else
					return 0;
				break;
		}
	}
	return 0;
}

Asset& Asset::operator=(Asset &asset)
{
printf("Asset::operator=\n");
	copy_location(&asset);
	copy_format(&asset, 1);
	return *this;
}


int Asset::equivalent(Asset &asset, 
	int test_audio, 
	int test_video)
{
	int result = (!strcmp(asset.path, path) &&
		format == asset.format);

	if(test_audio && result)
	{
		result = (channels == asset.channels && 
			sample_rate == asset.sample_rate && 
			bits == asset.bits && 
			byte_order == asset.byte_order && 
			signed_ == asset.signed_ && 
			header == asset.header && 
			dither == asset.dither &&
			!strcmp(acodec, asset.acodec));
	}


	if(test_video && result)
	{
		result = (layers == asset.layers && 
			frame_rate == asset.frame_rate &&
			width == asset.width &&
			height == asset.height &&
			!strcmp(vcodec, asset.vcodec) &&
			is_sphere == asset.is_sphere);
	}

	return result;
}

int Asset::operator==(Asset &asset)
{

	return equivalent(asset, 
		1, 
		1);
}

int Asset::operator!=(Asset &asset)
{
	return !(*this == asset);
}

int Asset::test_path(const char *path)
{
	if(!strcasecmp(this->path, path)) 
		return 1; 
	else 
		return 0;
}

int Asset::test_plugin_title(const char *path)
{
}

int Asset::read(FileXML *file, 
	int expand_relative)
{
	int result = 0;

// Check for relative path.
	if(expand_relative)
	{
		char new_path[BCTEXTLEN];
		char asset_directory[BCTEXTLEN];
		char input_directory[BCTEXTLEN];
		FileSystem fs;

		strcpy(new_path, path);
		fs.set_current_dir("");

		fs.extract_dir(asset_directory, path);

// No path in asset.
// Take path of XML file.
		if(!asset_directory[0])
		{
			fs.extract_dir(input_directory, file->filename);

// Input file has a path
			if(input_directory[0])
			{
				fs.join_names(path, input_directory, new_path);
			}
		}
	}


	while(!result)
	{
		result = file->read_tag();
		if(!result)
		{
			if(file->tag.title_is("/ASSET"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("AUDIO"))
			{
				read_audio(file);
			}
			else
			if(file->tag.title_is("AUDIO_OMIT"))
			{
				read_audio(file);
			}
			else
			if(file->tag.title_is("FORMAT"))
			{
				char *string = file->tag.get_property("TYPE");
				format = File::strtoformat(string);
				use_header = 
					file->tag.get_property("USE_HEADER", use_header);
			}
			else
			if(file->tag.title_is("FOLDER"))
			{
				strcpy(folder, file->read_text());
			}
			else
			if(file->tag.title_is("VIDEO"))
			{
				read_video(file);
			}
			else
			if(file->tag.title_is("VIDEO_OMIT"))
			{
				read_video(file);
			}
			else
			if(file->tag.title_is("INDEX"))
			{
				read_index(file);
			}
		}
	}

	boundaries();
//printf("Asset::read 2\n");
	return 0;
}

int Asset::read_audio(FileXML *file)
{
	if(file->tag.title_is("AUDIO")) audio_data = 1;
	channels = file->tag.get_property("CHANNELS", 2);
// This is loaded from the index file after the EDL but this 
// should be overridable in the EDL.
	if(!sample_rate) sample_rate = file->tag.get_property("RATE", 48000);
	bits = file->tag.get_property("BITS", 16);
	byte_order = file->tag.get_property("BYTE_ORDER", 1);
	signed_ = file->tag.get_property("SIGNED", 1);
	header = file->tag.get_property("HEADER", 0);
	dither = file->tag.get_property("DITHER", 0);

	audio_length = file->tag.get_property("AUDIO_LENGTH", (int64_t)0);
	acodec[0] = 0;
	file->tag.get_property("ACODEC", acodec);
	



	return 0;
}

int Asset::read_video(FileXML *file)
{
	if(file->tag.title_is("VIDEO")) video_data = 1;
	height = file->tag.get_property("HEIGHT", height);
	width = file->tag.get_property("WIDTH", width);
	layers = file->tag.get_property("LAYERS", layers);
// This is loaded from the index file after the EDL but this 
// should be overridable in the EDL.
	if(EQUIV(frame_rate, 0)) frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
	vcodec[0] = 0;
	file->tag.get_property("VCODEC", vcodec);

	video_length = file->tag.get_property("VIDEO_LENGTH", (int64_t)0);
	is_sphere = file->tag.get_property("IS_SPHERE", 0);

	return 0;
}

int Asset::read_index(FileXML *file)
{
	index_state->read_xml(file, channels);
	return 0;
}

int Asset::write_index(const char *path, int data_bytes)
{
	index_state->write_index(path, data_bytes, this, audio_length);
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.

int Asset::write(FileXML *file, 
	int include_index, 
	const char *output_path)
{
	char new_path[BCTEXTLEN];
	char asset_directory[BCTEXTLEN];
	char output_directory[BCTEXTLEN];
	FileSystem fs;

// Make path relative
	fs.extract_dir(asset_directory, path);
	if(output_path && output_path[0]) 
		fs.extract_dir(output_directory, output_path);
	else
		output_directory[0] = 0;

// Asset and EDL are in same directory.  Extract just the name.
	if(!strcmp(asset_directory, output_directory))
	{
		fs.extract_name(new_path, path);
	}
	else
	{
		strcpy(new_path, path);
	}

	file->tag.set_title("ASSET");
	file->tag.set_property("SRC", new_path);
	file->append_tag();
	file->append_newline();

	file->tag.set_title("FOLDER");
	file->append_tag();
	file->append_text(folder);
	file->tag.set_title("/FOLDER");
	file->append_tag();
	file->append_newline();

// Write the format information
	file->tag.set_title("FORMAT");

	file->tag.set_property("TYPE", 
		File::formattostr(format));
	file->tag.set_property("USE_HEADER", use_header);

	file->append_tag();
	file->append_newline();

// Requiring data to exist caused batch render to lose settings.
// But the only way to know if an asset doesn't have audio or video data 
// is to not write the block.
// So change the block name if the asset doesn't have the data.
	write_audio(file);
	write_video(file);
// index goes after source
	if(index_state->index_status == INDEX_READY && include_index) 
		write_index(file);  

	file->tag.set_title("/ASSET");
	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_audio(FileXML *file)
{
// Let the reader know if the asset has the data by naming the block.
	if(audio_data)
		file->tag.set_title("AUDIO");
	else
		file->tag.set_title("AUDIO_OMIT");
// Necessary for PCM audio
	file->tag.set_property("CHANNELS", channels);
	file->tag.set_property("RATE", sample_rate);
	file->tag.set_property("BITS", bits);
	file->tag.set_property("BYTE_ORDER", byte_order);
	file->tag.set_property("SIGNED", signed_);
	file->tag.set_property("HEADER", header);
	file->tag.set_property("DITHER", dither);
	if(acodec[0])
		file->tag.set_property("ACODEC", acodec);
	
	file->tag.set_property("AUDIO_LENGTH", audio_length);



// Rely on defaults operations for these.

// 	file->tag.set_property("AMPEG_BITRATE", ampeg_bitrate);
// 	file->tag.set_property("AMPEG_DERIVATIVE", ampeg_derivative);
// 
// 	file->tag.set_property("VORBIS_VBR", vorbis_vbr);
// 	file->tag.set_property("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
// 	file->tag.set_property("VORBIS_BITRATE", vorbis_bitrate);
// 	file->tag.set_property("VORBIS_MAX_BITRATE", vorbis_max_bitrate);
// 
// 	file->tag.set_property("MP3_BITRATE", mp3_bitrate);
// 



	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_video(FileXML *file)
{
	if(video_data)
		file->tag.set_title("VIDEO");
	else
		file->tag.set_title("VIDEO_OMIT");
	file->tag.set_property("HEIGHT", height);
	file->tag.set_property("WIDTH", width);
	file->tag.set_property("LAYERS", layers);
	file->tag.set_property("FRAMERATE", frame_rate);
	if(vcodec[0])
		file->tag.set_property("VCODEC", vcodec);

	file->tag.set_property("VIDEO_LENGTH", video_length);
	file->tag.set_property("IS_SPHERE", is_sphere);




	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_index(FileXML *file)
{
	index_state->write_xml(file);
	return 0;
}




const char* Asset::construct_param(const char *param, 
	const char *prefix, 
	char *return_value)
{
	if(prefix)
		sprintf(return_value, "%s%s", prefix, param);
	else
		strcpy(return_value, param);
	return return_value;
}

#define UPDATE_DEFAULT(x, y) defaults->update(construct_param(x, prefix, string), y);
#define GET_DEFAULT(x, y) defaults->get(construct_param(x, prefix, string), y);

void Asset::load_defaults(BC_Hash *defaults, 
	const char *prefix, 
	int do_format,
	int do_compression,
	int do_path,
	int do_data_types,
	int do_bits)
{
	char string[BCTEXTLEN];

// Can't save codec here because it's specific to render, record, and effect.
// The codec has to be UNKNOWN for file probing to work.

	if(do_path)
	{
		GET_DEFAULT("PATH", path);
	}

	if(do_compression)
	{
		GET_DEFAULT("AUDIO_CODEC", acodec);
		GET_DEFAULT("VIDEO_CODEC", vcodec);
	}

	if(do_format)
	{
		format = GET_DEFAULT("FORMAT", format);
		use_header = GET_DEFAULT("USE_HEADER", use_header);
	}

	if(do_data_types)
	{
		audio_data = GET_DEFAULT("AUDIO", 1);
		video_data = GET_DEFAULT("VIDEO", 1);
	}

	if(do_bits)
	{
		bits = GET_DEFAULT("BITS", 16);
		dither = GET_DEFAULT("DITHER", 0);
		signed_ = GET_DEFAULT("SIGNED", 1);
		byte_order = GET_DEFAULT("BYTE_ORDER", 1);



// Used by filefork
		channels = GET_DEFAULT("CHANNELS", 2);
		if(!sample_rate) sample_rate = GET_DEFAULT("RATE", 48000);
		header = GET_DEFAULT("HEADER", 0);
		audio_length = GET_DEFAULT("AUDIO_LENGTH", (int64_t)0);



		height = GET_DEFAULT("HEIGHT", height);
		width = GET_DEFAULT("WIDTH", width);
		layers = GET_DEFAULT("LAYERS", layers);
		if(EQUIV(frame_rate, 0)) frame_rate = GET_DEFAULT("FRAMERATE", frame_rate);
		video_length = GET_DEFAULT("VIDEO_LENGTH", (int64_t)0);
	}

	ampeg_bitrate = GET_DEFAULT("AMPEG_BITRATE", ampeg_bitrate);
	ampeg_derivative = GET_DEFAULT("AMPEG_DERIVATIVE", ampeg_derivative);

	vorbis_vbr = GET_DEFAULT("VORBIS_VBR", vorbis_vbr);
	vorbis_min_bitrate = GET_DEFAULT("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
	vorbis_bitrate = GET_DEFAULT("VORBIS_BITRATE", vorbis_bitrate);
	vorbis_max_bitrate = GET_DEFAULT("VORBIS_MAX_BITRATE", vorbis_max_bitrate);

	theora_fix_bitrate = GET_DEFAULT("THEORA_FIX_BITRATE", theora_fix_bitrate);
	theora_bitrate = GET_DEFAULT("THEORA_BITRATE", theora_bitrate);
	theora_quality = GET_DEFAULT("THEORA_QUALITY", theora_quality);
	theora_sharpness = GET_DEFAULT("THEORA_SHARPNESS", theora_sharpness);
	theora_keyframe_frequency = GET_DEFAULT("THEORA_KEYFRAME_FREQUENCY", theora_keyframe_frequency);
	theora_keyframe_force_frequency = GET_DEFAULT("THEORA_FORCE_KEYFRAME_FEQUENCY", theora_keyframe_force_frequency);



	mp3_bitrate = GET_DEFAULT("MP3_BITRATE", mp3_bitrate);
	mp4a_bitrate = GET_DEFAULT("MP4A_BITRATE", mp4a_bitrate);
	mp4a_quantqual = GET_DEFAULT("MP4A_QUANTQUAL", mp4a_quantqual);

	jpeg_quality = GET_DEFAULT("JPEG_QUALITY", jpeg_quality);
	aspect_ratio = GET_DEFAULT("ASPECT_RATIO", aspect_ratio);

// MPEG format information
	vmpeg_iframe_distance = GET_DEFAULT("VMPEG_IFRAME_DISTANCE", vmpeg_iframe_distance);
	vmpeg_pframe_distance = GET_DEFAULT("VMPEG_PFRAME_DISTANCE", vmpeg_pframe_distance);
	vmpeg_progressive = GET_DEFAULT("VMPEG_PROGRESSIVE", vmpeg_progressive);
	vmpeg_denoise = GET_DEFAULT("VMPEG_DENOISE", vmpeg_denoise);
	vmpeg_bitrate = GET_DEFAULT("VMPEG_BITRATE", vmpeg_bitrate);
	vmpeg_derivative = GET_DEFAULT("VMPEG_DERIVATIVE", vmpeg_derivative);
	vmpeg_quantization = GET_DEFAULT("VMPEG_QUANTIZATION", vmpeg_quantization);
	vmpeg_cmodel = GET_DEFAULT("VMPEG_CMODEL", vmpeg_cmodel);
	vmpeg_fix_bitrate = GET_DEFAULT("VMPEG_FIX_BITRATE", vmpeg_fix_bitrate);
	vmpeg_seq_codes = GET_DEFAULT("VMPEG_SEQ_CODES", vmpeg_seq_codes);
	vmpeg_preset = GET_DEFAULT("VMPEG_PRESET", vmpeg_preset);
	vmpeg_field_order = GET_DEFAULT("VMPEG_FIELD_ORDER", vmpeg_field_order);

	h264_bitrate = GET_DEFAULT("H264_BITRATE", h264_bitrate);
	h264_quantizer = GET_DEFAULT("H264_QUANTIZER", h264_quantizer);
	h264_fix_bitrate = GET_DEFAULT("H264_FIX_BITRATE", h264_fix_bitrate);


	divx_bitrate = GET_DEFAULT("DIVX_BITRATE", divx_bitrate);
	divx_rc_period = GET_DEFAULT("DIVX_RC_PERIOD", divx_rc_period);
	divx_rc_reaction_ratio = GET_DEFAULT("DIVX_RC_REACTION_RATIO", divx_rc_reaction_ratio);
	divx_rc_reaction_period = GET_DEFAULT("DIVX_RC_REACTION_PERIOD", divx_rc_reaction_period);
	divx_max_key_interval = GET_DEFAULT("DIVX_MAX_KEY_INTERVAL", divx_max_key_interval);
	divx_max_quantizer = GET_DEFAULT("DIVX_MAX_QUANTIZER", divx_max_quantizer);
	divx_min_quantizer = GET_DEFAULT("DIVX_MIN_QUANTIZER", divx_min_quantizer);
	divx_quantizer = GET_DEFAULT("DIVX_QUANTIZER", divx_quantizer);
	divx_quality = GET_DEFAULT("DIVX_QUALITY", divx_quality);
	divx_fix_bitrate = GET_DEFAULT("DIVX_FIX_BITRATE", divx_fix_bitrate);
	divx_use_deblocking = GET_DEFAULT("DIVX_USE_DEBLOCKING", divx_use_deblocking);

	ms_bitrate = GET_DEFAULT("MS_BITRATE", ms_bitrate);
	ms_bitrate_tolerance = GET_DEFAULT("MS_BITRATE_TOLERANCE", ms_bitrate_tolerance);
	ms_interlaced = GET_DEFAULT("MS_INTERLACED", ms_interlaced);
	ms_quantization = GET_DEFAULT("MS_QUANTIZATION", ms_quantization);
	ms_gop_size = GET_DEFAULT("MS_GOP_SIZE", ms_gop_size);
	ms_fix_bitrate = GET_DEFAULT("MS_FIX_BITRATE", ms_fix_bitrate);

	ac3_bitrate = GET_DEFAULT("AC3_BITRATE", ac3_bitrate);

	png_use_alpha = GET_DEFAULT("PNG_USE_ALPHA", png_use_alpha);
	exr_use_alpha = GET_DEFAULT("EXR_USE_ALPHA", exr_use_alpha);
	exr_compression = GET_DEFAULT("EXR_COMPRESSION", exr_compression);
	tiff_cmodel = GET_DEFAULT("TIFF_CMODEL", tiff_cmodel);
	tiff_compression = GET_DEFAULT("TIFF_COMPRESSION", tiff_compression);

	is_sphere = GET_DEFAULT("IS_SPHERE", is_sphere);
	boundaries();
}

void Asset::save_defaults(BC_Hash *defaults, 
	const char *prefix,
	int do_format,
	int do_compression,
	int do_path,
	int do_data_types,
	int do_bits)
{
	char string[BCTEXTLEN];

	UPDATE_DEFAULT("PATH", path);




	if(do_format)
	{
		UPDATE_DEFAULT("FORMAT", format);
		UPDATE_DEFAULT("USE_HEADER", use_header);
	}

	if(do_data_types)
	{
		UPDATE_DEFAULT("AUDIO", audio_data);
		UPDATE_DEFAULT("VIDEO", video_data);
	}

	if(do_compression)
	{
		UPDATE_DEFAULT("AUDIO_CODEC", acodec);
		UPDATE_DEFAULT("VIDEO_CODEC", vcodec);

		UPDATE_DEFAULT("AMPEG_BITRATE", ampeg_bitrate);
		UPDATE_DEFAULT("AMPEG_DERIVATIVE", ampeg_derivative);

		UPDATE_DEFAULT("VORBIS_VBR", vorbis_vbr);
		UPDATE_DEFAULT("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
		UPDATE_DEFAULT("VORBIS_BITRATE", vorbis_bitrate);
		UPDATE_DEFAULT("VORBIS_MAX_BITRATE", vorbis_max_bitrate);


		UPDATE_DEFAULT("THEORA_FIX_BITRATE", theora_fix_bitrate);
		UPDATE_DEFAULT("THEORA_BITRATE", theora_bitrate);
		UPDATE_DEFAULT("THEORA_QUALITY", theora_quality);
		UPDATE_DEFAULT("THEORA_SHARPNESS", theora_sharpness);
		UPDATE_DEFAULT("THEORA_KEYFRAME_FREQUENCY", theora_keyframe_frequency);
		UPDATE_DEFAULT("THEORA_FORCE_KEYFRAME_FEQUENCY", theora_keyframe_force_frequency);



		UPDATE_DEFAULT("MP3_BITRATE", mp3_bitrate);
		UPDATE_DEFAULT("MP4A_BITRATE", mp4a_bitrate);
		UPDATE_DEFAULT("MP4A_QUANTQUAL", mp4a_quantqual);





		UPDATE_DEFAULT("JPEG_QUALITY", jpeg_quality);
		UPDATE_DEFAULT("ASPECT_RATIO", aspect_ratio);

// MPEG format information
		UPDATE_DEFAULT("VMPEG_IFRAME_DISTANCE", vmpeg_iframe_distance);
		UPDATE_DEFAULT("VMPEG_PFRAME_DISTANCE", vmpeg_pframe_distance);
		UPDATE_DEFAULT("VMPEG_PROGRESSIVE", vmpeg_progressive);
		UPDATE_DEFAULT("VMPEG_DENOISE", vmpeg_denoise);
		UPDATE_DEFAULT("VMPEG_BITRATE", vmpeg_bitrate);
		UPDATE_DEFAULT("VMPEG_DERIVATIVE", vmpeg_derivative);
		UPDATE_DEFAULT("VMPEG_QUANTIZATION", vmpeg_quantization);
		UPDATE_DEFAULT("VMPEG_CMODEL", vmpeg_cmodel);
		UPDATE_DEFAULT("VMPEG_FIX_BITRATE", vmpeg_fix_bitrate);
		UPDATE_DEFAULT("VMPEG_SEQ_CODES", vmpeg_seq_codes);
		UPDATE_DEFAULT("VMPEG_PRESET", vmpeg_preset);
		UPDATE_DEFAULT("VMPEG_FIELD_ORDER", vmpeg_field_order);

		UPDATE_DEFAULT("H264_BITRATE", h264_bitrate);
		UPDATE_DEFAULT("H264_QUANTIZER", h264_quantizer);
		UPDATE_DEFAULT("H264_FIX_BITRATE", h264_fix_bitrate);

		UPDATE_DEFAULT("DIVX_BITRATE", divx_bitrate);
		UPDATE_DEFAULT("DIVX_RC_PERIOD", divx_rc_period);
		UPDATE_DEFAULT("DIVX_RC_REACTION_RATIO", divx_rc_reaction_ratio);
		UPDATE_DEFAULT("DIVX_RC_REACTION_PERIOD", divx_rc_reaction_period);
		UPDATE_DEFAULT("DIVX_MAX_KEY_INTERVAL", divx_max_key_interval);
		UPDATE_DEFAULT("DIVX_MAX_QUANTIZER", divx_max_quantizer);
		UPDATE_DEFAULT("DIVX_MIN_QUANTIZER", divx_min_quantizer);
		UPDATE_DEFAULT("DIVX_QUANTIZER", divx_quantizer);
		UPDATE_DEFAULT("DIVX_QUALITY", divx_quality);
		UPDATE_DEFAULT("DIVX_FIX_BITRATE", divx_fix_bitrate);
		UPDATE_DEFAULT("DIVX_USE_DEBLOCKING", divx_use_deblocking);


		UPDATE_DEFAULT("MS_BITRATE", ms_bitrate);
		UPDATE_DEFAULT("MS_BITRATE_TOLERANCE", ms_bitrate_tolerance);
		UPDATE_DEFAULT("MS_INTERLACED", ms_interlaced);
		UPDATE_DEFAULT("MS_QUANTIZATION", ms_quantization);
		UPDATE_DEFAULT("MS_GOP_SIZE", ms_gop_size);
		UPDATE_DEFAULT("MS_FIX_BITRATE", ms_fix_bitrate);

		UPDATE_DEFAULT("AC3_BITRATE", ac3_bitrate);


		UPDATE_DEFAULT("PNG_USE_ALPHA", png_use_alpha);
		UPDATE_DEFAULT("EXR_USE_ALPHA", exr_use_alpha);
		UPDATE_DEFAULT("EXR_COMPRESSION", exr_compression);
		UPDATE_DEFAULT("TIFF_CMODEL", tiff_cmodel);
		UPDATE_DEFAULT("TIFF_COMPRESSION", tiff_compression);



		UPDATE_DEFAULT("IS_SPHERE", is_sphere);
	}




	if(do_bits)
	{
		UPDATE_DEFAULT("BITS", bits);
		UPDATE_DEFAULT("DITHER", dither);
		UPDATE_DEFAULT("SIGNED", signed_);
		UPDATE_DEFAULT("BYTE_ORDER", byte_order);






// Used by filefork
		UPDATE_DEFAULT("CHANNELS", channels);
		UPDATE_DEFAULT("RATE", sample_rate);
		UPDATE_DEFAULT("HEADER", header);
		UPDATE_DEFAULT("AUDIO_LENGTH", audio_length);



		UPDATE_DEFAULT("HEIGHT", height);
		UPDATE_DEFAULT("WIDTH", width);
		UPDATE_DEFAULT("LAYERS", layers);
		UPDATE_DEFAULT("FRAMERATE", frame_rate);
		UPDATE_DEFAULT("VIDEO_LENGTH", video_length);

	}
}









int Asset::update_path(const char *new_path)
{
	strcpy(path, new_path);
	return 0;
}

void Asset::update_index(Asset *asset)
{
	index_state->copy_from(asset->index_state);
}


int Asset::dump()
{
	printf("  asset::dump\n");
	printf("   this=%p path=%s\n", this, path);
	printf("   index_status %d\n", index_state->index_status);
	printf("   format %d\n", format);
	printf("   audio_data %d channels %d samplerate %d bits %d byte_order %d signed %d header %d dither %d acodec %c%c%c%c\n",
		audio_data, channels, sample_rate, bits, byte_order, signed_, header, dither, acodec[0], acodec[1], acodec[2], acodec[3]);
	printf("   audio_length %lld\n", (long long)audio_length);
	printf("   video_data %d layers %d framerate %f width %d height %d vcodec %c%c%c%c aspect_ratio %f\n",
		video_data, layers, frame_rate, width, height, vcodec[0], vcodec[1], vcodec[2], vcodec[3], aspect_ratio);
	printf("   video_length %lld \n", (long long)video_length);
	printf("   ms_bitrate_tolerance=%d\n", ms_bitrate_tolerance);
	printf("   ms_quantization=%d\n", ms_quantization);
	printf("   ms_fix_bitrate=%d\n", ms_fix_bitrate);
	printf("   ms_interlaced=%d\n", ms_interlaced);
	printf("   h264_bitrate=%d\n", h264_bitrate);
	printf("   h264_quantizer=%d\n", h264_quantizer);
	printf("   h264_fix_bitrate=%d\n", h264_fix_bitrate);
	printf("   is_sphere=%d\n", is_sphere);
	return 0;
}


// For Indexable
int Asset::get_audio_channels()
{
	return channels;
}

int Asset::get_sample_rate()
{
	return sample_rate;
}

int64_t Asset::get_audio_samples()
{
	return audio_length;
}

int Asset::have_audio()
{
	return audio_data;
}

int Asset::have_video()
{
	return video_data;
}

int Asset::get_w()
{
	return width;
}

int Asset::get_h()
{
	return height;
}

double Asset::get_frame_rate()
{
	return frame_rate;
}


int Asset::get_video_layers()
{
	return layers;
}

int64_t Asset::get_video_frames()
{
	return video_length;
}
