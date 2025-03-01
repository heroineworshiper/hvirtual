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

#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "formattools.h"
#include "language.h"
#include "mwindow.h"
#include "performanceprefs.h"
#include "preferences.h"
#include <string.h>
#include "theme.h"

#define MASTER_NODE_FRAMERATE_TEXT "Master node framerate: %0.3f"
#if 0
N_(MASTER_NODE_FRAMERATE_TEXT)
#endif

static int *widths = 0;

PerformancePrefs::PerformancePrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	hot_node = -1;
}

PerformancePrefs::~PerformancePrefs()
{
	delete brender_tools;
	nodes[0].remove_all_objects();
	nodes[1].remove_all_objects();
	nodes[2].remove_all_objects();
	nodes[3].remove_all_objects();
    
    delete edit_port;
    delete brender_fragment;
    delete preroll;
    delete bpreroll;
    delete jobs;
}

void PerformancePrefs::create_objects()
{
	int x, y;
	int xmargin1;
	int xmargin2 = DP(170);
	int xmargin3 = DP(250);
	int xmargin4 = DP(380);
	char string[BCTEXTLEN];
	BC_Resources *resources = BC_WindowBase::get_resources();
	int margin = mwindow->theme->widget_border;

	if(!widths)
	{
		widths = new int[4];
		widths[0] = DP(30);
		widths[1] = DP(150);
		widths[2] = DP(50);
		widths[3] = DP(50);
	}

	node_list = 0;
	generate_node_list();

	xmargin1 = x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;
	
// 	add_subwindow(new BC_Title(x, 
// 		y, 
// 		_("Time Format"), 
// 		LARGEFONT, 
// 		resources->text_default));
// 
// 	y += get_text_height(LARGEFONT) + margin;

	add_subwindow(new BC_Title(x, y + margin, _("Cache size (MB):"), MEDIUMFONT, resources->text_default));
	cache_size = new CICacheSize(x + DP(230), 
		y, 
		pwindow, 
		this);
	cache_size->create_objects();

	y += DP(30);
	add_subwindow(new BC_Title(x, y + margin, _("Seconds to preroll renders:")));
	preroll = new PrefsRenderPreroll(pwindow, 
		this, 
		x + DP(230), 
		y);
	preroll->create_objects();
	y += DP(30);
    PrefsForceUniprocessor *toggle;
	add_subwindow(toggle = new PrefsForceUniprocessor(pwindow, x, y));

	y += toggle->get_h() + margin;


	add_subwindow(gl_rendering = new PrefsGLRendering(pwindow, this, x, y));
    y += gl_rendering->get_h() + margin;



// Background rendering
	add_subwindow(new BC_Bar(margin, y, 	get_w() - margin * 2));
	y += margin;


	add_subwindow(new BC_Title(x, y, _("Background Rendering (Video only)"), LARGEFONT, resources->text_default));
	y += DP(30);
	int y2 = y;

	add_subwindow(new PrefsUseBRender(pwindow, 
		x,
		y));

	y += DP(40);
	add_subwindow(new BC_Title(x, y, _("Frames per background rendering job:")));
	y += DP(20);
	brender_fragment = new PrefsBRenderFragment(pwindow, 
		this, 
		x, 
		y);
	brender_fragment->create_objects();
	
	
	y += DP(30);
	add_subwindow(new BC_Title(x, y + DP(5), _("Frames to preroll background:")));
	bpreroll = new PrefsBRenderPreroll(pwindow, 
		this, 
		x + xmargin3, 
		y + margin);
	bpreroll->create_objects();
	y += DP(30);

	x += xmargin4;
	add_subwindow(new BC_Title(x, y2, _("Output for background rendering:")));
	y2 += DP(20);


	brender_tools = 
		new FormatTools(mwindow,
			this, 
			pwindow->thread->preferences->brender_asset);
	brender_tools->set_w(get_w() - x);
	brender_tools->create_objects(x, 
		y2, 
		0, // Include tools for audio
		1, // Include tools for video
		0, // Include checkbox for audio
		0, // Include checkbox for video
		1, // prompt_video_compression
        0, // prompt_wrapper
		0, // Select compressors to be offered
		0, // Prompt for recording options
		0, // If nonzero, prompt for insertion strategy
		1); // Supply only file formats for background rendering
	x = xmargin1;
	
	if(y2 > y)
	{
		y = y2;
	}




// Renderfarm
	add_subwindow(new BC_Bar(margin, y, get_w() - margin * 2));
	y += margin;


	add_subwindow(new BC_Title(x, y, _("Render Farm"), LARGEFONT, resources->text_default));
	y += DP(25);

	add_subwindow(use_renderfarm = new PrefsRenderFarm(pwindow, this, x, y));
	add_subwindow(new BC_Title(x + xmargin4, y, _("Nodes:")));
	y += DP(30);
	add_subwindow(new BC_Title(x, y, _("Hostname:")));
	add_subwindow(new BC_Title(x + xmargin3, y, _("Port:")));
	add_subwindow(node_list = new PrefsRenderFarmNodes(pwindow, 
		this, 
		x + xmargin4, 
		y - margin));
	sprintf(string, _(MASTER_NODE_FRAMERATE_TEXT), 
		pwindow->thread->preferences->local_rate);
	add_subwindow(master_rate = new BC_Title(x + xmargin4, y + node_list->get_h(), string));

	y += DP(25);
	add_subwindow(edit_node = new PrefsRenderFarmEditNode(pwindow, 
		this, 
		x, 
		y));
	edit_port = new PrefsRenderFarmPort(pwindow, 
		this, 
		x + xmargin3, 
		y);
	edit_port->create_objects();

	y += DP(30);


	add_subwindow(new PrefsRenderFarmReplaceNode(pwindow, 
		this, 
		x, 
		y));
	add_subwindow(new PrefsRenderFarmNewNode(pwindow, 
		this, 
		x + xmargin2, 
		y));
	y += DP(30);
	add_subwindow(new PrefsRenderFarmDelNode(pwindow, 
		this, 
		x + xmargin2, 
		y));
	add_subwindow(new PrefsRenderFarmSortNodes(pwindow, 
		this, 
		x, 
		y));
	y += DP(30);
	add_subwindow(new PrefsRenderFarmReset(pwindow, 
		this, 
		x, 
		y));
	y += DP(35);
	add_subwindow(new BC_Title(x, 
		y, 
		_("Total jobs to create:")));
	add_subwindow(new BC_Title(x, 
		y + DP(30), 
		_("(overridden if creating a\nnew file at each label)")));
	jobs = new PrefsRenderFarmJobs(pwindow, 
		this, 
		x + xmargin3, 
		y);
	jobs->create_objects();
	y += DP(55);
// 	add_subwindow(new PrefsRenderFarmVFS(pwindow,
// 		this,
// 		x,
// 		y));
// 	add_subwindow(new BC_Title(x, 
// 		y, 
// 		_("Filesystem prefix on remote nodes:")));
// 	add_subwindow(new PrefsRenderFarmMountpoint(pwindow, 
// 		this, 
// 		x + xmargin3, 
// 		y));
// 	y += 30;
    update_enabled();
}

