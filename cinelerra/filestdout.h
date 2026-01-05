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
#include "filebase.h"
#include "file.inc"
#include <string>

// Command line encoder
class StdoutBaseConfig;
class StdoutAudioConfig;
class StdoutVideoConfig;
class StdoutText;
class StdoutPreset;


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
    static StdoutPreset* default_audio_presets[2];
    static StdoutPreset* default_video_presets[6];
    static StdoutPreset* default_mplex_presets[3];



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




class StdoutPresetsList : public BC_ListBox
{
public:
	StdoutPresetsList(StdoutBaseConfig *gui,
		int x,
		int y,
		int w, 
		int h);
	int selection_changed();
	int handle_event();
    StdoutBaseConfig *gui;
};

// Delete the highlighted preset
class StdoutDelete : public BC_GenericButton
{
public:
	StdoutDelete(StdoutBaseConfig *gui, int x, int y);
	int handle_event();
    StdoutBaseConfig *gui;
};

// Copy the highlighted preset to the current command text
class StdoutApply : public BC_GenericButton
{
public:
	StdoutApply(StdoutBaseConfig *gui, int x, int y);
	int handle_event();
    StdoutBaseConfig *gui;
};

// Save the current command to a new or existing preset
class StdoutSave : public BC_GenericButton
{
public:
	StdoutSave(StdoutBaseConfig *gui, int x, int y);
	int handle_event();
    StdoutBaseConfig *gui;
};

// Name or contents of a command
class StdoutText : public BC_TextBox
{
public:
    StdoutText(std::string *output,
        int x, 
		int y,
        int w,
        int rows);
	int handle_event();
    std::string *output;
};

class StdoutPreset
{
public:
    StdoutPreset();
    StdoutPreset(const char *title, 
        const char *command);
    StdoutPreset(const char *title, 
        const char *command, 
        int color_model);
    StdoutPreset(const char *title, 
        const char *command, 
        int bits, 
        int byte_order, 
        int signed_, 
        int dither);

    static StdoutPreset* createMplex(const char *title, 
        const char *command,
        int delete_temps);

    void reset();
    std::string command;
    std::string title;

    int delete_temps;

    int color_model;

    int bits;
    int byte_order;
    int signed_;
    int dither;
};


class ConfirmPreset : public BC_Window
{
public:
	ConfirmPreset(StdoutBaseConfig *gui);
	void create_objects(const char *text);
};

class StdoutBaseConfig : public BC_Window
{
public:
    StdoutBaseConfig(BC_WindowBase *parent_window, 
        Asset *asset, 
        const char *title,
        int option_type);
    virtual ~StdoutBaseConfig();

    void load_defaults();
    void save_defaults();

    const char* get_option_text();
    std::string* get_command_text();
    std::string* get_preset_title();
	void create_objects();
    virtual void create_objects2(int x, int y);
	int close_event();
    int resize_event(int w, int h);
    void save_preset();
    void delete_preset();
    void load_preset();
// update the widgets with the current asset values
    virtual void update();
    int get_preset(const char *title);
    int get_preset(std::string *title);

// Options which are saved to a defaults file
	ArrayList<BC_ListBoxItem*> *preset_names;
    ArrayList<StdoutPreset*> *preset_data;
//    std::string preset_title;

    StdoutText *command_title;
    StdoutText *command;
    StdoutDelete *delete_;
    StdoutSave *save;
    StdoutApply *apply;
    StdoutPresetsList *list;
    BC_Hash *defaults;
    BC_Bar *bar;
	BC_WindowBase *parent_window;
	Asset *asset;
    int option_type;
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

class StdoutAudioConfig : public StdoutBaseConfig
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

class StdoutVideoConfig : public StdoutBaseConfig
{
public:
	StdoutVideoConfig(BC_WindowBase *parent_window, Asset *asset);
	~StdoutVideoConfig();

	void create_objects2(int x, int y);
    void update();

    StdoutColormodel *cmodel;
	ArrayList<BC_ListBoxItem*> cmodels;
};

class StdoutMplexConfig : public StdoutBaseConfig
{
public:
	StdoutMplexConfig(BC_WindowBase *parent_window, Asset *asset);
	~StdoutMplexConfig();

	void create_objects2(int x, int y);
    BC_CheckBox *delete_temps;
};




#endif


