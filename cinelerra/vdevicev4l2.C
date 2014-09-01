
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#undef _FILE_OFFSET_BITS
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE

#include "assets.h"
#include "bcsignals.h"
#include "channel.h"
#include "chantables.h"
#include "clip.h"
#include "condition.h"
#include "file.h"
#include "mutex.h"
#include "picture.h"
#include "preferences.h"
#include "quicktime.h"
#include "recordconfig.h"
#include "vdevicev4l2.h"
#include "vframe.h"
#include "videodevice.h"

#ifdef HAVE_VIDEO4LINUX2

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>



// Webcam data
#define DHT_SIZE 420
#define HEADERFRAME1 0xaf


static unsigned char dht_data[DHT_SIZE] = {
  0xff, 0xc4, 0x01, 0xa2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
  0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x01, 0x00, 0x03,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
  0x0a, 0x0b, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05,
  0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d, 0x01, 0x02, 0x03, 0x00, 0x04,
  0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22,
  0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15,
  0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36,
  0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
  0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
  0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95,
  0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
  0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2,
  0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5,
  0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
  0xfa, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05,
  0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04,
  0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22,
  0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33,
  0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25,
  0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36,
  0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
  0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
  0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94,
  0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
  0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
};








VDeviceV4L2Put::VDeviceV4L2Put(VDeviceV4L2Thread *thread)
 : Thread(1, 0, 0)
{
	this->thread = thread;
	done = 0;
	lock = new Mutex("VDeviceV4L2Put::lock");
	more_buffers = new Condition(0, "VDeviceV4L2Put::more_buffers");
	putbuffers = new int[0xff];
	total = 0;
}

VDeviceV4L2Put::~VDeviceV4L2Put()
{
	if(Thread::running())
	{
		done = 1;
		more_buffers->unlock();
		Thread::cancel();
		Thread::join();
	}
	delete lock;
	delete more_buffers;
	delete [] putbuffers;
}

void VDeviceV4L2Put::run()
{
	while(!done)
	{
		more_buffers->lock("VDeviceV4L2Put::run");
		if(done) break;
		lock->lock("VDeviceV4L2Put::run");
		int buffer = -1;
		if(total)
		{
			buffer = putbuffers[0];
			for(int i = 0; i < total - 1; i++)
				putbuffers[i] = putbuffers[i + 1];
			total--;
		}
		lock->unlock();

		if(buffer >= 0)
		{
			struct v4l2_buffer arg;
			bzero(&arg, sizeof(arg));
			arg.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			arg.index = buffer;
			arg.memory = V4L2_MEMORY_MMAP;

			Thread::enable_cancel();
			thread->ioctl_lock->lock("VDeviceV4L2Put::run");
// This locks up if there's no signal.
			if(ioctl(thread->input_fd, VIDIOC_QBUF, &arg) < 0)
				perror("VDeviceV4L2Put::run 1 VIDIOC_QBUF");
			thread->ioctl_lock->unlock();
// Delay to keep mutexes from getting stuck
			usleep(1000000 * 1001 / 60000);
			Thread::disable_cancel();
		}
	}
}

void VDeviceV4L2Put::put_buffer(int number)
{
	lock->lock("VDeviceV4L2Put::put_buffer");
	putbuffers[total++] = number;
	lock->unlock();
	more_buffers->unlock();
}











VDeviceV4L2Thread::VDeviceV4L2Thread(VideoDevice *device, int color_model)
 : Thread(1, 0, 0)
{
	this->device = device;
	this->color_model = color_model;

	video_lock = new Condition(0, "VDeviceV4L2Thread::video_lock");
	buffer_lock = new Mutex("VDeviceV4L2Thread::buffer_lock");
	ioctl_lock = new Mutex("VDeviceV4L2Thread::ioctl_lock");
	device_buffers = 0;
	buffer_valid = 0;
	current_inbuffer = 0;
	current_outbuffer = 0;
	total_buffers = 0;
	first_frame = 1;
	done = 0;
	input_fd = 0;
	total_valid = 0;
	put_thread = 0;
}

