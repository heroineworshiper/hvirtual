#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "compressorgui.h"
#include "cursors.h"
#include "eqcanvas.h"
#include "language.h"
#include "theme.h"
#include "units.h"

#include <string.h>

CompressorWindow::CompressorWindow(CompressorEffect *plugin)
 : PluginClientWindow(plugin,
	DP(650), 
	DP(560), 
	DP(650), 
	DP(560),
	0)
{
	this->plugin = plugin;
    
	char string[BCTEXTLEN];
// set the default directory
	sprintf(string, "%scompressormulti.rc", BCASTDIR);
    defaults = new BC_Hash(string);
	defaults->load();
    plugin->config.current_band = defaults->get("CURRENT_BAND", plugin->config.current_band);

}

CompressorWindow::~CompressorWindow()
{
	defaults->update("CURRENT_BAND", plugin->config.current_band);
	defaults->save();
    delete defaults;

    delete eqcanvas;
    delete reaction;
    delete x_text;
    delete y_text;
    delete trigger;
    delete decay;
}


void CompressorWindow::create_objects()
{
    int margin = client->get_theme()->widget_border;
	int x = DP(35), y = margin;
	int control_margin = DP(150);
    BC_Title *title;
    BandConfig *band_config = &plugin->config.bands[plugin->config.current_band];
    BandConfig *prev_band = 0;
    if(plugin->config.current_band > 0)
    {
        prev_band = &plugin->config.bands[plugin->config.current_band - 1];
    }

    add_subwindow(title = new BC_Title(margin, y, _("Current band:")));
    
    int x1 = title->get_x() + title->get_w() + margin;
    char string[BCTEXTLEN];
    for(int i = 0; i < TOTAL_BANDS; i++)
    {
        sprintf(string, "%d", i + 1);
        add_subwindow(band[i] = new CompressorBand(this, 
            plugin, 
            x1, 
            y,
            i,
            string));
        x1 += band[i]->get_w() + margin;
    }
    
    y += band[0]->get_h() + 1;


    add_subwindow(title = new BC_Title(margin, y, _("Sound level (Press shift to snap to grid):")));
    y += title->get_h() + 1;
	add_subwindow(canvas = new CompressorCanvas(plugin, 
        this,
		x, 
		y, 
		get_w() - x - control_margin - DP(10), 
		get_h() * 2 / 3 - y));
    y += canvas->get_h() + DP(30);
    
    add_subwindow(title = new BC_Title(margin, y, _("Bandwidth:")));
    y += title->get_h();
    eqcanvas = new EQCanvas(this,
        margin,
        y,
        canvas->get_w() + x - margin,
        get_h() - y - margin,
        plugin->config.min_db,
        0.0);
    eqcanvas->freq_divisions = 10;
    eqcanvas->initialize();
    
    
	x = get_w() - control_margin;
    y = margin;
	add_subwindow(title = new BC_Title(x, y, _("Attack secs:")));
	y += title->get_h();
	reaction = new CompressorReaction(plugin, this, x, y);
    reaction->create_objects();
	y += reaction->get_h() + margin;
    
    
	add_subwindow(title = new BC_Title(x, y, _("Release secs:")));
	y += title->get_h();
	decay = new CompressorDecay(plugin, this, x, y);
    decay->create_objects();
	y += decay->get_h() + margin;
    
    
	add_subwindow(title = new BC_Title(x, y, _("Trigger Type:")));
	y += title->get_h();
	add_subwindow(input = new CompressorInput(plugin, x, y));
	input->create_objects();
	y += input->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Trigger:")));
	y += title->get_h();
    
    
	trigger = new CompressorTrigger(plugin, this, x, y);
    trigger->create_objects();
	if(plugin->config.input != CompressorConfig::TRIGGER) trigger->disable();
	y += trigger->get_h() + margin;


	add_subwindow(smooth = new CompressorSmooth(plugin, x, y));
    y += smooth->get_h() + margin;
	add_subwindow(solo = new CompressorSolo(plugin, x, y));
    y += solo->get_h() + margin;
	add_subwindow(bypass = new CompressorBypass(plugin, x, y));
    y += bypass->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Output:")));
    y += title->get_h();
    
    
	y_text = new CompressorY(plugin, this, x, y);
    y_text->create_objects();
    y += y_text->get_h() + margin;
    
    
	add_subwindow(title = new BC_Title(x, y, _("Input:")));
    y += title->get_h();
	x_text = new CompressorX(plugin, this, x, y);
    x_text->create_objects();
    y += x_text->get_h() + margin;

    
	add_subwindow(clear = new CompressorClear(plugin, x, y));
    y += clear->get_h() + margin;

    add_subwindow(title = new BC_Title(x, y, _("Freq range:")));
    y += title->get_h();

// the previous high frequency
    int *ptr = 0;
    if(prev_band)
    {
        ptr = &prev_band->freq;
    }
    
    add_subwindow(freq1 = new CompressorQPot(this, 
        plugin, 
        get_w() - (margin + BC_Pot::calculate_w()) * 2, 
        y, 
        ptr));

// the current high frequency
    ptr = &band_config->freq;
    if(plugin->config.current_band == TOTAL_BANDS - 1)
    {
        ptr = 0;
    }

    add_subwindow(freq2 = new CompressorQPot(this, 
        plugin, 
        get_w() - margin - BC_Pot::calculate_w(), 
        y, 
        ptr));
    y += freq1->get_h() + margin;


    add_subwindow(title = new BC_Title(x, y, _("Steepness:")));
    add_subwindow(q = new CompressorFPot(this, 
        plugin, 
        get_w() - margin - BC_Pot::calculate_w(), 
        y, 
        &plugin->config.q,
        0,
        1));
    y += q->get_h() + margin;
    
    add_subwindow(title = new BC_Title(x, y, _("Window size:")));
    y += title->get_h() + margin;
    add_subwindow(size = new CompressorSize(this,
        plugin,
        x,
        y));
    size->create_objects();
    size->update(plugin->config.window_size);
    y += size->get_h() + margin;



	canvas->create_objects();
    update_eqcanvas();
	show_window();
}

