/*
 * CINELERRA
 * Copyright (C) 2011-2022 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILESTDOUT_H
#define FILESTDOUT_H

#include "asset.inc" 
#include "bitspopup.inc"
#include "commandtools.h"
#include "filebase.h"
#include "file.inc"
#include <string>

// Command line encoder
class StdoutAudioConfig;
class StdoutVideoConfig;


class FileStdout : public FileBase
{
public:
    FileStdout(Asset *asset, File *file);
    ~FileStdout();
    
// table functions
    FileStdout();
    FileBase* create(File *file);
	void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int option_type,
	    const char *locked_compressor);
	int get_best_colormodel(Asset *asset, 
        VideoInConfig *in_config, 
        VideoOutConfig *out_config);
    const char* formattostr(int format);
    void fix_command(std::string *dst, 
        std::string *src, 
        int is_audio, 
        int is_video, 
        int is_mplex);


	int write_frames(VFrame ***frames, int len);
	int write_samples(double **buffer, 
			int64_t len);

    int reset_parameters_derived();
    int open_file(int rd, int wr);
	int close_file_derived();

// default values
    static CommandPreset* default_audio_presets[2];
    static CommandPreset* default_video_presets[6];
    static CommandPreset* default_mplex_presets[3];



// temporary output files
    std::string temp_audio_path;
    std::string *temp_video_path;
    int current_video;

// multiple video layers supported
    FILE **video_fd;
    FILE *audio_fd;

// temporary interleaved audio buffer
	uint8_t *temp_samples;
// bytes allocated
	int temp_allocated;
// don't wrap
    int failed;
};




class StdoutAudioHILO : public BC_Radial
{
public:
	StdoutAudioHILO(StdoutAudioConfig *gui, int x, int y);
	int handle_event();
	StdoutAudioConfig *gui;
};

class StdoutAudioLOHI : public BC_Radial
{
public:
	StdoutAudioLOHI(StdoutAudioConfig *gui, int x, int y);
	int handle_event();
	StdoutAudioConfig *gui;
};

class StdoutColormodel : public BC_PopupTextBox
{
public:
	StdoutColormodel(StdoutVideoConfig *gui,
        int *output_value,
		const char *text,
        int x, 
		int y,
        int w,
        int h);
	int handle_event();
	int *output_value;
    StdoutVideoConfig *gui;
};

class StdoutAudioConfig : public CommandTools
{
public:
	StdoutAudioConfig(BC_WindowBase *parent_window, Asset *asset);
	~StdoutAudioConfig();

	void create_objects2(int x, int y);
    void update();

	BitsPopup *bits_popup;
	StdoutAudioHILO *hilo;
	StdoutAudioLOHI *lohi;
    BC_CheckBox *dither;
    BC_CheckBox *signed_;
};

class StdoutVideoConfig : public CommandTools
{
public:
	StdoutVideoConfig(BC_WindowBase *parent_window, Asset *asset);
	~StdoutVideoConfig();

	void create_objects2(int x, int y);
    void update();

    StdoutColormodel *cmodel;
	ArrayList<BC_ListBoxItem*> cmodels;
};

class StdoutMplexConfig : public CommandTools
{
public:
	StdoutMplexConfig(BC_WindowBase *parent_window, Asset *asset);
	~StdoutMplexConfig();

	void create_objects2(int x, int y);
    BC_CheckBox *delete_temps;
};




#endif


