/* 
 *  dvconnect.c
 *
 *     Copyright (C) Peter Schlaile - May 2001
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/resource.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#if HAVE_LIBPOPT
#include <popt.h>
#endif

#include <stdlib.h>

static long cip_n_ntsc = 2436;
static long cip_d_ntsc = 38400;
static long cip_n_pal = 1;
static long cip_d_pal = 16;
static long syt_offset = 11000;

#define TARGETBUFSIZE   320
#define MAX_PACKET_SIZE 512
#define VBUF_SIZE      (320*512)

#define VIDEO1394_MAX_SIZE 0x4000000

enum {
	VIDEO1394_BUFFER_FREE = 0,
	VIDEO1394_BUFFER_QUEUED,
	VIDEO1394_BUFFER_READY
};

#define VIDEO1394_SYNC_FRAMES          0x00000001
#define VIDEO1394_INCLUDE_ISO_HEADERS  0x00000002
#define VIDEO1394_VARIABLE_PACKET_SIZE 0x00000004

struct video1394_mmap
{
	int channel;
	unsigned int sync_tag;
	unsigned int nb_buffers;
	unsigned int buf_size;
        /* For VARIABLE_PACKET_SIZE: Maximum packet size */
	unsigned int packet_size; 
	unsigned int fps;
	unsigned int syt_offset;
	unsigned int flags;
};


/* For TALK_QUEUE_BUFFER with VIDEO1394_VARIABLE_PACKET_SIZE use */
struct video1394_queue_variable
{
	unsigned int channel;
	unsigned int buffer;
	unsigned int* packet_sizes; /* Buffer of size: buf_size / packet_size*/
};

struct video1394_wait
{
	unsigned int channel;
	unsigned int buffer;
	struct timeval filltime;        /* time of buffer full */
};

#define VIDEO1394_LISTEN_CHANNEL		\
	_IOWR('#', 0x10, struct video1394_mmap)
#define VIDEO1394_UNLISTEN_CHANNEL		\
	_IOW ('#', 0x11, int)
#define VIDEO1394_LISTEN_QUEUE_BUFFER	\
	_IOW ('#', 0x12, struct video1394_wait)
#define VIDEO1394_LISTEN_WAIT_BUFFER	\
	_IOWR('#', 0x13, struct video1394_wait)
#define VIDEO1394_TALK_CHANNEL		\
	_IOWR('#', 0x14, struct video1394_mmap)
#define VIDEO1394_UNTALK_CHANNEL		\
	_IOW ('#', 0x15, int)
#define VIDEO1394_TALK_QUEUE_BUFFER 	\
	_IOW ('#', 0x16, size_t)
#define VIDEO1394_TALK_WAIT_BUFFER		\
	_IOW ('#', 0x17, struct video1394_wait)
#define VIDEO1394_LISTEN_POLL_BUFFER	\
	_IOWR('#', 0x18, struct video1394_wait)

static int cap_start_frame = 0;
static int cap_num_frames = 0xfffffff;
static int cap_verbose_mode;
static int frames_captured = 0;
static int broken_frames = 0;

/* ------------------------------------------------------------------------
   - buffer management
   ------------------------------------------------------------------------ */

static int max_buffer_blocks = 25*10;
static int ceil_buffer_blocks = 0;

struct buf_node {
	unsigned char data[144000]; /* FIXME: We are wasting space on NTSC! */
	int usage;
	struct buf_node* next;
};

struct buf_list {
	struct buf_node * first;
	struct buf_node * last;
	int usage;
};

static struct buf_list free_list = { NULL, NULL, 0 };
static struct buf_list buf_queue = { NULL, NULL, 0 };

static pthread_mutex_t  queue_mutex;
static pthread_mutex_t  wakeup_mutex;
static pthread_cond_t   wakeup_cond;
static pthread_t        file_io_thread;

void push_back(struct buf_list * l, struct buf_node * elem)
{
	pthread_mutex_lock(&queue_mutex);
	if (l->last) {
		l->last->next = elem;
	} else {
		l->first = elem;
	}
	elem->next = NULL;
	l->last = elem;
	l->usage++;
	pthread_mutex_unlock(&queue_mutex);
}

