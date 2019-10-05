
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

#ifndef PLUGINACLIENT_H
#define PLUGINACLIENT_H



#include "maxbuffers.h"
#include "pluginclient.h"
#include "samples.inc"


// Base class for GUI data.
class PluginClientFrame
{
public:
	PluginClientFrame();
// Period_d is 1 second
//	PluginClientFrame(int data_size, int period_n, int period_d);
	virtual ~PluginClientFrame();
    
    void reset();
    
// offset in EDL for synchronizing with playback
    int64_t edl_position;


// some commonly used data
// a user allocated buffer
    double *data;
// Maximum of window in frequency domain
	double freq_max;
// Maximum of window in time domain
	double time_max;
// the window size of a FFT / 2
	int data_size;
	int period_n;
	int period_d;
    int nyquist;
};

class PluginAClient : public PluginClient
{
public:
	PluginAClient(PluginServer *server);
	virtual ~PluginAClient();
	
	int get_render_ptrs();
	int init_realtime_parameters();

	int is_audio();
// These should return 1 if error or 0 if success.
// Multichannel buffer process for backwards compatibility
	virtual int process_realtime(int64_t size, 
		Samples **input_ptr, 
		Samples **output_ptr);
// Single channel buffer process for backwards compatibility and transitions
	virtual int process_realtime(int64_t size, 
		Samples *input_ptr, 
		Samples *output_ptr);

// Process buffer using pull method.  By default this loads the input into the
// buffer and calls process_realtime with input and output pointing to buffer.
// start_position - requested position relative to sample_rate. Relative
//     to start of EDL.  End of buffer if reverse.
// sample_rate - scale of start_position.
	virtual int process_buffer(int64_t size, 
		Samples **buffer,
		int64_t start_position,
		int sample_rate);
	virtual int process_buffer(int64_t size, 
		Samples *buffer,
		int64_t start_position,
		int sample_rate);


	virtual int process_loop(Samples *buffer, int64_t &write_length) { return 1; };
	virtual int process_loop(Samples **buffers, int64_t &write_length) { return 1; };
	int plugin_process_loop(Samples **buffers, int64_t &write_length);

	int plugin_start_loop(int64_t start, 
		int64_t end, 
		int64_t buffer_size, 
		int total_buffers);

	int plugin_get_parameters();

// Called by non-realtime client to read audio for processing.
// buffer - output wave
// channel - channel of the plugin input for multichannel plugin
// start_position - start of samples in forward.  End of samples in reverse.
//     Relative to start of EDL.  Scaled to sample_rate.
// len - number of samples to read
	int read_samples(Samples *buffer, 
		int channel, 
		int64_t start_position, 
		int64_t len);
	int read_samples(Samples *buffer, 
		int64_t start_position, 
		int64_t len);

// Called by realtime plugin to read audio from previous entity
// sample_rate - scale of start_position.  Provided so the client can get data
//     at a higher fidelity than provided by the EDL.
	int read_samples(Samples *buffer,
		int channel,
		int sample_rate,
		int64_t start_position,
		int64_t len);


// audio has to be sent to the GUI asynchronously
// server calls to send rendered data to the GUI instance
	void plugin_render_gui(void *data);
// user calls after seeking
    void send_reset_gui_frames();
// server calls in the GUI instance
    void reset_gui_frames();
// User calls to send data to the GUI instance
	void add_gui_frame(PluginClientFrame *frame);
// Called by client to send data to the GUI instance
//	void send_render_gui();

// Called by the GUI instance to get the number of GUI frames to show
	int pending_gui_frames();
// Called by processor instance to get the total number of frames sent in process_buffer
    int get_gui_frames();
// Get next GUI frame from frame_buffer.  Client must delete it.
// returns 0 when client has caught up
	PluginClientFrame* get_gui_frame();
// manage GUI data in the process_buffer routine
	void begin_process_buffer();
	void end_process_buffer();






// Get the sample rate of the EDL
	int get_project_samplerate();
// Get the requested sample rate
	int get_samplerate();

// get the buffer argument to process_buffer
    Samples* get_output(int channel);

	int64_t local_to_edl(int64_t position);
	int64_t edl_to_local(int64_t position);

// the buffers passed to process_buffer
	Samples **output_buffers;

// Frames for updating GUI
	ArrayList<PluginClientFrame*> frame_buffer;

// // point to the start of the buffers
// 	ArrayList<float**> input_ptr_master;
// 	ArrayList<float**> output_ptr_master;
// // point to the regions for a single render
// 	float **input_ptr_render;
// 	float **output_ptr_render;
// sample rate of EDL.  Used for normalizing keyframes
	int project_sample_rate;
// Local parameters set by non realtime plugin about the file to be generated.
// Retrieved by server to set output file format.
// In realtime plugins, these are set before every process_buffer as the
// requested rates.
	int sample_rate;
};



#endif