// called when the user selects a different band
void CompressorWindow::update()
{
    BandConfig *band_config = &plugin->config.bands[plugin->config.current_band];

    for(int i = 0; i < TOTAL_BANDS; i++)
    {
        if(plugin->config.current_band == i)
        {
            band[i]->update(1);
        }
        else
        {
            band[i]->update(0);
        }
    }

    int *ptr = 0;
    if(plugin->config.current_band > 0)
    {
        ptr = &plugin->config.bands[plugin->config.current_band - 1].freq;
    }
    else
    {
        ptr = 0;
    }

    freq1->output = ptr;
    if(ptr)
    {
        freq1->update(*ptr);
    }

// top band edits the penultimate band
    if(plugin->config.current_band < TOTAL_BANDS - 1)
    {
        ptr = &band_config->freq;
    }
    else
    {
        ptr = 0;
    }

    freq2->output = ptr;
    if(ptr)
    {
        freq2->update(*ptr);
    }

    q->update(plugin->config.q);
    solo->update(band_config->solo);
    bypass->update(band_config->bypass);
    size->update(plugin->config.window_size);

	if(atol(trigger->get_text()) != plugin->config.trigger)
	{
    	trigger->update((int64_t)plugin->config.trigger);
    }
    
	if(strcmp(input->get_text(), CompressorInput::value_to_text(plugin->config.input)))
	{
    	input->set_text(CompressorInput::value_to_text(plugin->config.input));
    }

	if(plugin->config.input != CompressorConfig::TRIGGER && trigger->get_enabled())
	{
    	trigger->disable();
	}
    else
	if(plugin->config.input == CompressorConfig::TRIGGER && !trigger->get_enabled())
	{
    	trigger->enable();
    }

	if(!EQUIV(atof(reaction->get_text()), band_config->reaction_len))
	{
    	reaction->update((float)band_config->reaction_len);
	}
    if(!EQUIV(atof(decay->get_text()), band_config->decay_len))
	{
    	decay->update((float)band_config->decay_len);
	}
    
    smooth->update(plugin->config.smoothing_only);
	if(canvas->current_operation == CompressorCanvas::DRAG)
	{
		x_text->update((float)band_config->levels.values[canvas->current_point].x);
		y_text->update((float)band_config->levels.values[canvas->current_point].y);
	}
    
    
    

	canvas->update();
    update_eqcanvas();
}




