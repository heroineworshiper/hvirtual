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

#include "apatchgui.h"
#include "autoconf.h"
#include "automation.h"
#include "autos.h"
#include "bcwindowbase.h"
#include "cpanel.h"
#include "cwindowgui.h" 
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "keyframe.h"
#include "keyframegui.h"
#include "keyframepopup.h"
#include "language.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "patchbay.h"
#include "patchgui.h" 
#include "track.h"
#include "vpatchgui.h"

KeyframePopup::KeyframePopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	key_hide = 0;
	key_delete = 0;
	key_copy = 0;
    paste = 0;
	key_linear = 0;
	key_bezier = 0;
	edit = 0;
    copy_default = 0;
    paste_default = 0;
}

KeyframePopup::~KeyframePopup()
{
}

void KeyframePopup::create_objects()
{
	add_item(key_hide = new KeyframePopupHide(mwindow, this));
}

int KeyframePopup::update(double position,
    Plugin *plugin, // enables preset operations
    Autos *autos, // enables default keyframe operations
    Auto *auto_) // enables single keyframe operations
{
    this->position = position;
	this->plugin = plugin;
	this->autos = autos;
	this->auto_ = auto_;

    delete paste;
    paste = 0;

	if(auto_)
	{
		if(!key_delete) add_item(key_delete = new KeyframePopupDelete(mwindow, this));
		if(!key_copy) add_item(key_copy = new KeyframePopupCopy(mwindow, this));
	}
	else
	{
		if(key_delete) delete key_delete;
		if(key_copy) delete key_copy;
		key_delete = 0;
		key_copy = 0;
	}

    if(autos)
        add_item(paste = new KeyframePopupPaste(mwindow, this, "Paste keyframe"));

	if(auto_ && autos && autos->type == Autos::AUTOMATION_TYPE_FLOAT)
	{
		if(!key_linear) add_item(key_linear = new KeyframePopupLinear(mwindow, this));
		if(!key_bezier) add_item(key_bezier = new KeyframePopupBezier(mwindow, this));
	}
	else
	{
		if(key_linear) delete key_linear;
		if(key_bezier) delete key_bezier;
		key_linear = 0;
		key_bezier = 0;
	}

    if(autos && !auto_)
    {
        if(!copy_default) add_item(copy_default = new KeyframePopupCopyDefault(mwindow, this));
        if(!paste_default) add_item(paste_default = new KeyframePopupPasteDefault(mwindow, this));
    }
    else
    {
        delete copy_default;
        delete paste_default;
        copy_default = 0;
        paste_default = 0;
    }

	if(plugin)
	{
		if(!edit) add_item(edit = new KeyframePopupEdit(mwindow, this));
	}
	else
	{
		delete edit;
		edit = 0;
	}
	return 0;
}

// int KeyframePopup::update(Automation *automation, 
// 	Autos *autos, 
// 	Auto *auto_keyframe)
// {
// 	this->keyframe_plugin = 0;
// 	this->keyframe_automation = automation;
// 	this->keyframe_autos = autos;
// 	this->keyframe_auto = auto_keyframe;
// 
// 	if(auto_keyframe && autos->type == Autos::AUTOMATION_TYPE_FLOAT)
// 	{
// 		if(!key_linear) add_item(key_linear = new KeyframePopupLinear(mwindow, this));
// 		if(!key_bezier) add_item(key_bezier = new KeyframePopupBezier(mwindow, this));
// 	}
// 	else
// 	{
// 		if(key_linear) delete key_linear;
// 		if(key_bezier) delete key_bezier;
// 		key_linear = 0;
// 		key_bezier = 0;
// 	}
// 
// 	if(auto_keyframe)
// 	{
// 		if(!key_delete) add_item(key_delete = new KeyframePopupDelete(mwindow, this));
// 		if(!key_copy) add_item(key_copy = new KeyframePopupCopy(mwindow, this));
// 	}
// 	else
// 	{
// 		if(key_delete) delete key_delete;
// 		if(key_copy) delete key_copy;
// 		key_delete = 0;
// 		key_copy = 0;
// 	}
// 	
// 	if(edit)
// 	{
// 		delete edit;
// 		edit = 0;
// 	}
// 
// /* snap to cursor */
// 	if(keyframe_auto)
// 	{
// 		double current_position = mwindow->edl->local_session->get_selectionstart(1);
// 		double new_position = keyframe_automation->track->from_units(keyframe_auto->position);
// 		mwindow->edl->local_session->set_selectionstart(new_position);
// 		mwindow->edl->local_session->set_selectionend(new_position);
// 
// 		if (current_position != new_position)
// 		{
// 			mwindow->edl->local_session->set_selectionstart(new_position);
// 			mwindow->edl->local_session->set_selectionend(new_position);
// 			mwindow->gui->lock_window();
// 			mwindow->gui->update(1, 1, 1, 1, 1, 1, 0);	
// 			mwindow->gui->unlock_window();
// 		}
// 	}
// 
// 	return 0;
// }





