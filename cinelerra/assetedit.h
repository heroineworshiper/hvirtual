
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef ASSETEDIT_H
#define ASSETEDIT_H

#include "asset.inc"
#include "awindow.inc"
#include "bcdialog.h"
#include "bitspopup.inc"
#include "browsebutton.h"
#include "formatpopup.h"
#include "guicast.h"
#include "indexable.inc"
#include "language.h"
#include "mwindow.h"


class AssetEditByteOrderHILO;
class AssetEditByteOrderLOHI;
class AssetEditPath;
class AssetEditPathText;
class AssetEditWindow;

class AssetEdit : public BC_DialogThread
{
public:
	AssetEdit(MWindow *mwindow);
	~AssetEdit();
	
	void edit_asset(Indexable *indexable);
	int set_asset(Indexable *indexable);
	void handle_close_event(int result);
	BC_Window* new_gui();

// not deleted until handle_close_event
	Indexable *indexable;
	MWindow *mwindow;
	AssetEditWindow *window;


// Changed parameters
	Asset *changed_params;
// changed parameters not included in an asset
    double new_nested_frame_rate;
    double nested_frame_rate;
    int64_t new_nested_sample_rate;
    int64_t nested_sample_rate;
};



// Pcm is the only format users should be able to fix.
// All other formats display information about the file in read-only.

class AssetEditWindow : public BC_Window
{
public:
	AssetEditWindow(MWindow *mwindow, AssetEdit *asset_edit);
	~AssetEditWindow();

	void create_objects();
	AssetEditPathText *path_text;
	AssetEditPath *path_button;
	AssetEditByteOrderHILO *hilo;
	AssetEditByteOrderLOHI *lohi;
	BitsPopup *bitspopup;
	int allow_edits;
	MWindow *mwindow;
	AssetEdit *asset_edit;
};


class AssetEditPath : public BrowseButton
{
public:
	AssetEditPath(MWindow *mwindow, 
		AssetEditWindow *fwindow, 
		BC_TextBox *textbox, 
		int y, 
		const char *text, 
		const char *window_title = _(PROGRAM_NAME " Path"), 
		const char *window_caption = _("Select a file"));
	~AssetEditPath();
	
	AssetEditWindow *fwindow;
};


class AssetEditPathText : public BC_TextBox
{
public:
	AssetEditPathText(AssetEditWindow *fwindow, int y);
	~AssetEditPathText();
	int handle_event();

	AssetEditWindow *fwindow;
};



class AssetEditFormat : public FormatPopup
{
public:
	AssetEditFormat(AssetEditWindow *fwindow, char* default_, int y);
	~AssetEditFormat();
	
	int handle_event();
	AssetEditWindow *fwindow;
};


class AssetEditChannels : public BC_TumbleTextBox
{
public:
	AssetEditChannels(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditRate : public BC_TextBox
{
public:
	AssetEditRate(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditNestedRate : public BC_TextBox
{
public:
	AssetEditNestedRate(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditFRate : public BC_TextBox
{
public:
	AssetEditFRate(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditNestedFRate : public BC_TextBox
{
public:
	AssetEditNestedFRate(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditHeader : public BC_TextBox
{
public:
	AssetEditHeader(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditByteOrderLOHI : public BC_Radial
{
public:
	AssetEditByteOrderLOHI(AssetEditWindow *fwindow, int value, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditByteOrderHILO : public BC_Radial
{
public:
	AssetEditByteOrderHILO(AssetEditWindow *fwindow, int value, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditSigned : public BC_CheckBox
{
public:
	AssetEditSigned(AssetEditWindow *fwindow, int value, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

#endif
