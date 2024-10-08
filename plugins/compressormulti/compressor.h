
/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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

#ifndef COMPRESSOR_H
#define COMPRESSOR_H



#include "bchash.inc"
#include "compressortools.h"
#include "fourier.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "samples.inc"
#include "vframe.inc"

class CompressorEffect;
class CompressorFFT;


//#define LOG_CROSSOVER
#define DRAW_AFTER_BANDPASS

#define TOTAL_BANDS 3

class CompressorConfig : public CompressorConfigBase
{
public:
	CompressorConfig();

	void copy_from(CompressorConfig &that);
	int equivalent(CompressorConfig &that);
	void interpolate(CompressorConfig &prev, 
		CompressorConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

    double q;
    int window_size;
};


class CompressorFFT :public CrossfadeFFT
{
public:
    CompressorFFT(CompressorEffect *plugin, int channel);
    ~CompressorFFT();
    
    int signal_process(int band);
	int post_process(int band);
	int read_samples(int64_t output_sample, 
		int samples, 
		Samples *buffer);

    CompressorEffect *plugin;
    int channel;
    CompressorFrame *frame;
};

// processing state of a single band
class BandState
{
public:
    BandState(CompressorEffect *plugin, int band);
    ~BandState();

    void delete_dsp();
    void reset();
    void reconfigure();
// calculate the envelope for only this band
    void calculate_envelope();
    void process_readbehind(int size, 
        int reaction_samples, 
        int decay_samples,
        int trigger);
    void process_readahead(int size, 
        int preview_samples,
        int reaction_samples, 
        int decay_samples,
        int trigger);
    void allocate_filtered(int new_size);

// bandpass filter for this band
    double *envelope;
    int envelope_allocated;
// The input for all channels with filtering by this band
    Samples **filtered_buffer;
// ending input value of smoothed input
	double next_target;
// starting input value of smoothed input
	double previous_target;
// samples between previous and next target value for readahead
	int target_samples;
// current sample from 0 to target_samples
	int target_current_sample;
// current smoothed input value
	double current_value;
// Temporaries for linear transfer
	ArrayList<compressor_point_t> levels;
    CompressorEffect *plugin;
    int band;
    CompressorEngine *engine;
};

class CompressorEffect : public PluginAClient
{
public:
	CompressorEffect(PluginServer *server);
	~CompressorEffect();

	int is_multichannel();
	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size, 
		Samples **buffer,
		int64_t start_position,
		int sample_rate);

// calculate envelopes of all the bands
    void calculate_envelope();

    void allocate_input(int new_size);

	void reset();
	void update_gui();
    void render_stop();
	void delete_dsp();

	PLUGIN_CLASS_MEMBERS(CompressorConfig)

#ifndef DRAW_AFTER_BANDPASS
// The out of band data for each channel with readahead
	Samples **input_buffer;
// Number of samples in the unfiltered input buffers
	int64_t input_size;
// input buffer size being calculated by the FFT readers
    int64_t new_input_size;
#endif // !DRAW_AFTER_BANDPASS
// Starting sample of the input buffer relative to project in the requested rate.
	int64_t input_start;
// Number of samples in the filtered input buffers
    int64_t filtered_size;

// detect seeking
    int64_t last_position;

// count spectrogram frames for each band
    int new_spectrogram_frames[TOTAL_BANDS];

    BandState *band_states[TOTAL_BANDS];
// The big FFT with multiple channels & multiple bands extracted per channel.
    CompressorFFT **fft;

	int need_reconfigure;
};


#endif