struct buf_node * pop_front(struct buf_list * l)
{
	struct buf_node * rval;

	pthread_mutex_lock(&queue_mutex);
	rval = l->first;

	if (l->first == l->last) {
		l->last = NULL;
	}
	if (rval) {
		l->first = rval->next;
		l->usage--;
	} else {
		l->first = NULL;
	}

	pthread_mutex_unlock(&queue_mutex);
	return rval;
}

struct buf_node * get_free_block()
{
	struct buf_node * f = pop_front(&free_list);
	if (!f) {
		if (buf_queue.usage >= max_buffer_blocks) {
			return NULL;
		}
		f = (struct buf_node*) malloc(sizeof(struct buf_node));
	}
	return f;
}

/* ------------------------------------------------------------------------
   - receiver
   ------------------------------------------------------------------------ */

static FILE* dst_fp;

void handle_packet(unsigned char* data, int len)
{
	int start_of_frame = 0;
	static int frame_size = 0;
	static int isPAL = 0;
	static int found_first_block = 0;
	static unsigned char* p_out = NULL;
	static struct buf_node * f = NULL;

        switch (len) {
        case 488: 
		if (data[12] == 0x1f && data[13] == 0x07) {
			start_of_frame = 1;
		}

		if (start_of_frame) {
			int frame_broken = 0;
			if (!found_first_block) {
				if (!cap_start_frame) {
					found_first_block = 1;
				} else {
					cap_start_frame--;
					if (cap_verbose_mode) {
						fprintf(stderr,
							"Skipped frame %8d\r",
							cap_start_frame);
					}
					return;
				}
				frame_size = 0;
			}
			if (!cap_num_frames) {
				return;
			}
			cap_num_frames--;
			
			if (frame_size) {
				if ((isPAL && frame_size != 144000)
				    || (!isPAL && frame_size != 120000)) {
					fprintf(stderr, 
						"Broken frame %8d "
						"(size = %d)!\n",
						frames_captured, frame_size);
					broken_frames++;
					frame_broken = 1;
				}
				if (cap_verbose_mode) {
					fprintf(stderr, 
						"Captured frame %8d\r",
						frames_captured);
				}
				frames_captured++;
			}
			isPAL = (data[15] & 0x80);

			if (f && !frame_broken) {
				f->usage = frame_size;
				push_back(&buf_queue, f);
				pthread_mutex_lock(&wakeup_mutex);
				pthread_cond_signal(&wakeup_cond);
				pthread_mutex_unlock(&wakeup_mutex);
				f = NULL;
			}

			if (!f) {
				f = get_free_block();
				if (!f) {
					fprintf(stderr, 
						"Buffer overrun: "
						"Frame discarded!\n");
					broken_frames++;
				}
			}
			if (f) {
				p_out = f->data;
			}
			frame_size = 0;
		}

		if (found_first_block && cap_num_frames != 0 && f) {
			if (frame_size < 144000) {
				memcpy(p_out, data + 12, 480);
				p_out += 480;
				frame_size += 480;
			} else {
				fprintf(stderr, "Frame buffer overrun: "
					"Frame discarded!\n");
				broken_frames++;
				p_out = f->data;
				frame_size = 0;
			}
		}

                break;
        case 8:
                break;
        default:
		fprintf(stderr, "Penguin on the bus? (packet size = %d!)\n",
			len);
                break;
        }
}

void* write_out_thread(void * arg)
{
	while (cap_num_frames != 0) {
		struct buf_node * f;
		pthread_mutex_lock(&wakeup_mutex);
		pthread_cond_wait(&wakeup_cond, &wakeup_mutex);
		pthread_mutex_unlock(&wakeup_mutex);

		while ((f = pop_front(&buf_queue)) != NULL) {
			fwrite(f->data, 1, f->usage, dst_fp);
			push_back(&free_list, f);
		}
	}
	return NULL;
}

void sig_int_recv_handler(int signum)
{
	char t [] = "Terminating on user's request...\n";
	write(2, t, sizeof(t));
	
	cap_num_frames = 0;
}

