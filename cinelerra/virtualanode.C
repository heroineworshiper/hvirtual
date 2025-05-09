
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

#include "aattachmentpoint.h"
#include "aedit.h"
#include "amodule.h"
#include "arender.h"
#include "atrack.h"
#include "automation.h"
#include "bcsignals.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "clip.h"
#include "floatautos.h"
#include "mwindow.h"
#include "module.h"
#include "panauto.h"
#include "plugin.h"
#include "renderengine.h"
#include "samples.h"
#include "track.h"
#include "transition.h"
#include "transportque.h"
#include "virtualaconsole.h"
#include "virtualanode.h"


#include <string.h>

VirtualANode::VirtualANode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_module)
 : VirtualNode(renderengine, 
 		vconsole, 
		real_module, 
		real_plugin,
		track, 
		parent_module)
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		pan_before[i] = pan_after[i] = 0;
	}
}

VirtualANode::~VirtualANode()
{
}





VirtualNode* VirtualANode::create_module(Plugin *real_plugin, 
							Module *real_module, 
							Track *track)
{
	return new VirtualANode(renderengine, 
		vconsole, 
		real_module,
		0,
		track,
		this);
}


VirtualNode* VirtualANode::create_plugin(Plugin *real_plugin)
{
	return new VirtualANode(renderengine, 
		vconsole, 
		0,
		real_plugin,
		track,
		this);
}



int VirtualANode::read_data(Samples *output_temp,
	int64_t size,
	int64_t start_position,
	int64_t sample_rate,
    double playhead_position)
{
	VirtualNode *previous_plugin = 0;

// Current edit in parent track
	AEdit *parent_edit = 0;
	if(parent_node && parent_node->track && renderengine)
	{
		int64_t edl_rate = renderengine->get_edl()->session->sample_rate;
		int64_t start_position_project = (int64_t)(start_position *
			edl_rate /
			sample_rate + 
			0.5);
		parent_edit = (AEdit*)parent_node->track->edits->editof(start_position_project, 
			renderengine->command->get_direction(),
			0);
	}


	if(vconsole->debug_tree) 
		printf("  VirtualANode::read_data position=%lld rate=%lld title=%s parent_node=%p parent_edit=%p\n", 
				(long long)start_position,
				(long long)sample_rate,
				track->title,
				parent_node,
				parent_edit);


// This is a plugin on parent module with a preceeding effect.
// Get data from preceeding effect on parent module.
	if(parent_node && 
		(previous_plugin = parent_node->get_previous_plugin(this)))
	{
		((VirtualANode*)previous_plugin)->render(output_temp,
			size,
			start_position,
			sample_rate,
            playhead_position);
	}
	else
// The current node is the first plugin on parent module.
// The parent module has an edit to read from or the current node
// has no source to read from.
// Read data from parent module
	if(parent_node && (parent_edit || !real_module))
	{
		((VirtualANode*)parent_node)->read_data(output_temp,
			size,
			start_position,
			sample_rate,
            playhead_position);
	}
	else
	if(real_module)
// This is the first node in the tree
	{
		((AModule*)real_module)->render(output_temp,
			size,
			start_position,
			renderengine->command->get_direction(),
			sample_rate,
			0);
	}
	return 0;
}

int VirtualANode::render(Samples *output_temp,
	int64_t size,
	int64_t start_position,
	int64_t sample_rate,
    double playhead_position)
{
	const int debug = 0;
if(debug) printf("VirtualANode::render %d this=%p\n", __LINE__, this);
	ARender *arender = ((VirtualAConsole*)vconsole)->arender;
	if(real_module)
	{
if(debug) printf("VirtualANode::render %d this=%p\n", __LINE__, this);
		render_as_module(arender->audio_out, 
			output_temp,
			size,
			start_position, 
			sample_rate,
            playhead_position);
if(debug) printf("VirtualANode::render %d this=%p\n", __LINE__, this);
	}
	else
	if(real_plugin)
	{
if(debug) printf("VirtualANode::render %d this=%p\n", __LINE__, this);
		render_as_plugin(output_temp,
			size,
			start_position,
			sample_rate,
            playhead_position);
if(debug) printf("VirtualANode::render %d this=%p\n", __LINE__, this);
	}
if(debug) printf("VirtualANode::render %d this=%p\n", __LINE__, this);
	return 0;
}