VDeviceV4L2Thread::~VDeviceV4L2Thread()
{
	if(Thread::running())
	{
		done = 1;
//printf("VDeviceV4L2Thread::~VDeviceV4L2Thread %d\n", __LINE__);
		Thread::cancel();
		Thread::join();
//printf("VDeviceV4L2Thread::~VDeviceV4L2Thread %d\n", __LINE__);
	}

	if(put_thread) delete put_thread;

	if(buffer_valid)
	{
		delete [] buffer_valid;
	}

// Buffers are not unmapped by close.
	if(device_buffers)
	{
		for(int i = 0; i < total_buffers; i++)
		{
			if(device_buffers[i]->get_data()) 
			{
				if(device_buffers[i]->get_compressed_allocated())
				{
					munmap(device_buffers[i]->get_data(),
						device_buffers[i]->get_compressed_allocated());
				}
				else
				{
//printf("VDeviceV4L2Thread::~VDeviceV4L2Thread %d %p\n", __LINE__, device_buffers[i]->get_data());
					munmap(device_buffers[i]->get_data(),
						device_buffers[i]->get_data_size());
				}
			}

			delete device_buffers[i];
		}

		delete [] device_buffers;
	}

	if(input_fd > 0) 
	{
		int streamoff_arg = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(ioctl(input_fd, VIDIOC_STREAMOFF, &streamoff_arg) < 0)
			perror("VDeviceV4L2Thread::~VDeviceV4L2Thread VIDIOC_STREAMOFF");
		close(input_fd);
//printf("VDeviceV4L2Thread::~VDeviceV4L2Thread %d\n", __LINE__);
	}

	delete video_lock;
	delete buffer_lock;
	delete ioctl_lock;
}

void VDeviceV4L2Thread::start()
{
	total_buffers = device->in_config->capture_length;
	total_buffers = MAX(total_buffers, 2);
	total_buffers = MIN(total_buffers, 0xff);
	put_thread = new VDeviceV4L2Put(this);
	put_thread->start();
	Thread::start();
}

// Allocate user space buffers
void VDeviceV4L2Thread::allocate_buffers(int number)
{
	if(number != total_buffers)
	{
		if(buffer_valid) delete [] buffer_valid;
		if(device_buffers)
		{
			for(int i = 0; i < total_buffers; i++)
			{
				delete device_buffers[i];
			}
			delete [] device_buffers;
		}
	}

	total_buffers = number;
	buffer_valid = new int[total_buffers];
	device_buffers = new VFrame*[total_buffers];
	for(int i = 0; i < total_buffers; i++)
	{
		device_buffers[i] = new VFrame;
	}
	bzero(buffer_valid, sizeof(int) * total_buffers);
}