int capture_raw(const char* filename, int channel, int nbuffers,
		int start_frame, int end_frame, int verbose_mode,
		char *device)
{
	int viddev;
	unsigned char *recv_buf;
	struct video1394_mmap v;
	struct video1394_wait w;
	int unused_buffers;
	unsigned char outbuf[2*65536];
	int outbuf_used = 0;

	cap_start_frame = start_frame;
	cap_num_frames = end_frame - start_frame;
	cap_verbose_mode = verbose_mode;
	
	if (!filename || strcmp(filename, "-") == 0) {
		dst_fp = stdout;
	} else {
		dst_fp = fopen(filename, "wb");
		if (!dst_fp) {
			perror("fopen input file");
			return(-1);
		}
	}

	if ((viddev = open(device, O_RDWR)) < 0) {
		perror("open video1394 device");
		return -1;
	}

	v.channel = channel;
	v.sync_tag = 0;
	v.nb_buffers = nbuffers;
	v.buf_size = VBUF_SIZE; 
	v.packet_size = MAX_PACKET_SIZE;
	v.syt_offset = syt_offset;
	v.flags = VIDEO1394_INCLUDE_ISO_HEADERS;
	w.channel = v.channel;
                      
	if (ioctl(viddev, VIDEO1394_LISTEN_CHANNEL, &v) < 0) {
		perror("VIDEO1394_LISTEN_CHANNEL");
		return -1;
	}

	if ((recv_buf = (unsigned char *) mmap(
			0, v.nb_buffers*v.buf_size, PROT_READ|PROT_WRITE,
			MAP_SHARED, viddev, 0)) == (unsigned char *)-1) {
		perror("mmap videobuffer");
		return -1;
	}

	pthread_mutex_init(&queue_mutex, NULL);
	pthread_mutex_init(&wakeup_mutex, NULL);
	pthread_cond_init(&wakeup_cond, NULL);

	pthread_create(&file_io_thread, NULL, write_out_thread, NULL);

	/* signal(SIGTERM, sig_int_recv_handler);
	   signal(SIGINT, sig_int_recv_handler); */

        unused_buffers = v.nb_buffers;
        w.buffer = 0;
       
	while (cap_num_frames != 0) {
	struct video1394_wait wcopy;
	unsigned char * curr;
	int ofs;
	
	while (unused_buffers--) {
		unsigned char * curr = recv_buf+ v.buf_size * w.buffer;
		
		memset(curr, 0, v.buf_size);
		
		wcopy = w;
		
		if (ioctl(viddev,VIDEO1394_LISTEN_QUEUE_BUFFER, &wcopy) < 0) {
			perror("VIDEO1394_LISTEN_QUEUE_BUFFER");
		}
		w.buffer++;
		w.buffer %= v.nb_buffers;
	}
	wcopy = w;
	if (ioctl(viddev, VIDEO1394_LISTEN_WAIT_BUFFER, &wcopy) < 0) {
		perror("VIDEO1394_LISTEN_WAIT_BUFFER");
	}
	curr = recv_buf + v.buf_size * w.buffer;
	ofs = 0;
	
	while (ofs < VBUF_SIZE) {
		while (outbuf_used < 4 && ofs < VBUF_SIZE) {
			outbuf[outbuf_used++] = curr[ofs++];
		}
		if (ofs != VBUF_SIZE) {
			int len = outbuf[2] + (outbuf[3] << 8) + 8;
			if (ofs + len - outbuf_used > VBUF_SIZE) {
				memcpy(outbuf + outbuf_used, curr+ofs, 
				VBUF_SIZE - ofs);
				outbuf_used += VBUF_SIZE - ofs;
				ofs = VBUF_SIZE;
			} else {
				memcpy(outbuf + outbuf_used,
				curr + ofs, len - outbuf_used);
				ofs += len - outbuf_used;
				outbuf_used = 0;
				handle_packet(outbuf, len - 8);
			}
		}
	}
	unused_buffers = 1;
	}

	if (broken_frames) {
		fprintf(stderr, "\nCaptured %d broken frames!\n", broken_frames);
	}

	munmap(recv_buf, v.nb_buffers * v.buf_size);
	
	if (ioctl(viddev, VIDEO1394_UNLISTEN_CHANNEL, &v.channel)<0) {
		perror("VIDEO1394_UNLISTEN_CHANNEL");
	}
	
	close(viddev);

	pthread_join(file_io_thread, NULL);

	if (dst_fp != stdout) {
		fclose(dst_fp);
	}

	return 0;
}

