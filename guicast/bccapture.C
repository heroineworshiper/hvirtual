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

#include "bccapture.h"
#include "bcresources.h"
#include "bctimer.h"
#include "bcwindowbase.h"
//#include "bccmodels.h"
#include "clip.h"
#include "keys.h"
#include "language.h"
#include "vframe.h"
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>





// Byte orders:
// 24 bpp packed:         bgr
// 24 bpp unpacked:       0bgr


// BC_CaptureThread::BC_CaptureThread(BC_Capture *capture)
//  : Thread(1, 0, 0)
// {
//     this->capture = capture;
//     XIEventMask masks[1];
//     unsigned char mask[(XI_LASTEVENT + 7)/8];
// 
//     memset(mask, 0, sizeof(mask));
//     XISetMask(mask, XI_RawMotion);
//     XISetMask(mask, XI_RawButtonPress);
//     XISetMask(mask, XI_RawKeyPress);
// 
//     masks[0].deviceid = XIAllMasterDevices;
//     masks[0].mask_len = sizeof(mask);
//     masks[0].mask = mask;
// 
//     XISelectEvents(capture->display, 
//         DefaultRootWindow(capture->display), 
//         masks, 
//         1);
//     XFlush(capture->display);
//     
// }
// 
// BC_CaptureThread::~BC_CaptureThread()
// {
//     if(running())
//     {
//         cancel();
//         join();
//     }
// }
// 
// void BC_CaptureThread::run()
// {
//     while(1)
//     {
//         XEvent event;
//         XNextEvent(capture->display, &event);
//         printf("BC_CaptureThread::run %d type=%d\n",
//             __LINE__,
//             event.type);
//     }
// }
// 


// ms to show the keypress
#define MIN_KEY_TIME 1000
KeypressState::KeypressState()
{
    reset();
}

void KeypressState::reset()
{
    id = -1;
    text[0] = 0;
    times = 0;
    time = 0;
    up = 1;
}

void KeypressState::begin(int id, char *text, int is_button)
{
    if((text && this->text && !strcmp(text, this->text)))
        times++;
    else
    if(is_button && id != WHEEL_UP && id != WHEEL_DOWN && id == this->id)
        times++;
    else
        times = 1;

    this->id = id;
    if(text) 
        strcpy(this->text, text);
    else
        this->text[0] = 0;
    up = 0;
    time = MIN_KEY_TIME;
}

void KeypressState::end(int id)
{
    if(id == this->id) up = 1;
}

int KeypressState::pressed()
{
    return id >= 0 && (!up || time == MIN_KEY_TIME);
}

void KeypressState::update(int elapsed)
{
    if(up && time > 0)
    {
        time -= elapsed;
        if(time <= 0)
        {
            time = 0;
            times = 0;
            id = -1;
        }
    }
}



BC_Capture::BC_Capture(int w, int h, const char *display_path)
{
	this->w = w;
	this->h = h;

//    thread = 0;
	data = 0;
	use_shm = 1;
	init_window(display_path);
	allocate_data();
    frame_time = new Timer;
    key_text[0] = 0;
}


BC_Capture::~BC_Capture()
{
//    delete thread;
	delete_data();
	XCloseDisplay(display);
}

int BC_Capture::init_window(const char *display_path)
{
	int bits_per_pixel;
	if(display_path && display_path[0] == 0) display_path = NULL;
	if((display = XOpenDisplay(display_path)) == NULL)
	{
  		printf(_("cannot connect to X server.\n"));
  		if(getenv("DISPLAY") == NULL)
    		printf(_("'DISPLAY' environment variable not set.\n"));
  		exit(-1);
		return 1;
 	}

	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);
	client_byte_order = (*(u_int32_t*)"a   ") & 0x00000001;
	server_byte_order = (XImageByteOrder(display) == MSBFirst) ? 0 : 1;
	char *data = 0;
	XImage *ximage;
	ximage = XCreateImage(display, 
					vis, 
					default_depth, 
					ZPixmap, 
					0, 
					data, 
					16, 
					16, 
					8, 
					0);
	bits_per_pixel = ximage->bits_per_pixel;
	XDestroyImage(ximage);
	bitmap_color_model = BC_WindowBase::evaluate_color_model(client_byte_order, server_byte_order, bits_per_pixel);

// test shared memory
// This doesn't ensure the X Server is on the local host
    if(use_shm && !XShmQueryExtension(display))
    {
        use_shm = 0;
    }

