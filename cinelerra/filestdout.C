/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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
#include "playbackconfig.h"
#include "recordconfig.h"
#include "theme.h"
#include "videodevice.inc"
#include <string.h>

extern "C"
{
#include <uuid.h>
}




CommandPreset* FileStdout::default_audio_presets[2] = 
{
    new CommandPreset("ffmpeg AAC",
        "ffmpeg -y -f f32le -ar %r -ac %c -i - -f mp4 -c:a aac -b:a 192k %1",
        BITSFLOAT,
        BYTE_ORDER_LOHI,
        1,
        0),
    new CommandPreset("null",
        "cat > /dev/null",
        BITSFLOAT,
        BYTE_ORDER_LOHI,
        1,
        0),
};

CommandPreset* FileStdout::default_video_presets[6] =
{
    new CommandPreset("ffmpeg HEVC CBR",
        "ffmpeg -y -f rawvideo -pix_fmt yuv420p -r %r -s:v %wx%h -i - -f h264 -c:v hevc -b:v 5M %1",
        BC_YUV420P),
    new CommandPreset("ffmpeg HEVC VBR",
        "ffmpeg -y -f rawvideo -pix_fmt yuv420p -r %r -s:v %wx%h -i - -f h264 -c:v hevc -qp:v 30 %1",
        BC_YUV420P),
    new CommandPreset("ffmpeg HEVC 444",
        "ffmpeg -y -f rawvideo -pix_fmt yuv444p -r %r -s:v %wx%h -i - -f h264 -c:v hevc -qp:v 30 %1",
        BC_YUV444P),
    new CommandPreset("ffmpeg HEVC 422",
        "ffmpeg -y -f rawvideo -pix_fmt yuv422p -r %r -s:v %wx%h -i - -f h264 -c:v hevc -qp:v 30 %1",
        BC_YUV422P),
    new CommandPreset("ffmpeg H.264 VBR",
        "ffmpeg -y -f rawvideo -pix_fmt yuv420p -r %r -s:v %wx%h -i - -f h264 -c:v h264 -qp:v 30 %1",
        BC_YUV420P),
    new CommandPreset("null",
        "cat > /dev/null",
        BC_YUV420P),
};

CommandPreset* FileStdout::default_mplex_presets[3] = 
{
    CommandPreset::createMplex("ffmpeg MP4", "ffmpeg -y -i %3 -i %2 -c:v copy -c:a copy %1", 0),
    CommandPreset::createMplex("ffmpeg MP4 video", "ffmpeg -y -i %2 -c:v copy %1", 0),
    CommandPreset::createMplex("ffmpeg MP4 audio", "ffmpeg -y -i %3 -c:a copy %1", 0)
};

// Need planer colormodels not in MWindow::colormodels
static int supported_cmodels[] = 
{
// ffmpeg requires planar 8 bit
    BC_YUV420P,
    BC_YUVA420P,
    BC_YUV422P,
    BC_YUV444P,
    BC_YUVA444P,
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
//PRINT_TRACE
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

//PRINT_TRACE
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
// printf("FileStdout::write_frames %d frame=%p cmodel=%d y=%p u=%p v=%p\n", 
// __LINE__, 
// file->temp_frame,
// asset->command_cmodel,
// file->temp_frame->get_y(),
// file->temp_frame->get_u(),
// file->temp_frame->get_v());

//PRINT_TRACE
                cmodel_transfer(file->temp_frame->get_rows(),
                    src->get_rows(),
                    file->temp_frame->get_y(),
                    file->temp_frame->get_u(),
                    file->temp_frame->get_v(),
                    file->temp_frame->get_a(),
                    src->get_y(),
                    src->get_u(),
                    src->get_v(),
                    src->get_a(),
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
int FileStdout::get_best_colormodel(Asset *asset, 
        VideoInConfig *in_config, 
        VideoOutConfig *out_config)
{
    if(in_config)
    {
        switch(in_config->driver)
        {
        case VIDEO4LINUX2:
            switch(in_config->v4l2_format)
            {
            case CAPTURE_YUYV:
                return BC_YUV422;
                break;
            }
            break;
        }
    }
	return asset->command_cmodel;
}



StdoutAudioConfig::StdoutAudioConfig(BC_WindowBase *parent_window, Asset *asset)
 : CommandTools(parent_window,
    asset,
    PROGRAM_NAME ": Audio Compression",
    AUDIO_PARAMS)
{
//printf("StdoutAudioConfig::StdoutAudioConfig %d\n", __LINE__);
// seed it with defaults
    for(int i = 0; i < sizeof(FileStdout::default_audio_presets) / sizeof(CommandPreset*); i++)
    {
        CommandPreset *preset = FileStdout::default_audio_presets[i];
        if(get_preset(&preset->title) < 0)
        {
            preset_names->append(new BC_ListBoxItem(preset->title.c_str()));
            preset_data->append(new CommandPreset(*preset));
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
 : CommandTools(parent_window,
    asset,
    PROGRAM_NAME ": Video Compression",
    VIDEO_PARAMS)
{
// seed it with defaults
    for(int i = 0; i < sizeof(FileStdout::default_video_presets) / sizeof(CommandPreset*); i++)
    {
        CommandPreset *preset = FileStdout::default_video_presets[i];
        if(get_preset(&preset->title) < 0)
        {
            preset_names->append(new BC_ListBoxItem(preset->title.c_str()));
            preset_data->append(new CommandPreset(*preset));
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
 : CommandTools(parent_window,
    asset,
    PROGRAM_NAME ": Wrapper Settings",
    MPLEX_PARAMS)
{
// seed it with defaults
    for(int i = 0; i < sizeof(FileStdout::default_mplex_presets) / sizeof(CommandPreset*); i++)
    {
        CommandPreset *preset = FileStdout::default_mplex_presets[i];
        if(get_preset(&preset->title) < 0)
        {
            preset_names->append(new BC_ListBoxItem(preset->title.c_str()));
            preset_data->append(new CommandPreset(*preset));
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