/* ------------------------------------------------------------------------
   - sender
   ------------------------------------------------------------------------ */

static const char*const* src_filenames;
static FILE* src_fp;
static int is_eof = 0;
static unsigned char * underrun_data_frame = NULL;
static int underrun_frame_ispal = 0;
static unsigned char *device = NULL;

static pthread_mutex_t  wakeup_rev_mutex;
static pthread_cond_t   wakeup_rev_cond;

int read_frame(FILE* fp, unsigned char* frame, int* isPAL)
{
	if (is_eof) {
		return -1;
	}
        if (fread(frame, 1, 120000, fp) != 120000) {
                return -1;
        }
	*isPAL = (frame[3] & 0x80);

	if (*isPAL) {
		if (fread(frame + 120000, 1, 144000 - 120000, fp)
		    != 144000 - 120000) {
			return -1;
		}
	}
	return 0;
}

void sig_int_send_handler(int signum)
{
	char t [] = "Terminating on user's request...\n";
	write(2, t, sizeof(t));
	
	is_eof = 1;
}

static int
open_next_input(void)
{
	if (src_fp && src_fp != stdin) {
		fclose (src_fp);
		src_fp = NULL;
	}
	if (!src_filenames || !*src_filenames)
		return 1;
	if (!strcmp (*src_filenames++, "-"))
		src_fp = stdin;
	else {
		src_fp = fopen(src_filenames[-1], "rb");
		if (!src_fp) {
			perror("fopen input file");
			return 1;
		}
	}
	return 0;
}


void fill_buf_queue(int fire)
{
	struct buf_node * f;

	while ((f = get_free_block()) != NULL) {
		int isPAL;
again:
		if (read_frame(src_fp, f->data, &isPAL) < 0) {
			is_eof = open_next_input ();
			if (!is_eof)
				goto again;
			push_back(&free_list, f);
			return;
		}
		if (isPAL) {
			f->usage = 144000;
		} else {
			f->usage = 120000;
		}
		push_back(&buf_queue, f);
		if (fire) {
			pthread_mutex_lock(&wakeup_rev_mutex);
			pthread_cond_signal(&wakeup_rev_cond);
			pthread_mutex_unlock(&wakeup_rev_mutex);
		}
	}
}

void* read_in_thread(void * arg)
{
	fill_buf_queue(0);

	pthread_mutex_lock(&wakeup_rev_mutex);
	pthread_cond_signal(&wakeup_rev_cond);
	pthread_mutex_unlock(&wakeup_rev_mutex);

	while (!is_eof) {
		pthread_mutex_lock(&wakeup_mutex);
		pthread_cond_wait(&wakeup_cond, &wakeup_mutex);
		pthread_mutex_unlock(&wakeup_mutex);

		fill_buf_queue(1);
	}
	return NULL;
}