void PerformancePrefs::update_enabled()
{
    if(pwindow->thread->preferences->use_gl_rendering)
    {
        use_renderfarm->update(0);
        use_renderfarm->disable();
    }
    else
    {
        use_renderfarm->update(pwindow->thread->preferences->use_renderfarm);
        use_renderfarm->enable();
    }
}

void PerformancePrefs::generate_node_list()
{
	int selected_row = node_list ? node_list->get_selection_number(0, 0) : -1;
	
	for(int i = 0; i < TOTAL_COLUMNS; i++)
		nodes[i].remove_all_objects();

	for(int i = 0; 
		i < pwindow->thread->preferences->renderfarm_nodes.size(); 
		i++)
	{
		BC_ListBoxItem *item;
		nodes[ENABLED_COLUMN].append(item = new BC_ListBoxItem(
			(char*)(pwindow->thread->preferences->renderfarm_enabled.get(i) ? "X" : " ")));
		if(i == selected_row) item->set_selected(1);

		nodes[HOSTNAME_COLUMN].append(item = new BC_ListBoxItem(
			pwindow->thread->preferences->renderfarm_nodes.get(i)));
		if(i == selected_row) item->set_selected(1);

		char string[BCTEXTLEN];
		sprintf(string, "%d", pwindow->thread->preferences->renderfarm_ports.get(i));
		nodes[PORT_COLUMN].append(item = new BC_ListBoxItem(string));
		if(i == selected_row) item->set_selected(1);

		sprintf(string, "%0.3f", pwindow->thread->preferences->renderfarm_rate.get(i));
		nodes[RATE_COLUMN].append(item = new BC_ListBoxItem(string));
		if(i == selected_row) item->set_selected(1);
	}
}

