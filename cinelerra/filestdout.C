/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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
#include "bitspopup.h"
#include "clip.h"
#include "errorbox.h"
#include "file.h"
#include "filestdout.h"
#include "filesystem.h"
#include "mwindow.h"
#include "theme.h"
#include <string.h>

extern "C"
{
#include <uuid.h>
}

#define MAX_PRESETS 255



StdoutPreset* FileStdout::default_audio_presets[2] = 
{
    new StdoutPreset("ffmpeg AAC",
        "ffmpeg -y -f f32le -ar %r -ac %c -i - -f mp4 -c:a aac -b:a 192k %1",
        BITSFLOAT,
        BYTE_ORDER_LOHI,
        1,
        0),
    new StdoutPreset("null",
        "cat > /dev/null",
        BITSFLOAT,
        BYTE_ORDER_LOHI,
        1,
        0),
};

StdoutPreset* FileStdout::default_video_presets[4] =
{
    new StdoutPreset("ffmpeg HEVC CBR",
        "ffmpeg -y -f rawvideo -pix_fmt yuv420p -r %r -s:v %wx%h -i - -f h264 -c:v hevc -b:v 5M %1",
        BC_YUV420P),
    new StdoutPreset("ffmpeg HEVC VBR",
        "ffmpeg -y -f rawvideo -pix_fmt yuv420p -r %r -s:v %wx%h -i - -f h264 -c:v hevc -qp:v 30 %1",
        BC_YUV420P),
    new StdoutPreset("ffmpeg H.264 VBR",
        "ffmpeg -y -f rawvideo -pix_fmt yuv420p -r %r -s:v %wx%h -i - -f h264 -c:v h264 -qp:v 30 %1",
        BC_YUV420P),
    new StdoutPreset("null",
        "cat > /dev/null",
        BC_YUV420P),
};

StdoutPreset* FileStdout::default_mplex_presets[3] = 
{
    StdoutPreset::createMplex("ffmpeg MP4", "ffmpeg -y -i %3 -i %2 -c:v copy -c:a copy %1", 0),
    StdoutPreset::createMplex("ffmpeg MP4 video", "ffmpeg -y -i %2 -c:v copy %1", 0),
    StdoutPreset::createMplex("ffmpeg MP4 audio", "ffmpeg -y -i %3 -c:a copy %1", 0)
};

// Need planer colormodels not in MWindow::colormodels
static int supported_cmodels[] = 
{
// ffmpeg requires planar 8 bit
    BC_YUV420P,
    BC_YUV422P,
    BC_YUV444P,
    BC_RGB888,
    BC_RGBA8888,
    BC_YUV888,
    BC_YUVA8888,
// this would ideally be what HDR codecs injested
    BC_RGB_FLOAT,
    BC_RGBA_FLOAT
};

FileStdout::FileStdout(Asset *asset, File *file)
 : FileBase(asset, file)
{
    reset_parameters_derived();
}

FileStdout::~FileStdout()
{
}

FileStdout::FileStdout()
 : FileBase()
{
    reset_parameters_derived();
    ids.append(FILE_STDOUT);
    has_audio = 1;
    has_video = 1;
    has_wrapper = 1;
    has_wr = 1;
}

FileBase* FileStdout::create(File *file)
{
    return new FileStdout(file->asset, file);
}


