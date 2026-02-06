/*
 * CINELERRA
 * Copyright (C) 1997-2025 Adam Williams <broadcast at earthling dot net>
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

#ifndef MWINDOW_INC
#define MWINDOW_INC

#define PROGRAM_NAME "Cinelerra"
#define CINELERRA_VERSION "10"
#define COPYRIGHT_DATE 2025
#define DEFAULT_THEME "S.U.V."
#define CONFIG_FILE "Cinelerra_rc"
// user presets
#define PRESETS_FILE "Cinelerra_presets"
// factory presets
#define FACTORY_FILE "Cinelerra_factory"
#define PICTURE_FILE "Cinelerra_picture"
#define PLUGIN_FILE "Cinelerra_plugins"
#define PLUGIN_FILE_VERSION 11

// Behavior of region selections
#define SELECTION_SAMPLES 0
#define SELECTION_EDIT 1

// min milliseconds between tweek undos to create new undo entries
#define UNDO_SPAN 1000

// Make tweeks span keyframes if a region is selected
#define USE_KEYFRAME_SPANNING

// Don't automatically merge contiguous edits
#define ENABLE_RAZOR

// If certain stages request VFrame::RAM, request GPU & do a GPU to RAM transfer
#define FORCE_GPU

// Index of vwindow that always exists
#define DEFAULT_VWINDOW 0

#define LABEL_COLORS 8


class MWindow;

#endif