void CompressorWindow::update_eqcanvas()
{
    plugin->calculate_envelope();


// dump envelope sum
//     printf("CompressorWindow::update_eqcanvas %d\n", __LINE__);
//     for(int i = 0; i < plugin->config.window_size / 2; i++)
//     {
//         double sum = 0;
//         for(int band = 0; band < TOTAL_BANDS; band++)
//         {
//             sum += plugin->engines[band]->envelope[i];
//         }
//         
//         printf("%f ", sum);
//         for(int band = 0; band < TOTAL_BANDS; band++)
//         {
//             printf("%f ", plugin->engines[band]->envelope[i]);
//         }
//         printf("\n");
//     }


#ifndef DRAW_AFTER_BANDPASS
    eqcanvas->update_spectrogram(plugin); 
#else
    eqcanvas->update_spectrogram(plugin,
        plugin->config.current_band * plugin->config.window_size / 2,
        TOTAL_BANDS * plugin->config.window_size / 2,
        plugin->config.window_size);
#endif

    
    // draw the active band on top of the others
    for(int pass = 0; pass < 2; pass++)
    {
        for(int band = 0; band < TOTAL_BANDS; band++)
        {
            if(band == plugin->config.current_band && pass == 0 ||
                band != plugin->config.current_band && pass == 1)
            {
                continue;
            }

            eqcanvas->draw_envelope(plugin->engines[band]->envelope,
                plugin->PluginAClient::project_sample_rate,
                plugin->config.window_size,
                band == plugin->config.current_band,
                0);
        }
    }
    eqcanvas->canvas->flash(1);
}

int CompressorWindow::resize_event(int w, int h)
{
	return 1;
}





CompressorFPot::CompressorFPot(CompressorWindow *gui, 
    CompressorEffect *plugin, 
    int x, 
    int y, 
    double *output,
    double min,
    double max)
 : BC_FPot(x,
    y,
    *output,
    min,
    max)
{
    this->gui = gui;
    this->plugin = plugin;
    this->output = output;
    set_precision(0.01);
}



int CompressorFPot::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
    gui->update_eqcanvas();
    return 1;
}






CompressorQPot::CompressorQPot(CompressorWindow *gui, 
    CompressorEffect *plugin, 
    int x, 
    int y, 
    int *output)
 : BC_QPot(x,
    y,
    output ? *output : 0)
{
    this->gui = gui;
    this->plugin = plugin;
    this->output = output;
}


int CompressorQPot::handle_event()
{
    if(output)
    {
        *output = get_value();
        plugin->send_configure_change();
        gui->update_eqcanvas();
    }
    return 1;
}







CompressorSize::CompressorSize(CompressorWindow *gui, 
    CompressorEffect *plugin, 
    int x, 
    int y)
 : BC_PopupMenu(x, 
    y, 
    DP(100), 
    "4096", 
    1)
{
    this->gui = gui;
    this->plugin = plugin;
}

int CompressorSize::handle_event()
{
    plugin->config.window_size = atoi(get_text());
    plugin->send_configure_change();
    gui->update_eqcanvas();
    return 1;
}


void CompressorSize::create_objects()
{
	add_item(new BC_MenuItem("2048"));
	add_item(new BC_MenuItem("4096"));
	add_item(new BC_MenuItem("8192"));
	add_item(new BC_MenuItem("16384"));
	add_item(new BC_MenuItem("32768"));
	add_item(new BC_MenuItem("65536"));
	add_item(new BC_MenuItem("131072"));
	add_item(new BC_MenuItem("262144"));
}


void CompressorSize::update(int size)
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", size);
	set_text(string);
}










CompressorCanvas::CompressorCanvas(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y, 
    int w, 
    int h)
 : CompressorCanvasBase(&plugin->config,
    plugin,
    window,
    x, 
    y, 
    w, 
    h)
{
}

void CompressorCanvas::update_window()
{
    ((CompressorWindow*)window)->update();
}



CompressorReaction::CompressorReaction(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
    (float)plugin->config.bands[plugin->config.current_band].reaction_len,
    (float)MIN_ATTACK,
    (float)MAX_ATTACK,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(0.1);
    set_precision(2);
}

int CompressorReaction::handle_event()
{
	plugin->config.bands[plugin->config.current_band].reaction_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}



CompressorDecay::CompressorDecay(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
    (float)plugin->config.bands[plugin->config.current_band].decay_len,
    (float)MIN_DECAY,
    (float)MAX_DECAY,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(0.1);
    set_precision(2);
}
int CompressorDecay::handle_event()
{
	plugin->config.bands[plugin->config.current_band].decay_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}