static int fill_buffer(unsigned char* targetbuf, unsigned int * packet_sizes)
{
        unsigned char* frame;
	struct buf_node * f_node;
	int isPAL;
	int frame_size;
        unsigned long vdata = 0;
        int i;
        static unsigned char continuity_counter = 0;
	static unsigned int cip_counter = 0;
	static unsigned int cip_n = 0;
	static unsigned int cip_d = 0;

	f_node = pop_front(&buf_queue);
	if (!f_node) {
		if (!is_eof) {
			if (!underrun_data_frame) {
				fprintf(stderr, "Buffer underrun ");
				if (ceil_buffer_blocks > 0 &&
				    max_buffer_blocks < ceil_buffer_blocks) {
					max_buffer_blocks += 25;
					fprintf(stderr,
					"(raising buffer limit +25 => %d)!\n",
					max_buffer_blocks);
				} else {
					fprintf(stderr,
						"(not raising buffer space, "
					        "hard limit reached)\n");
				}


				pthread_mutex_lock(&wakeup_rev_mutex);
				pthread_cond_wait(&wakeup_rev_cond, 
						  &wakeup_rev_mutex);
				pthread_mutex_unlock(&wakeup_rev_mutex);
				f_node = pop_front(&buf_queue);
			} else {
				f_node = get_free_block();

				if (underrun_frame_ispal) {
					f_node->usage = 144000;
				} else {
					f_node->usage = 120000;
				}
				memcpy(f_node->data, underrun_data_frame, 
				       f_node->usage);
			}

		} else {
			return -1;
		}
	}

	frame = f_node->data;
	frame_size = f_node->usage;

	isPAL = (frame_size == 144000);

	if (cip_counter == 0) {
		if (!isPAL) {
			cip_n = cip_n_ntsc;
			cip_d = cip_d_ntsc;
		} else {
			cip_n = cip_n_pal;
			cip_d = cip_d_pal;
		}
		cip_counter = cip_n;
	}

	for (i = 0; i < TARGETBUFSIZE && vdata < frame_size; i++) {
		unsigned char* p = targetbuf;
		int want_sync = 0;
		cip_counter += cip_n;
		
		if (cip_counter > cip_d) {
			want_sync = 1;
			cip_counter -= cip_d;
		}
		
		*p++ = 0x01; /* Source node ID ! */
		*p++ = 0x78; /* Packet size in quadlets (480 / 4) */
		*p++ = 0x00;
		*p++ = continuity_counter;
		
		*p++ = 0x80; /* const */
		*p++ = isPAL ? 0x80 : 0x00;
		*p++ = 0xff; /* timestamp */
		*p++ = 0xff; /* timestamp */
		
		/* Timestamping is now done in the kernel driver! */
		if (!want_sync) { /* video data */
			continuity_counter++;
			
			memcpy(p, frame + vdata, 480);
			p += 480;
			vdata += 480;
		}
		
		*packet_sizes++ = p - targetbuf;
		targetbuf += MAX_PACKET_SIZE;
	}
	*packet_sizes++ = 0;

	push_back(&free_list, f_node);

	pthread_mutex_lock(&wakeup_mutex);
	pthread_cond_signal(&wakeup_cond);
	pthread_mutex_unlock(&wakeup_mutex);

        return 0;
}

