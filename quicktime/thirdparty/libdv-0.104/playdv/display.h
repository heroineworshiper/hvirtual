/* 
 *  display.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser Public License
 *  along with libdv; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

#ifndef DV_DISPLAY_H
#define DV_DISPLAY_H

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "libdv/dv_types.h"
#include <gtk/gtk.h>

#if HAVE_LIBXV
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>
#endif // HAVE_LIBXV

#if HAVE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#endif // HAVE_SDL

#if HAVE_GTK
#include <gtk/gtk.h>
#endif // HAVE_GTK

#define DV_FOURCC_YV12  0x32315659	/* 4:2:0 Planar mode: Y + V + U  (3 planes) */
#define DV_FOURCC_YUY2  0x32595559	/* 4:2:2 Packed mode: Y0+U0+Y1+V0 (1 plane) */

#if HAVE_LIBXV
#define DV_DISPLAY_OPT_METHOD   0
#define DV_DISPLAY_OPT_ASPECT	1
#define DV_DISPLAY_OPT_SIZE	2
#define DV_DISPLAY_OPT_CALLBACK 3
#define DV_DISPLAY_OPT_XV_PORT  4
#define DV_DISPLAY_NUM_OPTS     5
#else 
#define DV_DISPLAY_OPT_METHOD   0
#define DV_DISPLAY_OPT_CALLBACK 1
#define DV_DISPLAY_NUM_OPTS     2
#endif /* HAVE_LIBXV */

typedef enum dv_dpy_lib_e {
  e_dv_dpy_Xv,
  e_dv_dpy_SDL,
  e_dv_dpy_gtk,
  e_dv_dpy_XShm,
} dv_dpy_lib_t;

typedef struct {
  dv_color_space_t color_space;
  gint		    width, height;
  guchar           *pixels[3];
  gint              pitches[3];

  /* Begin Private */
  dv_dpy_lib_t      lib;
  guint32           len; 
  guint32           format;   /* fourcc code for YUV modes */

#if HAVE_LIBXV
  /* -----------------------------------------------------------
   * Xv specific members
   */
  Display          *dpy;
  Screen           *scn;
  Window            rwin, win;
  gint		    dwidth, dheight,
		    swidth, sheight,
		    lwidth, lheight,
		    lxoff, lyoff,
		    flags,
		    pic_format;
  GC                gc;
  XEvent            event;
  XvPortID	    port;
  XShmSegmentInfo   shminfo;
  XvImage          *xv_image;
#endif // HAVE_LIBXV

#if HAVE_GTK
  /* -----------------------------------------------------------
   * GDK specific members
   */
  GtkWidget	   *window, *image;
#endif

#if HAVE_SDL
  SDL_Surface* sdl_screen;
  SDL_Overlay *overlay;
  SDL_Rect rect;
#endif

  gint 			arg_display,
			arg_aspect_val,
			arg_size_val,
			arg_xv_port;
  gchar			*arg_aspect_string;

#if HAVE_LIBPOPT
  struct poptOption	option_table[DV_DISPLAY_NUM_OPTS+1];
#endif // HAVE_LIBPOPT
} dv_display_t;

#ifdef __cplusplus
extern "C" {
#endif

extern dv_display_t *dv_display_new(void);
extern gboolean      dv_display_init(dv_display_t *dpy,
				     int *argc, char ***argv, 
				     int width, int height, 
				     dv_sample_t sampling,
				     char *w_name, char *i_name); // dv_display_init

extern void dv_display_show(dv_display_t *dv_dpy);
extern void dv_display_exit(dv_display_t *dv_dpy);
extern void dv_display_set_norm (dv_display_t *dv_dpy, dv_system_t norm);
extern void dv_display_check_format(dv_display_t *dv_dpy, int pic_format);

#ifdef __cplusplus
}
#endif

#endif // DV_DISPLAY_H