// capture user input
    XIEventMask masks[1];
    unsigned char mask[(XI_LASTEVENT + 7)/8];

    memset(mask, 0, sizeof(mask));
    XISetMask(mask, XI_RawButtonPress);
    XISetMask(mask, XI_RawButtonRelease);
    XISetMask(mask, XI_RawKeyPress);
    XISetMask(mask, XI_RawKeyRelease);

    masks[0].deviceid = XIAllMasterDevices;
    masks[0].mask_len = sizeof(mask);
    masks[0].mask = mask;

    XISelectEvents(display, 
        DefaultRootWindow(display), 
        masks, 
        1);
    XFlush(display);

	return 0;
}


int BC_Capture::allocate_data()
{
// try shared memory
	if(!display) return 1;
    if(use_shm)
	{
	    ximage = XShmCreateImage(display, vis, default_depth, ZPixmap, (char*)NULL, &shm_info, w, h);

		shm_info.shmid = shmget(IPC_PRIVATE, h * ximage->bytes_per_line, IPC_CREAT | 0777);
		if(shm_info.shmid < 0) perror("BC_Capture::allocate_data shmget");
		data = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
		shmctl(shm_info.shmid, IPC_RMID, 0);
		ximage->data = shm_info.shmaddr = (char*)data;  // setting ximage->data stops BadValue
		shm_info.readOnly = 0;

// Crashes here if remote server.
		BC_Resources::error = 0;
		XShmAttach(display, &shm_info);
    	XSync(display, False);
		if(BC_Resources::error)
		{
			XDestroyImage(ximage);
			shmdt(shm_info.shmaddr);
			use_shm = 0;
		}
	}

	if(!use_shm)
	{
// need to use bytes_per_line for some X servers
		data = 0;
		ximage = XCreateImage(display, vis, default_depth, ZPixmap, 0, (char*)data, w, h, 8, 0);
		data = (unsigned char*)malloc(h * ximage->bytes_per_line);
		XDestroyImage(ximage);

		ximage = XCreateImage(display, vis, default_depth, ZPixmap, 0, (char*)data, w, h, 8, 0);
	}

	row_data = new unsigned char*[h];
	for(int i = 0; i < h; i++)
	{
		row_data[i] = &data[i * ximage->bytes_per_line];
	}
// This differs from the depth parameter of the top_level.
	bits_per_pixel = ximage->bits_per_pixel;
	return 0;
}

int BC_Capture::delete_data()
{
	if(!display) return 1;
	if(data)
	{
		if(use_shm)
		{
			XShmDetach(display, &shm_info);
			XDestroyImage(ximage);
			shmdt(shm_info.shmaddr);
		}
		else
		{
			XDestroyImage(ximage);
		}

// data is automatically freed by XDestroyImage
		data = 0;
		delete row_data;
	}
	return 0;
}


int BC_Capture::get_w() { return w; }
int BC_Capture::get_h() { return h; }

// Capture a frame from the screen
#define CAPTURE_FRAME_HEAD \
	for(int i = 0; i < h; i++) \
	{ \
		unsigned char *input_row = row_data[i]; \
		unsigned char *output_row = (unsigned char*)frame->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{

#define CAPTURE_FRAME_TAIL \
		} \
	}

#define RGB_TO_YUV(y, u, v, r, g, b) \
{ \
	y = ((cmodel_yuv_table->rtoy_tab[r] + cmodel_yuv_table->gtoy_tab[g] + cmodel_yuv_table->btoy_tab[b]) >> 16); \
	u = ((cmodel_yuv_table->rtou_tab[r] + cmodel_yuv_table->gtou_tab[g] + cmodel_yuv_table->btou_tab[b]) >> 16); \
	v = ((cmodel_yuv_table->rtov_tab[r] + cmodel_yuv_table->gtov_tab[g] + cmodel_yuv_table->btov_tab[b]) >> 16); \
	CLAMP(y, 0, 0xff); \
	CLAMP(u, 0, 0xff); \
	CLAMP(v, 0, 0xff); \
}

