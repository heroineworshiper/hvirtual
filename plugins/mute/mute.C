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


#include "mute.h"
#include "language.h"
#include "samples.h"


REGISTER_PLUGIN(MuteMain)




MuteMain::MuteMain(PluginServer *server)
 : PluginAClient(server)
{
}

const char* MuteMain::plugin_title() { return N_("Mute"); }
int MuteMain::is_transition() { return 1; }



int MuteMain::process_realtime(int64_t size, 
	Samples *outgoing, 
	Samples *incoming)
{
    double *in_ptr = incoming->get_data();
    for(int i = 0; i < size; i++)
    {
        in_ptr[i] = 0;
    }
	return 0;
}








