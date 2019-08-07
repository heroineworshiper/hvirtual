
/*
 * CINELERRA
 * Copyright (C) 2017-2019 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
#include "confirmsave.h"
#include "bchash.h"
#include "bcsignals.h"
#include "errorbox.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "reverb.h"
#include "reverbwindow.h"
#include "samples.h"
#include "transportque.inc"
#include "units.h"

#include "vframe.h"

#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>





PluginClient* new_plugin(PluginServer *server)
{
	return new Reverb(server);
}



Reverb::Reverb(PluginServer *server)
 : PluginAClient(server)
{
	srand(time(0));
	redo_buffers = 1;       // set to redo buffers before the first render
	ref_channels = 0;
	ref_offsets = 0;
	ref_levels = 0;
	dsp_in = 0;
	dsp_in_length = 0;
    dsp_in_allocated = 0;
	need_reconfigure = 1;
    envelope = 0;
    last_position = 0;
    
    fft = 0;
}

Reverb::~Reverb()
{
	if(fft)
	{
		for(int i = 0; i < total_in_buffers; i++)
		{
			delete [] dsp_in[i];
			delete [] ref_channels[i];
			delete [] ref_offsets[i];
			delete [] ref_levels[i];
            delete fft[i];
		}

        delete [] fft;
		delete [] dsp_in;
		delete [] ref_channels;
		delete [] ref_offsets;
		delete [] ref_levels;
		delete engine;
	}
    
    delete [] envelope;
}

const char* Reverb::plugin_title() { return N_("Reverb"); }
int Reverb::is_realtime() { return 1; }
int Reverb::is_multichannel() { return 1; }
int Reverb::is_synthesis() { return 1; }


int Reverb::process_buffer(int64_t size, 
	Samples **buffer, 
	int64_t start_position,
	int sample_rate)
{
    need_reconfigure |= load_configuration();


// reset after seeking
    if(last_position != start_position)
    {
// printf("Reverb::process_buffer %d last_position=%ld start_position=%ld\n",
// __LINE__,
// last_position,
// start_position);

        dsp_in_length = 0;
        if(fft)
        {
		    for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
 			    if(fft[i]) fft[i]->delete_fft();
		    }
        }
        if(dsp_in)
        {
		    for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
                if(dsp_in[i]) bzero(dsp_in[i], sizeof(double) * dsp_in_allocated);
		    }
        }
    }
    else
    {
// printf("Reverb::process_buffer %d last_position=%ld start_position=%ld\n",
// __LINE__,
// last_position,
// start_position);
    }


    if(need_reconfigure)
    {
        need_reconfigure = 0;

        calculate_envelope();


        if(fft && fft[0]->window_size != config.window_size)
        {
		    for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
 			    delete fft[i];
		    }
            delete [] fft;
            fft = 0;
        }

        if(!fft)
        {
            fft = new ReverbFFT*[PluginClient::total_in_buffers];
		    for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
			    fft[i] = new ReverbFFT(this, i);
                fft[i]->initialize(config.window_size);
		    }
        }

// allocate the stuff
        if(!dsp_in)
        {
            dsp_in = new double*[PluginClient::total_in_buffers];
 		    ref_channels = new int*[PluginClient::total_in_buffers];
 		    ref_offsets = new int*[PluginClient::total_in_buffers];
 		    ref_levels = new double*[PluginClient::total_in_buffers];
		    for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
 			    dsp_in[i] = 0;
                ref_channels[i] = 0;
                ref_offsets[i] = 0;
                ref_levels[i] = 0;
		    }

            engine = new ReverbEngine(this);
        }


		for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
 			if(ref_channels[i]) delete [] ref_channels[i];
 			if(ref_offsets[i]) delete [] ref_offsets[i];
 			if(ref_levels[i]) delete [] ref_levels[i];


 			ref_channels[i] = new int[config.ref_total];
 			ref_offsets[i] = new int[config.ref_total];
 			ref_levels[i] = new double[config.ref_total];

// 1st reflection is fixed by the user
            ref_channels[i][0] = i;
            ref_offsets[i][0] = config.delay_init * project_sample_rate / 1000;
            ref_levels[i][0] = db.fromdb(config.ref_level1);
            
 			int64_t ref_division = config.ref_length * 
                project_sample_rate / 
                1000 / 
                (config.ref_total + 1);
 			for(int j = 1; j < config.ref_total; j++)
 			{
// set random channels for remaining reflections
 				ref_channels[i][j] = rand() % total_in_buffers;
 
// set random offsets after first reflection
 				ref_offsets[i][j] = ref_offsets[i][0];
 				ref_offsets[i][j] += ref_division * j - (rand() % ref_division) / 2;

// set changing levels
                double level_db = config.ref_level1 + 
                    (config.ref_level2 - config.ref_level1) *
                    (j - 1) / 
                    (config.ref_total - 1);
 				ref_levels[i][j] = DB::fromdb(level_db);
 			}
		}
    }


// guess DSP allocation from the reflection time & requested samples
    int new_dsp_allocated = size + 
     	((int64_t)config.delay_init + config.ref_length) * 
        project_sample_rate / 
        1000 + 
        1;
    reallocate_dsp(new_dsp_allocated);

// Always read in the new samples & process the bandpass, even if there is no
// bandpass.  This way the user can tweek the bandpass without causing glitches.
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
	{
        new_dsp_length = dsp_in_length;
        new_spectrogram_frames = 0;
//printf("Reverb::process_buffer %d start_position=%ld buffer[i]=%p size=%ld fft[i]=%p\n", 
//__LINE__, start_position, buffer[i], size, fft[i]);
        fft[i]->process_buffer(start_position, 
		    size, 
		    buffer[i],   // temporary storage for the bandpassed output
		    get_direction());
    }


// send the spectrograms to the plugin.  This consumes the pointers
	for(int i = 0; i < spectrogram_frames.size(); i++)
    {
        add_gui_frame(spectrogram_frames.get(i));
    }


// update the length with what the FFT reads appended
    dsp_in_length = new_dsp_length;
// printf("Reverb::process_buffer %d dsp_in_length=%d size=%ld spectrogram_frames=%d %d\n", 
// __LINE__, 
// dsp_in_length, 
// size,
// spectrogram_frames.size(),
// new_spectrogram_frames);

// remove the pointers
    spectrogram_frames.remove_all();




// now paint the reflections
    engine->process_packages();




// copy the DSP buffer to the output
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
    {
        memcpy(buffer[i]->get_data(), dsp_in[i], size * sizeof(double));
    }



// shift the DSP buffer forward
    int remane = dsp_in_allocated - size;
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
    {
        memcpy(dsp_in[i], dsp_in[i] + size, remane * sizeof(double));
        bzero(dsp_in[i] + remane, size * sizeof(double));
    }
//printf("Reverb::process_buffer %d size=%d dsp_in_allocated=%d\n", __LINE__, size, dsp_in_allocated);

    dsp_in_length -= size;
    
    if(get_direction() == PLAY_FORWARD)
    {
        last_position = start_position + size;
    }
    else
    {
        last_position = start_position - size;
    }

    

    return 0;
}


void Reverb::reallocate_dsp(int new_dsp_allocated)
{
    if(new_dsp_allocated > dsp_in_allocated)
    {
// copy samples already read into the new buffers
        for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
            double *old_dsp = dsp_in[i];
            double *new_dsp = new double[new_dsp_allocated];
            
            if(old_dsp)
            {
                memcpy(new_dsp, old_dsp, sizeof(double) * dsp_in_length);
                delete [] old_dsp;
            }
            bzero(new_dsp + dsp_in_allocated, 
                sizeof(double) * (new_dsp_allocated - dsp_in_allocated));
            dsp_in[i] = new_dsp;
        }
        dsp_in_allocated = new_dsp_allocated;
//printf("Reverb::reallocate_dsp %d dsp_in_allocated=%d\n", __LINE__, dsp_in_allocated);
    }

}


double Reverb::gauss(double sigma, double center, double x)
{
	if(EQUIV(sigma, 0)) sigma = 0.01;

	double result = 1.0 / 
		sqrt(2 * M_PI * sigma * sigma) * 
		exp(-(x - center) * (x - center) / 
			(2 * sigma * sigma));


	return result;
}

void Reverb::calculate_envelope()
{
// assume the window size changed
    if(envelope)
    {
        delete [] envelope;
        envelope = 0;
    }

    envelope = new double[config.window_size / 2];

    int max_freq = Freq::tofreq_f(TOTALFREQS - 1);
    int nyquist = PluginAClient::project_sample_rate / 2;
    int low = config.low;
    int high = config.high;


// limit the frequencies
    if(high >= max_freq)
    {
        high = nyquist;
    }

    if(low > high)
    {
        low = high;
    }

// frequency slots of the edge
    double edge = (1.0 - config.q) * TOTALFREQS / 2;
    double low_slot = Freq::fromfreq_f(low);
    double high_slot = Freq::fromfreq_f(high);
    for(int i = 0; i < config.window_size / 2; i++)
    {
        double freq = i * nyquist / (config.window_size / 2);
        double slot = Freq::fromfreq_f(freq);

// printf("Reverb::calculate_envelope %d i=%d freq=%f slot=%f slot1=%f\n", 
// __LINE__, 
// i, 
// freq,
// slot, 
// low_slot - edge);
        if(slot < low_slot - edge)
        {
            envelope[i] = 0.0;
        }
        else
        if(slot < low_slot)
        {
#ifndef LOG_CROSSOVER
            envelope[i] = 1.0 - (low_slot - slot) / edge;
#else
            envelope[i] = DB::fromdb((low_slot - slot) * INFINITYGAIN / edge);
#endif
        }
        else
        if(slot < high_slot)
        {
            envelope[i] = 1.0;
        }
        else
        if(slot < high_slot + edge)
        {
#ifndef LOG_CROSSOVER
            envelope[i] = 1.0 - (slot - high_slot) / edge;
#else
            envelope[i] = DB::fromdb((slot - high_slot) * INFINITYGAIN / edge);
#endif
        }
        else
        {
            envelope[i] = 0.0;
        }

//        printf("Reverb::calculate_envelope %d i=%d %f\n", __LINE__, i, envelope[i]);
    }
}



NEW_PICON_MACRO(Reverb)

NEW_WINDOW_MACRO(Reverb, ReverbWindow)


LOAD_CONFIGURATION_MACRO(Reverb, ReverbConfig)


void Reverb::save_data(KeyFrame *keyframe)
{
//printf("Reverb::save_data 1\n");
	FileXML output;
//printf("Reverb::save_data 1\n");

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
//printf("Reverb::save_data 1\n");

	output.tag.set_title("REVERB");
	output.tag.set_property("LEVELINIT", config.level_init);
	output.tag.set_property("DELAY_INIT", config.delay_init);
	output.tag.set_property("REF_LEVEL1", config.ref_level1);
	output.tag.set_property("REF_LEVEL2", config.ref_level2);
	output.tag.set_property("REF_TOTAL", config.ref_total);
//printf("Reverb::save_data 1\n");
	output.tag.set_property("REF_LENGTH", config.ref_length);
	output.tag.set_property("HIGH", config.high);
	output.tag.set_property("LOW", config.low);
    output.tag.set_property("Q", config.q);
    output.tag.set_property("WINDOW_SIZE", config.window_size);
//printf("Reverb::save_data config.ref_level2 %f\n", config.ref_level2);
	output.append_tag();
	output.append_newline();
//printf("Reverb::save_data 1\n");
	
	
	
	output.terminate_string();
//printf("Reverb::save_data 2\n");
}

void Reverb::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("REVERB"))
		{
			config.level_init = input.tag.get_property("LEVELINIT", config.level_init);
			config.delay_init = input.tag.get_property("DELAY_INIT", config.delay_init);
			config.ref_level1 = input.tag.get_property("REF_LEVEL1", config.ref_level1);
			config.ref_level2 = input.tag.get_property("REF_LEVEL2", config.ref_level2);
			config.ref_total = input.tag.get_property("REF_TOTAL", config.ref_total);
			config.ref_length = input.tag.get_property("REF_LENGTH", config.ref_length);
			config.high = input.tag.get_property("HIGH", config.high);
			config.low = input.tag.get_property("LOW", config.low);
			config.q = input.tag.get_property("Q", config.q);
			config.window_size = input.tag.get_property("WINDOW_SIZE", config.window_size);
		}
	}

	config.boundaries();
}

void Reverb::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
//printf("Reverb::update_gui %d %d\n", __LINE__, config.ref_length);
			thread->window->lock_window("Reverb::update_gui 1");
            ((ReverbWindow*)thread->window)->update();
			thread->window->unlock_window();
		}
        else
        {
			int total_frames = get_gui_update_frames();
//printf("ParametricEQ::update_gui %d %d\n", __LINE__, total_frames);
			if(total_frames)
			{
				thread->window->lock_window("ParametricEQ::update_gui 2");
				((ReverbWindow*)thread->window)->update_canvas();
				thread->window->unlock_window();
			}
        }
	}
}






ReverbFFT::ReverbFFT(Reverb *plugin, int channel)
{
    this->plugin = plugin;
    this->channel = channel;
}

ReverbFFT::~ReverbFFT()
{
}

int ReverbFFT::signal_process()
{
// Create new spectrogram for updating the GUI
    PluginClientFrame *frame = 0;
    if(plugin->new_spectrogram_frames >= plugin->spectrogram_frames.size())
    {
	    frame = new PluginClientFrame(window_size / 2, 
            window_size / 2, 
            plugin->PluginAClient::project_sample_rate);
        plugin->spectrogram_frames.append(frame);
        frame->data = new double[window_size / 2];
        bzero(frame->data, sizeof(double) * window_size / 2);
        frame->nyquist = plugin->PluginAClient::project_sample_rate / 2;
    }
    else
    {
        frame = plugin->spectrogram_frames.get(plugin->new_spectrogram_frames);
    }

    for(int i = 0; i < window_size / 2; i++)
    {
        double mag = sqrt(freq_real[i] * freq_real[i] + 
            freq_imag[i] * freq_imag[i]);

        double mag2 = plugin->envelope[i] * mag;
        double angle = atan2(freq_imag[i], freq_real[i]);

		freq_real[i] = mag2 * cos(angle);
		freq_imag[i] = mag2 * sin(angle);
// update the spectrogram with the output
        frame->data[i] = MAX(frame->data[i], mag2);

// get the maximum output in the frequency domane
        if(mag2 > frame->freq_max)
        {
            frame->freq_max = mag2;
        }
    }

    symmetry(window_size, freq_real, freq_imag);
    return 0;
}

int ReverbFFT::post_process()
{
    PluginClientFrame *frame = plugin->spectrogram_frames.get(plugin->new_spectrogram_frames);
// get the maximum output in the time domane
	double time_max = 0;
	for(int i = 0; i < window_size; i++)
	{
		if(fabs(output_real[i]) > time_max) time_max = fabs(output_real[i]);
	}

    if(time_max > frame->time_max)
    {
    	frame->time_max = time_max;
    }

// printf("ReverbFFT::post_process %d frame=%p data=%p freq_max=%f time_max=%f\n",
// __LINE__,
// frame,
// frame->data,
// frame->freq_max,
// frame->time_max);

    plugin->new_spectrogram_frames++;

    return 0;
}


int ReverbFFT::read_samples(int64_t output_sample, 
	int samples, 
	Samples *buffer)
{
// printf("ReverbFFT::read_samples %d channel=%d samples=%d\n", 
// __LINE__, 
// channel, 
// samples);
	int result = plugin->read_samples(buffer,
		channel,
		plugin->get_samplerate(),
		output_sample,
		samples);

// printf("ReverbFFT::read_samples %d output_sample=%ld dsp_in_length=%d samples=%d\n",
// __LINE__,
// output_sample,
// plugin->new_dsp_length,
// samples);

// append original samples to the DSP buffer as the initial reflection
    int new_dsp_allocation = plugin->new_dsp_length + samples;
    plugin->reallocate_dsp(new_dsp_allocation);
    double *dst = plugin->dsp_in[channel] + plugin->new_dsp_length;
    double *src = buffer->get_data();
    double level = DB::fromdb(plugin->config.level_init);
    if(plugin->config.level_init <= INFINITYGAIN)
    {
        level = 0;
    }

// printf("ReverbFFT::read_samples %d counter=%d samples=%d level_init=%f %f\n", 
// __LINE__, counter, samples, plugin->config.level_init, level);
    for(int i = 0; i < samples; i++)
    {
        *dst++ += *src++ * level;
    }

    plugin->new_dsp_length += samples;
    

    return result;
}








ReverbPackage::ReverbPackage()
 : LoadPackage() 
{
}


ReverbUnit::ReverbUnit(ReverbEngine *engine, Reverb *plugin)
 : LoadClient(engine)
{
	this->plugin = plugin;
}

ReverbUnit::~ReverbUnit()
{
}

static int counter = 0;
void ReverbUnit::process_package(LoadPackage *package)
{
	ReverbPackage *pkg = (ReverbPackage*)package;
    int channel = pkg->channel;

    for(int i = 0; i < plugin->config.ref_total; i++)
    {
        int src_channel = plugin->ref_channels[channel][i];
        int dst_offset = plugin->ref_offsets[channel][i];
        double level = plugin->ref_levels[channel][i];
        double *dst = plugin->dsp_in[channel] + dst_offset;
        double *src = plugin->get_output(src_channel)->get_data();
        int size = plugin->get_buffer_size();

        if(size + dst_offset > plugin->dsp_in_allocated)
        {
            printf("ReverbUnit::process_package %d size=%d dst_offset=%d needed=%d allocated=%d\n", 
            __LINE__, 
            size,
            dst_offset,
            size + dst_offset,
            plugin->dsp_in_allocated);
        }

        for(int j = 0; j < size; j++)
        {
            *dst++ += *src++ * level;
        }
    }
}



ReverbEngine::ReverbEngine(Reverb *plugin)
 : LoadServer(plugin->PluginClient::smp + 1, plugin->total_in_buffers)
{
	this->plugin = plugin;
}

ReverbEngine::~ReverbEngine()
{
}


void ReverbEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		ReverbPackage *package = (ReverbPackage*)LoadServer::get_package(i);
		package->channel = i;
	}
}

LoadClient* ReverbEngine::new_client()
{
	return new ReverbUnit(this, plugin);
}

LoadPackage* ReverbEngine::new_package()
{
	return new ReverbPackage;
}










ReverbConfig::ReverbConfig()
{
	level_init = 0;
	delay_init = 0;
	ref_level1 = -20;
	ref_level2 = INFINITYGAIN;
	ref_total = 128;
	ref_length = 600;
	high = Freq::tofreq(TOTALFREQS);
	low = Freq::tofreq(0);
	q = 1.0;
    window_size = 4096;
}

int ReverbConfig::equivalent(ReverbConfig &that)
{
	return (EQUIV(level_init, that.level_init) &&
		delay_init == that.delay_init &&
		EQUIV(ref_level1, that.ref_level1) &&
		EQUIV(ref_level2, that.ref_level2) &&
		ref_total == that.ref_total &&
		ref_length == that.ref_length &&
		high == that.high &&
		low == that.low &&
        EQUIV(q, that.q) && 
        window_size == that.window_size);
}

void ReverbConfig::copy_from(ReverbConfig &that)
{
	level_init = that.level_init;
	delay_init = that.delay_init;
	ref_level1 = that.ref_level1;
	ref_level2 = that.ref_level2;
	ref_total = that.ref_total;
	ref_length = that.ref_length;
	high = that.high;
	low = that.low;
    q = that.q;
    window_size = that.window_size;
}

void ReverbConfig::interpolate(ReverbConfig &prev, 
	ReverbConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	level_init = prev.level_init;
	delay_init = prev.delay_init;
	ref_level1 = prev.ref_level1;
	ref_level2 = prev.ref_level2;
	ref_total = prev.ref_total;
	ref_length = prev.ref_length;
    high = prev.high;
    low = prev.low;
    q = prev.q;
    window_size = prev.window_size;
}

void ReverbConfig::boundaries()
{
	CLAMP(level_init, INFINITYGAIN, 0);
	CLAMP(delay_init, 0, MAX_DELAY_INIT);
	CLAMP(ref_level1, INFINITYGAIN, 0);
	CLAMP(ref_level2, INFINITYGAIN, 0);
	CLAMP(ref_total, MIN_REFLECTIONS, MAX_REFLECTIONS);
	CLAMP(ref_length, MIN_REFLENGTH, MAX_REFLENGTH);
	CLAMP(high, 0, Freq::tofreq(TOTALFREQS));
	CLAMP(low, 0, Freq::tofreq(TOTALFREQS));
    CLAMP(q, 0.0, 1.0);
}

void ReverbConfig::dump()
{
	printf("ReverbConfig::dump %d level_init=%f delay_init=%d ref_level1=%f ref_level2=%f ref_total=%d ref_length=%d high=%d low=%d q=%f\n", 
        __LINE__,
		level_init,
		(int)delay_init, 
		ref_level1, 
		ref_level2, 
		(int)ref_total, 
		(int)ref_length, 
		(int)high, 
		(int)low,
        q);
}


