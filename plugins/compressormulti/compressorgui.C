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
    plugin->current_band = defaults->get("CURRENT_BAND", plugin->current_band);

}

CompressorWindow::~CompressorWindow()
{
	defaults->update("CURRENT_BAND", plugin->current_band);
	defaults->save();
    delete defaults;

    delete eqcanvas;
}


void CompressorWindow::create_objects()
{
    int margin = client->get_theme()->widget_border;
	int x = DP(35), y = margin;
	int control_margin = DP(150);
    BC_Title *title;
    BandConfig *band_config = &plugin->config.bands[plugin->current_band];

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


    add_subwindow(title = new BC_Title(margin, y, _("Sound level:")));
    y += title->get_h() + 1;
	add_subwindow(canvas = new CompressorCanvas(plugin, 
        this,
		x, 
		y, 
		get_w() - x - control_margin - DP(10), 
		get_h() * 2 / 3 - y));
	canvas->set_cursor(CROSS_CURSOR, 0, 0);
    y += canvas->get_h() + DP(30);
    
    add_subwindow(title = new BC_Title(margin, y, _("Bandwidth:")));
    y += title->get_h();
    eqcanvas = new EQCanvas(this,
        margin,
        y,
        canvas->get_w() + x - margin,
        get_h() - y - margin,
        INFINITYGAIN,
        0.0);
    eqcanvas->freq_divisions = 10;
    eqcanvas->initialize();
    
    
	x = get_w() - control_margin;
    y = margin;
	add_subwindow(new BC_Title(x, y, _("Attack secs:")));
	y += DP(20);
	add_subwindow(reaction = new CompressorReaction(plugin, x, y));
	y += DP(30);
	add_subwindow(new BC_Title(x, y, _("Release secs:")));
	y += DP(20);
	add_subwindow(decay = new CompressorDecay(plugin, x, y));
	y += DP(30);
	add_subwindow(new BC_Title(x, y, _("Trigger Type:")));
	y += DP(20);
	add_subwindow(input = new CompressorInput(plugin, x, y));
	input->create_objects();
	y += DP(30);
	add_subwindow(new BC_Title(x, y, _("Trigger:")));
	y += DP(20);
	add_subwindow(trigger = new CompressorTrigger(plugin, x, y));
	if(plugin->config.input != CompressorConfig::TRIGGER) trigger->disable();
	y += DP(30);


	add_subwindow(smooth = new CompressorSmooth(plugin, x, y));
    y += smooth->get_h() + margin;
	add_subwindow(solo = new CompressorSolo(plugin, x, y));
    y += solo->get_h() + margin;
	add_subwindow(bypass = new CompressorBypass(plugin, x, y));
    y += bypass->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Output:")));
    y += title->get_h();
	add_subwindow(y_text = new CompressorY(plugin, x, y));
    y += y_text->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Input:")));
    y += title->get_h();
	add_subwindow(x_text = new CompressorX(plugin, x, y));
    y += x_text->get_h() + margin;

    
	add_subwindow(clear = new CompressorClear(plugin, x, y));
    y += clear->get_h() + margin;

    add_subwindow(title = new BC_Title(x, y, _("Freq:")));
    add_subwindow(freq = new CompressorQPot(this, 
        plugin, 
        get_w() - margin - BC_Pot::calculate_w(), 
        y, 
        &band_config->freq));
    y += freq->get_h() + margin;
// top band edits the penultimate band
    if(plugin->current_band == TOTAL_BANDS - 1)
    {
        freq->output = &plugin->config.bands[plugin->current_band - 1].freq;
    }
    freq->update(*freq->output);


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
    y += title->get_h();
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

void CompressorWindow::update()
{
    BandConfig *band_config = &plugin->config.bands[plugin->current_band];

    for(int i = 0; i < TOTAL_BANDS; i++)
    {
        if(plugin->current_band == i)
        {
            band[i]->update(1);
        }
        else
        {
            band[i]->update(0);
        }
    }

// top band edits the penultimate band
    if(plugin->current_band == TOTAL_BANDS - 1)
    {
        freq->output = &plugin->config.bands[plugin->current_band - 1].freq;
    }
    else
    {
        freq->output = &band_config->freq;
    }
    freq->update(*freq->output);

    q->update(plugin->config.q);
    solo->update(band_config->solo);
    bypass->update(band_config->bypass);
    size->update(plugin->config.window_size);

	if(atol(trigger->get_text()) != plugin->config.trigger)
		trigger->update((int64_t)plugin->config.trigger);
	if(strcmp(input->get_text(), CompressorInput::value_to_text(plugin->config.input)))
		input->set_text(CompressorInput::value_to_text(plugin->config.input));

	if(plugin->config.input != CompressorConfig::TRIGGER && trigger->get_enabled())
		trigger->disable();
	else
	if(plugin->config.input == CompressorConfig::TRIGGER && !trigger->get_enabled())
		trigger->enable();

	if(!EQUIV(atof(reaction->get_text()), plugin->config.reaction_len))
		reaction->update((float)plugin->config.reaction_len);
	if(!EQUIV(atof(decay->get_text()), plugin->config.decay_len))
		decay->update((float)plugin->config.decay_len);
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
        plugin->current_band * plugin->config.window_size / 2,
        TOTAL_BANDS * plugin->config.window_size / 2,
        plugin->config.window_size);
#endif

    
    // draw the active band on top of the others
    for(int pass = 0; pass < 2; pass++)
    {
        for(int band = 0; band < TOTAL_BANDS; band++)
        {
            if(band == plugin->current_band && pass == 0 ||
                band != plugin->current_band && pass == 1)
            {
                continue;
            }

            eqcanvas->draw_envelope(plugin->engines[band]->envelope,
                plugin->PluginAClient::project_sample_rate,
                plugin->config.window_size,
                band == plugin->current_band);
        }
    }
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
    *output)
{
    this->gui = gui;
    this->plugin = plugin;
    this->output = output;
}


