/*
 * CINELERRA
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef BCWINDOWBASE_H
#define BCWINDOWBASE_H


//#define HAVE_LIBXXF86VM

// Window types
#define MAIN_WINDOW 0
#define SUB_WINDOW 1
#define POPUP_WINDOW 2

#ifdef HAVE_LIBXXF86VM
#define VIDMODE_SCALED_WINDOW 3
#endif

#define TOOLTIP_MARGIN 2
#define BC_INFINITY 65536

#include "arraylist.h"
#include "bcbar.inc"
#include "bcbitmap.inc"
#include "bcbutton.inc"
#include "bccapture.inc"
#include "bcclipboard.inc"
//#include "bccmodels.inc"
#include "bcdisplay.inc"
#include "bcdragwindow.inc"
#include "bcfilebox.inc"
#include "bclistbox.inc"
#include "bcmenubar.inc"
#include "bcmeter.inc"
#include "bcpan.inc"
#include "bcpbuffer.inc"
#include "bcpixmap.inc"
#include "bcpopup.inc"
#include "bcpopupmenu.inc"
#include "bcpot.inc"
#include "bcprogress.inc"
#include "bcrepeater.inc"
#include "bcresources.inc"
#include "bcscrollbar.inc"
#include "bcslider.inc"
#include "bcsubwindow.inc"
#include "bcsynchronous.inc"
#include "bctextbox.inc"
#include "bctimer.inc"
#include "bctitle.inc"
#include "bctoggle.inc"
#include "bctumble.inc"
#include "bcwindow.inc"
#include "bcwindowbase.inc"
#include "bcwindowevents.inc"
#include "condition.inc"
#include "bchash.inc"
#include "linklist.h"
#include "mutex.inc"
#include "vframe.inc"


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <X11/Xatom.h>
#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#ifdef HAVE_LIBXXF86VM
#include <X11/extensions/xf86vmode.h>
#endif
#ifdef HAVE_GL
#include <GL/glx.h>
#endif




using std::string;
typedef uint32_t wchr_t;

class BC_ResizeCall
{
public:
	BC_ResizeCall(int w, int h);
	int w, h;
};


// wrapper for x events & user functions to run in the window thread
class BC_Event
{
public:
    BC_Event();
    virtual ~BC_Event();
    XEvent *xevent;
    void (*user_function)(void *);
    void *user_data;
};

// Windows, subwindows, popupwindows inherit from this
class BC_WindowBase
{
public:
	BC_WindowBase();
	virtual ~BC_WindowBase();

	friend class BC_Bar;
	friend class BC_Bitmap;
	friend class BC_Button;
	friend class BC_GenericButton;
	friend class BC_Capture;
	friend class BC_Clipboard;
	friend class BC_Display;
	friend class BC_DragWindow;
	friend class BC_FileBox;
	friend class BC_FullScreen;
	friend class BC_ListBox;
	friend class BC_Menu;
	friend class BC_MenuBar;
	friend class BC_MenuItem;
	friend class BC_MenuPopup;
	friend class BC_Meter;
	friend class BC_Pan;
	friend class BC_PBuffer;
	friend class BC_Pixmap;
	friend class BC_Popup;
	friend class BC_PopupMenu;
	friend class BC_Pot;
	friend class BC_ProgressBar;
	friend class BC_Repeater;
	friend class BC_Resources;
	friend class BC_ScrollBar;
	friend class BC_Slider;
	friend class BC_SubWindow;
	friend class BC_Synchronous;
	friend class BC_TextBox;
	friend class BC_Title;
	friend class BC_Toggle;
	friend class BC_Tumbler;
	friend class BC_Window;
	friend class BC_WindowEvents;

// Main loop
	int run_window();
// Schedule a user function to run in the run_window thread.
// Deleted by run_window
// Window is locked before running the function.
    void put_event(void (*user_function)(void *), void *data);
// Terminal event dispatchers
	virtual int close_event();
	virtual int resize_event(int w, int h);
	virtual int repeat_event(int64_t duration) { return 0; };
	virtual int focus_in_event() { return 0; };
	virtual int focus_out_event() { return 0; };
	virtual int button_press_event() { return 0; };
	virtual int button_release_event() { return 0; };
	virtual int cursor_motion_event() { return 0; };
	virtual int cursor_leave_event();
	virtual int cursor_enter_event();
	virtual int keypress_event() { return 0; };
	virtual int keyrelease_event() { return 0; };
	virtual int translation_event() { return 0; };
	virtual int drag_start_event() { return 0; };
	virtual int drag_motion_event() { return 0; };
	virtual int drag_stop_event() { return 0; };
	virtual int uses_text() { return 0; };
    virtual int get_enabled() { return 0; };
// Only if opengl is enabled
	virtual int expose_event() { return 0; };
	virtual void create_objects() { return; };

// Wait until event loop is running
	void init_wait();
// Check if a hardware accelerated colormodel is available and reserve it
	int accel_available(int color_model, int lock_it); 
// Get color model adjusted for byte order and pixel size
	int get_color_model();
// return the colormap pixel of the color for all bit depths
	int get_color(int64_t color);
// return the currently selected color
	int64_t get_color();
	int show_window(int flush = 1);
	int hide_window(int flush = 1);
	int get_hidden();
	int get_video_on();
// Shouldn't deference a pointer to delete a window if a parent is 
// currently being deleted.  This returns 1 if any parent is being deleted.
	int get_deleting();



//============================= OpenGL functions ===============================
// OpenGL functions must be called from inside a BC_Synchronous command.
// Create openGL context and bind it to the current window.
// If it's called inside start_video/stop_video, the context is bound to the window.
// If it's called outside start_video/stop_video, the context is bound to the pixmap.
// Must be called at the beginning of any opengl routine to make sure
// the context is current.
// No locking is performed.
	void enable_opengl();
	void disable_opengl();
	void flip_opengl();

// Calls the BC_Synchronous version of the function with the window_id.
// Not run in OpenGL thread because it has its own lock.
	unsigned int get_shader(char *title, int *got_it);
	void put_shader(unsigned int handle, char *title);


// return 1 if initialization worked
    int exists();
	int flash(int x, int y, int w, int h, int flush = 1);
	int flash(int flush = 1);
	void flush();
	void sync_display();
// Lock out other threads
	int lock_window(const char *location = 0);
	int unlock_window();
	int get_window_lock();

	BC_MenuBar* add_menubar(BC_MenuBar *menu_bar);
	BC_WindowBase* add_subwindow(BC_WindowBase *subwindow);
	BC_WindowBase* add_tool(BC_WindowBase *subwindow);
// Use this to get events for the popup window.
// Events are not propagated to the popup window.
	BC_WindowBase* add_popup(BC_WindowBase *window);
	void remove_popup(BC_WindowBase *window);

	static BC_Resources* get_resources();
// User must create synchronous object first
	static BC_Synchronous* get_synchronous();
//	static BC_CModels* get_cmodels();

// Dimensions
	virtual int get_w();
	virtual int get_h();
	virtual int get_x();
	virtual int get_y();
	int get_root_w(int ignore_dualhead = 0, int lock_display = 0);
	int get_root_h(int lock_display);
	int get_root_x(int lock_display);
	int get_root_y(int lock_display);
// Get current position
	int get_abs_cursor_x(int lock_window);
	int get_abs_cursor_y(int lock_window);
	int get_relative_cursor_x();
	int get_relative_cursor_y();
// Return 1 if cursor is over an unobscured part of this window.
// An argument is provided for excluding a drag popup
	int get_cursor_over_window();
// For traversing windows... return 1 if this or any subwindow is win
 	int match_window(Window win);

// 1 or 0 if a button is down
	int get_button_down();
// must clear it if the widget owning the button down is deleted
    void clear_button_down();
// Number of button pressed 1 - 5
	int get_buttonpress();
	int get_has_focus();
	int get_dragging();
	int get_keypress();
#ifdef X_HAVE_UTF8_STRING
	char* get_keypress_utf8();
#endif
// Get cursor position of last event
	int get_cursor_x();
	int get_cursor_y();
// Cursor position of drag start
	int get_drag_x();
	int get_drag_y();
	int relative_cursor_x(BC_WindowBase *pov);
	int relative_cursor_y(BC_WindowBase *pov);
	int alt_down();
	int shift_down();
	int ctrl_down();
	int get_double_click();
	int get_triple_click();
// Bottom right corner
	int get_x2();
	int get_y2();
	int get_bg_color();
	void set_bg_color(int color);
	BC_Pixmap* get_bg_pixmap();
	int get_text_ascent(int font);
	int get_text_descent(int font);
	int get_text_height(int font, const char *text = 0);
	int get_text_width(int font, const char *text, int length = -1);
// truncate the text with ... & return a new string
    string* get_truncated_text(int font, const string *text, int max_w);
	BC_Clipboard* get_clipboard();
	void set_dragging(int value);
	int set_w(int w);
	int set_h(int h);
	BC_WindowBase* get_top_level();
	BC_WindowBase* get_parent();
// Event happened in this window
	int is_event_win();
	int cursor_inside();
// Deactivate everything and activate this subwindow
	virtual int activate();
// Deactivate this subwindow
	virtual int deactivate();
	void set_active_subwindow(BC_WindowBase *subwindow);
// Get value of toggle value when dragging a selection
	int get_toggle_value();
// Get if toggle is being dragged
	int get_toggle_drag();

// Set the gc to the color
	void set_color(int64_t color);
	void set_line_width(int value);
	void set_line_dashes(int value);
	int get_bgcolor();
	void set_font(int font);
// Set the cursor to a macro from cursors.h
// Set override if the caller is enabling hourglass or hiding the cursor
	void set_cursor(int cursor, int override /* = 0 */, int flush);
