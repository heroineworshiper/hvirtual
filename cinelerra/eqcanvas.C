#include "clip.h"
#include "eqcanvas.h"
#include "pluginclient.h"

#define MINOR_DIVISIONS 5
EQCanvas::EQCanvas(BC_WindowBase *parent, 
    int x, 
    int y, 
    int w, 
    int h, 
    float min_db,
    float max_db)
{
    this->parent = parent;
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->min_db = min_db;
    this->max_db = max_db;
    last_frame = 0;
    canvas = 0;
    
    db_divisions = (max_db - min_db) / MINOR_DIVISIONS;
    freq_divisions = 5;
}



EQCanvas::~EQCanvas()
{
    delete canvas;
    delete last_frame;
}

    
void EQCanvas::initialize()
{
    int big_tick = DP(10);
    int small_tick = DP(5);
    int db_width = parent->get_text_width(SMALLFONT, 
		"-00", 
		-1) + big_tick;
    int freq_height = parent->get_text_ascent(SMALLFONT);

	canvas_x = x + db_width;
	canvas_y = y;
	canvas_w = w - db_width;
	canvas_h = h - freq_height - big_tick;
	parent->add_subwindow(canvas = new BC_SubWindow(canvas_x, 
		canvas_y, 
		canvas_w, 
		canvas_h, 
		BLACK));

// Draw canvas titles
// DB
	parent->set_font(SMALLFONT);
    int ascent = parent->get_text_ascent(SMALLFONT);
    int minor_divisions = MINOR_DIVISIONS;
    int pixels_per_division = (max_db - min_db) / 
        (db_divisions * minor_divisions);
    int db_per_division = 1;
    if(pixels_per_division < 3)
    {
        db_divisions /= 2;
        db_per_division *= 2;
    }

    
	char string[BCTEXTLEN];
	for(int i = 0; i <= db_divisions; i++)
	{
		int y1 = canvas_y + canvas_h - 
            i * (canvas_h / db_divisions) - DP(2);
		int y2 = y1 + ascent;
		int x2 = canvas_x - big_tick;
		int x3 = canvas_x - DP(2);

        
		if(i == 0)
			sprintf(string, "oo");
		else
			sprintf(string, "%d", (int)(i * minor_divisions * db_per_division + min_db));

		parent->set_color(BLACK);
        int text_w = parent->get_text_width(SMALLFONT,
            string,
            -1);
        int x1 = canvas_x - big_tick - text_w;
		parent->draw_text(x1 + 1, y2 + 1, string);
		parent->draw_line(x2 + 1, y1 + 1, x3 + 1, y1 + 1);
		parent->set_color(RED);
		parent->draw_text(x1, y2, string);
		parent->draw_line(x2, y1, x3, y1);

		if(i < db_divisions)
		{
			for(int j = 1; j < minor_divisions; j++)
			{
				int y3 = y1 - j * (canvas_h / db_divisions) / minor_divisions;
				int x4 = x3 - DP(5);
				parent->set_color(BLACK);
				parent->draw_line(x4 + 1, y3 + 1, x3 + 1, y3 + 1);
				parent->set_color(RED);
				parent->draw_line(x4, y3, x3, y3);
			}
		}
	}

// freq
	for(int i = 0; i <= freq_divisions; i++)
	{
		int freq = Freq::tofreq(i * TOTALFREQS / freq_divisions);
		sprintf(string, "%d", freq);
		int x1 = canvas_x + i * canvas_w / freq_divisions;
		int x2 = x1 - parent->get_text_width(SMALLFONT, string);
		int y1 = canvas_y + canvas_h;
		int y2 = y1 + big_tick;
		int y3 = y1 + small_tick;
		int y4 = y2 + 
            parent->get_text_ascent(SMALLFONT);

		
		parent->set_color(BLACK);
		parent->draw_text(x2 + 1, y4 + 1, string);
		parent->draw_line(x1 + 1, y1 + 1, x1 + 1, y2 + 1);
		parent->set_color(RED);
		parent->draw_text(x2, y4, string);
		parent->draw_line(x1, y1, x1, y2);

		if(i < freq_divisions)
		{
			for(int j = 0; j < minor_divisions; j++)
			{
				int x3 = (int)(x1 + 
					(canvas_w / freq_divisions) -
					exp(-(double)j * 0.7) * 
					(canvas_w / freq_divisions));
				parent->set_color(BLACK);
				parent->draw_line(x3 + 1, y1 + 1, x3 + 1, y3 + 1);
				parent->set_color(RED);
				parent->draw_line(x3, y1, x3, y3);
			}
		}
	}
    
    draw_grid();
}


