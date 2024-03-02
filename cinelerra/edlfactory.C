/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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


#include "asset.h"
#include "bchash.h"
#include "edl.h"
#include "edlfactory.h"
#include "edlsession.h"
#include "filesystem.h"
#include "localsession.h"
#include "mwindow.h"
#include <string.h>

EDLFactory EDLFactory::instance;

EDLFactory::EDLFactory()
{
}

int EDLFactory::asset_to_edl(EDL *new_edl, 
	Asset *new_asset, 
	RecordLabels *labels,
    int conform)
{
    return asset_to_edl(new_edl, 
	    new_asset, 
	    labels,
        conform,
        MWindow::defaults->get("AUTOASPECT", 0));
}



int EDLFactory::asset_to_edl(EDL *new_edl, 
	Asset *new_asset, 
	RecordLabels *labels,
    int conform,
    int auto_aspect)
{
const int debug = 0;
if(debug) printf("MWindow::asset_to_edl %d new_asset->layers=%d\n", 
__LINE__,
new_asset->layers);
// These parameters would revert the project if VWindow displayed an asset
// of different size than the project.
	if(new_asset->video_data)
	{
		new_edl->session->video_tracks = new_asset->layers;
        
// Change frame rate, sample rate, and output size if desired.
        if(conform)
        {
            new_edl->session->frame_rate = new_asset->frame_rate;
            new_edl->session->output_w = new_asset->width;
            new_edl->session->output_h = new_asset->height;
            if(auto_aspect)
            {
                MWindow::create_aspect_ratio(new_edl->session->aspect_w, 
			        new_edl->session->aspect_h, 
			        new_asset->width, 
			        new_asset->height);
            }
        }
	}
	else
    {
		new_edl->session->video_tracks = 0;
    }

if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);





	if(new_asset->audio_data)
	{
		new_edl->session->audio_tracks = new_asset->channels;
// Change frame rate, sample rate, and output size if desired.
        if(conform)
        {
// set the channels for a file preview
            if(new_edl->session->audio_channels < 0)
                new_edl->session->audio_channels = new_asset->channels;
            new_edl->session->sample_rate = new_asset->sample_rate;
        }
	}
	else
		new_edl->session->audio_tracks = 0;
//printf("MWindow::asset_to_edl 2 %d %d\n", new_edl->session->video_tracks, new_edl->session->audio_tracks);

if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);
	new_edl->create_default_tracks();
//printf("MWindow::asset_to_edl 2 %d %d\n", new_edl->session->video_tracks, new_edl->session->audio_tracks);
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);



//printf("MWindow::asset_to_edl 3\n");
	new_edl->insert_asset(new_asset,
		0,
		0, 
		0, 
		labels);
//printf("MWindow::asset_to_edl 3\n");
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);





	char string[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(string, new_asset->path);
//printf("MWindow::asset_to_edl 3\n");

	strcpy(new_edl->local_session->clip_title, string);
//printf("MWindow::asset_to_edl 4 %s\n", string);
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);

	return 0;
}

void EDLFactory::transition_to_edl(EDL *edl, 
    const char *title,
    int data_type)
{
//     edl->session->video_tracks = 0;
//     edl->session->audio_tracks = 0;
// 
//     if(data_type == TRACK_AUDIO)
//     {
//         edl->session->audio_channels = 1;
//         edl->session->audio_tracks = 1;
//         edl->session->sample_rate = 8000;
//     }
// 
//     if(data_type == TRACK_VIDEO)
//     {
//         edl->session->frame_rate = 30;
//         edl->session->output_w = 320;
//         edl->session->output_h = 240;
//         edl->session->aspect_w = 4;
//         edl->session->aspect_h = 3;
//         edl->session->video_tracks = 1;
//     }
// 
//     edl->create_default_tracks();
}








