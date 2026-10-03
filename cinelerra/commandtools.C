/*
 * CINELERRA
 * Copyright (C) 1997-2026 Adam Williams <broadcast at earthling dot net>
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
#include "clip.h"
#include "commandtools.h"
#include "errorbox.h"
#include "file.inc"
#include "filesystem.h"
#include "language.h"
#include "mwindow.h"
#include "theme.h"
#include <string.h>

#define MAX_PRESETS 255

CommandPresetsList::CommandPresetsList(CommandTools *gui,
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

int CommandPresetsList::selection_changed()
{
    return 1;
}

int CommandPresetsList::handle_event()
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
CommandDelete::CommandDelete(CommandTools *gui, int x, int y)
 : BC_GenericButton(x, y, DELETE_TEXT)
{
    this->gui = gui;
    set_tooltip("Delete the highlighted preset.");
}
int CommandDelete::handle_event()
{
    gui->delete_preset();
    return 1;
}


CommandApply::CommandApply(CommandTools *gui, int x, int y)
 : BC_GenericButton(x, y, APPLY_TEXT)
{
    this->gui = gui;
    set_tooltip("Apply the highlighted preset to the command line.");
}
int CommandApply::handle_event()
{
    gui->load_preset();
    return 1;
}



CommandSave::CommandSave(CommandTools *gui, int x, int y)
 : BC_GenericButton(x, y, SAVE_TEXT)
{
    this->gui = gui;
    set_tooltip("Save the command & title as a preset.");
}
int CommandSave::handle_event()
{
    gui->save_preset();
    return 1;
}





CommandText::CommandText(std::string *output,
    int x, 
	int y,
    int w,
    int rows)
 : BC_TextBox(x, y, w, rows, output->c_str())
{
    this->output = output;
}

int CommandText::handle_event()
{
    output->assign(get_text());
    return 1;
}


CommandPreset::CommandPreset()
{
    reset();
}

CommandPreset::CommandPreset(const char *title, const char *command, 
    int color_model)
{
    reset();
    this->title.assign(title);
    this->command.assign(command);
    this->color_model = color_model;
}

CommandPreset::CommandPreset(const char *title, const char *command, 
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

void CommandPreset::reset()
{
    color_model = BC_YUV420P;
    bits = BITSLINEAR16;
    byte_order = BYTE_ORDER_LOHI;
    signed_ = 1;
    dither = 0;
}

CommandPreset* CommandPreset::createMplex(const char *title,
    const char *command,
    int delete_temps)
{
    CommandPreset *result = new CommandPreset;
    result->reset();
    result->title.assign(title);
    result->command.assign(command);
    result->delete_temps = delete_temps;
    return result;
}


ConfirmPreset::ConfirmPreset(CommandTools *gui)
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
	if(new_w > get_root_w(1) / 2) 
    {
        new_w = get_root_w(1) / 2;
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


CommandTools::CommandTools(BC_WindowBase *parent_window, 
    Asset *asset, 
    const char *window_title,
    int option_type)
 : BC_Window(window_title,
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	MWindow::theme->command_w,
	MWindow::theme->command_h)
{
//printf("CommandTools::CommandTools %d\n", __LINE__);
	this->parent_window = parent_window;
	this->asset = asset;
    this->option_type = option_type;
    load_defaults();
}

CommandTools::~CommandTools()
{
    save_defaults();
    delete defaults;

	preset_names->remove_all_objects();
    delete preset_names;
	preset_data->remove_all_objects();
    delete preset_data;
}

void CommandTools::load_defaults()
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
    preset_data = new ArrayList<CommandPreset*>;

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

        CommandPreset *preset = new CommandPreset;

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
//    sprintf(string, "%sPRESET_TITLE", option_text);
//    defaults->get(string, &preset_title);

// the current command line comes from the asset
}

void CommandTools::save_defaults()
{
    defaults->clear();

    const char *option_text = get_option_text();
	char string[BCTEXTLEN];
    for(int i = 0; i < preset_names->size() && i < preset_data->size(); i++)
    {
        CommandPreset *preset = preset_data->get(i);
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
//    sprintf(string, "%sPRESET_TITLE", option_text);
//    defaults->update(string, &preset_title);

// command line comes from the asset

	defaults->save();
}


const char* CommandTools::get_option_text()
{
    switch(option_type)
    {
        case AUDIO_PARAMS: return "AUDIO_";
        case VIDEO_PARAMS: return "VIDEO_";
        case MPLEX_PARAMS: return "MPLEX_";
    }
    return "";
}

std::string* CommandTools::get_command_text()
{
    switch(option_type)
    {
        case AUDIO_PARAMS: return &asset->audio_command;
        case VIDEO_PARAMS: return &asset->video_command;
        case MPLEX_PARAMS: return &asset->wrapper_command;
    }
    return 0;
}

std::string* CommandTools::get_preset_title()
{
    switch(option_type)
    {
        case AUDIO_PARAMS: return &asset->audio_preset;
        case VIDEO_PARAMS: return &asset->video_preset;
        case MPLEX_PARAMS: return &asset->wrapper_preset;
    }
    return 0;
}

void CommandTools::create_objects()
{
    BC_Title *title;
    int margin = MWindow::theme->widget_border;
    int x = margin, y = margin;

	lock_window("CommandTools::create_objects");

    int button_w = 0;
    button_w = MAX(button_w, BC_GenericButton::calculate_w(this, DELETE_TEXT));
    button_w = MAX(button_w, BC_GenericButton::calculate_w(this, APPLY_TEXT));
    button_w = MAX(button_w, BC_GenericButton::calculate_w(this, SAVE_TEXT));
    int x2 = get_w() - margin - button_w;

    add_tool(title = new BC_Title(x, y, _("Presets:")));
	y += title->get_h() + margin;
    
    add_tool(list = new CommandPresetsList(this,
	    x,
	    y,
	    x2 - x - margin, 
	    DP(100)));
    int y2 = y + list->get_h() + margin;
    add_tool(delete_ = new CommandDelete(this, x2, y));
    y += delete_->get_h() + margin;
    add_tool(save = new CommandSave(this, x2, y));
    y += save->get_h() + margin;
    add_tool(apply = new CommandApply(this, x2, y));
    y += apply->get_h() + margin;

    y = y2;
    add_tool(title = new BC_Title(x, y, _("Preset title:")));
	y += title->get_h() + margin;

    add_subwindow(command_title = new CommandText(get_preset_title(),
        x, 
	    y,
        get_w() - x - margin,
        1));
    y += command_title->get_h() + margin;

    add_tool(title = new BC_Title(x, y, _("Command line:")));
	y += title->get_h() + margin;

    add_subwindow(command = new CommandText(get_command_text(),
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

void CommandTools::create_objects2(int x, int y)
{
    
}

int CommandTools::close_event()
{
	set_done(0);
	return 1;
}

int CommandTools::resize_event(int w, int h)
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

void CommandTools::save_preset()
{
// ignore if no title
	if(command_title->get_text()[0])
    {
// replace existing preset
        int got_it = 0;
        CommandPreset *dst = 0;
        for(int i = 0; 
            i < preset_names->size() && i < preset_data->size(); 
            i++)
        {
// printf("CommandTools::save_preset %d %s %s %d\n", 
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
            dst = new CommandPreset;
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

void CommandTools::delete_preset()
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


void CommandTools::load_preset()
{
// ignore if nothing selected
    int number = list->get_selection_number(0, 0);
	if(number >= 0)
    {
        CommandPreset *src = preset_data->get(number);
        command->update(src->command.c_str());
        std::string *preset_title = get_preset_title();
        preset_title->assign(preset_names->get(number)->get_text());
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

void CommandTools::update()
{
}

int CommandTools::get_preset(const char *title)
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

int CommandTools::get_preset(std::string *title)
{
    return get_preset(title->c_str());
}









