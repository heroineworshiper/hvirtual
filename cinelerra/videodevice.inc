/*
 * CINELERRA
 * Copyright (C) 2011-2013 Adam Williams <broadcast at earthling dot net>
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

#ifndef VIDEODEVICE_INC
#define VIDEODEVICE_INC

class VideoDevice;

// Flags for mainwindow->preferences->vconfig_in->driver
#define VIDEO4LINUX          0
#define VIDEO4LINUX2         1
//#define VIDEO4LINUX2MJPG     2
#define SCREENCAPTURE        3
#define CAPTURE_LML          4
#define CAPTURE_FIREWIRE     5
#define CAPTURE_BUZ          6
#define CAPTURE_IEC61883     7
#define CAPTURE_DVB          8
//#define CAPTURE_JPEG_WEBCAM  20
//#define CAPTURE_YUYV_WEBCAM  21
//#define CAPTURE_MPEG         22
//#define VIDEO4LINUX2JPEG     23

#define VIDEO4LINUX_TITLE            "Video4Linux"
#define VIDEO4LINUX2_TITLE           "Video4Linux2"
#define VIDEO4LINUX2MJPG_TITLE       "Video4Linux2 MJPG"
#define VIDEO4LINUX2JPEG_TITLE       "Video4Linux2 JPEG"
#define SCREENCAPTURE_TITLE          "Screencapture"
#define CAPTURE_LML_TITLE            "LML 33"
#define CAPTURE_BUZ_TITLE            "Buz"
#define CAPTURE_FIREWIRE_TITLE       "DV 1394"
#define CAPTURE_IEC61883_TITLE       "IEC 61883"
#define CAPTURE_DVB_TITLE            "DVB"
#define CAPTURE_JPEG_WEBCAM_TITLE    "JPEG Webcam"
#define CAPTURE_YUYV_WEBCAM_TITLE    "YUYV Webcam"
#define CAPTURE_MPEG_TITLE           "Video4Linux2 MPEG"


// input formats for video4linux2
#define CAPTURE_RGB 0
#define CAPTURE_YUYV 1
#define CAPTURE_JPEG 2
#define CAPTURE_JPEG_NOHEAD 3
#define CAPTURE_MJPG 4
#define CAPTURE_MJPG_1FIELD 5

#define CAPTURE_RGB_TITLE "RGB"
#define CAPTURE_YUYV_TITLE "YUYV"
#define CAPTURE_JPEG_TITLE "JPEG"
#define CAPTURE_JPEG_NOHEAD_TITLE "JPEG no headers"
#define CAPTURE_MJPG_TITLE "MJPG 2 field"
#define CAPTURE_MJPG_1FIELD_TITLE "MJPG 1 field"



#define PLAYBACK_X11 	  10
#define PLAYBACK_X11_XV   14
#define PLAYBACK_X11_GL   17
#define PLAYBACK_BUZ 	  11
#define PLAYBACK_LML 	  12
#define PLAYBACK_FIREWIRE 13
#define PLAYBACK_DV1394   15
#define PLAYBACK_IEC61883 16
#define PLAYBACK_PREVIEW  24

// This is passed to File::get_best_colormodel by FileThread so FileThread can
// do the colorspace conversion using the shortest path.
#define PLAYBACK_ASYNCHRONOUS 18

#define PLAYBACK_X11_TITLE  	"X11"
#define PLAYBACK_X11_XV_TITLE   "X11-XV"
#define PLAYBACK_X11_GL_TITLE   "X11-OpenGL"
#define PLAYBACK_BUZ_TITLE  	"Buz"
#define PLAYBACK_LML_TITLE  	"LML 33"
#define PLAYBACK_FIREWIRE_TITLE "RAW 1394"
#define PLAYBACK_DV1394_TITLE   "DV 1394"
#define PLAYBACK_IEC61883_TITLE "IEC 61883"
#define PLAYBACK_PREVIEW_TITLE  "Preview"

// Output formats required for direct frame copy
// RGB is for either RGB or RGBA as defined in the VFrame
#define OUTPUT_RGB  0
#define OUTPUT_MJPA 1
#define OUTPUT_DV   2

#endif
