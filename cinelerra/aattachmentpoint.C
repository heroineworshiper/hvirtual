
/*
 * CINELERRA
 * Copyright (C) 2009-2019 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "keyframe.h"
#include "playbackconfig.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "samples.h"
#include "transportque.h"

AAttachmentPoint::AAttachmentPoint(RenderEngine *renderengine, Plugin *plugin)
: AttachmentPoint(renderengine, plugin, TRACK_AUDIO)
{
	buffer_vector = 0;
	buffer_allocation = 0;
}

AAttachmentPoint::~AAttachmentPoint()
{
	delete_buffer_vector();
}

void AAttachmentPoint::delete_buffer_vector()
{
	if(buffer_vector)
	{
		for(int i = 0; i < virtual_plugins.total; i++)
			delete buffer_vector[i];
		delete [] buffer_vector;
	}
	buffer_vector = 0;
	buffer_allocation = 0;
}

void AAttachmentPoint::new_buffer_vector(int total, int size)
{
	if(buffer_vector && size > buffer_allocation)
		delete_buffer_vector();

	if(!buffer_vector)
	{
		buffer_allocation = size;
		buffer_vector = new Samples*[virtual_plugins.total];
		for(int i = 0; i < virtual_plugins.total; i++)
		{
			buffer_vector[i] = new Samples(buffer_allocation);
		}
	}
}

int AAttachmentPoint::get_buffer_size()
{
	return renderengine->config->aconfig->fragment_size;
}

void AAttachmentPoint::render(Samples *output, 
	int buffer_number,
	int64_t start_position, 
	int64_t len,
	int64_t sample_rate,
    double playhead_position)
{
	if(!plugin_server || !plugin->on) return;
    int project_sample_rate = renderengine->get_edl()->session->sample_rate;
    int direction = renderengine->command->get_direction();
// the output buffers
    Samples **output_temp = 0;
// starting offsets for the output buffers
    int *output_offsets = 0;
// how many output buffers
    int output_temps = 0;
// Plugin server to do signal processing
    PluginServer *edl_plugin_server = 0;
    int sign = 1;

// plugin DB
	if(plugin_server->multichannel)
	{
        edl_plugin_server = plugin_servers.get(0);
// Reuse previous data if we already processed
		if(is_processed &&
			this->start_position == start_position && 
			this->len == len && 
			this->sample_rate == sample_rate)
		{
			memcpy(output->get_data(), 
				buffer_vector[buffer_number]->get_data(), 
				sizeof(double) * len);
			return;
		}

// Update status
		this->start_position = start_position;
		this->len = len;
		this->sample_rate = sample_rate;
		is_processed = 1;

// Allocate buffers for all the channels
		new_buffer_vector(virtual_plugins.size(), len);

// Create temporary buffer vector with output argument substituted in
        output_temps = virtual_plugins.size();
		output_temp = new Samples*[output_temps];
        output_offsets = new int[output_temps];
		for(int i = 0; i < virtual_plugins.size(); i++)
		{
			if(i == buffer_number)
			{
            	output_temp[i] = output;
			}
            else
			{
            	output_temp[i] = buffer_vector[i];
            }
            output_offsets[i] = output_temp[i]->get_offset();
		}
    }
    else
    {
        edl_plugin_server = plugin_servers.get(buffer_number);
        output_temps = 1;
		output_temp = new Samples*[output_temps];
        output_offsets = new int[output_temps];
		output_temp[0] = output;
        output_offsets[0] = output_temp[0]->get_offset();
    }



// process in fragments bounded by keyframes. 
// Plugin decides to use the content of the next or prev keyframe based on direction.
    KeyFrame *keyframe = 0;
    if(direction == PLAY_FORWARD)
    {
        sign = 1;
        keyframe = edl_plugin_server->get_prev_keyframe(
            start_position *
                project_sample_rate /
                sample_rate);
    }
    else
    {
        sign = -1;
        keyframe = edl_plugin_server->get_next_keyframe(
            start_position *
                project_sample_rate /
                sample_rate);
    }

// keyframe position in local samplerate
    int64_t keyframe_position = keyframe->position *
        sample_rate /
        project_sample_rate;
// advance in local samplerate
    for(int64_t offset = 0, fragment = 0; offset < len; offset += fragment)
    {
        fragment = len - offset;
        int limit_fragment = 1;
//printf("AAttachmentPoint::render %d this=%p keyframe=%p\n", __LINE__, this, keyframe);

// get the next keyframe
        if(keyframe)
        {
            if(direction == PLAY_FORWARD)
            {
                keyframe = (KeyFrame*)keyframe->next;
            }
            else
    // backward
            {
                keyframe = (KeyFrame*)keyframe->previous;
            }
        }

// previous keyframe is before the current position
        if(direction == PLAY_FORWARD && keyframe_position <= start_position + offset ||
            direction == PLAY_REVERSE && keyframe_position >= start_position - offset)
        {
// limit to the next keyframe's position
            if(keyframe)
            {
                keyframe_position = keyframe->position *
                    sample_rate /
                    project_sample_rate;
            }
            else
            {
                limit_fragment = 0;
            }
        }

// truncate to next keyframe
        if(limit_fragment &&
            (keyframe_position - start_position) * sign - offset < fragment)
        {
            fragment = (keyframe_position - start_position) * sign - offset;
        }

// minimum fragment size
        if(fragment <= 0)
        {
            printf("AAttachmentPoint::render %d invalid fragment length=%ld start_position=%ld offset=%ld\n",
                __LINE__,
                fragment,
                start_position,
                offset);
            fragment = 1;
        }

// Plugins assume the buffer offset is random
        for(int i = 0; i < output_temps; i++)
        {
            output_temp[i]->set_offset(output_offsets[i] + offset);
        }

// set the playhead position
        edl_plugin_server->playhead_position = playhead_position + 
            (double)offset * 
                sign / 
                sample_rate;

// process the fragment
// printf("AAttachmentPoint::render %d this=%p title=%s keyframe=%p fragment=%ld offset=%d allocated=%d\n", 
// __LINE__, 
// this,
// edl_plugin_server->title,
// keyframe,
// fragment,
// output_temp[0]->get_offset(),
// output_temp[0]->get_allocated());
		edl_plugin_server->process_buffer(output_temp,
			start_position + offset * sign,
			fragment,
			sample_rate,
			plugin->length *
				sample_rate /
				project_sample_rate,
			direction);

	}

// reset the buffer offsets
	for(int i = 0; i < output_temps; i++)
	{
		output_temp[i]->set_offset(output_offsets[i]);
	}

// Delete temporary buffer vector
	delete [] output_temp;
    delete [] output_offsets;
}




