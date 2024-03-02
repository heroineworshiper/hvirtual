/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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


// output for the file preview

#include "filepreviewer.h"
#include "mutex.h"
#include "previewer.inc"
#include <unistd.h>
#include "vdevicepreview.h"
#include "videodevice.h"


VDevicePreview::VDevicePreview(VideoDevice *device)
 : VDeviceBase(device)
{
    reset_parameters();
}

VDevicePreview::~VDevicePreview()
{
    close_all();
}

int VDevicePreview::reset_parameters()
{
    output_frame = 0;
    return 0;
}

int VDevicePreview::close_all()
{
    delete output_frame;
    reset_parameters();
    return 0;
}

int VDevicePreview::open_output()
{
    return 0;
}

int VDevicePreview::write_buffer(VFrame *output, EDL *edl)
{
//    printf("VDevicePreview::write_buffer %d\n", __LINE__);
    device->previewer->write_frame(output);
}

void VDevicePreview::new_output_buffer(VFrame **result, 
	int file_colormodel, 
	EDL *edl)
{
    if(output_frame && 
        (output_frame->get_w() != device->out_w ||
        output_frame->get_h() != device->out_h ||
        output_frame->get_color_model() != BC_RGB888))
    {
        delete output_frame;
        output_frame = 0;
    }

    if(!output_frame)
    {
        output_frame = new VFrame(
			0, 
			-1,
			device->out_w,
			device->out_h,
			BC_RGB888,
			-1);
    } 
    *result = output_frame;
}