int send_raw(const char*const* filenames, int channel, int nbuffers,
		int start_frame, int end_frame, int verbose_mode, 
		const char * underrun_data_filename, char *device)
{
	int viddev;
	unsigned char *send_buf;
	struct video1394_mmap v;
	struct video1394_queue_variable w;
	int unused_buffers;
	int got_frame;
	unsigned int packet_sizes[321];

	if ( filenames == NULL )
		return -1;

	src_filenames = filenames;
	if (!*src_filenames) {
		src_filenames = 0;
		src_fp = stdin;
	}
	else if (open_next_input ())
		return -1;
  
	if (underrun_data_filename) {
		FILE * fp = fopen(underrun_data_filename, "rb");
		if (!fp) {
			perror("fopen underrun data file");
			return -1;
		}
		underrun_data_frame = (unsigned char*) malloc(144000);
		if (read_frame(fp, underrun_data_frame, 
			       &underrun_frame_ispal) < 0) {
			fprintf(stderr, "Short read on reading underrun data "
				"frame...\n");
			fclose(fp);
			return -1;
		}
		fclose(fp);
	}

	while (start_frame > 0) {
		int isPAL,i;
		unsigned char frame[144000]; /* PAL is large enough... */

		for (i = 0; i < start_frame; i++) {
			if (verbose_mode) {
				fprintf(stderr, "Skipped frame %8d\r",
					start_frame - i);
			}
			if (read_frame(src_fp, frame, &isPAL) < 0) {
				break;
			}
		}
		start_frame -= i;
		if (!start_frame)
			break;
		if (open_next_input ())
			return -1;
	}

	if ((viddev = open(device,O_RDWR)) < 0) {
		perror("open video1394 device");
		if (src_fp && src_fp != stdin)
			fclose (src_fp);
		return -1;
  	}
                      
	v.channel = channel;
	v.sync_tag = 0;
	v.nb_buffers = nbuffers;
	v.buf_size = TARGETBUFSIZE * MAX_PACKET_SIZE; 
	v.packet_size = MAX_PACKET_SIZE;
	v.flags = VIDEO1394_VARIABLE_PACKET_SIZE;
	v.syt_offset = syt_offset;
	w.channel = v.channel;

	if (ioctl(viddev, VIDEO1394_TALK_CHANNEL, &v) < 0) {
		perror("VIDEO1394_TALK_CHANNEL");
		close (viddev);
		if (src_fp && src_fp != stdin)
			fclose (src_fp);
		return -1;
	}

	if ((send_buf = (unsigned char *) mmap(
			0, v.nb_buffers * v.buf_size, PROT_READ|PROT_WRITE,
			MAP_SHARED, viddev, 0)) == (unsigned char *)-1) {
		perror("mmap videobuffer");
		close (viddev);
		if (src_fp && src_fp != stdin)
		fclose (src_fp);
		return -1;
	}

	if (verbose_mode) {
		fprintf(stderr, "Filling buffers...\r");
	}

	/* signal(SIGTERM, sig_int_send_handler);
	   signal(SIGINT, sig_int_send_handler); */

	pthread_mutex_init(&queue_mutex, NULL);
	pthread_mutex_init(&wakeup_mutex, NULL);
	pthread_cond_init(&wakeup_cond, NULL);
	pthread_mutex_init(&wakeup_rev_mutex, NULL);
	pthread_cond_init(&wakeup_rev_cond, NULL);

	pthread_create(&file_io_thread, NULL, read_in_thread, NULL);

	pthread_mutex_lock(&wakeup_rev_mutex);
	pthread_cond_wait(&wakeup_rev_cond, &wakeup_rev_mutex);
	pthread_mutex_unlock(&wakeup_rev_mutex);

	if (verbose_mode) {
		fprintf(stderr, "Transmitting...\r");
	}

	unused_buffers = v.nb_buffers;
	w.buffer = 0;
	got_frame = 1;
	w.packet_sizes = packet_sizes;
	memset(packet_sizes, 0, sizeof(packet_sizes));

	for (;start_frame < end_frame;) {
		while (unused_buffers--) {
			got_frame = (fill_buffer(
			send_buf + w.buffer * v.buf_size,
			packet_sizes) < 0) ? 0 : 1;
			
			if (!got_frame) {
				break;
			}
			if (ioctl(viddev, VIDEO1394_TALK_QUEUE_BUFFER, &w)<0) {
				perror("VIDEO1394_TALK_QUEUE_BUFFER");
			}
			if (verbose_mode) {
				fprintf(stderr, "Sent frame %8d\r",
				start_frame);
			}
			w.buffer ++;
			w.buffer %= v.nb_buffers;
			start_frame++;
		}
		if (!got_frame) {
			break;
		}
		if (ioctl(viddev, VIDEO1394_TALK_WAIT_BUFFER, &w) < 0) {
			perror("VIDEO1394_TALK_WAIT_BUFFER");
		}
		unused_buffers = 1;
	}
  
	w.buffer = (v.nb_buffers + w.buffer - 1) % v.nb_buffers;
	
	if (ioctl(viddev, VIDEO1394_TALK_WAIT_BUFFER, &w) < 0) {
		perror("VIDEO1394_TALK_WAIT_BUFFER");
	}
	
	munmap(send_buf, v.nb_buffers * v.buf_size);
	
	if (ioctl(viddev, VIDEO1394_UNTALK_CHANNEL, &v.channel)<0) {
		perror("VIDEO1394_UNTALK_CHANNEL");
	}
	
	close(viddev);

	pthread_join(file_io_thread, NULL);

  return 0;
}

int rt_raisepri (int pri)
{
#ifdef _SC_PRIORITY_SCHEDULING
	struct sched_param scp;

	/*
	 * Verify that scheduling is available
	 */
	if (sysconf (_SC_PRIORITY_SCHEDULING) == -1) {
		fprintf (stderr, "WARNING: RR-scheduler not available, "
			 "disabling.\n");
		return (-1);
	} else {
		memset (&scp, '\0', sizeof (scp));
		scp.sched_priority = sched_get_priority_max (SCHED_RR) - pri;
		if (sched_setscheduler (0, SCHED_RR, &scp) < 0)	{
			fprintf (stderr, "WARNING: Cannot set RR-scheduler\n");
			return (-1);
		}
	}
#endif
	return (0);
}