// If we're the first plugin in the parent module, data needs to be read from 
// what comes before the parent module.  Otherwise, data needs to come from the
// previous plugin.
void VirtualANode::render_as_plugin(Samples *output_temp,
	int64_t size,
	int64_t start_position, 
	int64_t sample_rate,
    double playhead_position)
{
	if(!attachment ||
		!real_plugin ||
		!real_plugin->on) return;


	((AAttachmentPoint*)attachment)->render(
		output_temp, 
		plugin_buffer_number,
		start_position,
		size,
	  	sample_rate,
        playhead_position);
}

int VirtualANode::render_as_module(Samples **audio_out, 
				Samples *output_temp,
				int64_t len,
				int64_t start_position,
				int64_t sample_rate,
                double playhead_position)
{
	int in_output = 0;
	int direction = renderengine->command->get_direction();
	EDL *edl = vconsole->renderengine->get_edl();
	const int debug = 0;

// Process last subnode.  This calls read_data, propogates up the chain 
// of subnodes, and finishes the chain.
	if(subnodes.total)
	{
		VirtualANode *node = (VirtualANode*)subnodes.values[subnodes.total - 1];
		node->render(output_temp,
			len,
			start_position,
			sample_rate,
            playhead_position);

	}
	else
// Read data from previous entity
	{
		read_data(output_temp,
			len,
			start_position,
			sample_rate,
            playhead_position);
	}

// for(int k = 0; k < len; k++)
// {
// 	output_temp->get_data()[k] = (k / 10) % 2;
// }

if(debug) printf("VirtualANode::render_as_module %d\n", __LINE__);
	render_fade(output_temp->get_data(),
				len,
				start_position,
				sample_rate,
				track->automation->autos[AUTOMATION_FADE],
				direction,
				0);
if(debug) printf("VirtualANode::render_as_module %d\n", __LINE__);

// Get the peak but don't limit
// Calculate position relative to project for meters
	int64_t project_sample_rate = edl->session->sample_rate;
	int64_t start_position_project = start_position * 
		project_sample_rate /
		sample_rate;
if(debug) printf("VirtualANode::render_as_module %d\n", __LINE__);
	if(real_module && renderengine->command->realtime)
	{
		ARender *arender = ((VirtualAConsole*)vconsole)->arender;
// Starting sample of meter block
		int64_t meter_render_start;
// Ending sample of meter block
		int64_t meter_render_end;
// Number of samples in each meter fragment normalized to requested rate
		int meter_render_fragment = arender->meter_render_fragment * 
			sample_rate /
			project_sample_rate;


// Scan fragment in meter sized fragments
		for(int i = 0; i < len; )
		{
			int current_level = ((AModule*)real_module)->current_level;
			double peak = 0;
			meter_render_start = i;
			meter_render_end = i + meter_render_fragment;
			if(meter_render_end > len) 
				meter_render_end = len;
// Number of samples into the fragment this meter sized fragment is,
// normalized to project sample rate.
			int64_t meter_render_start_project = meter_render_start *
				project_sample_rate /
				sample_rate;

// Scan meter sized fragment
			for( ; i < meter_render_end; i++)
			{
				double sample = fabs(output_temp->get_data()[i]);
				if(sample > peak) peak = sample;
			}

			((AModule*)real_module)->level_history[current_level] = 
				peak;
			((AModule*)real_module)->level_samples[current_level] = 
				(direction == PLAY_FORWARD) ?
				(start_position_project + meter_render_start_project) :
				(start_position_project - meter_render_start_project);
			((AModule*)real_module)->current_level = 
				arender->get_next_peak(current_level);
		}
	}
if(debug) printf("VirtualANode::render_as_module %d\n", __LINE__);

// process pans and copy the output to the output channels
// Keep rendering unmuted fragments until finished.
	int mute_position = 0;

	for(int i = 0; i < len; )
	{
		int mute_constant;
		int mute_fragment = len - i;
		int mute_fragment_project = mute_fragment *
			project_sample_rate /
			sample_rate;
		start_position_project = start_position + 
			((direction == PLAY_FORWARD) ? i : -i);
		start_position_project = start_position_project *
			project_sample_rate / 
			sample_rate;

// How many samples until the next mute?
		get_mute_fragment(start_position_project,
				mute_constant, 
				mute_fragment_project,
				track->automation->autos[AUTOMATION_MUTE],
				direction,
				0);
// Fragment is playable
		if(!mute_constant)
		{
			for(int j = 0; 
				j < MAX_CHANNELS; 
				j++)
			{
				if(audio_out[j])
				{
					double *buffer = audio_out[j]->get_data();

					render_pan(output_temp->get_data() + mute_position, 
								buffer + mute_position,
								mute_fragment,
								start_position,
								sample_rate,
								(Autos*)track->automation->autos[AUTOMATION_PAN],
								j,
								direction,
								0);
//printf("VirtualANode::render_as_module %d %lld\n", __LINE__, start_position);
				}
			}
		}

		len -= mute_fragment;
		i += mute_fragment;
		mute_position += mute_fragment;
	}
if(debug) printf("VirtualANode::render_as_module %d\n", __LINE__);

	return 0;
}