static const char *titles[] = 
{
	N_("On"),
	N_("Hostname"),
	N_("Port"),
	N_("Framerate")
};


void PerformancePrefs::update_node_list()
{
	node_list->update(nodes,
						titles,
						widths,
						TOTAL_COLUMNS,
						node_list->get_xposition(),
						node_list->get_yposition(),
						node_list->get_selection_number(0, 0));
}


void PerformancePrefs::update_rates()
{
//printf("PerformancePrefs::update_rates %d\n", __LINE__);
	char string[BCTEXTLEN];
	for(int i = 0; 
		i < mwindow->preferences->renderfarm_rate.size(); 
		i++)
	{
		if(i < nodes[RATE_COLUMN].size())
		{
			sprintf(string, "%0.3f", mwindow->preferences->renderfarm_rate.get(i));
			nodes[RATE_COLUMN].get(i)->set_text(string);
		}
	}
	
	sprintf(string, _(MASTER_NODE_FRAMERATE_TEXT), 
		mwindow->preferences->local_rate);
	master_rate->update(string);
	
	update_node_list();
}


PrefsUseBRender::PrefsUseBRender(PreferencesWindow *pwindow, 
	int x,
	int y)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->preferences->use_brender, 
	_("Use background rendering"))
{
	this->pwindow = pwindow;
}

int PrefsUseBRender::handle_event()
{
	pwindow->thread->redraw_overlays = 1;
	pwindow->thread->redraw_times = 1;
	pwindow->thread->preferences->use_brender = get_value();
	return 1;
}






PrefsBRenderFragment::PrefsBRenderFragment(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_TumbleTextBox(subwindow, 
 	(int64_t)pwindow->thread->preferences->brender_fragment,
	(int64_t)1, 
	(int64_t)65535,
	x,
	y,
	DP(100))
{
	this->pwindow = pwindow;
}
int PrefsBRenderFragment::handle_event()
{
	pwindow->thread->preferences->brender_fragment = atol(get_text());
	return 1;
}











CICacheSize::CICacheSize(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow)
 : BC_TumbleTextBox(subwindow,
 	(int64_t)pwindow->thread->preferences->cache_size / 0x100000,
	(int64_t)MIN_CACHE_SIZE / 0x100000,
	(int64_t)MAX_CACHE_SIZE / 0x100000,
	x, 
	y, 
	DP(100))
{ 
	this->pwindow = pwindow;
	set_increment(1);
}

int CICacheSize::handle_event()
{
	int64_t result;
	result = (int64_t)atol(get_text()) * 0x100000;
	CLAMP(result, MIN_CACHE_SIZE, MAX_CACHE_SIZE);
	pwindow->thread->preferences->cache_size = result;
	return 0;
}


PrefsRenderPreroll::PrefsRenderPreroll(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y)
 : BC_TumbleTextBox(subwindow, 
 	(float)pwindow->thread->preferences->render_preroll,
	(float)0, 
	(float)100,
	x,
	y,
	DP(100))
{
	this->pwindow = pwindow;
	set_increment(0.1);
}
PrefsRenderPreroll::~PrefsRenderPreroll()
{
}
int PrefsRenderPreroll::handle_event()
{
	pwindow->thread->preferences->render_preroll = atof(get_text());
	return 1;
}


PrefsBRenderPreroll::PrefsBRenderPreroll(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y)
 : BC_TumbleTextBox(subwindow, 
 	(int64_t)pwindow->thread->preferences->brender_preroll,
	(int64_t)0, 
	(int64_t)100,
	x,
	y,
	DP(100))
{
	this->pwindow = pwindow;
}
int PrefsBRenderPreroll::handle_event()
{
	pwindow->thread->preferences->brender_preroll = atol(get_text());
	return 1;
}








PrefsGLRendering::PrefsGLRendering(PreferencesWindow *pwindow, 
    PerformancePrefs *subwindow, 
    int x, 
    int y)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->preferences->use_gl_rendering,
	_("Use OpenGL for rendering"))
{
	this->pwindow = pwindow;
    this->subwindow = subwindow;
}
int PrefsGLRendering::handle_event()
{
	pwindow->thread->preferences->use_gl_rendering = get_value();
    subwindow->update_enabled();
	return 1;
}