// replace some keys with strings
typedef struct 
{
    KeySym keysym;
    int id;
    const char *text;
} key_table_t;
const key_table_t translation[] = 
{
    { XK_Insert, INSERT, "INSERT" },
    { XK_Return, RETURN, "RETURN" },
    { XK_Up, UP, "UP" },
    { XK_Down, DOWN, "DOWN" },
    { XK_Left, LEFT, "LEFT" },
    { XK_Right, RIGHT, "RIGHT" },
    { XK_Next,  PGDN, "PGDN" },
    { XK_Prior, PGUP, "PGUP" },
    { XK_BackSpace, BACKSPACE, "BACKSPACE" },
    { XK_Escape, ESC, "ESC" },
    { XK_Tab, TAB, "TAB" },
    { XK_ISO_Left_Tab, LEFTTAB, "TAB" },
    { XK_underscore, '_', "_" },
    { XK_asciitilde, '~', "~" },
    { XK_Delete, DELETE, "DEL" },
    { XK_Home, HOME, "HOME" },
    { XK_End, END, "END" },
	{ XK_KP_Enter, KPENTER, "NUMPAD ENTER" },
	{ XK_KP_Add, KPPLUS, "NUMPAD +" },
	{ XK_KP_1, KP1, "NUMPAD 1" },
	{ XK_KP_End, KP1, "NUMPAD 1" },
	{ XK_KP_2, KP2, "NUMPAD 2" },
	{ XK_KP_Down, KP2, "NUMPAD 2" },
	{ XK_KP_3, KP3, "NUMPAD 3" },
	{ XK_KP_Page_Down, KP3, "NUMPAD 3" },
	{ XK_KP_4, KP4, "NUMPAD 4" },
	{ XK_KP_Left, KP4, "NUMPAD 4" },
	{ XK_KP_5, KP5, "NUMPAD 5" },
	{ XK_KP_Begin, KP5, "NUMPAD 5" },
	{ XK_KP_6, KP6, "NUMPAD 6" },
	{ XK_KP_Right, KP6, "NUMPAD 6" },
	{ XK_KP_7, KP7, "NUMPAD 7" },
	{ XK_KP_Home, KP7, "NUMPAD 7" },
	{ XK_KP_8, KP8, "NUMPAD 8" },
	{ XK_KP_Up, KP8, "NUMPAD 8" },
	{ XK_KP_9, KP9, "NUMPAD 9" },
	{ XK_KP_Page_Up, KP9, "NUMPAD 9" },
	{ XK_KP_0, KPINS, "NUMPAD 0" },
	{ XK_KP_Insert, KPINS, "NUMPAD 0" },
	{ XK_KP_Decimal, KPDEL, "NUMPAD ." },
	{ XK_KP_Delete, KPDEL, "NUMPAD ." },
    { XK_KP_Divide, '/', "/" },
    { XK_KP_Multiply, '*', "*" },
    { XK_KP_Subtract, '-', "-" },
    
    { XK_F1, KEY_F1, "F1" },
    { XK_F2, KEY_F2, "F2" },
    { XK_F3, KEY_F3, "F3" },
    { XK_F4, KEY_F4, "F4" },
    { XK_F5, KEY_F5, "F5" },
    { XK_F6, KEY_F6, "F6" },
    { XK_F7, KEY_F7, "F7" },
    { XK_F8, KEY_F8, "F8" },
    { XK_F9, KEY_F9, "F9" },
    { XK_F10, KEY_F10, "F10" },
    { XK_F11, KEY_F11, "F11" },
    { XK_F12, KEY_F12, "F12" },
    { ' ', ' ', "SPACE" }
};


static int keysym_to_key(KeySym keysym)
{
    for(int i = 0; i < sizeof(translation) / sizeof(key_table_t); i++)
    {
        if(translation[i].keysym == keysym)
        {
            return translation[i].id;
        }
    }
    return keysym & 0xff;
}

static int keysym_to_text(char *dst, KeySym keysym, char *keys_return)
{
    for(int i = 0; i < sizeof(translation) / sizeof(key_table_t); i++)
    {
        if(translation[i].keysym == keysym)
        {
            strcpy(dst, translation[i].text);
            return translation[i].id;
        }
    }

    strcpy(dst, keys_return);
    return keysym & 0xff;
}

int BC_Capture::capture_frame(int &x1, 
	int &y1, 
	int cursor_size, // the scale of the cursor if nonzero
    int keypress_size)
{
	if(!display) return 1;
	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x1 > get_top_w() - w) x1 = get_top_w() - w;
	if(y1 > get_top_h() - h) y1 = get_top_h() - h;


// Read the raw data
	if(use_shm)
		XShmGetImage(display, rootwin, ximage, x1, y1, 0xffffffff);
	else
		XGetSubImage(display, rootwin, x1, y1, w, h, 0xffffffff, ZPixmap, ximage, 0, 0);