CompressorX::CompressorX(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
     (float)0.0,
    plugin->config.min_db,
    plugin->config.max_db,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(0.1);
    set_precision(2);
}
int CompressorX::handle_event()
{
    BandConfig *band_config = &plugin->config.bands[plugin->config.current_band];

	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < band_config->levels.total)
	{
		band_config->levels.values[current_point].x = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->canvas->update();
		plugin->send_configure_change();
	}
	return 1;
}



CompressorY::CompressorY(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
     (float)0.0,
    plugin->config.min_db,
    plugin->config.max_db,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(0.1);
    set_precision(2);
}
int CompressorY::handle_event()
{
    BandConfig *band_config = &plugin->config.bands[plugin->config.current_band];

	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < band_config->levels.total)
	{
		band_config->levels.values[current_point].y = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->canvas->update();
		plugin->send_configure_change();
	}
	return 1;
}





CompressorTrigger::CompressorTrigger(CompressorEffect *plugin, 
    CompressorWindow *window,
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
    (int)plugin->config.trigger,
    MIN_TRIGGER,
    MAX_TRIGGER,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
}
int CompressorTrigger::handle_event()
{
	plugin->config.trigger = atol(get_text());
	plugin->send_configure_change();
	return 1;
}




CompressorInput::CompressorInput(CompressorEffect *plugin, int x, int y) 
 : BC_PopupMenu(x, 
	y, 
	DP(100), 
	CompressorInput::value_to_text(plugin->config.input), 
	1)
{
	this->plugin = plugin;
}
int CompressorInput::handle_event()
{
	plugin->config.input = text_to_value(get_text());
	((CompressorWindow*)plugin->thread->window)->update();
	plugin->send_configure_change();
	return 1;
}

void CompressorInput::create_objects()
{
	for(int i = 0; i < 3; i++)
	{
		add_item(new BC_MenuItem(value_to_text(i)));
	}
}

const char* CompressorInput::value_to_text(int value)
{
	switch(value)
	{
		case CompressorConfig::TRIGGER: return "Trigger";
		case CompressorConfig::MAX: return "Maximum";
		case CompressorConfig::SUM: return "Total";
	}

	return "Trigger";
}

int CompressorInput::text_to_value(char *text)
{
	for(int i = 0; i < 3; i++)
	{
		if(!strcmp(value_to_text(i), text)) return i;
	}

	return CompressorConfig::TRIGGER;
}






CompressorClear::CompressorClear(CompressorEffect *plugin, int x, int y) 
 : BC_GenericButton(x, y, _("Clear"))
{
	this->plugin = plugin;
}

int CompressorClear::handle_event()
{
    BandConfig *band_config = &plugin->config.bands[plugin->config.current_band];

	band_config->levels.remove_all();
//plugin->config.dump();
	((CompressorWindow*)plugin->thread->window)->update();
	plugin->send_configure_change();
	return 1;
}



CompressorSmooth::CompressorSmooth(CompressorEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.smoothing_only, _("Smooth only"))
{
	this->plugin = plugin;
}

int CompressorSmooth::handle_event()
{
	plugin->config.smoothing_only = get_value();
	plugin->send_configure_change();
	return 1;
}


CompressorSolo::CompressorSolo(CompressorEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.bands[plugin->config.current_band].solo, _("Solo band"))
{
	this->plugin = plugin;
}

int CompressorSolo::handle_event()
{
	plugin->config.bands[plugin->config.current_band].solo = get_value();
    for(int i = 0; i < TOTAL_BANDS; i++)
    {
        if(i != plugin->config.current_band)
        {
            plugin->config.bands[i].solo = 0;
        }
    }
	plugin->send_configure_change();
	return 1;
}


CompressorBypass::CompressorBypass(CompressorEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.bands[plugin->config.current_band].bypass, _("Bypass band"))
{
	this->plugin = plugin;
}

int CompressorBypass::handle_event()
{
	plugin->config.bands[plugin->config.current_band].bypass = get_value();
	plugin->send_configure_change();
	return 1;
}


CompressorBand::CompressorBand(CompressorWindow *window, 
    CompressorEffect *plugin, 
    int x, 
    int y,
    int number,
    char *text)
 : BC_Radial(x, y, plugin->config.current_band == number, text)
{
    this->window = window;
    this->plugin = plugin;
    this->number = number;
}

int CompressorBand::handle_event()
{
    if(plugin->config.current_band != number)
    {
        plugin->config.current_band = number;
        window->update();
    }
    return 1;
}