KeyframePopupDelete::KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Delete keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupDelete::~KeyframePopupDelete()
{
}

int KeyframePopupDelete::handle_event()
{
    mwindow->delete_keyframe(popup->auto_);
	popup->auto_ = 0;
	return 1;
}





KeyframePopupLinear::KeyframePopupLinear(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Make linear"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupLinear::~KeyframePopupLinear()
{
}

int KeyframePopupLinear::handle_event()
{
    mwindow->set_keyframe_mode(popup->auto_, Auto::LINEAR);
	return 1;
}





KeyframePopupBezier::KeyframePopupBezier(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Make bezier"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupBezier::~KeyframePopupBezier()
{
}

int KeyframePopupBezier::handle_event()
{
    mwindow->set_keyframe_mode(popup->auto_, Auto::BEZIER);
	return 1;
}





KeyframePopupHide::KeyframePopupHide(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Hide keyframe type"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupHide::handle_event()
{
    mwindow->edl->session->auto_conf->autos[popup->autos->overlay_type] = 0;
	mwindow->gui->update(0,
	        1,      // 1 for incremental drawing.  2 for full refresh
	        0,
	        0,
	        0,
            0,   
            0);
	mwindow->gui->mainmenu->update_toggles(1);
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("KeyframePopupHide::handle_event");

// // Get the array index of the curve
// 	int update_gui = 0;
// 	if(popup->autos)
// 	{
//         		if(popup->keyframe_autos->type == Autos::AUTOMATION_TYPE_PLUGIN)
// 		{
// 			mwindow->edl->session->auto_conf->plugins = 0;
// 			update_gui = 1;
// 		}
// 		else
// 		{
// 			Track *track = popup->keyframe_autos->track;
// 			if(track)
// 			{
// 				Automation *automation = track->automation;
// 				if(automation)
// 				{
// 					for(int i = 0; i < AUTOMATION_TOTAL; i++)
// 					{
// 						if(automation->autos[i] == popup->keyframe_autos)
// 						{
// 							mwindow->edl->session->auto_conf->autos[i] = 0;
// 							update_gui = 1;
// 							break;
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// 
// 	if(update_gui)
// 	{
// 		mwindow->gui->update(0,
// 	        	1,      // 1 for incremental drawing.  2 for full refresh
// 	        	0,
// 	        	0,
// 	        	0,
//             	0,   
//             	0);
// 		mwindow->gui->mainmenu->update_toggles(1);
// 		mwindow->gui->unlock_window();
// 		mwindow->gwindow->gui->update_toggles(1);
// 		mwindow->gui->lock_window("KeyframePopupHide::handle_event");
// 	}

	return 1;
}



KeyframePopupCopy::KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Copy keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupCopy::handle_event()
{
    mwindow->copy_keyframe(popup->auto_);
	return 1;
}


KeyframePopupEdit::KeyframePopupEdit(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Presets..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupEdit::handle_event()
{
	mwindow->show_keyframe_gui(popup->plugin, 0);
	return 1;
}






KeyframePopupCopyDefault::KeyframePopupCopyDefault(MWindow *mwindow, 
    KeyframePopup *popup)
 : BC_MenuItem(_("Copy default keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int KeyframePopupCopyDefault::handle_event()
{
    mwindow->copy_keyframe(popup->autos->default_auto);
	return 1;
}





KeyframePopupPasteDefault::KeyframePopupPasteDefault(MWindow *mwindow, 
    KeyframePopup *popup)
 : BC_MenuItem(_("Paste default keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int KeyframePopupPasteDefault::handle_event()
{
    mwindow->paste_automation(0,
        popup->autos, 
        popup->autos->default_auto);
	return 1;
}


KeyframePopupPaste::KeyframePopupPaste(MWindow *mwindow, 
    KeyframePopup *popup,
    const char *text)
 : BC_MenuItem(_(text))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int KeyframePopupPaste::handle_event()
{
    mwindow->paste_automation(popup->position,
        popup->autos, 
        popup->auto_);
	return 1;
}