void FileStdout::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int option_type,
    const char *locked_compressor)
{
	if(option_type == AUDIO_PARAMS)
	{
		StdoutAudioConfig *window = new StdoutAudioConfig(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
    else
	if(option_type == VIDEO_PARAMS)
	{
		StdoutVideoConfig *window = new StdoutVideoConfig(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
    else
    if(option_type == MPLEX_PARAMS)
    {
		StdoutMplexConfig *window = new StdoutMplexConfig(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
    }
}

const char* FileStdout::formattostr(int format)
{
    switch(format)
    {
		case FILE_STDOUT:
			return COMMAND_NAME;
			break;
    }
    return 0;
}

int FileStdout::reset_parameters_derived()
{
    video_fd = 0;
    audio_fd = 0;
    temp_video_path = 0;
    temp_samples = 0;
    temp_allocated = 0;
    failed = 0;
    return 0;
}

int FileStdout::open_file(int rd, int wr)
{
	uuid_t id;
	char string[BCTEXTLEN];
    if(wr)
    {
// Use temporary filenames if a wrapper is desired,
// if 2 streams are desired, or 
// if multiple video layers are desired.
        if(asset->do_wrapper ||
            (asset->audio_data && asset->video_data) ||
            (asset->video_data && asset->layers > 1))
        {
            if(asset->audio_data)
            {
	            uuid_generate(id);
                uuid_unparse(id, string);
                temp_audio_path.assign(asset->path);
                temp_audio_path.append(".");
                temp_audio_path.append(string);
            }

            if(asset->video_data)
            {
                temp_video_path = new std::string[asset->layers];
                if(asset->layers > 1)
                {
                    for(int i = 0; i < asset->layers; i++)
                    {
	                    uuid_generate(id);
                        uuid_unparse(id, string);
                        temp_video_path[i].assign(asset->path);
                        temp_video_path[i].append(".");
                        temp_video_path[i].append(string);
                    }
                }
                else
                {
	                uuid_generate(id);
                    uuid_unparse(id, string);
                    temp_video_path[0].assign(asset->path);
                    temp_video_path[0].append(".");
                    temp_video_path[0].append(string);
                }
            }
        }
        else
        {
// Use provided filename
            if(asset->audio_data)
            {
                temp_audio_path.assign(asset->path);
            }
            else
            if(asset->video_data)
            {
                temp_video_path = new std::string[asset->layers];
                temp_video_path[0].assign(asset->path);
            }
        }

        if(asset->audio_data)
        {
            std::string audio_command;
            fix_command(&audio_command, 
                &asset->audio_command, 
                1, 
                0, 
                0);
            printf("FileStdout::open_file %d running %s\n", 
                __LINE__, 
                audio_command.c_str());
            audio_fd = popen(audio_command.c_str(), "w");
            if(!audio_fd)
            {
                printf("FileStdout::open_file %d: audio rendering failed\n", 
                    __LINE__);
                failed = 1;
                return 1;
            }
        }

        if(asset->video_data)
        {
            video_fd = new FILE*[asset->layers];
            current_video = 0;
            for(int i = 0; i < asset->layers; i++)
            {
                std::string video_command;
                fix_command(&video_command, 
                    &asset->video_command, 
                    0, 
                    1, 
                    0);
                printf("FileStdout::open_file %d running %s\n", 
                    __LINE__, 
                    video_command.c_str());
                video_fd[i] = popen(video_command.c_str(), "w");
                if(!video_fd[i])
                {
                    printf("FileStdout::open_file %d: vijeo rendering failed\n", 
                        __LINE__);
                    failed = 1;
                    return 1;
                }
            }
        }
    }
	return 0;
}

int FileStdout::close_file_derived()
{
    int result = 0;
// Close the pipes
    if(video_fd)
    {
        for(int i = 0; i < asset->layers; i++)
        {
            if(video_fd[i]) fclose(video_fd[i]);
        }
        delete [] video_fd;
        video_fd = 0;
    }
    
    if(audio_fd)
    {
        fclose(audio_fd);
        audio_fd = 0;
    }

// Do the wrapper
    if(!failed &&
        asset && 
        asset->do_wrapper)
//          &&
//         asset->audio_data && 
//         asset->video_data)
    {
        current_video = 0;
        std::string mplex_command;
        fix_command(&mplex_command, 
            &asset->wrapper_command, 
            0, 
            0, 
            1);


        printf("FileStdout::close_file_derived %d running %s\n",
            __LINE__,
            mplex_command.c_str());
        result = system(mplex_command.c_str());
        if(result)
        {
            printf("FileStdout::close_file_derived %d: wrapper failed\n", 
                __LINE__);
        }
        else
// delete the temporaries
        if(asset->command_delete_temps)
        {
            if(temp_video_path)
            {
                for(int i = 0; i < asset->layers; i++)
                {
                    remove(temp_video_path[i].c_str());
                }
            }

            if(temp_audio_path.length())
            {
                remove(temp_audio_path.c_str());
            }
        }
    }
    
    if(temp_video_path)
    {
        delete [] temp_video_path;
        temp_video_path = 0;
    }

    if(temp_samples)
    {
        delete [] temp_samples;
    }

	return result;
}

void FileStdout::fix_command(std::string *dst, 
    std::string *src, 
    int is_audio, 
    int is_video, 
    int is_mplex)
{
    dst->clear();
    
    const char *ptr = src->c_str();
    int current_video_input = 0;
    while(*ptr != 0)
    {
        if(*ptr == '%')
        {
            ptr++;
            if(*ptr != 0)
            {
                switch(*ptr)
                {
// %%
                    case '%':
                        dst->push_back('%');
                        break;

// audio source file for wrapper
                    case '3':
                        if(is_mplex)
                        {
                            dst->append(temp_audio_path);
                        }
                        break;

// multiple video source files are supported for the wrapper
                    case '2':
                        if(is_mplex && temp_video_path)
                        {
                            dst->append(temp_video_path[current_video++]);
                        }
                        break;

// output filename
                    case '1':
                        if(is_audio)
                        {
                            dst->append(temp_audio_path);
                        }
                        else
                        if(is_video)
                        {
                            if(temp_video_path)
                            {
                                dst->append(temp_video_path[current_video++]);
                            }
                        }
                        else
                        if(is_mplex)
                        {
                            dst->append(asset->path);
                        }
                        break;

                    case 'r':
                        if(is_audio)
                        {
                            char string[BCTEXTLEN];
                            sprintf(string, "%d", asset->sample_rate);
                            dst->append(string);
                        }
                        else
                        if(is_video)
                        {
                            char string[BCTEXTLEN];
                            sprintf(string, "%f", asset->frame_rate);
                            dst->append(string);
                        }
                        break;

// audio channels
                    case 'c':
                        if(is_audio)
                        {
                            char string[BCTEXTLEN];
                            sprintf(string, "%d", asset->channels);
                            dst->append(string);
                        }
                        break;

// width
                    case 'w':
                        if(is_video)
                        {
                            char string[BCTEXTLEN];
                            sprintf(string, "%d", asset->width);
                            dst->append(string);
                        }
                        break;

// height
                    case 'h':
                        if(is_video)
                        {
                            char string[BCTEXTLEN];
                            sprintf(string, "%d", asset->height);
                            dst->append(string);
                        }
                        break;
                }
                ptr++;
            }
        }
        else
        {
            dst->push_back(*ptr);
            ptr++;
        }
    }
}

int FileStdout::write_frames(VFrame ***frames, int len)
{

	int result = 0;
    for(int i = 0; i < asset->layers && !result; i++)
	{
		for(int j = 0; j < len && !result; j++)
		{
			VFrame *src = frames[i][j];
            if(src->get_color_model() != asset->command_cmodel)
            {
                if(file->temp_frame &&
                    !file->temp_frame->params_match(asset->width, 
                    asset->height, 
                    asset->width,
                    asset->command_cmodel))
                {
                    delete file->temp_frame;
                    file->temp_frame = 0;
                }

                if(!file->temp_frame)
                {
                    file->temp_frame = new VFrame();
                    file->temp_frame->set_use_shm(0);
                    file->temp_frame->reallocate(0, // data
                        -1, // shmid
					    0, // y_offset
					    0, // u_offset
					    0, // v_offset
					    asset->width,
					    asset->height,
					    asset->command_cmodel,
					    -1); // bytes per line
                }
// printf("FileStdout::write_frames %d %d %d %p %p %p %p\n", 
// __LINE__, 
// asset->layers, 
// len,
// file->temp_frame,
// file->temp_frame->get_y(),
// file->temp_frame->get_u(),
// file->temp_frame->get_v());

                cmodel_transfer(file->temp_frame->get_rows(),
                    src->get_rows(),
                    file->temp_frame->get_y(),
                    file->temp_frame->get_u(),
                    file->temp_frame->get_v(),
                    src->get_y(),
                    src->get_u(),
                    src->get_v(),
                    0,
                    0,
                    asset->width,
					asset->height,
                    0,
                    0,
                    asset->width,
					asset->height,
                    src->get_color_model(),
                    asset->command_cmodel,
                    0,
                    asset->width,
                    asset->width);
//PRINT_TRACE

                src = file->temp_frame;
            }
//printf("FileStdout::write_frames %d\n", __LINE__);

            int bytes_written = fwrite(src->get_data(),
                1,
                src->get_data_size(),
                video_fd[i]);
//printf("FileStdout::write_frames %d %d %d\n", __LINE__, bytes_written, src->get_data_size());
            if(bytes_written < src->get_data_size())
            {
                failed = 1;
                result = 1;
            }
        }
    }
//PRINT_TRACE
	return result;
}

int FileStdout::write_samples(double **buffer, 
		int64_t len)
{
    int bytes = asset->channels * 
        len * 
        file->bytes_per_sample(asset->command_bits);
    if(!temp_samples || temp_allocated < bytes)
    {
        if(temp_samples)
        {
            delete [] temp_samples;
            temp_samples = 0;
        }

        if(!temp_samples)
        {
            temp_samples = new uint8_t[bytes];
        }

        temp_allocated = bytes;
    }

    samples_to_raw(temp_samples, 
		buffer,
		len, 
		asset->command_bits, 
		asset->channels,
		asset->command_byte_order,
		asset->command_signed_);

// printf("FileStdout::write_samples %d: len=%ld bits=%d channels=%d bytes_per_sample=%d\n", 
// __LINE__, 
// len, 
// asset->command_bits,
// asset->channels,
// file->bytes_per_sample(asset->command_bits));
// float *temp_f = (float*)temp_samples;
// for(int i = 0; i < len; i++)
// {
//     printf("%d %f %f\n", i, temp_f[i * 2], temp_f[i * 2 + 1]);
// }

    int result = fwrite(temp_samples, 1, bytes, audio_fd);
    if(result != bytes)
    {
        failed = 1;
        return 1;
    }
	return 0;
}

// the user must set the colormodel in the encoding parameters
int FileStdout::get_best_colormodel(Asset *asset, int driver)
{
	return asset->command_cmodel;
}


StdoutPresetsList::StdoutPresetsList(StdoutBaseConfig *gui,
	int x,
	int y,
	int w, 
	int h)
 : BC_ListBox(x, 
	y, 
	w, 
	h,
	LISTBOX_TEXT,
	gui->preset_names)
{
    this->gui = gui;
}

int StdoutPresetsList::selection_changed()
{
    return 1;
}

int StdoutPresetsList::handle_event()
{
    int number = get_selection_number(0, 0);
	if(number >= 0)
    {
        gui->load_preset();
    }
    return 1;
}




#define DELETE_TEXT _("Delete")
#define APPLY_TEXT _("Load")
#define SAVE_TEXT _("Save")
StdoutDelete::StdoutDelete(StdoutBaseConfig *gui, int x, int y)
 : BC_GenericButton(x, y, DELETE_TEXT)
{
    this->gui = gui;
    set_tooltip("Delete the highlighted preset.");
}
int StdoutDelete::handle_event()
{
    gui->delete_preset();
    return 1;
}


StdoutApply::StdoutApply(StdoutBaseConfig *gui, int x, int y)
 : BC_GenericButton(x, y, APPLY_TEXT)
{
    this->gui = gui;
    set_tooltip("Apply the highlighted preset to the command line.");
}
int StdoutApply::handle_event()
{
    gui->load_preset();
    return 1;
}



StdoutSave::StdoutSave(StdoutBaseConfig *gui, int x, int y)
 : BC_GenericButton(x, y, SAVE_TEXT)
{
    this->gui = gui;
    set_tooltip("Save the command & title as a preset.");
}
int StdoutSave::handle_event()
{
    gui->save_preset();
    return 1;
}





StdoutText::StdoutText(std::string *output,
    int x, 
	int y,
    int w,
    int rows)
 : BC_TextBox(x, y, w, rows, output->c_str())
{
    this->output = output;
}

int StdoutText::handle_event()
{
    output->assign(get_text());
    return 1;
}


StdoutPreset::StdoutPreset()
{
    reset();
}

StdoutPreset::StdoutPreset(const char *title, const char *command, 
    int color_model)
{
    reset();
    this->title.assign(title);
    this->command.assign(command);
    this->color_model = color_model;
}

StdoutPreset::StdoutPreset(const char *title, const char *command, 
    int bits, 
    int byte_order, 
    int signed_, 
    int dither)
{
    reset();
    this->title.assign(title);
    this->command.assign(command);
    this->bits = bits;
    this->byte_order = byte_order;
    this->signed_ = signed_;
    this->dither = dither;
}

void StdoutPreset::reset()
{
    color_model = BC_YUV420P;
    bits = BITSLINEAR16;
    byte_order = BYTE_ORDER_LOHI;
    signed_ = 1;
    dither = 0;
}

StdoutPreset* StdoutPreset::createMplex(const char *title,
    const char *command,
    int delete_temps)
{
    StdoutPreset *result = new StdoutPreset;
    result->reset();
    result->title.assign(title);
    result->command.assign(command);
    result->delete_temps = delete_temps;
    return result;
}


ConfirmPreset::ConfirmPreset(StdoutBaseConfig *gui)
 : BC_Window(PROGRAM_NAME ": Preset Exists", 
 		gui->get_abs_cursor_x(1) - DP(160), 
		gui->get_abs_cursor_y(1) - DP(120), 
		DP(320), 
		DP(150))
{
}

void ConfirmPreset::create_objects(const char *text)
{
    int margin = MWindow::theme->widget_border;
	int x = margin, y = margin;
	lock_window("ConfirmPreset::create_objects");
    
    int text_w = get_text_width(MEDIUMFONT, text);
    int new_w = x + text_w + margin;

// limit to a certain size
	if(new_w > get_root_w() / 2) 
    {
        new_w = get_root_w() / 2;
    }

	if(new_w > get_w())
	{
		resize_window(new_w, get_h());
	}

	add_subwindow(new BC_Title(x, 
		y, 
		text));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);
	unlock_window();
}


StdoutBaseConfig::StdoutBaseConfig(BC_WindowBase *parent_window, 
    Asset *asset, 
    const char *window_title,
    int option_type)
 : BC_Window(window_title,
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	MWindow::theme->command_w,
	MWindow::theme->command_h)
{
//printf("StdoutBaseConfig::StdoutBaseConfig %d\n", __LINE__);
	this->parent_window = parent_window;
	this->asset = asset;
    this->option_type = option_type;
    load_defaults();
}

StdoutBaseConfig::~StdoutBaseConfig()
{
    save_defaults();
    delete defaults;

	preset_names->remove_all_objects();
    delete preset_names;
	preset_data->remove_all_objects();
    delete preset_data;
}

void StdoutBaseConfig::load_defaults()
{
	char string[BCTEXTLEN];
    switch(option_type)
    {
        case AUDIO_PARAMS:
            sprintf(string, "%saudio_commandlines", BCASTDIR);
            break;

        case VIDEO_PARAMS:
            sprintf(string, "%svideo_commandlines", BCASTDIR);
            break;

        case MPLEX_PARAMS:
            sprintf(string, "%smplex_commandlines", BCASTDIR);
            break;
    }
	FileSystem fs;
    fs.complete_path(string);
    defaults = new BC_Hash(string);
    defaults->load();

    preset_names = new ArrayList<BC_ListBoxItem*>;
    preset_data = new ArrayList<StdoutPreset*>;

// load the presets
	std::string title;
    const char *option_text = get_option_text();

    for(int i = 0; i < MAX_PRESETS; i++)
    {
        sprintf(string, "%sPRESET_TITLE%d", option_text, i);
        title.erase();
        defaults->get(string, &title);
        if(!title.size())
        {
            break;
        }

        StdoutPreset *preset = new StdoutPreset;

        sprintf(string, "%sPRESET_TEXT%d", option_text, i);
        defaults->get(string, &preset->command);
        sprintf(string, "%sPRESET_COLOR_MODEL%d", option_text, i);
        preset->color_model = defaults->get(string, preset->color_model);
        sprintf(string, "%sPRESET_BITS%d", option_text, i);
        preset->bits = defaults->get(string, preset->bits);
        sprintf(string, "%sPRESET_BYTE_ORDER%d", option_text, i);
        preset->byte_order = defaults->get(string, preset->byte_order);
        sprintf(string, "%sPRESET_SIGNED%d", option_text, i);
        preset->signed_ = defaults->get(string, preset->signed_);
        sprintf(string, "%sPRESET_DITHER%d", option_text, i);
        preset->dither = defaults->get(string, preset->dither);

        preset_names->append(new BC_ListBoxItem(title.c_str()));
        preset_data->append(preset);
    }

// the contents of the preset title textbox
    sprintf(string, "%sPRESET_TITLE", option_text);
    defaults->get(string, &preset_title);

// the current command line comes from the asset
}

void StdoutBaseConfig::save_defaults()
{
    defaults->clear();

    const char *option_text = get_option_text();
	char string[BCTEXTLEN];
    for(int i = 0; i < preset_names->size() && i < preset_data->size(); i++)
    {
        StdoutPreset *preset = preset_data->get(i);
        sprintf(string, "%sPRESET_TITLE%d", option_text, i);
        defaults->update(string, preset_names->get(i)->get_text());

        sprintf(string, "%sPRESET_TEXT%d", option_text, i);
        defaults->update(string, &preset->command);
        sprintf(string, "%sPRESET_COLOR_MODEL%d", option_text, i);
        defaults->update(string, preset->color_model);
        sprintf(string, "%sPRESET_BITS%d", option_text, i);
        defaults->update(string, preset->bits);
        sprintf(string, "%sPRESET_BYTE_ORDER%d", option_text, i);
        defaults->update(string, preset->byte_order);
        sprintf(string, "%sPRESET_SIGNED%d", option_text, i);
        defaults->update(string, preset->signed_);
        sprintf(string, "%sPRESET_DITHER%d", option_text, i);
        defaults->update(string, preset->dither);
    }

// save the current preset textbox
    sprintf(string, "%sPRESET_TITLE", option_text);
    defaults->update(string, &preset_title);

// command line comes from the asset

	defaults->save();
}


const char* StdoutBaseConfig::get_option_text()
{
    switch(option_type)
    {
        case AUDIO_PARAMS: return "AUDIO_";
        case VIDEO_PARAMS: return "VIDEO_";
        case MPLEX_PARAMS: return "MPLEX_";
    }
    return "";
}

std::string* StdoutBaseConfig::get_command_text()
{
    switch(option_type)
    {
        case AUDIO_PARAMS: return &asset->audio_command;
        case VIDEO_PARAMS: return &asset->video_command;
        case MPLEX_PARAMS: return &asset->wrapper_command;
    }
    return 0;
}

void StdoutBaseConfig::create_objects()
{
    BC_Title *title;
    int margin = MWindow::theme->widget_border;
    int x = margin, y = margin;

	lock_window("StdoutBaseConfig::create_objects");

    int button_w = 0;
    button_w = MAX(button_w, BC_GenericButton::calculate_w(this, DELETE_TEXT));
    button_w = MAX(button_w, BC_GenericButton::calculate_w(this, APPLY_TEXT));
    button_w = MAX(button_w, BC_GenericButton::calculate_w(this, SAVE_TEXT));
    int x2 = get_w() - margin - button_w;

    add_tool(title = new BC_Title(x, y, _("Presets:")));
	y += title->get_h() + margin;
    
    add_tool(list = new StdoutPresetsList(this,
	    x,
	    y,
	    x2 - x - margin, 
	    DP(100)));
    int y2 = y + list->get_h() + margin;
    add_tool(delete_ = new StdoutDelete(this, x2, y));
    y += delete_->get_h() + margin;
    add_tool(save = new StdoutSave(this, x2, y));
    y += save->get_h() + margin;
    add_tool(apply = new StdoutApply(this, x2, y));
    y += apply->get_h() + margin;

    y = y2;
    add_tool(title = new BC_Title(x, y, _("Preset title:")));
	y += title->get_h() + margin;

    add_subwindow(command_title = new StdoutText(&preset_title,
        x, 
	    y,
        get_w() - x - margin,
        1));
    y += command_title->get_h() + margin;

    add_tool(title = new BC_Title(x, y, _("Command line:")));
	y += title->get_h() + margin;

    std::string *command_text = get_command_text();
    add_subwindow(command = new StdoutText(command_text,
        x, 
	    y,
        get_w() - x - margin,
        1));
    y += command->get_h() + margin;

    if(option_type == MPLEX_PARAMS)
    {
        add_tool(title = new BC_Title(x, y, "%3 becomes the audio filename.\n"
            "%2 becomes the video filename.\n"
            "%1 becomes the output filename."));
	}
    else
    if(option_type == AUDIO_PARAMS)
    {
        add_tool(title = new BC_Title(x, y, 
            "%r becomes the sample rate\n"
            "%c becomes the channels\n"
            "%1 becomes the output filename"));
    }
    else
    if(option_type == VIDEO_PARAMS)
    {
        add_tool(title = new BC_Title(x, y, 
            "%r becomes the frame rate\n"
            "%w becomes the width\n"
            "%h becomes the height\n"
            "%1 becomes the output filename"));
    }

    y += title->get_h() + margin;

    
    add_tool(bar = new BC_Bar(x, y, get_w() - margin - x));
    y += margin + bar->get_h();
    

    create_objects2(x, y);

    BC_OKButton *button;
	add_subwindow(button = new BC_OKButton(this));
    button->set_esc(1);
	show_window(1);
	unlock_window();
}

void StdoutBaseConfig::create_objects2(int x, int y)
{
    
}

int StdoutBaseConfig::close_event()
{
	set_done(0);
	return 1;
}

int StdoutBaseConfig::resize_event(int w, int h)
{
    int margin = MWindow::theme->widget_border;
    int x = margin, y = margin;

    command_title->reposition_window(command_title->get_x(),
		command_title->get_y(),
		w - command_title->get_x() - margin);
    command->reposition_window(command->get_x(),
		command->get_y(),
		w - command->get_x() - margin);
    bar->reposition_window(bar->get_x(), 
        bar->get_y(), 
        w - bar->get_x() - margin);
    return 0;
}

void StdoutBaseConfig::save_preset()
{
// ignore if no title
	if(command_title->get_text()[0])
    {
// replace existing preset
        int got_it = 0;
        StdoutPreset *dst = 0;
        for(int i = 0; 
            i < preset_names->size() && i < preset_data->size(); 
            i++)
        {
// printf("StdoutBaseConfig::save_preset %d %s %s %d\n", 
// __LINE__, 
// preset_names->get(i)->get_text(),
// command_title->get_text(),
// strcmp(preset_names->get(i)->get_text(),
//                command_title->get_text()));
            if(!strcmp(preset_names->get(i)->get_text(),
                command_title->get_text()))
            {
                dst = preset_data->get(i);
                got_it = 1;
                break;
            }
        }

// confirm replace
        int result = 0;
        if(got_it)
        {
            char string[BCTEXTLEN];
            sprintf(string, "Overwrite '%s'?", command_title->get_text());
            ConfirmPreset confirm(this);
            confirm.create_objects(string);
            result = confirm.run_window();
        }

// create a new preset
        if(!got_it)
        {
            preset_names->append(new BC_ListBoxItem(command_title->get_text()));
            dst = new StdoutPreset;
            preset_data->append(dst);
        }

        if(!result)
        {
            dst->command.assign(command->get_text());
            dst->color_model = asset->command_cmodel;
            dst->bits = asset->command_bits;
            dst->byte_order = asset->command_byte_order;
            dst->signed_ = asset->command_signed_;
            dst->dither = asset->command_dither;
            save_defaults();

            list->update(preset_names,
		        0,
		        0,
		        1);
        }
    }
    else
    {
		ErrorBox error(PROGRAM_NAME ": Error",
			get_abs_cursor_x(1),
			get_abs_cursor_y(1));
		error.create_objects("Need a title to save the preset");
		error.raise_window();
		error.run_window();
    }

}

void StdoutBaseConfig::delete_preset()
{
// ignore if no selection
    int number = list->get_selection_number(0, 0);
	if(number >= 0 && 
        number < preset_names->size() && 
        number < preset_data->size())
    {
        int result = 0;
        char string[BCTEXTLEN];
        sprintf(string, "Delete '%s'?", preset_names->get(number)->get_text());
        ConfirmPreset confirm(this);
        confirm.create_objects(string);
        result = confirm.run_window();


        if(!result)
        {
            preset_names->remove_object_number(number);
            preset_data->remove_object_number(number);
            list->update(preset_names,
		        0, // column_titles
		        0, // column_widths
		        1, // columns
                0, // xposition
                0, // yposition
                -1, // highlighted_number
                1); // recalc_positions
            save_defaults();
        }
    }
}


void StdoutBaseConfig::load_preset()
{
// ignore if nothing selected
    int number = list->get_selection_number(0, 0);
	if(number >= 0)
    {
        StdoutPreset *src = preset_data->get(number);
        command->update(src->command.c_str());
        preset_title.assign(preset_names->get(number)->get_text());
        command_title->update(preset_names->get(number)->get_text());

// copy only the parameters for the option_type so a video preset doesn't
// overwrite the audio settings
        std::string *command_text = get_command_text();
        command_text->assign(src->command);

        if(option_type == VIDEO_PARAMS)
            asset->command_cmodel = src->color_model;
        if(option_type == AUDIO_PARAMS)
        {
            asset->command_bits = src->bits;
            asset->command_byte_order = src->byte_order;
            asset->command_signed_ = src->signed_;
            asset->command_dither = src->dither;
        }

        update();
        save_defaults();
    }
}

void StdoutBaseConfig::update()
{
}

int StdoutBaseConfig::get_preset(const char *title)
{
    for(int i = 0; i < preset_names->size() && i < preset_data->size(); i++)
    {
        if(!strcmp(preset_names->get(i)->get_text(),
            title))
        {
            return i;
        }
    }
    
    return -1;
}

int StdoutBaseConfig::get_preset(std::string *title)
{
    return get_preset(title->c_str());
}



StdoutAudioConfig::StdoutAudioConfig(BC_WindowBase *parent_window, Asset *asset)
 : StdoutBaseConfig(parent_window,
    asset,
    PROGRAM_NAME ": Audio Compression",
    AUDIO_PARAMS)
{
//printf("StdoutAudioConfig::StdoutAudioConfig %d\n", __LINE__);
// seed it with defaults
    for(int i = 0; i < sizeof(FileStdout::default_audio_presets) / sizeof(StdoutPreset*); i++)
    {
        StdoutPreset *preset = FileStdout::default_audio_presets[i];
        if(get_preset(&preset->title) < 0)
        {
            preset_names->append(new BC_ListBoxItem(preset->title.c_str()));
            preset_data->append(new StdoutPreset(*preset));
        }
    }
}

StdoutAudioConfig::~StdoutAudioConfig()
{
	if(bits_popup)
	{
		delete bits_popup;
	}
}

void StdoutAudioConfig::create_objects2(int x, int y)
{
    BC_Title *title;
    BC_CheckBox *box;
    int margin = MWindow::theme->widget_border;


	add_tool(title = new BC_Title(x, y, _("Sample format to write to stdin:")));
	y += title->get_h() + margin;
	bits_popup = new BitsPopup(this, 
        x, 
        y, 
        &asset->command_bits, 
        0, // IMA4
        0, // ulaw
        0, // ADPCM
        1, // float
        0, // 32 linear
        1); // 8 linear
	bits_popup->create_objects();
	y += bits_popup->get_h() + margin;

	x = margin;
	add_subwindow(dither = new BC_CheckBox(x, y, &asset->command_dither, _("Dither")));
	y += dither->get_h() + margin;

	add_subwindow(signed_ = new BC_CheckBox(x, y, &asset->command_signed_, _("Signed")));
	y += signed_->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Byte order:")));
    x += title->get_w() + margin;
	add_subwindow(hilo = new StdoutAudioHILO(this, x, y));
	x += hilo->get_w() + margin;
    add_subwindow(lohi = new StdoutAudioLOHI(this, x, y));
}

void StdoutAudioConfig::update()
{
    bits_popup->update(File::bitstostr(asset->command_bits));
    dither->update(asset->command_dither);
    signed_->update(asset->command_signed_);
    hilo->update((asset->command_byte_order == BYTE_ORDER_HILO));
    lohi->update((asset->command_byte_order == BYTE_ORDER_LOHI));
}



StdoutAudioHILO::StdoutAudioHILO(StdoutAudioConfig *gui, int x, int y)
 : BC_Radial(x, y, gui->asset->command_byte_order == BYTE_ORDER_HILO, _("Hi Lo"))
{
	this->gui = gui;
}
int StdoutAudioHILO::handle_event()
{
	gui->asset->command_byte_order = BYTE_ORDER_HILO;
	gui->lohi->update(0);
	return 1;
}




StdoutAudioLOHI::StdoutAudioLOHI(StdoutAudioConfig *gui, int x, int y)
 : BC_Radial(x, y, gui->asset->command_byte_order == BYTE_ORDER_LOHI, _("Lo Hi"))
{
	this->gui = gui;
}
int StdoutAudioLOHI::handle_event()
{
	gui->asset->command_byte_order = BYTE_ORDER_LOHI;
	gui->hilo->update(0);
	return 1;
}


StdoutVideoConfig::StdoutVideoConfig(BC_WindowBase *parent_window, Asset *asset)
 : StdoutBaseConfig(parent_window,
    asset,
    PROGRAM_NAME ": Video Compression",
    VIDEO_PARAMS)
{
// seed it with defaults
    for(int i = 0; i < sizeof(FileStdout::default_video_presets) / sizeof(StdoutPreset*); i++)
    {
        StdoutPreset *preset = FileStdout::default_video_presets[i];
        if(get_preset(&preset->title) < 0)
        {
            preset_names->append(new BC_ListBoxItem(preset->title.c_str()));
            preset_data->append(new StdoutPreset(*preset));
        }
    }
}

StdoutVideoConfig::~StdoutVideoConfig()
{
    delete cmodel;
}


void StdoutVideoConfig::create_objects2(int x, int y)
{
	char string[BCTEXTLEN];
    BC_Title *title;
    int margin = MWindow::theme->widget_border;
    
    for(int i = 0; i < sizeof(supported_cmodels) / sizeof(int); i++)
    {
        cmodel_to_text(string, supported_cmodels[i]);
        cmodels.append(new BC_ListBoxItem(string));
    }
    



    add_subwindow(title = new BC_Title(x, y, _("Color Model to write to stdin:")));
    y += title->get_h() + margin;
    cmodel_to_text(string, asset->command_cmodel);
    cmodel = new StdoutColormodel(this,
        &asset->command_cmodel,
        string,
        x, 
        y,
        DP(300),
        DP(300));
    cmodel->create_objects();
    cmodel->set_read_only(1);
}


void StdoutVideoConfig::update()
{
    char string[BCTEXTLEN];
    cmodel_to_text(string, asset->command_cmodel);
    cmodel->update(string);
}


StdoutColormodel::StdoutColormodel(StdoutVideoConfig *gui,
    int *output_value,
    const char *text,
	int x, 
	int y,
    int w,
    int h)
 : BC_PopupTextBox(gui,
    &gui->cmodels,
    text,
    x,
    y,
    w,
    h)
{
    this->output_value = output_value;
    this->gui = gui;
}

int StdoutColormodel::handle_event()
{
    int number = get_number();
    int total = sizeof(supported_cmodels) / sizeof(int);
    if(number < total && number >= 0)
    {
        *output_value = supported_cmodels[number];
    }
    return 0;
}







StdoutMplexConfig::StdoutMplexConfig(BC_WindowBase *parent_window, Asset *asset)
 : StdoutBaseConfig(parent_window,
    asset,
    PROGRAM_NAME ": Wrapper Settings",
    MPLEX_PARAMS)
{
// seed it with defaults
    for(int i = 0; i < sizeof(FileStdout::default_mplex_presets) / sizeof(StdoutPreset*); i++)
    {
        StdoutPreset *preset = FileStdout::default_mplex_presets[i];
        if(get_preset(&preset->title) < 0)
        {
            preset_names->append(new BC_ListBoxItem(preset->title.c_str()));
            preset_data->append(new StdoutPreset(*preset));
        }
    }
}

StdoutMplexConfig::~StdoutMplexConfig()
{
}


void StdoutMplexConfig::create_objects2(int x, int y)
{
    BC_Title *title;
    BC_CheckBox *box;
    int margin = MWindow::theme->widget_border;


	add_subwindow(box = new BC_CheckBox(x, 
        y, 
        &asset->command_delete_temps, 
        _("Delete temporary files")));
}