int CompressorQPot::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
    gui->update_eqcanvas();
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
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
    this->window = window;
	current_operation = NONE;

    graph_x = 0;
    graph_y = 0;
    graph_w = w - graph_x;
    graph_h = h - graph_y;
    subdivisions = 6;
    divisions = (int)(plugin->config.max_db - plugin->config.min_db) / subdivisions;
}

void CompressorCanvas::create_objects()
{
    draw_scales();
    update();
}

void CompressorCanvas::draw_scales()
{
	window->set_font(SMALLFONT);
	window->set_color(get_resources()->default_text_color);

    int big_line = DP(10);
    int small_line = DP(5);
// output divisions
	for(int i = 0; i <= divisions; i++)
	{
		int y = get_y() + DP(10) + graph_y + graph_h / divisions * i;
		int x = get_x() - big_line;
		char string[BCTEXTLEN];
		
		sprintf(string, "%.0f", plugin->config.max_db - 
            (float)i / divisions * 
            (plugin->config.max_db - plugin->config.min_db));
		int text_w = get_text_width(SMALLFONT, string);
		window->draw_text(x - text_w, y, string);
		
		int y1 = get_y() + graph_y + graph_h / divisions * i;
		int y2 = get_y() + graph_y + graph_h / divisions * (i + 1);
		for(int j = 0; j < subdivisions; j++)
		{
			y = y1 + (y2 - y1) * j / subdivisions;
			if(j == 0)
			{
				window->draw_line(get_x() - big_line, y, get_x(), y);
			}
			else
			if(i < divisions)
			{
				window->draw_line(get_x() - small_line, y, get_x(), y);
			}
		}
	}

// input divisions
	for(int i = 0; i <= divisions; i++)
	{
		int y = get_y() + get_h();
		int x = get_x() + graph_x + graph_w * i / divisions;
        int y1 = y + window->get_text_ascent(SMALLFONT);
		char string[BCTEXTLEN];

		sprintf(string, 
            "%.0f", 
            (float)i / divisions * 
                (plugin->config.max_db - plugin->config.min_db) + plugin->config.min_db);
		int text_w = get_text_width(SMALLFONT, string);
        window->draw_text(x - text_w, y1 + big_line, string);

		int x1 = get_x() + graph_x + graph_w * i / divisions;
		int x2 = get_x() + graph_x + graph_w * (i + 1) / divisions;
		for(int j = 0; j < subdivisions; j++)
		{
			x = x1 + (x2 - x1) * j / subdivisions;
			if(j == 0)
			{
				window->draw_line(x, 
                    y, 
                    x, 
                    y + big_line);
			}
			else
			if(i < divisions)
			{
				window->draw_line(x, 
                    y, 
                    x, 
                    y + small_line);
			}
		}
	}


}