// draw cursor in input image
    if(cursor_size)
    {
		XFixesCursorImage *cursor;
		cursor = XFixesGetCursorImage(display);
		if(cursor)
		{
// 			printf("BC_Capture::capture_frame %d cursor=%p colormodel=%d\n", 
// 				__LINE__,
// 				cursor, 
// 				frame->get_color_model());
			
			int scale = cursor_size;
			cursor_x = cursor->x - x1 - cursor->xhot * scale;
			cursor_y = cursor->y - y1 - cursor->yhot * scale;
            cursor_w  = cursor->width * scale;
            cursor_h  = cursor->height * scale;
			for(int i = 0; i < cursor->height; i++)
			{
				for(int yscale = 0; yscale < scale; yscale++)
				{
					if(cursor_y + i * scale + yscale >= 0 && 
						cursor_y + i * scale + yscale < h)
					{
						unsigned char *src = (unsigned char*)(cursor->pixels + 
							i * cursor->width);
						int dst_y = cursor_y + i * scale + yscale;
						int dst_x = cursor_x;
						for(int j = 0; j < cursor->width; j++)
						{
							for(int xscale = 0; xscale < scale ; xscale++)
							{
								if(cursor_x + j * scale + xscale >= 0 && 
									cursor_x + j * scale + xscale < w)
								{
									int a = src[3];
									int invert_a = 0xff - a;
									int r = src[2];
									int g = src[1];
									int b = src[0];
									switch(bitmap_color_model)
									{
										case BC_RGB888:
										{
											unsigned char *dst = row_data[dst_y] +
												dst_x * 3;
											dst[0] = (r * a + dst[0] * invert_a) / 0xff;
											dst[1] = (g * a + dst[1] * invert_a) / 0xff;
											dst[2] = (b * a + dst[2] * invert_a) / 0xff;
											break;
										}
                                        
                                        case BC_BGR8888:
                                        {
											unsigned char *dst = row_data[dst_y] +
												dst_x * 4;
											dst[0] = (b * a + dst[0] * invert_a) / 0xff;
											dst[1] = (g * a + dst[1] * invert_a) / 0xff;
											dst[2] = (r * a + dst[2] * invert_a) / 0xff;
                                            break;
                                        }

                                        default:
                                            printf("BC_Capture::capture_frame %d: unsupported color model %d\n",
                                                __LINE__,
                                                bitmap_color_model);
                                            break;
									}
								}
								dst_x++;
							}
							src += sizeof(long);
						}
					}
				}
			}

		
		
// This frees cursor->pixels
			XFree(cursor);
		}
    }