/* ------------------------------------------------------------------------
   - main()
   ------------------------------------------------------------------------ */

#define DV_CONNECT_OPT_VERSION          0
#define DV_CONNECT_OPT_SEND             1
#define DV_CONNECT_OPT_VERBOSE          2
#define DV_CONNECT_OPT_CHANNEL          3
#define DV_CONNECT_OPT_KBUFFERS         4
#define DV_CONNECT_OPT_START_FRAME      5
#define DV_CONNECT_OPT_END_FRAME        6
#define DV_CONNECT_OPT_CIP_N_NTSC       7
#define DV_CONNECT_OPT_CIP_D_NTSC       8
#define DV_CONNECT_OPT_SYT_OFFSET       9
#define DV_CONNECT_OPT_MAX_BUFFERS      10
#define DV_CONNECT_OPT_UNDERRUN_DATA    11
#define DV_CONNECT_OPT_AUTOHELP         12
#define DV_CONNECT_OPT_DEVICE           13
#define DV_CONNECT_NUM_OPTS             14


int main(int argc, const char** argv)
{
#if HAVE_LIBPOPT
	int verbose_mode = 0;
	int send_mode = 0;
	int channel = 63;
	int buffers = 8;
	int start = 0;
	int end = 0xfffffff;
	const char* underrun_data = NULL;

        struct poptOption option_table[DV_CONNECT_NUM_OPTS+1]; 
        int rc;             /* return code from popt */
        poptContext optCon; /* context for parsing command-line options */

        option_table[DV_CONNECT_OPT_VERSION] = (struct poptOption) {
                longName: "version", 
                val: 'v', 
                descrip: "show dvconnect version number"
        }; /* version */

        option_table[DV_CONNECT_OPT_SEND] = (struct poptOption) {
                longName:   "send", 
                shortName:  's', 
                arg:        &send_mode,
                descrip:    "send data instead of capturing"
        }; /* send mode */
	
        option_table[DV_CONNECT_OPT_VERBOSE] = (struct poptOption) {
                longName:   "verbose", 
                shortName:  'v', 
                arg:        &verbose_mode,
                descrip:    "show status information while receiving / sending"
        }; /* verbose mode */

        option_table[DV_CONNECT_OPT_CHANNEL] = (struct poptOption) {
                longName:   "channel", 
                shortName:  'c', 
                argInfo:    POPT_ARG_INT, 
                arg:        &channel,
                argDescrip: "number",
                descrip:    "channel to send / receive on "
		"(range: 0 - 63, default = 63)"
        }; /* channel */

        option_table[DV_CONNECT_OPT_KBUFFERS] = (struct poptOption) {
                longName:   "kbuffers", 
                shortName:  'k', 
                argInfo:    POPT_ARG_INT, 
                arg:        &buffers,
                argDescrip: "number",
                descrip:    "number of kernel video frame buffers to use. "
		"default = 8"
        }; /* buffers */

        option_table[DV_CONNECT_OPT_START_FRAME] = (struct poptOption) {
                longName:   "start-frame", 
                argInfo:    POPT_ARG_INT, 
                arg:        &start,
                argDescrip: "count",
                descrip:    "start at <count> frame (defaults to 0)"
        }; /* start-frame */

        option_table[DV_CONNECT_OPT_END_FRAME] = (struct poptOption) {
                longName:   "end-frame", 
                shortName:  'e', 
                argInfo:    POPT_ARG_INT, 
                arg:        &end,
                argDescrip: "count",
                descrip:    "end at <count> frame (defaults to unlimited)"
        }; /* end-frames */

        option_table[DV_CONNECT_OPT_CIP_N_NTSC] = (struct poptOption) {
                longName:   "cip-n-ntsc", 
                argInfo:    POPT_ARG_INT, 
                arg:        &cip_n_ntsc,
                descrip:    "cip n ntsc timing counter (default: 2436)"
        }; /* cip_n_ntsc */

        option_table[DV_CONNECT_OPT_CIP_D_NTSC] = (struct poptOption) {
                longName:   "cip-d-ntsc", 
                argInfo:    POPT_ARG_INT, 
                arg:        &cip_d_ntsc,
                descrip:    "cip d ntsc timing counter (default: 38400)"
        }; /* cip_d_ntsc */

        option_table[DV_CONNECT_OPT_SYT_OFFSET] = (struct poptOption) {
                longName:   "syt-offset", 
                argInfo:    POPT_ARG_INT, 
                arg:        &syt_offset,
                descrip:    "syt offset (default: 10000 range: 10000-26000)"
        }; /* syt offset */

        option_table[DV_CONNECT_OPT_MAX_BUFFERS] = (struct poptOption) {
                longName:   "buffers", 
                shortName:  'b', 
                argInfo:    POPT_ARG_INT, 
                arg:        &max_buffer_blocks,
                argDescrip: "number",
                descrip:    "max number of file io thread buffers to use. "
		"default = 250"
        }; /* buffers */

        option_table[DV_CONNECT_OPT_UNDERRUN_DATA] = (struct poptOption) {
                longName:   "underrun-data", 
                shortName:  'u', 
                argInfo:    POPT_ARG_STRING, 
                arg:        &underrun_data,
                argDescrip: "filename",
                descrip:    "file with the frame to send in case of underruns"
        }; /* underrun-data */

        option_table[DV_CONNECT_OPT_AUTOHELP] = (struct poptOption) {
                argInfo: POPT_ARG_INCLUDE_TABLE,
                arg:     poptHelpOptions,
                descrip: "Help options",
        }; /* autohelp */

        option_table[DV_CONNECT_OPT_DEVICE] = (struct poptOption) {
                longName:   "device", 
                shortName:  'd', 
                argInfo: POPT_ARG_STRING,
                arg:     &device,
                descrip: "Specify the video1394 device (default /dev/video1394/0)",
        }; /* device */

        option_table[DV_CONNECT_NUM_OPTS] = (struct poptOption) { 
                NULL, 0, 0, NULL, 0 };

        optCon = poptGetContext(NULL, argc, 
                                (const char **)argv, option_table, 0);
        poptSetOtherOptionHelp(optCon, "<raw dv files or '-- -' for stdin/stdout>");

        while ((rc = poptGetNextOpt(optCon)) > 0) {
                switch (rc) {
                case 'v':
                        fprintf(stderr,"dvconnect: version %s, "
                                "http://libdv.sourceforge.net/\n",
                                "CVS 05/06/2001");
                        return 0;
                        break;
                default:
                        break;
                }
        }

        if (rc < -1) {
                /* an error occurred during option processing */
                fprintf(stderr, "%s: %s\n",
                        poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                        poptStrerror(rc));
		return -1;
        }

	if (channel < 0 || channel > 63) {
		fprintf(stderr, "Invalid channel chosen: %d\n"
			"Should be in the range 0 - 63\n", channel);
		return -1;
	}
	if (buffers < 0) {
		fprintf(stderr, "Number of buffers should be > 0!\n");
		return -1;
	}

	setpriority (PRIO_PROCESS, 0, -20);
	if (rt_raisepri (1) != 0) {
		setpriority (PRIO_PROCESS, 0, -20);
	}

#if _POSIX_MEMLOCK > 0
	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
	{
		if (verbose_mode)
			fprintf(stderr, "Cannot disable swapping\n");
	} 
	else
#endif
	{
		/* Prevent excessive underruns from locking down all mem. */
		ceil_buffer_blocks = 10*max_buffer_blocks;
	}

	if ( device == NULL )
		device = "/dev/video1394/0";

	if (send_mode) {
		send_raw(poptGetArgs(optCon), channel, buffers, start, end,
			verbose_mode, underrun_data, device);
	} else {
		capture_raw(poptGetArg(optCon), channel, buffers, start, end, 
			    verbose_mode, device);
	}
	return 0;
#else
#warning dvconnect can not work without libpopt!	

#endif
}