#define POINT_W DP(10)

// get Y from X
int CompressorCanvas::x_to_y(int band, int x)
{
	double x_db = plugin->config.min_db + (double)x / graph_w * 
        (plugin->config.max_db - plugin->config.min_db);
	double y_db = plugin->config.calculate_db(band, x_db);
	int y = graph_y + graph_h - 
        (int)((y_db - plugin->config.min_db) * 
        graph_h / 
        (plugin->config.max_db - plugin->config.min_db)); 

//printf("CompressorCanvas::x_to_y %d x=%d x_db=%f y_db=%f y=%d\n", 
//__LINE__, x, x_db, y_db, y);
    return y;
}

// get X from DB
int CompressorCanvas::db_to_x(double db)
{
    int x = graph_x + 
        (double)(db - plugin->config.min_db) *
        graph_w /
        (plugin->config.max_db - plugin->config.min_db);
    return x;
}

// get Y from DB
int CompressorCanvas::db_to_y(double db)
{
    int y = graph_y + graph_h - 
        (int)((db - plugin->config.min_db) * 
        graph_h / 
        (plugin->config.max_db - plugin->config.min_db)); 

//printf("CompressorCanvas::x_to_y %d x=%d x_db=%f y_db=%f y=%d\n", 
//__LINE__, x, x_db, y_db, y);
    return y;
}


double CompressorCanvas::x_to_db(int x)
{
	CLAMP(x, 0, get_w());
	double x_db = (double)(x - graph_x) *
        (plugin->config.max_db - plugin->config.min_db) / 
        graph_w +
        plugin->config.min_db;
    CLAMP(x_db, plugin->config.min_db, plugin->config.max_db);
    return x_db;
}

double CompressorCanvas::y_to_db(int y)
{
	CLAMP(y, 0, get_h());
	double y_db = (double)(y - graph_y) * 
        (plugin->config.min_db - plugin->config.max_db) / 
        graph_h + 
        plugin->config.max_db;
//printf("CompressorCanvas::cursor_motion_event %d x=%d y=%d x_db=%f y_db=%f\n", 
//__LINE__, x, y, x_db, y_db);
    CLAMP(y_db, plugin->config.min_db, plugin->config.max_db);
    return y_db;
}