int VirtualANode::render_fade(double *buffer,
				int64_t len,
				int64_t input_position,
				int64_t sample_rate,
				Autos *autos,
				int direction,
				int use_nudge)
{
	double value, fade_value;
	FloatAuto *previous = 0;
	FloatAuto *next = 0;
	EDL *edl = vconsole->renderengine->get_edl();
	const int debug = 0;
	int64_t project_sample_rate = edl->session->sample_rate;
	if(use_nudge) input_position += track->nudge * 
		sample_rate / 
		project_sample_rate;

if(debug) printf("VirtualANode::render_fade %d\n", __LINE__);
// Normalize input position to project sample rate here.
// Automation functions are general to video and audio so it 
// can't normalize itself.
	int64_t input_position_project = input_position * 
		project_sample_rate / 
		sample_rate;
	int64_t len_project = len * 
		project_sample_rate / 
		sample_rate;

if(debug) printf("VirtualANode::render_fade %d\n", __LINE__);
	if(((FloatAutos*)autos)->automation_is_constant(input_position_project, 
		len_project,
		direction,
		fade_value))
	{
if(debug) printf("VirtualANode::render_fade %d\n", __LINE__);
		if(fade_value <= INFINITYGAIN)
			value = 0;
		else
			value = DB::fromdb(fade_value);
		for(int64_t i = 0; i < len; i++)
		{
			buffer[i] *= value;
		}
if(debug) printf("VirtualANode::render_fade %d\n", __LINE__);
	}
	else
	{
if(debug) printf("VirtualANode::render_fade %d\n", __LINE__);
		for(int64_t i = 0; i < len; i++)
		{
			int64_t slope_len = len - i;
			input_position_project = input_position * 
				project_sample_rate / 
				sample_rate;

			fade_value = ((FloatAutos*)autos)->get_value(input_position_project, 
				direction,
				previous,
				next);

			if(fade_value <= INFINITYGAIN)
				value = 0;
			else
				value = DB::fromdb(fade_value);

			buffer[i] *= value;

			if(direction == PLAY_FORWARD)
				input_position++;
			else
				input_position--;
		}
if(debug) printf("VirtualANode::render_fade %d\n", __LINE__);
	}
if(debug) printf("VirtualANode::render_fade %d\n", __LINE__);

	return 0;
}

