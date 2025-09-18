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

#include "apatchgui.h"
#include "autoconf.h"
#include "automation.h"
#include "autos.h"
#include "bcwindowbase.h"
#include "cpanel.h"
#include "cwindowgui.h" 
#include "cwindow.h"
#include "editkeyframe.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
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
    key_bezier2 = 0;
    key_bezier3 = 0;
	preset = 0;
    edit = 0;
//    copy_default = 0;
    paste_default = 0;
    bar = 0;
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

// force the order by deleting & recreating all the items
    delete edit;
    edit = 0;
    delete paste;
    paste = 0;
	delete preset;
	preset = 0;
	delete key_delete;
	key_delete = 0;
	delete key_copy;
	key_copy = 0;
//    delete copy_default;
    delete paste_default;
//    copy_default = 0;
    paste_default = 0;
	delete key_linear;
	delete key_bezier;
	delete key_bezier2;
    delete key_bezier3;
	key_linear = 0;
	key_bezier = 0;
	key_bezier2 = 0;
    key_bezier3 = 0;
    delete bar;
    bar = 0;

// editing text plugin keyframes is a bridge too far
// it would require amending the plugin popup & counts as something which
// already has a dedicated interface
// float is particularly difficult to edit without a text mode
    if(autos && 
        (autos->type == Autos::AUTOMATION_TYPE_FLOAT))
    {
        add_item(edit = new KeyframePopupEdit(mwindow, this));
        if(auto_)
            edit->set_text(_("Edit keyframe..."));
        else
            edit->set_text(_("Edit default keyframe..."));
    }

	if(auto_)
	{
		if(!key_delete) add_item(key_delete = new KeyframePopupDelete(mwindow, this));
	}

	if(!key_copy && auto_)
        add_item(key_copy = new KeyframePopupCopy(mwindow, this, _("Copy keyframe")));

    if(autos)
        add_item(paste = new KeyframePopupPaste(mwindow, this, "Paste keyframe"));

	if(auto_ && autos && autos->type == Autos::AUTOMATION_TYPE_FLOAT)
	{
        add_item(bar = new BC_MenuItem("-"));
		add_item(key_linear = new KeyframePopupLinear(mwindow, this));
		add_item(key_bezier = new KeyframePopupBezier(mwindow, this));
		add_item(key_bezier2 = new KeyframePopupBezier2(mwindow, this));
//		add_item(key_bezier3 = new KeyframePopupBezier3(mwindow, this));
        key_linear->set_checked(((FloatAuto*)auto_)->mode == FloatAuto::LINEAR);
        key_bezier->set_checked(((FloatAuto*)auto_)->mode == FloatAuto::BEZIER_LOCKED);
        key_bezier2->set_checked(((FloatAuto*)auto_)->mode == FloatAuto::BEZIER_UNLOCKED);
//        key_bezier3->set_checked(((FloatAuto*)auto_)->mode == FloatAuto::BEZIER_TANGENT);
	}

    if(autos && !auto_)
    {
        if(!key_copy) add_item(key_copy = new KeyframePopupCopy(
            mwindow, 
            this,
            _("Copy default keyframe")));
        if(!paste_default) 
            add_item(paste_default = new KeyframePopupPasteDefault(mwindow, 
                this));
    }

	if(plugin)
	{
		if(!preset) add_item(preset = new KeyframePopupPreset(mwindow, this));
	}
	return 0;
}






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
 : BC_MenuItem(_("Linear"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupLinear::handle_event()
{
    mwindow->set_keyframe_mode((FloatAuto*)popup->auto_, FloatAuto::LINEAR);
	return 1;
}





KeyframePopupBezier::KeyframePopupBezier(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Locked bezier"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupBezier::handle_event()
{
    mwindow->set_keyframe_mode((FloatAuto*)popup->auto_, FloatAuto::BEZIER_LOCKED);
	return 1;
}



KeyframePopupBezier2::KeyframePopupBezier2(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Unlocked bezier"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupBezier2::handle_event()
{
    mwindow->set_keyframe_mode((FloatAuto*)popup->auto_, FloatAuto::BEZIER_UNLOCKED);
	return 1;
}


KeyframePopupBezier3::KeyframePopupBezier3(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Tangent bezier"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupBezier3::handle_event()
{
    mwindow->set_keyframe_mode((FloatAuto*)popup->auto_, FloatAuto::BEZIER_TANGENT);
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



KeyframePopupCopy::KeyframePopupCopy(MWindow *mwindow, 
    KeyframePopup *popup,
    const char *title)
 : BC_MenuItem(title)
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupCopy::handle_event()
{
    if(popup->auto_)
        mwindow->copy_keyframe(popup->auto_);
    else
        mwindow->copy_keyframe(popup->autos->default_auto);
	return 1;
}


KeyframePopupPreset::KeyframePopupPreset(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Presets..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupPreset::handle_event()
{
	mwindow->show_keyframe_gui(popup->plugin, 0);
	return 1;
}




KeyframePopupEdit::KeyframePopupEdit(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem("")
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupEdit::handle_event()
{
    if(popup->auto_)
        mwindow->edit_keyframe->start(popup->auto_);
    else
        mwindow->edit_keyframe->start(popup->autos->default_auto);
	return 1;
}





// KeyframePopupCopyDefault::KeyframePopupCopyDefault(MWindow *mwindow, 
//     KeyframePopup *popup)
//  : BC_MenuItem(_("Copy default keyframe"))
// {
// 	this->mwindow = mwindow;
// 	this->popup = popup;
// }
// int KeyframePopupCopyDefault::handle_event()
// {
//     mwindow->copy_keyframe(popup->autos->default_auto);
// 	return 1;
// }





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