void CompressorCanvas::update()
{
	int y1, y2;

// headroom boxes
    set_color(window->get_bg_color());
    draw_box(graph_x, 0, get_w(), graph_y);
    draw_box(graph_w, graph_y, get_w() - graph_w, get_h() - graph_y);
//     const int checker_w = DP(10);
//     const int checker_h = DP(10);
//     set_color(MDGREY);
//     for(int i = 0; i < get_h(); i += checker_h)
//     {
//         for(int j = (i % 2) * checker_w; j < get_w(); j += checker_w * 2)
//         {
//             if(!(i >= graph_y && 
//                 i + checker_h < graph_y + graph_h &&
//                 j >= graph_x &&
//                 j + checker_w < graph_x + graph_w))
//             {
//                 draw_box(j, i, checker_w, checker_h);
//             }
//         }
//     }

// canvas boxes
	clear_box(graph_x, graph_y, graph_w, graph_h);

	draw_3d_border(0, 
		0, 
		get_w(), 
		get_h(), 
		window->get_bg_color(),
		BLACK,
		MDGREY, 
		window->get_bg_color());



	set_line_dashes(1);
	set_color(GREEN);
	
	for(int i = 1; i < divisions; i++)
	{
		int y = graph_y + graph_h * i / divisions;
		draw_line(graph_x, y, graph_x + graph_w, y);
// 0db 
        if(i == 1)
        {
            draw_line(graph_x, y + 1, graph_x + graph_w, y + 1);
        }
		
		int x = graph_x + graph_w * i / divisions;
		draw_line(x, graph_y, x, graph_y + graph_h);
// 0db 
        if(i == divisions - 1)
        {
            draw_line(x + 1, graph_y, x + 1, graph_y + graph_h);
        }
	}
	set_line_dashes(0);


	set_font(MEDIUMFONT);
	draw_text(plugin->get_theme()->widget_border, 
		get_h() / 2, 
		_("Output"));
	draw_text(get_w() / 2 - get_text_width(MEDIUMFONT, _("Input")) / 2, 
		get_h() - plugin->get_theme()->widget_border, 
		_("Input"));


    for(int pass = 0; pass < 2; pass++)
    {
        for(int band = 0; band < TOTAL_BANDS; band++)
        {
// draw the active band on top of the others
            if(band == plugin->current_band && pass == 0 ||
                band != plugin->current_band && pass == 1)
            {
                continue;
            }

            if(band == plugin->current_band)
            {
	            set_color(WHITE);
	            set_line_width(2);
            }
            else
            {
	            set_color(MEGREY);
	            set_line_width(1);
            }

// draw the line
	        for(int i = graph_x; i <= graph_x + graph_w; i++)
	        {
		        y2 = x_to_y(band, i);

		        if(i > graph_x)
		        {
			        draw_line(i - 1, y1, i, y2);
		        }

		        y1 = y2;
	        }

	        set_line_width(1);

// draw the points
            if(band == plugin->current_band)
            {
                BandConfig *band_config = &plugin->config.bands[band];
	            int total = band_config->levels.total ? band_config->levels.total : 1;
	            for(int i = 0; i < band_config->levels.total; i++)
	            {
		            double x_db = plugin->config.get_x(band, i);
		            double y_db = plugin->config.get_y(band, i);

		            int x = db_to_x(x_db);
		            int y = db_to_y(y_db);

		            draw_box(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
	            }
            }
        }
    }
	
	flash();
}

int CompressorCanvas::button_press_event()
{
    BandConfig *band_config = &plugin->config.bands[plugin->current_band];
// Check existing points
	if(is_event_win() && 
        cursor_inside())
	{
		for(int i = 0; i < band_config->levels.total; i++)
		{
			double x_db = plugin->config.get_x(plugin->current_band, i);
			double y_db = plugin->config.get_y(plugin->current_band, i);

			int x = db_to_x(x_db);
			int y = db_to_y(y_db);

			if(get_cursor_x() <= x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() <= y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				current_operation = DRAG;
				current_point = i;
				return 1;
			}
		}



        if(get_cursor_x() >= graph_x &&
            get_cursor_x() < graph_x + graph_w &&
            get_cursor_y() >= graph_y &&
            get_cursor_y() < graph_y + graph_h)
        {
// Create new point
		    double x_db = x_to_db(get_cursor_x());
		    double y_db = y_to_db(get_cursor_y());

		    current_point = plugin->config.set_point(plugin->current_band, x_db, y_db);
		    current_operation = DRAG;
		    ((CompressorWindow*)plugin->thread->window)->update();
		    plugin->send_configure_change();
		    return 1;
        }
	}
	return 0;
//plugin->config.dump();
}

int CompressorCanvas::button_release_event()
{
    BandConfig *band_config = &plugin->config.bands[plugin->current_band];

	if(current_operation == DRAG)
	{
		if(current_point > 0)
		{
			if(band_config->levels.values[current_point].x <
				band_config->levels.values[current_point - 1].x)
            {
				plugin->config.remove_point(plugin->current_band, current_point);
            }
		}

		if(current_point < band_config->levels.total - 1)
		{
			if(band_config->levels.values[current_point].x >=
				band_config->levels.values[current_point + 1].x)
            {
				plugin->config.remove_point(plugin->current_band, current_point);
            }
		}

		((CompressorWindow*)plugin->thread->window)->update();
		plugin->send_configure_change();
		current_operation = NONE;
		return 1;
	}

	return 0;
}

int CompressorCanvas::cursor_motion_event()
{
    BandConfig *band_config = &plugin->config.bands[plugin->current_band];

	if(current_operation == DRAG)
	{
		int x = get_cursor_x();
		int y = get_cursor_y();
		double x_db = x_to_db(x);
		double y_db = y_to_db(y);
//printf("CompressorCanvas::cursor_motion_event %d x=%d y=%d x_db=%f y_db=%f\n", 
//__LINE__, x, y, x_db, y_db);
		band_config->levels.values[current_point].x = x_db;
		band_config->levels.values[current_point].y = y_db;
		((CompressorWindow*)plugin->thread->window)->update();
		plugin->send_configure_change();
		return 1;
//plugin->config.dump();
	}
	else
// Change cursor over points
	if(is_event_win() && cursor_inside())
	{
		int new_cursor = CROSS_CURSOR;

		for(int i = 0; i < band_config->levels.total; i++)
		{
			double x_db = plugin->config.get_x(plugin->current_band, i);
			double y_db = plugin->config.get_y(plugin->current_band, i);

			int x = db_to_x(x_db);
			int y = db_to_y(y_db);

			if(get_cursor_x() <= x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() <= y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				new_cursor = UPRIGHT_ARROW_CURSOR;
				break;
			}
		}

// out of active area
        if(get_cursor_x() >= graph_x + graph_w ||
            get_cursor_y() < graph_y)
        {
            new_cursor = UPRIGHT_ARROW_CURSOR;
        }

		if(new_cursor != get_cursor())
		{
			set_cursor(new_cursor, 0, 1);
		}
	}
	return 0;
}





CompressorReaction::CompressorReaction(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, (float)plugin->config.reaction_len)
{
	this->plugin = plugin;
}

int CompressorReaction::handle_event()
{
	plugin->config.reaction_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorReaction::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.reaction_len += 0.1;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.reaction_len -= 0.1;
		}
		update((float)plugin->config.reaction_len);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}