PrefsRenderFarm::PrefsRenderFarm(PreferencesWindow *pwindow, 
    PerformancePrefs *subwindow, 
    int x, 
    int y)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->preferences->use_renderfarm,
	_("Use render farm"))
{
	this->pwindow = pwindow;
    this->subwindow = subwindow;
}
int PrefsRenderFarm::handle_event()
{
	pwindow->thread->preferences->use_renderfarm = get_value();
	return 1;
}




PrefsForceUniprocessor::PrefsForceUniprocessor(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->preferences->force_uniprocessor,
	_("Force single processor use"))
{
	this->pwindow = pwindow;
}
PrefsForceUniprocessor::~PrefsForceUniprocessor()
{
}
int PrefsForceUniprocessor::handle_event()
{
	pwindow->thread->preferences->force_uniprocessor = get_value();
	return 1;
}







PrefsRenderFarmConsolidate::PrefsRenderFarmConsolidate(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->preferences->renderfarm_consolidate,
	_("Consolidate output files on completion"))
{
	this->pwindow = pwindow;
}
PrefsRenderFarmConsolidate::~PrefsRenderFarmConsolidate()
{
}
int PrefsRenderFarmConsolidate::handle_event()
{
	pwindow->thread->preferences->renderfarm_consolidate = get_value();
	return 1;
}