int VirtualANode::render_pan(double *input, // start of input fragment
	double *output,            // start of output fragment
	int64_t fragment_len,      // fragment length in input scale
	int64_t input_position,    // starting sample of input buffer in project
	int64_t sample_rate,       // sample rate of input_position
	Autos *autos,
	int channel,
	int direction,
	int use_nudge)
{
	double slope = 0.0;
	double intercept = 1.0;
	EDL *edl = vconsole->renderengine->get_edl();
	int64_t project_sample_rate = edl->session->sample_rate;
	if(use_nudge) input_position += track->nudge * 
		sample_rate / 
		project_sample_rate;

	for(int i = 0; i < fragment_len; )
	{
		int64_t slope_len = (fragment_len - i)  *
							project_sample_rate /
							sample_rate;

// Get slope intercept formula for next fragment
		get_pan_automation(slope, 
						intercept, 
						input_position * 
							project_sample_rate / 
							sample_rate,
						slope_len,
						autos,
						channel,
						direction);

		slope_len = slope_len * sample_rate / project_sample_rate;
		slope = slope * sample_rate / project_sample_rate;
		slope_len = MIN(slope_len, fragment_len - i);

//printf("VirtualANode::render_pan 3 %d %lld %f %p %p\n", i, slope_len, slope, output, input);
		if(!EQUIV(slope, 0))
		{
			for(double j = 0; j < slope_len; j++, i++)
			{
				value = slope * j + intercept;
				output[i] += input[i] * value;
			}
		}
		else
		{
			for(int j = 0; j < slope_len; j++, i++)
			{
				output[i] += input[i] * intercept;
			}
		}


		if(direction == PLAY_FORWARD)
			input_position += slope_len;
		else
			input_position -= slope_len;

//printf("VirtualANode::render_pan 4\n");
	}

	return 0;
}


void VirtualANode::get_pan_automation(double &slope,
	double &intercept,
	int64_t input_position,
	int64_t &slope_len,
	Autos *autos,
	int channel,
	int direction)
{
	intercept = 0;
	slope = 0;

	PanAuto *prev_keyframe = 0;
	PanAuto *next_keyframe = 0;
	prev_keyframe = (PanAuto*)autos->get_prev_auto(input_position, 
		direction, 
		(Auto* &)prev_keyframe);
	next_keyframe = (PanAuto*)autos->get_next_auto(input_position, 
		direction, 
		(Auto* &)next_keyframe);
	
	if(direction == PLAY_FORWARD)
	{
// Two distinct automation points within range
		if(next_keyframe->position > prev_keyframe->position)
		{
			slope = ((double)next_keyframe->values[channel] - prev_keyframe->values[channel]) / 
				((double)next_keyframe->position - prev_keyframe->position);
			intercept = ((double)input_position - prev_keyframe->position) * slope + 
				prev_keyframe->values[channel];

			if(next_keyframe->position < input_position + slope_len)
				slope_len = next_keyframe->position - input_position;
		}
		else
// One automation point within range
		{
			slope = 0;
			intercept = prev_keyframe->values[channel];
		}
	}
	else
	{
// Two distinct automation points within range
		if(next_keyframe->position < prev_keyframe->position)
		{
			slope = ((double)next_keyframe->values[channel] - prev_keyframe->values[channel]) / 
				((double)next_keyframe->position - prev_keyframe->position);
			intercept = ((double)input_position - prev_keyframe->position) * slope + 
				prev_keyframe->values[channel];

			if(next_keyframe->position > input_position - slope_len)
				slope_len = input_position - next_keyframe->position;
		}
		else
// One automation point within range
		{
			slope = 0;
			intercept = next_keyframe->values[channel];
		}
	}
}