CompressorDecay::CompressorDecay(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, (float)plugin->config.decay_len)
{
	this->plugin = plugin;
}
int CompressorDecay::handle_event()
{
	plugin->config.decay_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorDecay::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.decay_len += 0.1;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.decay_len -= 0.1;
		}
		update((float)plugin->config.decay_len);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}



CompressorX::CompressorX(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, "")
{
	this->plugin = plugin;
}
int CompressorX::handle_event()
{
    BandConfig *band_config = &plugin->config.bands[plugin->current_band];

	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < band_config->levels.total)
	{
		band_config->levels.values[current_point].x = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->canvas->update();
		plugin->send_configure_change();
	}
	return 1;
}



CompressorY::CompressorY(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, "")
{
	this->plugin = plugin;
}
int CompressorY::handle_event()
{
    BandConfig *band_config = &plugin->config.bands[plugin->current_band];

	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < band_config->levels.total)
	{
		band_config->levels.values[current_point].y = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->canvas->update();
		plugin->send_configure_change();
	}
	return 1;
}





CompressorTrigger::CompressorTrigger(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, (int64_t)plugin->config.trigger)
{
	this->plugin = plugin;
}
int CompressorTrigger::handle_event()
{
	plugin->config.trigger = atol(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorTrigger::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.trigger++;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.trigger--;
		}
		update((int64_t)plugin->config.trigger);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
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
    BandConfig *band_config = &plugin->config.bands[plugin->current_band];

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
 : BC_CheckBox(x, y, plugin->config.bands[plugin->current_band].solo, _("Solo band"))
{
	this->plugin = plugin;
}

int CompressorSolo::handle_event()
{
	plugin->config.bands[plugin->current_band].solo = get_value();
    for(int i = 0; i < TOTAL_BANDS; i++)
    {
        if(i != plugin->current_band)
        {
            plugin->config.bands[i].solo = 0;
        }
    }
	plugin->send_configure_change();
	return 1;
}


CompressorBypass::CompressorBypass(CompressorEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.bands[plugin->current_band].bypass, _("Bypass band"))
{
	this->plugin = plugin;
}

int CompressorBypass::handle_event()
{
	plugin->config.bands[plugin->current_band].bypass = get_value();
	plugin->send_configure_change();
	return 1;
}


CompressorBand::CompressorBand(CompressorWindow *window, 
    CompressorEffect *plugin, 
    int x, 
    int y,
    int number,
    char *text)
 : BC_Radial(x, y, plugin->current_band == number, text)
{
    this->window = window;
    this->plugin = plugin;
    this->number = number;
}

int CompressorBand::handle_event()
{
    if(plugin->current_band != number)
    {
        plugin->current_band = number;
        window->update();
    }
    return 1;
}