PrefsRenderFarmPort::PrefsRenderFarmPort(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_TumbleTextBox(subwindow, 
 	(int64_t)pwindow->thread->preferences->renderfarm_port,
	(int64_t)1, 
	(int64_t)65535,
	x,
	y,
	DP(100))
{
	this->pwindow = pwindow;
}

PrefsRenderFarmPort::~PrefsRenderFarmPort()
{
}

int PrefsRenderFarmPort::handle_event()
{
	pwindow->thread->preferences->renderfarm_port = atol(get_text());
	return 1;
}



PrefsRenderFarmNodes::PrefsRenderFarmNodes(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_ListBox(x, 
		y, 
		DP(340), 
		DP(230),
		LISTBOX_TEXT,                         // Display text list or icons
		subwindow->nodes,
		titles,
		widths,
		4)
{
	this->subwindow = subwindow;
	this->pwindow = pwindow;
}
PrefsRenderFarmNodes::~PrefsRenderFarmNodes()
{
}

int PrefsRenderFarmNodes::column_resize_event()
{
	for(int i = 0; i < 3; i++)
		widths[i] = get_column_width(i);
	return 1;
}

int PrefsRenderFarmNodes::handle_event()
{
SET_TRACE
	if(get_selection_number(0, 0) >= 0)
	{
		subwindow->hot_node = get_selection_number(1, 0);
		subwindow->edit_node->update(get_selection(1, 0)->get_text());
		subwindow->edit_port->update(get_selection(2, 0)->get_text());
		if(get_cursor_x() < widths[0])
		{
			pwindow->thread->preferences->renderfarm_enabled.values[subwindow->hot_node] = 
				!pwindow->thread->preferences->renderfarm_enabled.values[subwindow->hot_node];
			subwindow->generate_node_list();
			subwindow->update_node_list();
		}
	}
	else
	{
		subwindow->hot_node = -1;
		subwindow->edit_node->update("");
	}
SET_TRACE
	return 1;
}	
int PrefsRenderFarmNodes::selection_changed()
{
	handle_event();
	return 1;
}







PrefsRenderFarmEditNode::PrefsRenderFarmEditNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y)
 : BC_TextBox(x, y, DP(240), 1, "")
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

PrefsRenderFarmEditNode::~PrefsRenderFarmEditNode()
{
}

int PrefsRenderFarmEditNode::handle_event()
{
	return 1;
}






PrefsRenderFarmNewNode::PrefsRenderFarmNewNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y)
 : BC_GenericButton(x, y, _("Add Node"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}
PrefsRenderFarmNewNode::~PrefsRenderFarmNewNode()
{
}
int PrefsRenderFarmNewNode::handle_event()
{
	pwindow->thread->preferences->add_node(subwindow->edit_node->get_text(),
		pwindow->thread->preferences->renderfarm_port,
		1,
		0.0);
	pwindow->thread->preferences->reset_rates();
	subwindow->generate_node_list();
	subwindow->update_node_list();
	subwindow->hot_node = -1;
	return 1;
}







PrefsRenderFarmReplaceNode::PrefsRenderFarmReplaceNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y)
 : BC_GenericButton(x, y, _("Apply Changes"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}
PrefsRenderFarmReplaceNode::~PrefsRenderFarmReplaceNode()
{
}
int PrefsRenderFarmReplaceNode::handle_event()
{
	if(subwindow->hot_node >= 0)
	{
		pwindow->thread->preferences->edit_node(subwindow->hot_node, 
			subwindow->edit_node->get_text(),
			pwindow->thread->preferences->renderfarm_port,
			pwindow->thread->preferences->renderfarm_enabled.values[subwindow->hot_node]);
		subwindow->generate_node_list();
		subwindow->update_node_list();
	}
	return 1;
}





PrefsRenderFarmDelNode::PrefsRenderFarmDelNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y)
 : BC_GenericButton(x, y, _("Delete Node"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}
PrefsRenderFarmDelNode::~PrefsRenderFarmDelNode()
{
}
int PrefsRenderFarmDelNode::handle_event()
{
	if(strlen(subwindow->edit_node->get_text()) &&
		subwindow->hot_node >= 0)
	{

		pwindow->thread->preferences->delete_node(subwindow->hot_node);
		
		subwindow->generate_node_list();
		subwindow->update_node_list();
		subwindow->hot_node = -1;
	}
	return 1;
}





PrefsRenderFarmSortNodes::PrefsRenderFarmSortNodes(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Sort nodes"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

PrefsRenderFarmSortNodes::~PrefsRenderFarmSortNodes()
{
}

int PrefsRenderFarmSortNodes::handle_event()
{
	pwindow->thread->preferences->sort_nodes();
	subwindow->generate_node_list();
	subwindow->update_node_list();
	subwindow->hot_node = -1;
	return 1;
}





PrefsRenderFarmReset::PrefsRenderFarmReset(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Reset rates"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmReset::handle_event()
{
	pwindow->thread->preferences->reset_rates();
	subwindow->generate_node_list();
	subwindow->update_node_list();

	char string[BCTEXTLEN];
	sprintf(string, 
		MASTER_NODE_FRAMERATE_TEXT, 
		pwindow->thread->preferences->local_rate);
	subwindow->master_rate->update(string);
	subwindow->hot_node = -1;
	return 1;
}







PrefsRenderFarmJobs::PrefsRenderFarmJobs(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y)
 : BC_TumbleTextBox(subwindow, 
 	(int64_t)pwindow->thread->preferences->renderfarm_job_count,
	(int64_t)1, 
	(int64_t)100,
	x,
	y,
	DP(100))
{
	this->pwindow = pwindow;
}
PrefsRenderFarmJobs::~PrefsRenderFarmJobs()
{
}
int PrefsRenderFarmJobs::handle_event()
{
	pwindow->thread->preferences->renderfarm_job_count = atol(get_text());
	return 1;
}



PrefsRenderFarmMountpoint::PrefsRenderFarmMountpoint(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y)
 : BC_TextBox(x, 
 	y, 
	DP(100),
	1,
	pwindow->thread->preferences->renderfarm_mountpoint)
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}
PrefsRenderFarmMountpoint::~PrefsRenderFarmMountpoint()
{
}
int PrefsRenderFarmMountpoint::handle_event()
{
	strcpy(pwindow->thread->preferences->renderfarm_mountpoint, get_text());
	return 1;
}




PrefsRenderFarmVFS::PrefsRenderFarmVFS(PreferencesWindow *pwindow,
	PerformancePrefs *subwindow,
	int x,
	int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->renderfarm_vfs, _("Use virtual filesystem"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmVFS::handle_event()
{
	pwindow->thread->preferences->renderfarm_vfs = get_value();
	return 1;
}

