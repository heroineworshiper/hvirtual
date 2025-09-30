/*
 * CINELERRA
 * Copyright (C) 2008-2025 Adam Williams <broadcast at earthling dot net>
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

#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "theme.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "zoombar.h"



EditPanel::EditPanel(MWindow *mwindow, 
	BC_WindowBase *subwindow,
	int x, 
	int y, 
	int editing_mode, 
	int use_editing_mode,
	int use_keyframe, 
	int use_splice,   // Extra buttons
	int use_overwrite,
	int use_lift,
	int use_extract,
	int use_copy, 
	int use_paste, 
	int use_undo,
	int use_fit,
	int use_labels,
	int use_toclip,
	int use_meters,
	int is_mwindow,
	int use_cut,
    int use_razor)
{
	this->editing_mode = editing_mode;
	this->use_editing_mode = use_editing_mode;
	this->use_keyframe = use_keyframe;
	this->use_splice = use_splice;
	this->use_overwrite = use_overwrite;
	this->use_lift = 0;
	this->use_extract = 0;
	this->use_copy = use_copy;
	this->use_paste = use_paste;
	this->use_undo = use_undo;
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->use_fit = use_fit;
	this->use_labels = use_labels;
	this->use_toclip = use_toclip;
	this->use_meters = use_meters;
	this->is_mwindow = is_mwindow;
	this->use_cut = use_cut;
    this->use_razor = use_razor;

	this->x = x;
	this->y = y;
	this->meter_panel = 0;
	arrow = 0;
	ibeam = 0;
	keyframe = 0;
	fit = 0;
	fit_autos = 0;
	prevlabel = 0;
	nextlabel = 0;
	prevedit = 0;
	nextedit = 0;
	meters = 0;
    razor = 0;
}

EditPanel::~EditPanel()
{
}

void EditPanel::set_meters(MeterPanel *meter_panel)
{
	this->meter_panel = meter_panel;
}


void EditPanel::update()
{
	int new_editing_mode = mwindow->edl->session->editing_mode;
	if(arrow) arrow->update(new_editing_mode == EDITING_ARROW);
	if(ibeam) ibeam->update(new_editing_mode == EDITING_IBEAM);
	if(keyframe) keyframe->update(mwindow->edl->session->auto_keyframes);
	if(meters) 
	{
//printf("EditPanel::update %d %p %p\n", __LINE__, subwindow, (BC_WindowBase*)mwindow->cwindow->gui);
		if(subwindow == (BC_WindowBase*)mwindow->cwindow->gui)
		{
//printf("EditPanel::update %d %d\n", __LINE__, mwindow->edl->session->cwindow_meter);
			meters->update(mwindow->edl->session->cwindow_meter);
			mwindow->cwindow->gui->update_meters();
		}
		else
		{
//printf("EditPanel::update %d %d\n", __LINE__, mwindow->edl->session->vwindow_meter);
			meters->update(mwindow->edl->session->vwindow_meter);
		}
	}
    if(label_color) label_color->update();
	subwindow->flush();
}

// void EditPanel::delete_buttons()
// {
// 	if(use_editing_mode)
// 	{
// 		if(arrow) delete arrow;
// 		if(ibeam) delete ibeam;
// 	}
// 	
// 	if(use_keyframe)
// 		delete keyframe;
// 
// 
// 	if(inpoint) delete inpoint;
// 	if(outpoint) delete outpoint;
// 	if(use_copy) delete copy;
// 	if(use_splice) delete splice;
// 	if(use_overwrite) delete overwrite;
// 	if(use_lift) delete lift;
// 	if(use_extract) delete extract;
// 	if(cut) delete cut;
// 	if(copy) delete copy;
// 	if(use_paste) delete paste;
//     delete razor;
//     razor = 0;
// 
// 	if(use_labels)
// 	{	
// 		delete labelbutton;
// 		delete prevlabel;
// 		delete nextlabel;
// 		delete prevedit;
// 		delete nextedit;
// 	}
// 
// 	if(use_fit) 
// 	{
// 		delete fit;
// 		delete fit_autos;
// 	}
// 	if(use_undo)
// 	{
// 		delete undo;
// 		delete redo;
// 	}
// 	
// 	prevlabel = 0;
// 	nextlabel = 0;
// 	prevedit = 0; 
// 	nextedit = 0;
// }

int EditPanel::calculate_w(MWindow *mwindow, 
    int use_keyframe, 
    int total_buttons)
{
	int result = 0;
	int button_w = mwindow->theme->get_image_set("ibeam")[0]->get_w();
	if(use_keyframe)
	{
		result += button_w + mwindow->theme->toggle_margin;
	}
	
	result += button_w * total_buttons;
	return result;
}

int EditPanel::calculate_h(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("ibeam")[0]->get_h();
}

void EditPanel::create_buttons()
{
	x1 = x, y1 = y;


SET_TRACE
	if(use_editing_mode)
	{
		subwindow->add_subwindow(arrow = new ArrowButton(mwindow, this, x1, y1));
		x1 += arrow->get_w();
		subwindow->add_subwindow(ibeam = new IBeamButton(mwindow, this, x1, y1));
		x1 += ibeam->get_w();
	}

	if(use_keyframe)
	{
		subwindow->add_subwindow(keyframe = new KeyFrameButton(mwindow, this, x1, y1));
		x1 += keyframe->get_w();
		x1 += mwindow->theme->toggle_margin;
	}

// Mandatory
	subwindow->add_subwindow(inpoint = new EditInPoint(mwindow, this, x1, y1));
	x1 += inpoint->get_w();
	subwindow->add_subwindow(outpoint = new EditOutPoint(mwindow, this, x1, y1));
	x1 += outpoint->get_w();
	if(use_splice)
	{
		subwindow->add_subwindow(splice = new EditSplice(mwindow, this, x1, y1));
		x1 += splice->get_w();
	}
	if(use_overwrite)
	{
		subwindow->add_subwindow(overwrite = new EditOverwrite(mwindow, this, x1, y1));
		x1 += overwrite->get_w();
	}
	if(use_lift)
	{
		subwindow->add_subwindow(lift = new EditLift(mwindow, this, x1, y1));
		x1 += lift->get_w();
	}
	if(use_extract)
	{
		subwindow->add_subwindow(extract = new EditExtract(mwindow, this, x1, y1));
		x1 += extract->get_w();
	}
	if(use_toclip)
	{
		subwindow->add_subwindow(clip = new EditToClip(mwindow, this, x1, y1));
		x1 += clip->get_w();
	}
	
	if(use_cut)
	{
		subwindow->add_subwindow(cut = new EditCut(mwindow, this, x1, y1));
		x1 += cut->get_w();
	}
	if(use_copy)
	{
		subwindow->add_subwindow(copy = new EditCopy(mwindow, this, x1, y1));
		x1 += copy->get_w();
	}
	if(use_paste)
	{
		subwindow->add_subwindow(paste = new EditPaste(mwindow, this, x1, y1));
		x1 += paste->get_w();
	}
    if(use_razor)
    {
		subwindow->add_subwindow(razor = new EditRazor(mwindow, this, x1, y1));
		x1 += razor->get_w();
    }
	
	if(use_meters)
	{
		if(!meter_panel)
		{
			printf("EditPanel::create_objects: meter_panel == 0\n");
            use_meters = 0;
		}
        else
        {
		    subwindow->add_subwindow(meters = new MeterShow(mwindow, meter_panel, x1, y1));
		    x1 += meters->get_w();
        }
	}

	if(use_labels)
	{
// create label color icons
// decode unscaled image for chroma key
        VFrame image(MWindow::theme->get_image_data("label_color.png"));
	    int src_w = image.get_w();
	    int src_h = image.get_h();
	    int dst_w = image.get_w();
	    int dst_h = image.get_h();
        if(BC_Resources::dpi >= MIN_DPI)
        {
            dst_w = src_w * BC_Resources::dpi / BASE_DPI;
            dst_h = src_h * BC_Resources::dpi / BASE_DPI;
        }
        VFrame temp(src_w, src_h, image.get_color_model());
        VFrame temp2(dst_w, dst_h, image.get_color_model());
        for(int i = 0; i < LABEL_COLORS; i++)
        {
            BC_Theme::swap_color(&temp, &image, 0x00ff00, MWindow::label_colors[i]);
// scale it to the DPI after chroma key
            VFrame::scale(&temp2, &temp);
            label_colors[i] = new BC_Pixmap(subwindow, 
		        &temp2,
		        PIXMAP_ALPHA);
        }

		subwindow->add_subwindow(labelbutton = new EditLabelbutton(mwindow, 
			this, 
			x1, 
			y1));
		x1 += labelbutton->get_w();
        
        subwindow->add_subwindow(label_color = new LabelColor(this, 
			x1, 
			y1 + (labelbutton->get_h() - BC_PopupMenu::calculate_h()) / 2));
		label_color->create_objects();
		x1 += label_color->get_w();
        
		subwindow->add_subwindow(prevlabel = new EditPrevLabel(mwindow, 
			this, 
			x1, 
			y1,
			is_mwindow));
		x1 += prevlabel->get_w();
		subwindow->add_subwindow(nextlabel = new EditNextLabel(mwindow, 
			this, 
			x1, 
			y1,
			is_mwindow));
		x1 += nextlabel->get_w();
	}

// all windows except VWindow since it's only implemented in MWindow.
	if(use_cut)
	{
		subwindow->add_subwindow(prevedit = new EditPrevEdit(mwindow, 
			this, 
			x1, 
			y1,
			is_mwindow));
		x1 += prevedit->get_w();
		subwindow->add_subwindow(nextedit = new EditNextEdit(mwindow, 
			this, 
			x1, 
			y1,
			is_mwindow));
		x1 += nextedit->get_w();
	}

	if(use_fit)
	{
		subwindow->add_subwindow(fit = new EditFit(mwindow, this, x1, y1));
		x1 += fit->get_w();
		subwindow->add_subwindow(fit_autos = new EditFitAutos(mwindow, this, x1, y1));
		x1 += fit_autos->get_w();
	}

	if(use_undo)
	{
		subwindow->add_subwindow(undo = new EditUndo(mwindow, this, x1, y1));
		x1 += undo->get_w();
		subwindow->add_subwindow(redo = new EditRedo(mwindow, this, x1, y1));
		x1 += redo->get_w();
	}
SET_TRACE
}



void EditPanel::toggle_label()
{
	mwindow->toggle_label(is_mwindow);
}

void EditPanel::prev_label()
{
	int shift_down = subwindow->shift_down();
	if(is_mwindow)
	{
		mwindow->gui->unlock_window();
	}
	else
		subwindow->unlock_window();

	mwindow->gui->mbuttons->transport->handle_transport(STOP, 1.0, 1, 0, 0);

	if(!is_mwindow)
		subwindow->lock_window("EditPanel::prev_label 1");

	mwindow->gui->lock_window("EditPanel::prev_label 2");

	mwindow->prev_label(shift_down);

	if(!is_mwindow)
		mwindow->gui->unlock_window();
}

void EditPanel::next_label()
{
	int shift_down = subwindow->shift_down();
	if(is_mwindow)
	{
		mwindow->gui->unlock_window();
	}
	else
		subwindow->unlock_window();

	mwindow->gui->mbuttons->transport->handle_transport(STOP, 1.0, 1, 0, 0);

	if(!is_mwindow)
		subwindow->lock_window("EditPanel::next_label 1");

	mwindow->gui->lock_window("EditPanel::next_label 2");

	mwindow->next_label(shift_down);

	if(!is_mwindow)
		mwindow->gui->unlock_window();
}



void EditPanel::prev_edit()
{
	int shift_down = subwindow->shift_down();
	if(is_mwindow)
	{
		mwindow->gui->unlock_window();
	}
	else
		subwindow->unlock_window();

	mwindow->gui->mbuttons->transport->handle_transport(STOP, 1.0, 1, 0, 0);

	if(!is_mwindow)
		subwindow->lock_window("EditPanel::prev_edit 1");

	mwindow->gui->lock_window("EditPanel::prev_edit 2");

	mwindow->prev_edit_handle(shift_down);

	if(!is_mwindow)
		mwindow->gui->unlock_window();
}

void EditPanel::next_edit()
{
	int shift_down = subwindow->shift_down();
	if(is_mwindow)
	{
		mwindow->gui->unlock_window();
	}
	else
		subwindow->unlock_window();

	mwindow->gui->mbuttons->transport->handle_transport(STOP, 1.0, 1, 0, 0);

	if(!is_mwindow)
		subwindow->lock_window("EditPanel::next_edit 1");

	mwindow->gui->lock_window("EditPanel::next_edit 2");

	mwindow->next_edit_handle(shift_down);

	if(!is_mwindow)
		mwindow->gui->unlock_window();
}







void EditPanel::reposition_buttons(int x, int y)
{
	this->x = x; 
	this->y = y;
	x1 = x, y1 = y;

	if(use_editing_mode)
	{
		arrow->reposition_window(x1, y1);
		x1 += arrow->get_w();
		ibeam->reposition_window(x1, y1);
		x1 += ibeam->get_w();
	}

	if(use_keyframe)
	{
		keyframe->reposition_window(x1, y1);
		x1 += keyframe->get_w();
		x1 += mwindow->theme->toggle_margin;
	}

	inpoint->reposition_window(x1, y1);
	x1 += inpoint->get_w();
	outpoint->reposition_window(x1, y1);
	x1 += outpoint->get_w();
	if(use_splice)
	{
		splice->reposition_window(x1, y1);
		x1 += splice->get_w();
	}
	if(use_overwrite)
	{
		overwrite->reposition_window(x1, y1);
		x1 += overwrite->get_w();
	}
	if(use_lift)
	{
		lift->reposition_window(x1, y1);
		x1 += lift->get_w();
	}
	if(use_extract)
	{
		extract->reposition_window(x1, y1);
		x1 += extract->get_w();
	}
	if(use_toclip)
	{
		clip->reposition_window(x1, y1);
		x1 += clip->get_w();
	}
	if(use_cut)
	{
		cut->reposition_window(x1, y1);
		x1 += cut->get_w();
	}
	if(use_copy)
	{
		copy->reposition_window(x1, y1);
		x1 += copy->get_w();
	}
	if(use_paste)
	{
		paste->reposition_window(x1, y1);
		x1 += paste->get_w();
	}

    if(use_razor)
    {
        razor->reposition_window(x1, y1);
        x1 += paste->get_w();
    }

	if(use_meters)
	{
		meters->reposition_window(x1, y1);
		x1 += meters->get_w();
	}

	if(use_labels)
	{
		labelbutton->reposition_window(x1, y1);
		x1 += labelbutton->get_w();
        label_color->reposition_window(x1, 
            y1 + (labelbutton->get_h() - BC_PopupMenu::calculate_h()) / 2);
        x1 += label_color->get_w();
		prevlabel->reposition_window(x1, y1);
		x1 += prevlabel->get_w();
		nextlabel->reposition_window(x1, y1);
		x1 += nextlabel->get_w();
	}

	if(prevedit) 
	{
		prevedit->reposition_window(x1, y1);
		x1 += prevedit->get_w();
	}
	
	if(nextedit)
	{
		nextedit->reposition_window(x1, y1);
		x1 += nextedit->get_w();
	}

	if(use_fit)
	{
		fit->reposition_window(x1, y1);
		x1 += fit->get_w();
		fit_autos->reposition_window(x1, y1);
		x1 += fit_autos->get_w();
	}

	if(use_undo)
	{
		undo->reposition_window(x1, y1);
		x1 += undo->get_w();
		redo->reposition_window(x1, y1);
		x1 += redo->get_w();
	}
}



void EditPanel::create_objects()
{
	create_buttons();
}

int EditPanel::get_w()
{
	return x1 - x;
}


void EditPanel::copy_selection()
{
	mwindow->copy();
}

void EditPanel::splice_selection()
{
}

void EditPanel::overwrite_selection()
{
}

void EditPanel::set_inpoint()
{
	mwindow->set_inpoint(1);
}

void EditPanel::set_outpoint()
{
	mwindow->set_outpoint(1);
}

void EditPanel::clear_inpoint()
{
	mwindow->delete_inpoint();
}

void EditPanel::clear_outpoint()
{
	mwindow->delete_outpoint();
}

void EditPanel::to_clip()
{
	subwindow->unlock_window();
	mwindow->to_clip();
	subwindow->lock_window("EditPanel::to_clip");
}


EditInPoint::EditInPoint(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("inbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("In point ( [ )"));
}
EditInPoint::~EditInPoint()
{
}
int EditInPoint::handle_event()
{
	panel->set_inpoint();
	return 1;
}
int EditInPoint::keypress_event()
{
	if(get_keypress() == '[') 
	{
		panel->set_inpoint();
		return 1;
	}
	return 0;
}

EditOutPoint::EditOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("outbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Out point ( ] )"));
}
EditOutPoint::~EditOutPoint()
{
}
int EditOutPoint::handle_event()
{
	panel->set_outpoint();
	return 1;
}
int EditOutPoint::keypress_event()
{
	if(get_keypress() == ']') 
	{
		panel->set_outpoint();
		return 1;
	}
	return 0;
}


EditNextLabel::EditNextLabel(MWindow *mwindow, 
	EditPanel *panel, 
	int x, 
	int y,
	int is_mwindow)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextlabel"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	this->is_mwindow = is_mwindow;
	set_tooltip(_("Next label ( ctrl -> )"));
}
EditNextLabel::~EditNextLabel()
{
}
int EditNextLabel::keypress_event()
{
	if(get_keypress() == RIGHT && ctrl_down())
		return handle_event();
	return 0;
}
int EditNextLabel::handle_event()
{
	panel->next_label();
	return 1;
}

EditPrevLabel::EditPrevLabel(MWindow *mwindow, 
	EditPanel *panel, 
	int x, 
	int y,
	int is_mwindow)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevlabel"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	this->is_mwindow = is_mwindow;
	set_tooltip(_("Previous label ( ctrl <- )"));
}
EditPrevLabel::~EditPrevLabel()
{
}
int EditPrevLabel::keypress_event()
{
	if(get_keypress() == LEFT && ctrl_down())
		return handle_event();
	return 0;
}
int EditPrevLabel::handle_event()
{
	panel->prev_label();
	return 1;
}



EditNextEdit::EditNextEdit(MWindow *mwindow, 
	EditPanel *panel, 
	int x, 
	int y,
	int is_mwindow)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextedit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	this->is_mwindow = is_mwindow;
	set_tooltip(_("Next edit ( alt -> )"));
}
EditNextEdit::~EditNextEdit()
{
}
int EditNextEdit::keypress_event()
{
	if(get_keypress() == RIGHT && alt_down())
		return handle_event();
	return 0;
}
int EditNextEdit::handle_event()
{
	panel->next_edit();
	return 1;
}

EditPrevEdit::EditPrevEdit(MWindow *mwindow, 
	EditPanel *panel, 
	int x, 
	int y,
	int is_mwindow)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevedit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	this->is_mwindow = is_mwindow;
	set_tooltip(_("Previous edit (alt <- )"));
}
EditPrevEdit::~EditPrevEdit()
{
}
int EditPrevEdit::keypress_event()
{
	if(get_keypress() == LEFT && alt_down())
		return handle_event();
	return 0;
}
int EditPrevEdit::handle_event()
{
	panel->prev_edit();
	return 1;
}



EditLift::EditLift(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->lift_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Lift"));
}
EditLift::~EditLift()
{
}
int EditLift::handle_event()
{
	return 1;
}

EditOverwrite::EditOverwrite(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->overwrite_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Overwrite ( b )"));
}
EditOverwrite::~EditOverwrite()
{
}
int EditOverwrite::handle_event()
{
	panel->overwrite_selection();
	return 1;
}
int EditOverwrite::keypress_event()
{
	if(get_keypress() == 'b')
	{
		handle_event();
		return 1;
	}
	return 0;
}

EditExtract::EditExtract(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->extract_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Extract"));
}
EditExtract::~EditExtract()
{
}
int EditExtract::handle_event()
{
//	mwindow->extract_selection();
	return 1;
}

EditToClip::EditToClip(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("toclip"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("To clip ( i )"));
}
EditToClip::~EditToClip()
{
}
int EditToClip::handle_event()
{
	panel->to_clip();
	return 1;
}

int EditToClip::keypress_event()
{
	if(get_keypress() == 'i')
	{
		handle_event();
		return 1;
	}
	return 0;
}

EditSplice::EditSplice(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->splice_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Splice ( v )"));
}
EditSplice::~EditSplice()
{
}
int EditSplice::handle_event()
{
	panel->splice_selection();
	return 1;
}
int EditSplice::keypress_event()
{
	if(get_keypress() == 'v')
	{
		handle_event();
		return 1;
	}
	return 0;
}

EditCut::EditCut(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("cut"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Cut ( x )"));
}
EditCut::~EditCut()
{
}
int EditCut::keypress_event()
{
	if(get_keypress() == 'x')
		return handle_event();
	return 0;
}

int EditCut::handle_event()
{
	if(!panel->is_mwindow) mwindow->gui->lock_window("EditCut::handle_event");
	mwindow->cut();
	if(!panel->is_mwindow) mwindow->gui->unlock_window();
	return 1;
}

EditCopy::EditCopy(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("copy"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Copy ( c )"));
}
EditCopy::~EditCopy()
{
}

int EditCopy::keypress_event()
{
	if(get_keypress() == 'c')
		return handle_event();
	return 0;
}
int EditCopy::handle_event()
{
	panel->copy_selection();
	return 1;
}

EditAppend::EditAppend(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->append_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Append to end of track"));
}
EditAppend::~EditAppend()
{
}


int EditAppend::handle_event()
{
	return 1;
}


EditInsert::EditInsert(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->insert_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Insert before beginning of track"));
}
EditInsert::~EditInsert()
{
}


int EditInsert::handle_event()
{
	
	return 1;
}


EditPaste::EditPaste(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("paste"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Paste ( v )"));
}
EditPaste::~EditPaste()
{
}

int EditPaste::keypress_event()
{
	if(get_keypress() == 'v')
		return handle_event();
	return 0;
}
int EditPaste::handle_event()
{
	if(!panel->is_mwindow) mwindow->gui->lock_window("EditPaste::handle_event");
	mwindow->paste();
	if(!panel->is_mwindow) mwindow->gui->unlock_window();
	return 1;
}



EditRazor::EditRazor(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("razor"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Razor"));
}

int EditRazor::handle_event()
{
	if(!panel->is_mwindow) mwindow->gui->lock_window("EditRazor::handle_event");
	mwindow->razor();
	if(!panel->is_mwindow) mwindow->gui->unlock_window();
	return 1;
}




EditTransition::EditTransition(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->transition_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Set transition"));
}
EditTransition::~EditTransition()
{
}
int EditTransition::handle_event()
{
	return 1;
}

EditPresentation::EditPresentation(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->presentation_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Set presentation up to current position"));
}
EditPresentation::~EditPresentation()
{
}
int EditPresentation::handle_event()
{
	return 1;
}

EditUndo::EditUndo(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("undo"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Undo ( z )"));
}
EditUndo::~EditUndo()
{
}
int EditUndo::keypress_event()
{
	if(get_keypress() == 'z')
		return handle_event();
	return 0;
}
int EditUndo::handle_event()
{
	mwindow->undo_entry(panel->subwindow);
	return 1;
}

EditRedo::EditRedo(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("redo"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Redo ( shift Z )"));
}
EditRedo::~EditRedo()
{
}
int EditRedo::keypress_event()
{
	if(get_keypress() == 'Z')
		return handle_event();
	return 0;
}
int EditRedo::handle_event()
{
	mwindow->redo_entry(panel->subwindow);
	return 1;
};





EditLabelbutton::EditLabelbutton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("labelbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Toggle label at current position ( l )"));
}

EditLabelbutton::~EditLabelbutton()
{
}
int EditLabelbutton::keypress_event()
{
	if(get_keypress() == 'l')
		return handle_event();
	return 0;
}
int EditLabelbutton::handle_event()
{
	panel->toggle_label();
	return 1;
}


LabelColor::LabelColor(EditPanel *panel, int x, int y)
 : BC_PopupMenu(x, 
 	y,
	panel->label_colors[0]->get_w() + 
        BC_WindowBase::get_resources()->popupmenu_margin * 2 +
        BC_WindowBase::get_resources()->popupmenu_triangle_margin,
    "",
	1,
	MWindow::theme->get_image_set("mode_popup", 0),
	10)
{
    this->panel = panel;
    set_icon(panel->label_colors[MWindow::session->label_color]);
    set_tooltip(_("Label color"));
}

void LabelColor::create_objects()
{
    for(int i = 0; i < LABEL_COLORS; i++)
        add_item(new LabelColorItem(this, i));
}

int LabelColor::handle_event()
{
    MWindow::instance->update_edit_panels();
    return 1;
}

void LabelColor::update()
{
    set_icon(panel->label_colors[MWindow::session->label_color]);
}


LabelColorItem::LabelColorItem(LabelColor *popup, int color)
 : BC_MenuItem(popup->panel->label_colors[color])
{
    this->popup = popup;
    this->color = color;
}

int LabelColorItem::handle_event()
{
    MWindow::session->label_color = color;
    popup->handle_event();
    return 1;
}






EditFit::EditFit(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Fit selection to display ( f )"));
}
EditFit::~EditFit()
{
}
int EditFit::keypress_event()
{
	if(!alt_down() && get_keypress() == 'f') 
	{
		handle_event();
		return 1;
	}
	return 0;
}
int EditFit::handle_event()
{
	mwindow->fit_selection();
	return 1;
}









EditFitAutos::EditFitAutos(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fitautos"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Fit autos to display ( Alt + f )"));
}
EditFitAutos::~EditFitAutos()
{
}
int EditFitAutos::keypress_event()
{
	if(alt_down() && get_keypress() == 'f') 
	{
		handle_event();
		return 1;
	}
	return 0;
}
int EditFitAutos::handle_event()
{
	mwindow->fit_autos();
	return 1;
}













ArrowButton::ArrowButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, 
 	y, 
	mwindow->theme->get_image_set("arrow"),
	mwindow->edl->session->editing_mode == EDITING_ARROW,
	"",
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Drag and drop editing mode"));
}

int ArrowButton::handle_event()
{
	update(1);
	panel->ibeam->update(0);
	mwindow->set_editing_mode(EDITING_ARROW, 
		!panel->is_mwindow, 
		panel->is_mwindow);
// Nothing after this
	return 1;
}


IBeamButton::IBeamButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, 
 	y, 
	mwindow->theme->get_image_set("ibeam"),
	mwindow->edl->session->editing_mode == EDITING_IBEAM,
	"",
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Cut and paste editing mode"));
}

int IBeamButton::handle_event()
{
	update(1);
	panel->arrow->update(0);
	mwindow->set_editing_mode(EDITING_IBEAM, 
		!panel->is_mwindow, 
		panel->is_mwindow);
// Nothing after this
	return 1;
}

KeyFrameButton::KeyFrameButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, 
 	y, 
	mwindow->theme->get_image_set("autokeyframe"),
	mwindow->edl->session->auto_keyframes,
	"",
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Generate keyframes while tweeking"));
}

int KeyFrameButton::handle_event()
{
	mwindow->set_auto_keyframes(get_value(), 
		!panel->is_mwindow, 
		panel->is_mwindow);
	return 1;
}