void EQCanvas::draw_grid()
{
	canvas->set_line_dashes(1);
	canvas->set_color(GREEN);
    for(int i = 1; i < db_divisions; i++)
    {
        int y = canvas_h - 
            i * canvas_h / db_divisions;
        canvas->draw_line(0, y, canvas_w, y);
    }

    for(int i = 1; i < freq_divisions; i++)
    {
        int x = i * canvas_w / freq_divisions;
        canvas->draw_line(x, 0, x, canvas_h);
    }
    canvas->set_line_dashes(0);
}


// update the spectrogram in a plugin
void EQCanvas::update_spectrogram(PluginClient *plugin)
{
    int total_frames = plugin->get_gui_update_frames();
	PluginClientFrame *frame = plugin->get_gui_frame();

    if(frame)
    {
        delete last_frame;
        last_frame = frame;
    }
    else
    {
        frame = last_frame;
    }

// printf("EQCanvas::update_spectrogram %d frame=%p data=%p freq_max=%f time_max=%f\n", 
// __LINE__, 
// frame,
// frame ? frame->data : 0,
// frame ? frame->freq_max : 0,
// frame ? frame->time_max : 0);
    canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
    draw_grid();

// Draw most recent frame
	if(frame && !EQUIV(frame->freq_max, 0.0) && frame->data)
	{
		int y1 = 0;
		int y2 = 0;
		canvas->set_color(MEGREY);
        if(!EQUIV(frame->freq_max, 0))
        {
		    for(int i = 0; i < canvas->get_w(); i++)
		    {
			    int freq = Freq::tofreq(i * TOTALFREQS / canvas->get_w());
			    int index = (int64_t)freq * (int64_t)frame->data_size / 
                    frame->nyquist;
			    if(index < frame->data_size)
			    {
				    double magnitude = frame->data[index] / 
					    frame->freq_max * 
					    frame->time_max;
				    y2 = (int)(canvas->get_h() - 
					    (DB::todb(magnitude) - INFINITYGAIN) *
					    canvas->get_h() / 
					    -INFINITYGAIN);
				    CLAMP(y2, 0, canvas->get_h() - 1);
				    if(i > 0)
				    {
					    canvas->draw_line(i - 1, y1, i, y2);
				    }
				    y1 = y2;
			    }
		    }
        }
    }

// keep the last_frame
    if(frame)
    {
        total_frames--;
	}


// Delete remaining expired frames
	while(total_frames > 0)
	{
		PluginClientFrame *frame = plugin->get_gui_frame();

		if(frame) delete frame;
		total_frames--;
	}
    
}


void EQCanvas::draw_envelope(double *envelope, 
    int samplerate,
    int window_size)
{
    int niquist = samplerate / 2;
    
	canvas->set_color(WHITE);
	canvas->set_line_width(2);

    int y1;
    for(int i = 0; i < canvas->get_w(); i++)
    {
        int freq = Freq::tofreq(i * TOTALFREQS / canvas->get_w());
        int index = (int64_t)freq * (int64_t)window_size / 2 / niquist;
        if(freq < niquist && index < window_size / 2)
        {
            double mag = envelope[index];
            int y2 = (int)(DB::todb(mag) * canvas->get_h() / INFINITYGAIN);
            
            if(y2 >= canvas->get_h())
            {
                y2 = canvas->get_h() - 1;
            }
            
            if(i > 0)
            {
                canvas->draw_line(i - 1, y1, i, y2);
            }
            y1 = y2;
        }
        else
        if(i > 0)
        {
            int y2 = canvas->get_h() - 1;
            canvas->draw_line(i - 1, y1, i, y2);
            y1 = y2;
        }
    }

    canvas->set_line_width(1);
	canvas->flash(1);
    
}










