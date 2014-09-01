
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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
#include "edl.h"
#include "indexable.h"

#include <string.h>

Indexable::Indexable(int is_asset) : Garbage(is_asset ? "Asset" : "EDL")
{
	index_state = new IndexState;
	index_state->reset();
	this->is_asset = is_asset;
	strcpy(folder, MEDIA_FOLDER);
}


Indexable::~Indexable()
{
	delete index_state;
}

int Indexable::get_audio_channels()
{
	return 0;
}

int Indexable::get_sample_rate()
{
	return 1;
}

int64_t Indexable::get_audio_samples()
{
	return 0;
}




void Indexable::copy_indexable(Indexable *src)
{
	strcpy(path, src->path);
	index_state->copy_from(src->index_state);
}

int Indexable::have_audio()
{
	return 0;
}

int Indexable::get_w()
{
	return 0;
}

int Indexable::get_h()
{
	return 0;
}

double Indexable::get_frame_rate()
{
	return 0;
}

int Indexable::have_video()
{
	return 0;
}

int Indexable::get_video_layers()
{
	return 0;
}

int64_t Indexable::get_video_frames()
{
	return 0;
}