// drain input events
// printf("BC_Capture::capture_frame %d\n",
// __LINE__);
    while(XPending(display))
	{
		XEvent event;
		XNextEvent(display, &event);
//         printf("BC_Capture::capture_frame %d type=%d\n",
//             __LINE__,
//             event.type);
// unwrap the event
        if(event.type == GenericEvent)
        {
            XGetEventData(display, &event.xcookie);
            XGenericEventCookie *cookie = &event.xcookie;
//             printf("BC_Capture::capture_frame %d cookie->evtype=%d\n",
//                 __LINE__,
//                 cookie->evtype);
            XIDeviceEvent *device_event = (XIDeviceEvent *)cookie->data;
//             printf("BC_Capture::capture_frame %d detail=%d\n",
//                 __LINE__,
//                 device_event->detail);
            switch(cookie->evtype)
            {
                case XI_RawButtonPress:
                    button.begin(device_event->detail, 0, 1);
// cancel released modifiers
                    if(ctrl.up) ctrl.reset();
                    if(shift.up) shift.reset();
                    if(alt.up) alt.reset();
                    break;
                case XI_RawButtonRelease:
                    button.end(device_event->detail);
                    break;
                case XI_RawKeyPress:
                {
                    KeySym keysym = XkbKeycodeToKeysym(display, device_event->detail, 0, 0);
                    KeySym keysym2 = keysym;
                    XKeyEvent xkey;
                    xkey.display = display;
                    xkey.keycode = device_event->detail;
                    xkey.state = 0;
                    if(shift.id > 0 && !shift.up) xkey.state = ShiftMask;
                    char keys_return[BCTEXTLEN];
                    int len = XLookupString(&xkey, keys_return, BCTEXTLEN, &keysym2, 0);
                    keys_return[len] = 0;
                    char string[BCTEXTLEN];
                    int id = keysym_to_text(string, keysym, keys_return);
                    int got_modifier = 0;
                    printf("BC_Capture::capture_frame %d detail=%d keysym=0x%x string=%s\n",
                        __LINE__,
                        device_event->detail,
                        (int)keysym,
                        string);

                    switch(keysym)
                    {
                        case XK_Control_L:
                        case XK_Control_R:
                            ctrl.begin(1, 0, 0);
                            got_modifier = 1;
                            break;
                        case XK_Shift_L:
                        case XK_Shift_R:
                            shift.begin(1, 0, 0);
                            got_modifier = 1;
                            break;
                        case XK_Alt_L:
                        case XK_Alt_R:
                            alt.begin(1, 0, 0);
                            got_modifier = 1;
                            break;
                        default:
                            key.begin(id, string, 0);
                            break;
                    }

// Don't show a modifier & an alnum simultaneously if they're
// not pressed simultaneously
// cancel released alnums
                    if(got_modifier)
                    {
                        if(key.up) key.reset();
                        if(button.up) button.reset();
                    }
                    else
// cancel released modifiers
                    {
                        if(ctrl.up) ctrl.reset();
                        if(shift.up) shift.reset();
                        if(alt.up) alt.reset();
                    }
                    break;
                }

                case XI_RawKeyRelease:
                {
                    KeySym keysym = XkbKeycodeToKeysym(display, device_event->detail, 0, 0);
                    switch(keysym)
                    {
                        case XK_Control_L:
                        case XK_Control_R:
                            ctrl.end(1);
                            break;
                        case XK_Shift_L:
                        case XK_Shift_R:
                            shift.end(1);
                            break;
                        case XK_Alt_L:
                        case XK_Alt_R:
                            alt.end(1);
                            break;
                        default:
                            key.end(keysym_to_key(keysym));
                            break;
                    }
                    break;
                }
            }

            XFreeEventData(display, &event.xcookie);
        }
	}


// if anything was pressed in the current frame, 
// disable the keys which were not pressed in the current frame
// so the keys don't show as simultaneously pressed.
// This fails if multiple keys are pressed & released in a single frame.
//     if(button.pressed() || 
//         ctrl.pressed() || 
//         shift.pressed() || 
//         alt.pressed() || 
//         key.pressed())
//     {
// //        if(!button.pressed()) button.reset();
//         if(!ctrl.pressed()) ctrl.reset();
//         if(!shift.pressed()) shift.reset();
//         if(!alt.pressed()) alt.reset();
//         if(!key.pressed()) key.reset();
//     }

    int elapsed = frame_time->get_difference(1);

    button.update(elapsed);
    ctrl.update(elapsed);
    shift.update(elapsed);
    alt.update(elapsed);
    key.update(elapsed);


    key_text[0] = 0;
    if(ctrl.id >= 0) strcat(key_text, "CTRL ");
    if(shift.id >= 0) strcat(key_text, "SHIFT ");
    if(alt.id >= 0) strcat(key_text, "ALT ");
    if(key.id >= 0)
    {
        strcat(key_text, key.text);
        if(key.times > 1)
        {
            char string[BCTEXTLEN];
            sprintf(string, "x%d", key.times);
            strcat(key_text, string);
        }
    }

//     printf("BC_Capture::capture_frame %d button_pressed=%dx%d ctrl=%dx%d shift=%dx%d alt=%dx%d key=%dx%d\n",
//         __LINE__,
//         button.id,
//         button.times,
//         ctrl.id,
//         ctrl.times,
//         shift.id,
//         shift.times,
//         alt.times,
//         alt.times,
//         key.id,
//         key.times);

//    printf("BC_Capture::capture_frame %d %s\n", __LINE__, key_text);

//     if(keypress_size)
//     {
//         if(!thread) 
//         {
//             thread = new BC_CaptureThread(this);
//             thread->start();
//         }
//     }
//     else
//     if(thread)
//     {
//         delete thread;
//         thread = 0;
//     }

//memset(row_data[0], 0xff, 1920 * 512);
// printf("BC_Capture::capture_frame %d %d %d\n", 
// __LINE__, 
// frame->get_color_model(), 
// bitmap_color_model);
// TODO: do it in the caller





	return 0;
}

int BC_Capture::get_top_w()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return WidthOfScreen(screen_ptr);
}

int BC_Capture::get_top_h()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return HeightOfScreen(screen_ptr);
}