void VDeviceV4L2Thread::run()
{
// Set up the device
	int error = 0;
	Thread::enable_cancel();

//printf("VDeviceV4L2Thread::run %d\n", __LINE__);


	if((input_fd = open(device->in_config->v4l2_in_device, 
		O_RDWR)) < 0)
	{
		perror("VDeviceV4L2Thread::run");
		error = 1;
	}

	if(!error)
	{
		device->set_cloexec_flag(input_fd, 1);


		struct v4l2_capability cap;
		if(ioctl(input_fd, VIDIOC_QUERYCAP, &cap))
			perror("VDeviceV4L2Thread::run VIDIOC_QUERYCAP");

// printf("VDeviceV4L2Thread::run input_fd=%d driver=%s card=%s bus_info=%s version=%d\n",
// input_fd,
// cap.driver,
// cap.card,
// cap.bus_info,
// cap.version);
// printf("    %s%s%s%s%s%s%s%s%s%s%s%s\n", 
// (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ? "V4L2_CAP_VIDEO_CAPTURE " : "",
// (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) ? "V4L2_CAP_VIDEO_OUTPUT " : "",
// (cap.capabilities & V4L2_CAP_VIDEO_OVERLAY) ? "V4L2_CAP_VIDEO_OVERLAY " : "",
// (cap.capabilities & V4L2_CAP_VBI_CAPTURE) ? "V4L2_CAP_VBI_CAPTURE " : "",
// (cap.capabilities & V4L2_CAP_VBI_OUTPUT) ? "V4L2_CAP_VBI_OUTPUT " : "",
// (cap.capabilities & V4L2_CAP_RDS_CAPTURE) ? "V4L2_CAP_RDS_CAPTURE " : "",
// (cap.capabilities & V4L2_CAP_TUNER) ? "V4L2_CAP_TUNER " : "",
// (cap.capabilities & V4L2_CAP_AUDIO) ? "V4L2_CAP_AUDIO " : "",
// (cap.capabilities & V4L2_CAP_RADIO) ? "V4L2_CAP_RADIO " : "",
// (cap.capabilities & V4L2_CAP_READWRITE) ? "V4L2_CAP_READWRITE " : "",
// (cap.capabilities & V4L2_CAP_ASYNCIO) ? "V4L2_CAP_ASYNCIO " : "",
// (cap.capabilities & V4L2_CAP_STREAMING) ? "V4L2_CAP_STREAMING " : "");


// Set up frame rate
		struct v4l2_streamparm v4l2_parm;
		v4l2_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(ioctl(input_fd, VIDIOC_G_PARM, &v4l2_parm) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_G_PARM");
		if(v4l2_parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
		{
			v4l2_parm.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;


			v4l2_parm.parm.capture.timeperframe.numerator = 1;
			v4l2_parm.parm.capture.timeperframe.denominator = 
				(unsigned long)((float)1 / 
				device->frame_rate * 
				10000000);
			if(ioctl(input_fd, VIDIOC_S_PARM, &v4l2_parm) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_PARM");

			if(ioctl(input_fd, VIDIOC_G_PARM, &v4l2_parm) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_G_PARM");
		}

// Set up data format
		struct v4l2_format v4l2_params;
		v4l2_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(ioctl(input_fd, VIDIOC_G_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_G_FMT");
		v4l2_params.fmt.pix.width = device->in_config->w;
		v4l2_params.fmt.pix.height = device->in_config->h;




// Probe the compression format
		int pixel_formats[] =
		{
/* RGB formats */
			V4L2_PIX_FMT_RGB332,
			V4L2_PIX_FMT_RGB444,
			V4L2_PIX_FMT_RGB555,
			V4L2_PIX_FMT_RGB565,
			V4L2_PIX_FMT_RGB555X,
			V4L2_PIX_FMT_RGB565X,
			V4L2_PIX_FMT_BGR24  ,
			V4L2_PIX_FMT_RGB24  ,
			V4L2_PIX_FMT_BGR32  ,
			V4L2_PIX_FMT_RGB32  ,

/* Grey formats */
			V4L2_PIX_FMT_GREY ,
			V4L2_PIX_FMT_Y10  ,
			V4L2_PIX_FMT_Y16  ,

/* Palette formats */
			V4L2_PIX_FMT_PAL8 ,

/* Luminance+Chrominance formats */
			V4L2_PIX_FMT_YVU410 ,
			V4L2_PIX_FMT_YVU420 ,
			V4L2_PIX_FMT_YUYV   ,
			V4L2_PIX_FMT_YYUV   ,
			V4L2_PIX_FMT_YVYU   ,
			V4L2_PIX_FMT_UYVY   ,
			V4L2_PIX_FMT_VYUY   ,
			V4L2_PIX_FMT_YUV422P,
			V4L2_PIX_FMT_YUV411P,
			V4L2_PIX_FMT_Y41P   ,
			V4L2_PIX_FMT_YUV444 ,
			V4L2_PIX_FMT_YUV555 ,
			V4L2_PIX_FMT_YUV565 ,
			V4L2_PIX_FMT_YUV32  ,
			V4L2_PIX_FMT_YUV410 ,
			V4L2_PIX_FMT_YUV420 ,
			V4L2_PIX_FMT_HI240  ,
			V4L2_PIX_FMT_HM12   ,

/* two planes -- one Y, one Cr + Cb interleaved  */
			V4L2_PIX_FMT_NV12 ,
			V4L2_PIX_FMT_NV21 ,
			V4L2_PIX_FMT_NV16 ,
			V4L2_PIX_FMT_NV61 ,

/* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
			V4L2_PIX_FMT_SBGGR8,
			V4L2_PIX_FMT_SGBRG8,
			V4L2_PIX_FMT_SGRBG8,
			V4L2_PIX_FMT_SRGGB8,
			V4L2_PIX_FMT_SBGGR10,
			V4L2_PIX_FMT_SGBRG10,
			V4L2_PIX_FMT_SGRBG10,
			V4L2_PIX_FMT_SRGGB10,
/* 10bit raw bayer DPCM compressed to 8 bits */
			V4L2_PIX_FMT_SGRBG10DPCM8,
/*
 * 10bit raw bayer, expanded to 16 bits
 * xxxxrrrrrrrrrrxxxxgggggggggg xxxxggggggggggxxxxbbbbbbbbbb...
 */
			V4L2_PIX_FMT_SBGGR16,

/* compressed formats */
			V4L2_PIX_FMT_MJPEG,
			V4L2_PIX_FMT_JPEG ,
			V4L2_PIX_FMT_DV   ,
			V4L2_PIX_FMT_MPEG ,

/*  Vendor-specific formats   */
			V4L2_PIX_FMT_CPIA1  ,
			V4L2_PIX_FMT_WNVA   ,
			V4L2_PIX_FMT_SN9C10X,
			V4L2_PIX_FMT_SN9C20X_I420,
			V4L2_PIX_FMT_PWC1    ,
			V4L2_PIX_FMT_PWC2    ,
			V4L2_PIX_FMT_ET61X251,
			V4L2_PIX_FMT_SPCA501 ,
			V4L2_PIX_FMT_SPCA505 ,
			V4L2_PIX_FMT_SPCA508 ,
			V4L2_PIX_FMT_SPCA561 ,
			V4L2_PIX_FMT_PAC207  ,
			V4L2_PIX_FMT_MR97310A,
			V4L2_PIX_FMT_SN9C2028,
			V4L2_PIX_FMT_SQ905C  ,
			V4L2_PIX_FMT_PJPG    ,
			V4L2_PIX_FMT_OV511   ,
			V4L2_PIX_FMT_OV518   ,
			V4L2_PIX_FMT_STV0680

		};

// Show formats
#if 1
   		for(int i = 0; i < sizeof(pixel_formats) / sizeof(int); i++)
   		{
   			v4l2_params.fmt.pix.pixelformat = pixel_formats[i];
   			if(ioctl(input_fd, VIDIOC_S_FMT, &v4l2_params) >= 0)
   			{
   				printf("VDeviceV4L2Thread::run %d format %d %c%c%c%c is good\n", 
   					__LINE__, 
   					i,
   					((unsigned char*)&pixel_formats[i])[0],
   					((unsigned char*)&pixel_formats[i])[1],
   					((unsigned char*)&pixel_formats[i])[2],
   					((unsigned char*)&pixel_formats[i])[3]);
   			}
   		}
#endif


//printf("VDeviceV4L2Thread::run %d\n", __LINE__);


		if(device->in_config->driver == VIDEO4LINUX2JPEG)
			v4l2_params.fmt.pix.pixelformat = 
				V4L2_PIX_FMT_MJPEG;
		else
		if(device->in_config->driver == CAPTURE_JPEG_WEBCAM)
			v4l2_params.fmt.pix.pixelformat = 
				V4L2_PIX_FMT_MJPEG;
		else
		if(device->in_config->driver == CAPTURE_MPEG)
			v4l2_params.fmt.pix.pixelformat = 
				V4L2_PIX_FMT_MPEG;
		else
		if(device->in_config->driver == CAPTURE_YUYV_WEBCAM)
			v4l2_params.fmt.pix.pixelformat = 
				V4L2_PIX_FMT_YUYV;
		else
			v4l2_params.fmt.pix.pixelformat = 
				VDeviceV4L2::cmodel_to_device(color_model);

   		printf("VDeviceV4L2Thread::run %d using format %c%c%c%c\n", 
   			__LINE__, 
   			((unsigned char*)&v4l2_params.fmt.pix.pixelformat)[0],
   			((unsigned char*)&v4l2_params.fmt.pix.pixelformat)[1],
   			((unsigned char*)&v4l2_params.fmt.pix.pixelformat)[2],
   			((unsigned char*)&v4l2_params.fmt.pix.pixelformat)[3]);

		if(ioctl(input_fd, VIDIOC_S_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_S_FMT");
		if(ioctl(input_fd, VIDIOC_G_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_G_FMT");

// Set input
		Channel *device_channel = 0;
		if(device->channel->input >= 0 &&
			device->channel->input < device->get_inputs()->total)
		{
			device_channel = device->get_inputs()->values[
				device->channel->input];
		}

// Try first input
		if(!device_channel)
		{
			if(device->get_inputs()->total)
			{
				device_channel = device->get_inputs()->values[0];
				printf("VDeviceV4L2Thread::run user channel not found.  Using %s\n",
					device_channel->device_name);
			}
			else
			{
				printf("VDeviceV4L2Thread::run channel \"%s\" not found.\n",
					device->channel->title);
			}
		}




// Set picture controls.  This driver requires the values to be set once to default
// values and then again to different values before it takes up the values.
// Unfortunately VIDIOC_S_CTRL resets the audio to mono in 2.6.7.
		PictureConfig *picture = device->picture;
		for(int i = 0; i < picture->controls.total; i++)
		{
			struct v4l2_control ctrl_arg;
			struct v4l2_queryctrl arg;
			PictureItem *item = picture->controls.values[i];
			arg.id = item->device_id;
			if(!ioctl(input_fd, VIDIOC_QUERYCTRL, &arg))
			{
				ctrl_arg.id = item->device_id;
				ctrl_arg.value = 0;
				if(ioctl(input_fd, VIDIOC_S_CTRL, &ctrl_arg) < 0)
					perror("VDeviceV4L2Thread::run VIDIOC_S_CTRL");
			}
			else
			{
				printf("VDeviceV4L2Thread::run VIDIOC_S_CTRL 1 id %d failed\n",
					item->device_id);
			}
		}


		for(int i = 0; i < picture->controls.total; i++)
		{
			struct v4l2_control ctrl_arg;
			struct v4l2_queryctrl arg;
			PictureItem *item = picture->controls.values[i];
			arg.id = item->device_id;

			if(!ioctl(input_fd, VIDIOC_QUERYCTRL, &arg))
			{
				ctrl_arg.id = item->device_id;
				ctrl_arg.value = item->value;
				if(ioctl(input_fd, VIDIOC_S_CTRL, &ctrl_arg) < 0)
					perror("VDeviceV4L2Thread::run VIDIOC_S_CTRL");
			}
			else
			{
				printf("VDeviceV4L2Thread::run VIDIOC_S_CTRL 2 id %d failed\n",
					item->device_id);
			}
		}


// Translate input to API structures
		struct v4l2_tuner tuner;
		int input = 0;

		if(ioctl(input_fd, VIDIOC_G_TUNER, &tuner) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_G_TUNER");

// printf("VDeviceV4L2Thread::run audmode=%d rxsubchans=%d\n",
// tuner.audmode,
// tuner.rxsubchans);
		if(device_channel)
		{
			tuner.index = device_channel->device_index;
			input = device_channel->device_index;
		}
		else
		{
			tuner.index = 0;
			input = 0;
		}


		tuner.type = V4L2_TUNER_ANALOG_TV;
		tuner.audmode = V4L2_TUNER_MODE_STEREO;
		tuner.rxsubchans = V4L2_TUNER_SUB_STEREO;

		if(ioctl(input_fd, VIDIOC_S_INPUT, &input) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_S_INPUT");






// Set norm
		v4l2_std_id std_id;
		switch(device->channel->norm)
		{
			case NTSC: std_id = V4L2_STD_NTSC; break;
			case PAL: std_id = V4L2_STD_PAL; break;
			case SECAM: std_id = V4L2_STD_SECAM; break;
			default: std_id = V4L2_STD_NTSC_M; break;
		}

		if(ioctl(input_fd, VIDIOC_S_STD, &std_id))
			perror("VDeviceV4L2Thread::run VIDIOC_S_STD");


		if(cap.capabilities & V4L2_CAP_TUNER)
		{
			if(ioctl(input_fd, VIDIOC_S_TUNER, &tuner) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_TUNER");

// Set frequency
			struct v4l2_frequency frequency;
			frequency.tuner = device->channel->tuner;
			frequency.type = V4L2_TUNER_ANALOG_TV;
			frequency.frequency = (int)(chanlists[
				device->channel->freqtable].list[
					device->channel->entry].freq * 0.016) +
					device->channel->fine_tune;
// printf("VDeviceV4L2Thread::run tuner=%d freq=%d norm=%d\n",
// device->channel->tuner,
// frequency.frequency,
// device->channel->norm);
			if(ioctl(input_fd, VIDIOC_S_FREQUENCY, &frequency) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_FREQUENCY");
		}

// 		struct v4l2_crop crop;
// 		if(ioctl(input_fd, VIDIOC_G_CROP, &crop) < 0)
// 				perror("VDeviceV4L2Thread::run VIDIOC_G_CROP");
// 		printf("VDeviceV4L2Thread::run %d\n", __LINE__);

// Set compression
		if(device->in_config->driver == VIDEO4LINUX2JPEG ||
			device->in_config->driver == CAPTURE_JPEG_WEBCAM)
		{
			struct v4l2_jpegcompression jpeg_arg;
			if(ioctl(input_fd, VIDIOC_G_JPEGCOMP, &jpeg_arg) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_G_JPEGCOMP");
			jpeg_arg.quality = device->quality / 2;
			if(ioctl(input_fd, VIDIOC_S_JPEGCOMP, &jpeg_arg) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_JPEGCOMP");
		}

// Allocate buffers.  Errors here are fatal.
		Thread::disable_cancel();
		struct v4l2_requestbuffers requestbuffers;

//		printf("VDeviceV4L2Thread::run requested %d buffers\n", total_buffers);
		requestbuffers.count = total_buffers;
		requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		requestbuffers.memory = V4L2_MEMORY_MMAP;
		if(ioctl(input_fd, VIDIOC_REQBUFS, &requestbuffers) < 0)
		{
			perror("VDeviceV4L2Thread::run VIDIOC_REQBUFS");
			error = 1;
		}
		else
		{
// The requestbuffers.count changes in the 2.6.5 version of the API
			allocate_buffers(requestbuffers.count);

printf("VDeviceV4L2Thread::run got %d buffers\n", total_buffers);
			for(int i = 0; i < total_buffers; i++)
			{
				struct v4l2_buffer buffer;
				buffer.type = requestbuffers.type;
				buffer.index = i;
				if(ioctl(input_fd, VIDIOC_QUERYBUF, &buffer) < 0)
				{
					perror("VDeviceV4L2Thread::run VIDIOC_QUERYBUF");
					error = 1;
					break;
				}

				unsigned char *data = (unsigned char*)mmap(NULL,
					buffer.length,
					PROT_READ | PROT_WRITE,
					MAP_SHARED,
					input_fd,
					buffer.m.offset);
//printf("VDeviceV4L2Thread::run %d %p\n", __LINE__, data);
				if(data == MAP_FAILED)
				{
					perror("VDeviceV4L2Thread::run mmap");
					error = 1;
					break;
				}

				VFrame *frame = device_buffers[i];
				if(device->in_config->driver == VIDEO4LINUX2JPEG ||
					device->in_config->driver == CAPTURE_JPEG_WEBCAM ||
					device->in_config->driver == CAPTURE_MPEG)
				{
					frame->set_compressed_memory(data,
						0,
						0,
						buffer.length);
				}
				else
				{
					int y_offset = 0;
					int u_offset = 0;
					int v_offset = 0;

					switch(color_model)
					{
						case BC_YUV422P:
							u_offset = device->in_config->w * device->in_config->h;
							v_offset = device->in_config->w * device->in_config->h + device->in_config->w * device->in_config->h / 2;
							break;
						case BC_YUV420P:
						case BC_YUV411P:
// In 2.6.7, the v and u are inverted for 420 but not 422
							v_offset = device->in_config->w * device->in_config->h;
							u_offset = device->in_config->w * device->in_config->h + device->in_config->w * device->in_config->h / 4;
							break;
					}

//printf("VDeviceV4L2Thread::run color_model=%d\n", color_model);
					frame->reallocate(data,
						0,
						y_offset,
						u_offset,
						v_offset,
						device->in_config->w,
						device->in_config->h,
						color_model,
						VFrame::calculate_bytes_per_pixel(color_model) *
							device->in_config->w);
				}
			}
		}
		Thread::enable_cancel();
	}



// Start capturing
	if(!error)
	{
		for(int i = 0; i < total_buffers; i++)
		{
			struct v4l2_buffer buffer;
			buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buffer.memory = V4L2_MEMORY_MMAP;
			buffer.index = i;
			if(ioctl(input_fd, VIDIOC_QBUF, &buffer) < 0)
			{
				perror("VDeviceV4L2Thread::run 2 VIDIOC_QBUF");
				break;
			}
		}


		int streamon_arg = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(ioctl(input_fd, VIDIOC_STREAMON, &streamon_arg) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_STREAMON");
	}

	Thread::disable_cancel();


// Read buffers continuously
	first_frame = 0;
	int total_errors = 0;
	while(!done && !error)
	{

		struct v4l2_buffer buffer;
		bzero(&buffer, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;


// The driver returns the first buffer not queued, so only one buffer
// can be unqueued at a time.
		Thread::enable_cancel();
		ioctl_lock->lock("VDeviceV4L2Thread::run");
		int result = ioctl(input_fd, VIDIOC_DQBUF, &buffer);
		ioctl_lock->unlock();

// Delay so the mutexes don't get stuck
		usleep(1000000 * 1001 / 60000);
		Thread::disable_cancel();


		if(result < 0)
		{
			if(total_errors < 10)
			{
				printf("VDeviceV4L2Thread::run %d %s\n", __LINE__, strerror(errno));
				total_errors++;
			}
			
			Thread::enable_cancel();
// Crystal Eye driver returns invalid result
//			usleep(100000);
//			usleep(10000);
			Thread::disable_cancel();
		}
		else
		{
			buffer_lock->lock("VDeviceV4L2Thread::run");

// Set output frame as valid and set data size
			current_inbuffer = buffer.index;
			if(color_model == BC_COMPRESSED)
			{
				device_buffers[current_inbuffer]->set_compressed_size(
					buffer.bytesused);
//printf("VDeviceV4L2Thread::run %d %d %d\n", __LINE__, current_inbuffer, buffer.bytesused);
			}

			if(!buffer_valid[current_inbuffer])
			{
// Increase valid total only if current is invalid
				buffer_valid[current_inbuffer] = 1;
				total_valid++;
				buffer_lock->unlock();
				video_lock->unlock();
			}
			else
			{
// Driver won't block on the next QBUF call because we're not requeueing the buffer.
				buffer_lock->unlock();
				video_lock->unlock();
				usleep(33000);
			}
		}
	}
}

VFrame* VDeviceV4L2Thread::get_buffer(int *timed_out)
{
	VFrame *result = 0;
	*timed_out = 0;

// Acquire buffer table
	buffer_lock->lock("VDeviceV4L2Thread::get_buffer 1");


// Test for buffer availability
	while(total_valid < 2 && !*timed_out && !first_frame)
	{
		buffer_lock->unlock();
		*timed_out = video_lock->timed_lock(BUFFER_TIMEOUT, 
			"VDeviceV4L2Thread::read_frame 2");
		buffer_lock->lock("VDeviceV4L2Thread::get_buffer 3");
	}

// Copy frame
	if(total_valid >= 2)
	{
		result = device_buffers[current_outbuffer];
	}

	buffer_lock->unlock();

	return result;
}

void VDeviceV4L2Thread::put_buffer()
{
	buffer_lock->lock("VDeviceV4L2Thread::put_buffer");
	buffer_valid[current_outbuffer] = 0;

// Release buffer for capturing.
	put_thread->put_buffer(current_outbuffer);

	current_outbuffer++;
	total_valid--;
	if(current_outbuffer >= total_buffers)
		current_outbuffer = 0;
	buffer_lock->unlock();
}


















VDeviceV4L2::VDeviceV4L2(VideoDevice *device)
 : VDeviceBase(device)
{
	initialize();
}

VDeviceV4L2::~VDeviceV4L2()
{
	close_all();
}

int VDeviceV4L2::close_all()
{
	if(thread) delete thread;
	thread = 0;
	return 0;
}


int VDeviceV4L2::initialize()
{
	thread = 0;
	return 0;
}



int VDeviceV4L2::open_input()
{
	int result = get_sources(device,
		device->in_config->v4l2_in_device);
	device->channel->use_frequency = 1;
	device->channel->use_fine = 1;
	return result;
}

int VDeviceV4L2::get_sources(VideoDevice *device,
	char *path)
{
	int input_fd = -1;


	device->channel->use_norm = 1;
	device->channel->use_input = 1;
	if(device->in_config->driver != VIDEO4LINUX2JPEG) 
		device->channel->has_scanning = 1;
	else
		device->channel->has_scanning = 0;

	if((input_fd = open(path, O_RDWR)) < 0)
	{
		printf("VDeviceV4L2::open_input %s: %s\n", path, strerror(errno));
		return 1;
	}
	else
	{
// Get the inputs
		int i = 0;
		int done = 0;
		char *new_source;

		while(!done && i < 20)
		{
			struct v4l2_input arg;
			bzero(&arg, sizeof(arg));
			arg.index = i;
			
			if(ioctl(input_fd, VIDIOC_ENUMINPUT, &arg) < 0)
			{
// Finished
				done = 1;
			}
			else
			{
				Channel *channel = device->new_input_source((char*)arg.name);
				channel->device_index = i;
				channel->tuner = arg.tuner;
			}
			i++;
		}

// Get the picture controls
		for(i = V4L2_CID_BASE; i < V4L2_CID_LASTP1; i++)
		{
			struct v4l2_queryctrl arg;
			bzero(&arg, sizeof(arg));
			arg.id = i;
// This returns errors for unsupported controls which is what we want.
			if(!ioctl(input_fd, VIDIOC_QUERYCTRL, &arg))
			{
// Test if control exists already
				if(!device->picture->get_item((const char*)arg.name, arg.id))
				{
// Add control
					PictureItem *item = device->picture->new_item((const char*)arg.name);
					item->device_id = arg.id;
					item->min = arg.minimum;
					item->max = arg.maximum;
					item->step = arg.step;
					item->default_ = arg.default_value;
					item->type = arg.type;
					item->value = arg.default_value;
				}
			}
		}

// Load defaults for picture controls
		device->picture->load_defaults();

		close(input_fd);
	}
	return 0;
}

int VDeviceV4L2::cmodel_to_device(int color_model)
{
	switch(color_model)
	{
		case BC_YUV422:
			return V4L2_PIX_FMT_YUYV;
			break;
		case BC_YUV411P:
			return V4L2_PIX_FMT_Y41P;
			break;
		case BC_YUV420P:
			return V4L2_PIX_FMT_YVU420;
			break;
		case BC_YUV422P:
			return V4L2_PIX_FMT_YUV422P;
			break;
		case BC_RGB888:
			return V4L2_PIX_FMT_RGB24;
			break;
	}
	return 0;
}

int VDeviceV4L2::get_best_colormodel(Asset *asset)
{
	int result = BC_RGB888;
	result = File::get_best_colormodel(asset, device->in_config->driver);
	return result;
}

int VDeviceV4L2::has_signal()
{
	if(thread)
	{
		struct v4l2_tuner tuner;
		if(ioctl(thread->input_fd, VIDIOC_G_TUNER, &tuner) < 0)
			perror("VDeviceV4L2::has_signal VIDIOC_G_TUNER");
		return tuner.signal;
	}
	return 0;
}





int VDeviceV4L2::read_buffer(VFrame *frame)
{
	int result = 0;

	if((device->channel_changed || device->picture_changed) && thread)
	{
		delete thread;
		thread = 0;
	}

	if(!thread)
	{
		device->channel_changed = 0;
		device->picture_changed = 0;
		thread = new VDeviceV4L2Thread(device, frame->get_color_model());
		thread->start();

//printf("VDeviceV4L2::read_buffer %d\n", __LINE__);

// Have to make sure the 1st frame is valid for the single frame capture mode.
// Wait for device to start
		sleep(1);
// Flush starting frames
		for(int i = 0; i < thread->total_buffers; i++)
		{
			int timed_out;
//printf("VDeviceV4L2::read_buffer %d\n", __LINE__);
			VFrame *buffer = thread->get_buffer(&timed_out);
//printf("VDeviceV4L2::read_buffer %d\n", __LINE__);
			if(buffer)
			{
				thread->put_buffer();
			}
			
			if(timed_out) break;
		}
//printf("VDeviceV4L2::read_buffer %d timed_out=%d\n", __LINE__, timed_out);
	}

// Get buffer from thread
	int timed_out;
	VFrame *buffer = thread->get_buffer(&timed_out);
	if(buffer)
	{
// translate webcam data into JPEG
		if(device->in_config->driver == CAPTURE_JPEG_WEBCAM)
		{
			frame->allocate_compressed_data(
				buffer->get_compressed_size() + DHT_SIZE);
    		memcpy (frame->get_data(), 
				buffer->get_data(), 
				HEADERFRAME1);
    		memcpy (frame->get_data() + HEADERFRAME1, 
				dht_data, 
				DHT_SIZE);
    		memcpy (frame->get_data() + HEADERFRAME1 + DHT_SIZE,
	    		buffer->get_data() + HEADERFRAME1,
	    		buffer->get_compressed_size() - HEADERFRAME1);
			frame->set_compressed_size(buffer->get_compressed_size() + DHT_SIZE);
		}
		else
		{
			frame->copy_from(buffer);
		}

		thread->put_buffer();

// printf("VDeviceV4L2::read_buffer %d\n", 
// __LINE__);

	}
	else
	{
// Driver in 2.6.4 needs to be restarted when it loses sync.
		if(timed_out)
		{
			delete thread;
			thread = 0;
		}
		result = 1;
	}


	return result;
}

#endif