// Set the cursor to a character in the X cursor library.  Used by test.C
	void set_x_cursor(int cursor);
	int get_cursor();
// Shows the cursor after it's hidden by video playback
	void unhide_cursor();
// Called by video updating routines to hide the cursor after a timeout
	void update_video_cursor();

// Entry point for starting hourglass.  
// Converts all cursors and saves the previous cursor.
	void start_hourglass();
	void stop_hourglass();

// Recursive part of hourglass commands.
	void start_hourglass_recursive();
	void stop_hourglass_recursive();

// Drawing
	void copy_area(int x1, int y1, int x2, int y2, int w, int h, BC_Pixmap *pixmap = 0);
	void clear_box(int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_box(int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
// draw a box in the current color with a checker for alpha
    void draw_box_alpha(int x, 
        int y, 
        int w, 
        int h, 
        int a, 
        int checker_w,
        int checker_h,
        BC_Pixmap *pixmap = 0);
	void draw_circle(int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_arc(int x, 
		int y, 
		int w, 
		int h, 
		int start_angle,
		int angle_length,
		BC_Pixmap *pixmap = 0);
	void draw_disc(int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_text(int x, int y, const char *text, int length = -1, BC_Pixmap *pixmap = 0);
	void draw_xft_text(int x, 
		int y, 
		const char *text, 
		int length, 
		BC_Pixmap *pixmap,
		int x2,
		int k,
		int y2,
		int j,
		int i);
// truncate the text to a ... version that fits in the width, using the current_font
	void truncate_text(char *result, const char *text, int w);
	void draw_center_text(int x, int y, const char *text, int length = -1);
	void draw_line(int x1, int y1, int x2, int y2, BC_Pixmap *pixmap = 0);
	void draw_polygon(ArrayList<int> *x, ArrayList<int> *y, BC_Pixmap *pixmap = 0);
	void fill_polygon(ArrayList<int> *x, ArrayList<int> *y, BC_Pixmap *pixmap = 0);
	void draw_rectangle(int x, int y, int w, int h);
	void draw_3segment(int x, 
		int y, 
		int w, 
		int h, 
		BC_Pixmap *left_image,
		BC_Pixmap *mid_image,
		BC_Pixmap *right_image,
		BC_Pixmap *pixmap = 0);
	void draw_3segment(int x, 
		int y, 
		int w, 
		int h, 
		VFrame *left_image,
		VFrame *mid_image,
		VFrame *right_image,
		BC_Pixmap *pixmap = 0);
// For drawing a changing level
	void draw_3segmenth(int x, 
		int y, 
		int w, 
		int total_x,
		int total_w,
		VFrame *image,
		BC_Pixmap *pixmap);
	void draw_3segmenth(int x, 
		int y, 
		int w, 
		int total_x,
		int total_w,
		BC_Pixmap *src,
		BC_Pixmap *dst = 0);
// For drawing a single level
	void draw_3segmenth(int x, 
		int y, 
		int w, 
		VFrame *image,
		BC_Pixmap *pixmap = 0);
	void draw_3segmenth(int x, 
		int y, 
		int w,
		BC_Pixmap *src,
		BC_Pixmap *dst = 0);
	void draw_3segmentv(int x, 
		int y, 
		int h,
		BC_Pixmap *src,
		BC_Pixmap *dst = 0);
	void draw_3segmentv(int x, 
		int y, 
		int h,
		VFrame *src,
		BC_Pixmap *dst = 0);
	void draw_9segment(int x, 
		int y, 
		int w,
		int h,
		VFrame *src,
		BC_Pixmap *dst = 0);
	void draw_9segment(int x, 
		int y, 
		int w,
		int h,
		BC_Pixmap *src,
		BC_Pixmap *dst = 0);
	void draw_3d_box(int x, 
		int y, 
		int w, 
		int h, 
		int light1, 
		int light2, 
		int middle, 
		int shadow1, 
		int shadow2, 
		BC_Pixmap *pixmap = 0);
	void draw_3d_box(int x, 
		int y, 
		int w, 
		int h, 
		int is_down, 
		BC_Pixmap *pixmap = 0);
	void draw_3d_border(int x, 
		int y, 
		int w, 
		int h, 
		int light1, 
		int light2, 
		int shadow1, 
		int shadow2);
	void draw_3d_border(int x, 
		int y, 
		int w, 
		int h, 
		int is_down);
	void draw_colored_box(int x, int y, int w, int h, int down, int highlighted);
	void draw_check(int x, int y);
	void draw_triangle_down_flat(int x, int y, int w, int h);
	void draw_triangle_up(int x, int y, int w, int h, 
		int light1, int light2, int middle, int shadow1, int shadow2);
	void draw_triangle_down(int x, int y, int w, int h, 
		int light1, int light2, int middle, int shadow1, int shadow2);
	void draw_triangle_left(int x, int y, int w, int h, 
		int light1, int light2, int middle, int shadow1, int shadow2);
	void draw_triangle_right(int x, int y, int w, int h, 
		int light1, int light2, int middle, int shadow1, int shadow2);
// Set the gc to opaque
	void set_opaque();
	void set_inverse();
	void set_background(VFrame *bitmap);
// Change the window title.  The title is translated internally.
	void set_title(const char *text);
	char* get_title();
// draw bitmaps to the foreground/win instead of the back buffer/pixmap
	void start_video();
	void stop_video();
	int get_id();
	void set_done(int return_value);
// Get a bitmap to draw on the window with
	BC_Bitmap* new_bitmap(int w, int h, int color_model = -1);
// Draw a bitmap on the window
	void draw_bitmap(BC_Bitmap *bitmap, 
		int dont_wait, 
		int dest_x = 0, 
		int dest_y = 0, 
		int dest_w = 0, 
		int dest_h = 0,
		int src_x = 0,
		int src_y = 0,
		int src_w = 0,
		int src_h = 0,
		BC_Pixmap *pixmap = 0);
	void draw_pixel(int x, int y, BC_Pixmap *pixmap = 0);
// Draw a pixmap on the window
	void draw_pixmap(BC_Pixmap *pixmap, 
		int dest_x = 0, 
		int dest_y = 0, 
		int dest_w = -1, 
		int dest_h = -1,
		int src_x = 0,
		int src_y = 0,
		BC_Pixmap *dst = 0);
// Draw a vframe on the window
	void draw_vframe(VFrame *frame, 
		int dest_x = 0, 
		int dest_y = 0, 
		int dest_w = -1, 
		int dest_h = -1, 
		int src_x = 0, 
		int src_y = 0, 
		int src_w = 0, 
		int src_h = 0, 
		BC_Pixmap *pixmap = 0);
	void draw_border(char *text, int x, int y, int w, int h);
// Draw a region of the background
	void draw_top_background(BC_WindowBase *parent_window, int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_top_tiles(BC_WindowBase *parent_window, int x, int y, int w, int h);
	void draw_background(int x, int y, int w, int h);
	void draw_tiles(BC_Pixmap *tile, 
		int origin_x, 
		int origin_y, 
		int x, 
		int y, 
		int w, 
		int h);

// bits for reader
    int is_event_subwin();
    void reposition_cursor(int x, int y);
    void draw_fg_box(int x, int y, int w, int h);
    void draw_fg_circle(int x, int y, int w, int h);
    void draw_fg_disc(int x, int y, int w, int h);
    void draw_fg_line(int x1, int y1, int x2, int y2);
    void draw_fg_pixel(int x, int y);
// get the foreground window for drawing
    Window get_win();

	void slide_left(int distance);
	void slide_right(int distance);
	void slide_up(int distance);
	void slide_down(int distance);

	int cycle_textboxes(int amount);

	int raise_window(int do_flush = 1);
	int set_tooltips(int tooltips_enabled);
	int resize_window(int w, int h);
	int reposition_window(int x, int y);
	int reposition_window(int x, int y, int w /* = -1 */, int h /* = -1*/ );
// Cause a repeat event to be dispatched every duration.
// duration is milliseconds
	int set_repeat(int64_t duration);
// Stop a repeat event from being dispatched.
	int unset_repeat(int64_t duration);
	int set_tooltip(const char *text);
	int show_tooltip(int w = -1, int h = -1);
	int hide_tooltip();
	int set_icon(VFrame *data);
// use the window manager border
    void set_border(int value);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

#ifdef HAVE_LIBXXF86VM
// Mode switch methods.
   void closest_vm(int *vm, int *width, int *height);
   void scale_vm(int vm);
   void restore_vm();
#endif

    BC_Bitmap* get_temp_bitmap(int w, int h, int color_model);
	
	int test_keypress;
  	char keys_return[KEYPRESSLEN];


private:
// Create a window
	virtual int create_window(BC_WindowBase *parent_window,
				const char *title, 
				int x,
				int y,
				int w, 
				int h, 
				int minw, 
				int minh, 
				int allow_resize,
				int private_color, 
				int hide,
				int bg_color,
				const char *display_name,
				int window_type,
				BC_Pixmap *bg_pixmap,
				int group_it);

	static Display* init_display(const char *display_name);
// Get display from top level
	Display* get_display();
	int get_screen();
	virtual int initialize();
	int get_atoms();
	void init_cursors();
	int init_colors();
	int init_window_shape();



	XFontStruct* query_font(const char *font_string, int size);
	XFontSet query_fontset(const char *font_string, int size);
	void* query_xft_font(const char *font_string, double size);
	int init_fonts();



	static int evaluate_color_model(int client_byte_order, int server_byte_order, int depth);
	int create_private_colors();
	int create_color(int color);
	int create_shared_colors();
// Get width of a single line.  Used by get_text_width
	int get_single_text_width(int font, const char *text, int length);
	int allocate_color_table();
	int init_gc();


	int get_color_rgb8(int color);
	int64_t get_color_rgb16(int color);
	int64_t get_color_bgr16(int color);
	int64_t get_color_bgr24(int color);
	XFontStruct* get_font_struct(int font);
#ifdef HAVE_XFT
	XftFont* get_xft_struct(int font);
#endif
	Cursor get_cursor_struct(int cursor);
    XFontSet get_fontset(int font);
    XFontSet get_curr_fontset(void);
    void set_fontset(int font);
	int dispatch_event(BC_Event *event);

	int get_key_masks(XEvent *event);
    void init_im();
    void delete_im();
    int keysym_lookup(XEvent *event);

	int trigger_tooltip();
	int untrigger_tooltip();
	void draw_tooltip();
	int arm_repeat(int64_t duration);
// delete all repeater opjects for a close
	int unset_all_repeaters();

// Block and get event from common events.
	BC_Event* get_event();
// Return number of events in table.
	int get_event_count();
// Put X event in common events.  Deleted by run_window
	void put_event(XEvent *event);

// Recursive event dispatchers
	int dispatch_resize_event(int w, int h);
	int dispatch_focus_in();
	int dispatch_focus_out();
	int dispatch_motion_event();
	int dispatch_keypress_event();
	int dispatch_keyrelease_event();
	int dispatch_repeat_event(int64_t duration);
	int dispatch_repeat_event_master(int64_t duration);
	int dispatch_button_press();
	int dispatch_button_release();
	int dispatch_cursor_leave();
	int dispatch_cursor_enter();
	int dispatch_translation_event();
	int dispatch_drag_start();
	int dispatch_drag_motion();
	int dispatch_drag_stop();
	int dispatch_expose_event();

// Get the port ID for a color model or return -1 for failure
	int grab_port_id(BC_WindowBase *window, int color_model);

	int find_next_textbox(BC_WindowBase **first_textbox, BC_WindowBase **next_textbox, int &result);
	int find_prev_textbox(BC_WindowBase **last_textbox, BC_WindowBase **prev_textbox, int &result);


	void translate_coordinates(Window src_w, 
		Window dest_w,
		int src_x,
		int src_y,
		int *dest_x_return,
		int *dest_y_return);

// Top level window above this window
	BC_WindowBase* top_level;
// Window just above this window
	BC_WindowBase* parent_window;
// list of window bases in this window
	BC_SubWindowList* subwindows;
	ArrayList<BC_WindowBase*> popups;
// Position of window
	int x, y, w, h;
// Default colors
	int light1, light2, medium, dark1, dark2, bg_color;
// Type of window defined above
	int window_type;
// Pointer to the active menubar in the window.
	BC_MenuBar* active_menubar;
// pointer to the active popup menu in the window
	BC_PopupMenu* active_popup_menu;    
// pointer to the active subwindow
	BC_WindowBase* active_subwindow;

// Window parameters
	int allow_resize;
	int hidden, private_color, bits_per_pixel, color_model;
	int server_byte_order, client_byte_order;
// number of colors in color table
	int total_colors;
// last color found in table
	int current_color_value, current_color_pixel;
// table for every color allocated
	int color_table[256][2];    
// Turn on optimization
	int video_on;
// Event handler completion
	int done;
// Return value of event handler
	int return_value;
// Motion event compression
	int motion_events, last_motion_x, last_motion_y;
// window of buffered motion
	Window last_motion_win;       
// Resize event compression
	int resize_events, last_resize_w, last_resize_h;
	int translation_events, last_translate_x, last_translate_y;
	int prev_x, prev_y;
// Since the window manager automatically translates the window at boot, 
// use the first translation event to get correction factors
	int translation_count;
	int x_correction, y_correction;
// Key masks
	int ctrl_mask, shift_mask, alt_mask;
// Cursor motion information
	int cursor_x, cursor_y;
// reject cursor motion event caused by repositon_cursor
    Window reject_win;
    int reject_x, reject_y;
// Button status information
	int button_down, button_number;
// When button was pressed and whether it qualifies as a double click
	int64_t button_time1;
	int64_t button_time2;
	int64_t button_time3;
	int double_click;
	int triple_click;
// Which button is down.  1, 2, 3, 4, 5
	int button_pressed;
// Last key pressed
	int key_pressed;
// key data
	KeySym keysym;
#ifdef X_HAVE_UTF8_STRING
	char* key_pressed_utf8;
	int wkey_string_length;
	wchr_t wkey_string[4];
#endif
// During a selection drag involving toggles, set the same value for each toggle
	int toggle_value;
	int toggle_drag;
// Whether the window has the focus
	int has_focus;

	static BC_Resources resources;
//	static BC_CModels cmodels;
	
#ifndef SINGLE_THREAD
// Array of repeaters for multiple repeating objects.
	ArrayList<BC_Repeater*> repeaters;
#endif
// Text for tooltip if one exists
	char tooltip_text[BCTEXTLEN];
// If the current window's tooltip is visible
	int tooltip_on;
// Repeat ID of tooltip
//	int64_t tooltip_id;
// Popup window for tooltip
	BC_Popup *tooltip_popup;
// If this subwindow has already shown a tooltip since the last EnterNotify
	int tooltip_done;
// If the tooltip shouldn't be hidden
	int persistant_tooltip;



// Font sets
    XFontSet largefontset, mediumfontset, smallfontset, curr_fontset, clockfontset;

// Fonts
	int current_font;
	XFontStruct *largefont, *mediumfont, *smallfont, *clockfont;

// Must be void so users don't need to include the wrong libpng version.
	void *largefont_xft, *mediumfont_xft, *smallfont_xft, *clockfont_xft;


	int line_width;
	int line_dashes;
	int64_t current_color;
// Coordinate of drag start
	int drag_x, drag_y;
// Boundaries the cursor must pass to start a drag
	int drag_x1, drag_x2, drag_y1, drag_y2;
// Dragging is specific to the subwindow
	int is_dragging;
// Don't delete the background pixmap
	int shared_bg_pixmap;
	char title[BCTEXTLEN];

// X Window parameters
	int screen;
	Window rootwin;
// windows previous events happened in
 	Window event_win, drag_win;
	Visual *vis;
	Colormap cmap;
// Name of display
	char display_name[BCTEXTLEN];
// Display for all synchronous operations
	Display *display;
 	Window win;
#ifdef HAVE_GL
// The first context to be created and the one whose texture id 
// space is shared with the other contexts.
	GLXContext gl_win_context;
#endif
	int window_lock;
	GC gc;
// Depth given by the X Server
	int default_depth;
	Atom DelWinXAtom;
	Atom ProtoXAtom;
	Atom RepeaterXAtom;
	Atom SetDoneXAtom;
// Number of times start_hourglass was called
	int hourglass_total;
// Cursor set by last set_cursor which wasn't an hourglass or transparent.
	int current_cursor;
// If hourglass overrides current cursor.  Only effective in top level.
	int is_hourglass;
// If transparent overrides all cursors.  Only effective in subwindow.
	int is_transparent;
	Cursor arrow_cursor;
	Cursor cross_cursor;
	Cursor ibeam_cursor;
	Cursor vseparate_cursor;
	Cursor hseparate_cursor;
	Cursor move_cursor;
	Cursor temp_cursor;
	Cursor left_cursor;
	Cursor right_cursor;
	Cursor upright_arrow_cursor;
	Cursor upleft_resize_cursor;
	Cursor upright_resize_cursor;
	Cursor downleft_resize_cursor;
	Cursor downright_resize_cursor;
	Cursor hourglass_cursor;
	Cursor transparent_cursor;

	int xvideo_port_id;
	ArrayList<BC_ResizeCall*> resize_history;
// Back buffer
	BC_Pixmap *pixmap;
// Background tile if tiled
	BC_Pixmap *bg_pixmap;
// Icon
	BC_Popup *icon_window;
	BC_Pixmap *icon_pixmap;
	BC_Pixmap **_7segment_pixmaps;
// has a window manager border
    int has_border;
// Temporary
	BC_Bitmap *temp_bitmap;
// Clipboard
#ifndef SINGLE_THREAD
	static BC_Clipboard *clipboard;
#endif

#ifdef HAVE_LIBXXF86VM
// Mode switch information.
   int vm_switched;
   XF86VidModeModeInfo orig_modeline;
#endif

#ifdef X_HAVE_UTF8_STRING
//	XIM im;		/* Used to communicate with the input method (IM) server */
//	XIC ic;		/* Used for retaining the state, properties, and semantics of communication with the input method (IM) server */
	// Used to communicate with the input method (IM) server
	XIM input_method;
	// Used for retaining the state, properties, and semantics
	//  of communication with the input method (IM) server
	XIC input_context;
#endif



#ifndef SINGLE_THREAD
// Common events to run in the window thread
// Sources are the X server, repeater threads, & user threads.
	ArrayList<BC_Event*> common_events;
// Locks for common events
// Locking order:
// 1) event_condition
// 2) event_lock
	Mutex *event_lock;
	Condition *event_condition;
#else
	Condition *completion_lock;
#endif

// Lock that waits until the event handler is running
	Condition *init_lock;

	int dump_windows();


	BC_WindowEvents *event_thread;
	int is_deleting;
// Hide cursor when video is enabled
	Timer *cursor_timer;
// unique ID of window.
	int id;
};



#endif
