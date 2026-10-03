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

// common interface bits for the many command line editors which output to a file
// must be deleted manually

#ifndef COMMANDTOOLS_H
#define COMMANDTOOLS_H


#include "asset.inc" 
#include "guicast.h"

#include <string>

class CommandTools;
class CommandText;
class CommandPreset;

class CommandPresetsList : public BC_ListBox
{
public:
	CommandPresetsList(CommandTools *gui,
		int x,
		int y,
		int w, 
		int h);
	int selection_changed();
	int handle_event();
    CommandTools *gui;
};

// Delete the highlighted preset
class CommandDelete : public BC_GenericButton
{
public:
	CommandDelete(CommandTools *gui, int x, int y);
	int handle_event();
    CommandTools *gui;
};

// Copy the highlighted preset to the current command text
class CommandApply : public BC_GenericButton
{
public:
	CommandApply(CommandTools *gui, int x, int y);
	int handle_event();
    CommandTools *gui;
};

// Save the current command to a new or existing preset
class CommandSave : public BC_GenericButton
{
public:
	CommandSave(CommandTools *gui, int x, int y);
	int handle_event();
    CommandTools *gui;
};

// Name or contents of a command
class CommandText : public BC_TextBox
{
public:
    CommandText(std::string *output,
        int x, 
		int y,
        int w,
        int rows);
	int handle_event();
    std::string *output;
};

class CommandPreset
{
public:
    CommandPreset();
    CommandPreset(const char *title, 
        const char *command);
    CommandPreset(const char *title, 
        const char *command, 
        int color_model);
    CommandPreset(const char *title, 
        const char *command, 
        int bits, 
        int byte_order, 
        int signed_, 
        int dither);

    static CommandPreset* createMplex(const char *title, 
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
	ConfirmPreset(CommandTools *gui);
	void create_objects(const char *text);
};

class CommandTools : public BC_Window
{
public:
    CommandTools(BC_WindowBase *parent_window, 
        Asset *asset, 
        const char *title,
        int option_type);
    virtual ~CommandTools();

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
    ArrayList<CommandPreset*> *preset_data;
//    std::string preset_title;

    CommandText *command_title;
    CommandText *command;
    CommandDelete *delete_;
    CommandSave *save;
    CommandApply *apply;
    CommandPresetsList *list;
    BC_Hash *defaults;
    BC_Bar *bar;
	BC_WindowBase *parent_window;
	Asset *asset;
    int option_type;
};




#endif






