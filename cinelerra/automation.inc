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

#ifndef AUTOMATION_INC
#define AUTOMATION_INC

class Automation;

// overlay & automation types


// These must match the following arrays:
// xml_titles in autoconf.C and automation.C.
// auto_colors in trackcanvas.C
// auto_operations in trackcanvas.C
// auto_pixmaps in trackcanvas.C
// auto_text in gwindowgui.C

#define AUTOMATION_MUTE 0
#define AUTOMATION_CAMERA_X 1
#define AUTOMATION_CAMERA_Y 2
#define AUTOMATION_CAMERA_Z 3
#define AUTOMATION_PROJECTOR_X 4
#define AUTOMATION_PROJECTOR_Y 5
#define AUTOMATION_PROJECTOR_Z 6
#define AUTOMATION_FADE 7
#define AUTOMATION_PAN 8
#define AUTOMATION_MODE 9
#define AUTOMATION_MASK 10
#define AUTOMATION_SPEED 11
#define AUTOMATION_TOTAL 12

#define TRANSITION_OVERLAYS 12
#define PLUGIN_KEYFRAMES 13
#define OVERLAY_TOTAL 14

#endif
