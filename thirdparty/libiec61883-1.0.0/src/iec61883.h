/*
 * libiec61883/iec61883.h - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _IEC61883_H
#define _IEC61883_H

#include <libraw1394/raw1394.h>
#include <endian.h>

#ifdef __cplusplus
extern "C" {
#endif

enum iec61883_datarate {
    IEC61883_DATARATE_100 = 0,
    IEC61883_DATARATE_200,
    IEC61883_DATARATE_400
};

/*******************************************************************************
 * Audio and Music Data Transport Protocol 
 **/

enum iec61883_amdtp_format {
	IEC61883_AMDTP_FORMAT_RAW,
	IEC61883_AMDTP_FORMAT_IEC958_PCM,
	IEC61883_AMDTP_FORMAT_IEC958_AC3
};

enum iec61883_cip_mode {
	IEC61883_MODE_BLOCKING_EMPTY,
	IEC61883_MODE_BLOCKING_NODATA,
	IEC61883_MODE_NON_BLOCKING,
};

enum iec61883_amdtp_sample_format {
	IEC61883_AMDTP_INPUT_NA = -1,
	IEC61883_AMDTP_INPUT_LE16 = 0,
	IEC61883_AMDTP_INPUT_BE16,
	IEC61883_AMDTP_INPUT_LE20,
	IEC61883_AMDTP_INPUT_BE20,
	IEC61883_AMDTP_INPUT_LE24,
	IEC61883_AMDTP_INPUT_BE24
};

#define IEC61883_FDF_SFC_32KHZ   0x00
#define IEC61883_FDF_SFC_44K1HZ  0x01
#define IEC61883_FDF_SFC_48KHZ   0x02
#define IEC61883_FDF_SFC_88K2HZ  0x03
#define IEC61883_FDF_SFC_96KHZ   0x04
#define IEC61883_FDF_SFC_176K4HZ 0x05
#define IEC61883_FDF_SFC_192KHZ  0x06

typedef struct iec61883_amdtp* iec61883_amdtp_t;

/**
 * iec61883_amdtp_recv_t - AMDTP receive callback function prototype
 * @data: pointer to the buffer containing audio data
 * @nsamples: the number of samples in the buffer
 * @dimension: the number of audio channels
 * @rate: sampling frequency
 * @format: one of enum iec61883_amdtp_format
 * @sample_format: one of enum iec61883_amdtp_sample_format
 * @callback_data: the opaque pointer you supplied in the init function
 *
 * The buffer contains consumer-ready 16bit raw PCM if it is in that format.
 * Otherwise, the data consists of quadlets containing only one sample with 
 * any necessary padding minus the label if pcm, otherwise, for IEC958
 * audio data, the header remains intact.
 *
 * Returns:
 * 0 for success or -1 for failure
 */
typedef int 
(*iec61883_amdtp_recv_t) (iec61883_amdtp_t amdtp, char *data, int nsamples, 
	unsigned int dbc, unsigned int dropped, void *callback_data);

typedef int 
(*iec61883_amdtp_xmit_t) (iec61883_amdtp_t amdtp, char *data, int nevents, 
	unsigned int dbc, unsigned int dropped, void *callback_data);

/**
 * iec61883_amdtp_xmit_init - setup transmission of AMDTP
 * @handle: the libraw1394 handle to use for all operations
 * @rate: one of enum iec61883_datarate
 * @format: one of enum iec61883_amdtp_format to describe audio data format
 * @sample_format: one of enum iec61883_amdtp_sample_format
 * @mode: one of iec61883_cip_mode
 * @dimension: the number of audio channels
 * @get_data: a function pointer to your callback routine
 * @callback_data: an opaque pointer to provide to your callback function
 *
 * AMDTP =  Audio and Music Data Transport Protocol
 *
 * Returns: 
 * A pointer to an iec61883_amdtp object upon success or NULL on failure.
 **/
iec61883_amdtp_t
iec61883_amdtp_xmit_init(raw1394handle_t handle,
		int rate, int format, int sample_format, int mode, int dimension,
		iec61883_amdtp_xmit_t get_data, void *callback_data);

/**
 * iec61883_amdtp_recv_init - setup reception of AMDTP
 * @handle: the libraw1394 handle to use for all operations
 * @put_data: a function pointer to your callback routine
 * @callback_data: an opaque pointer to provide to your callback function
 *
 * Returns: 
 * A pointer to an iec61883_amdtp object upon success or NULL on failure.
 **/
iec61883_amdtp_t
iec61883_amdtp_recv_init(raw1394handle_t handle,
		iec61883_amdtp_recv_t put_data, void *callback_data);

/** 
 * iec61883_amdtp_xmit_start - start transmission of AMDTP
 * @amdtp: pointer to iec61883_amdtp object
 * @channel: the isochronous channel number to use
 **/ 
int
iec61883_amdtp_xmit_start(iec61883_amdtp_t amdtp, int channel);

/**
 * iec61883_amdtp_xmit_stop - stop transmission of AMDTP
 * @amdtp: pointer to iec61883_amdtp object
 **/
void
iec61883_amdtp_xmit_stop(iec61883_amdtp_t amdtp);

/** 
 * iec61883_amdtp_recv_start - start reception of AMDTP
 * @amdtp: pointer to iec61883_amdtp object
 * @channel: the isochronous channel number to use
 **/ 
int
iec61883_amdtp_recv_start(iec61883_amdtp_t amdtp, int channel);

/**
 * iec61883_amdtp_recv_stop - stop reception of AMDTP
 * @amdtp: pointer to iec61883_amdtp object
 **/
void
iec61883_amdtp_recv_stop(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_close - destroy an amdtp object
 * @amdtp: pointer to iec61883_amdtp object
 **/
void
iec61883_amdtp_close(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_get_buffers - get the size of the buffer
 * @amdtp: pointer to iec61883_amdtp object
 **/
unsigned int
iec61883_amdtp_get_buffers(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_set_buffers - set the size of the buffer
 * @amdtp: pointer to iec61883_amdtp object
 * @packets: the size of the buffer in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_amdtp_set_buffers(iec61883_amdtp_t amdtp, unsigned int packets);

/**
 * iec61883_amdtp_get_prebuffers - get the size of the pre-transmission buffer
 * @amdtp: pointer to iec61883_amdtp object
 **/
unsigned int
iec61883_amdtp_get_prebuffers(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_set_prebuffers - set the size of the pre-transmission buffer
 * @amdtp: pointer to iec61883_amdtp object
 * @packets: the size of the buffer in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_amdtp_set_prebuffers(iec61883_amdtp_t amdtp, unsigned int packets);

/**
 * iec61883_amdtp_get_irq_interval - get the interrupt rate
 * @amdtp: pointer to iec61883_amdtp object
 **/
unsigned int
iec61883_amdtp_get_irq_interval(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_set_irq_interval - set the interrupt rate
 * @amdtp: pointer to iec61883_amdtp object
 * @packets: the size of the interval in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_amdtp_set_irq_interval(iec61883_amdtp_t amdtp, unsigned int packets);

/**
 * iec61883_amdtp_get_synch - get behavior on close
 * @amdtp: pointer to iec61883_amdtp object
 *
 * If synch is not zero, then when stopping reception all packets in
 * raw1394 buffers are flushed and invoking your callback function and when
 * stopping transmission all packets in the raw1394 buffers are sent before
 * the close function returns.
 **/
int
iec61883_amdtp_get_synch(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_set_synch - set behavior on close
 * @amdtp: pointer to iec61883_amdtp object
 * @synch: 1 if you want sync behaviour, 0 otherwise
 *
 * If synch is not zero, then when stopping reception all packets in
 * raw1394 buffers are flushed and invoking your callback function and when
 * stopping transmission all packets in the raw1394 buffers are sent before
 * the close function returns.
 **/
void
iec61883_amdtp_set_synch(iec61883_amdtp_t amdtp, int synch);

/**
 * iec61883_amdtp_get_speed - get data rate for transmission
 * @amdtp: pointer to iec61883_amdtp object
 *
 * Returns:
 * one of enum raw1394_iso_speed (S100, S200, or S400)
 **/
int
iec61883_amdtp_get_speed(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_set_speed - set data rate for transmission
 * @amdtp: pointer to iec61883_amdtp object
 * @speed: one of enum raw1394_iso_speed (S100, S200, or S400)
 **/
void
iec61883_amdtp_set_speed(iec61883_amdtp_t amdtp, int speed);

/**
 * iec61883_amdtp_get_dropped - get the total number of dropped packets
 * @amdtp: pointer to iec61883_amdtp object
 *
 * Returns:
 * The total number of dropped packets since starting transmission or reception.
 **/
unsigned int
iec61883_amdtp_get_dropped(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_get_callback_data - get the callback data you supplied
 * @amdtp: pointer to iec61883_amdtp object
 *
 * This function is useful in bus reset callback routines because only
 * the libraw1394 handle is available, and libiec61883 uses the userdata
 * field of the libraw1394 handle.
 *
 * Returns:
 * An opaque pointer to whatever you supplied when you called init.
 **/
void *
iec61883_amdtp_get_callback_data(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_get_dimension - get the AMDTP dimension
 * @amdtp: pointer to iec61883_amdtp object
 *
 * This function is useful in your callbacks.
 *
 * Returns:
 * The numeric value of the current dimension.
 **/
int
iec61883_amdtp_get_dimension(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_get_rate - get the AMDTP sampling frequency
 * @amdtp: pointer to iec61883_amdtp object
 *
 * This function is useful in your callbacks.
 *
 * Returns:
 * The numeric value of the sampling frequency.
 **/
int
iec61883_amdtp_get_rate(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_get_format - get the AMDTP data format
 * @amdtp: pointer to iec61883_amdtp object
 *
 * This function is useful in your callbacks.
 *
 * Returns:
 * One of enum iec61883_amdtp_format.
 **/
enum iec61883_amdtp_format
iec61883_amdtp_get_format(iec61883_amdtp_t amdtp);

/**
 * iec61883_amdtp_get_sample_format - get the AMDTP sample format
 * @amdtp: pointer to iec61883_amdtp object
 *
 * This function is useful in your callbacks.
 *
 * Returns:
 * One of enum iec61883_amdtp_sample_format.
 **/
enum iec61883_amdtp_sample_format
iec61883_amdtp_get_sample_format(iec61883_amdtp_t amdtp);



/*******************************************************************************
 * DV Digital Video
 **/

typedef struct iec61883_dv* iec61883_dv_t;

typedef int 
(*iec61883_dv_recv_t)(unsigned char *data, int len, unsigned int dropped, 
	void *callback_data);

typedef int 
(*iec61883_dv_xmit_t)(unsigned char *data, int n_dif_blocks, 
	unsigned int dropped, void *callback_data);

/**
 * iec61883_dv_recv_init - setup reception of a DV stream
 * @handle: the libraw1394 handle to use for all operations
 * @put_data: a function pointer to your reception callback routine
 * @callback_data: an opaque data pointer to provide to your callback function
 *
 * Returns:
 * A pointer to an iec61883_dv object upon success or NULL on failure.
 **/
iec61883_dv_t
iec61883_dv_recv_init(raw1394handle_t handle,
		iec61883_dv_recv_t put_data,
		void *callback_data);

/**
 * iec61883_dv_xmit_init - setup transmission of a DV stream
 * @handle: the libraw1394 handle to use for all operations
 * @is_pal: set to non-zero if transmitting a PAL stream (applies to transmission only)
 * @get_data: a function pointer to your transmission callback routine
 * @callback_data: an opaque data pointer to provide to your callback function
 *
 * Returns:
 * A pointer to an iec61883_dv object upon success or NULL on failure.
 **/
iec61883_dv_t
iec61883_dv_xmit_init(raw1394handle_t handle, int is_pal,
		iec61883_dv_xmit_t get_data,
		void *callback_data);

/**
 * iec61883_dv_recv_start - start receiving a DV stream
 * @dv: pointer to iec61883_dv object
 * @channel: the isochronous channel number
 *
 * Returns:
 * 0 for success or -1 for failure
 **/
int
iec61883_dv_recv_start(iec61883_dv_t dv, int channel);

/**
 * iec61883_dv_xmit_start - start transmitting a DV stream
 * @dv: pointer to iec61883_dv object
 * @channel: the isochronous channel number
 *
 * Returns:
 * 0 for success or -1 for failure
 **/
int
iec61883_dv_xmit_start(iec61883_dv_t dv, int channel);

/**
 * iec61883_dv_recv_stop - stop reception of a DV stream
 * @dv: pointer to iec61883_dv ojbect
 **/
void
iec61883_dv_recv_stop(iec61883_dv_t dv);

/**
 * iec61883_dv_xmit_stop - stop transmission of a DV stream
 * @dv: pointer to iec61883_dv object
 **/
void
iec61883_dv_xmit_stop(iec61883_dv_t dv);

/**
 * iec61883_dv_close - stop reception or transmission and destroy iec61883_dv object
 * @dv: pointer to iec61883_dv object
 **/
void
iec61883_dv_close(iec61883_dv_t dv);

/**
 * iec61883_dv_get_buffers - get the size of the buffer
 * @dv: pointer to iec61883_dv object
 **/
unsigned int
iec61883_dv_get_buffers(iec61883_dv_t dv);

/**
 * iec61883_dv_set_buffers - set the size of the buffer
 * @dv: pointer to iec61883_dv object
 * @packets: the size of the buffer in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_dv_set_buffers(iec61883_dv_t dv, unsigned int packets);

/**
 * iec61883_dv_get_prebuffers - get the size of the pre-transmission buffer
 * @dv: pointer to iec61883_dv object
 **/
unsigned int
iec61883_dv_get_prebuffers(iec61883_dv_t dv);

/**
 * iec61883_dv_set_prebuffers - set the size of the pre-transmission buffer
 * @dv: pointer to iec61883_dv object
 * @packets: the size of the buffer in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_dv_set_prebuffers(iec61883_dv_t dv, unsigned int packets);

/**
 * iec61883_dv_get_irq_interval - get the interrupt rate
 * @dv: pointer to iec61883_dv object
 **/
unsigned int
iec61883_dv_get_irq_interval(iec61883_dv_t dv);

/**
 * iec61883_dv_set_irq_interval - set the interrupt rate
 * @dv: pointer to iec61883_dv object
 * @packets: the size of the interval in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_dv_set_irq_interval(iec61883_dv_t dv, unsigned int packets);

/**
 * iec61883_dv_get_synch - get behavior on close
 * @dv: pointer to iec61883_dv object
 *
 * If synch is not zero, then when stopping reception all packets in
 * raw1394 buffers are flushed and invoking your callback function and when
 * stopping transmission all packets in the raw1394 buffers are sent before
 * the close function returns.
 **/
int
iec61883_dv_get_synch(iec61883_dv_t dv);

/**
 * iec61883_dv_set_synch - set behavior on close
 * @dv: pointer to iec61883_dv object
 * @synch: 1 if you want sync behaviour, 0 otherwise
 *
 * If synch is not zero, then when stopping reception all packets in
 * raw1394 buffers are flushed and invoking your callback function and when
 * stopping transmission all packets in the raw1394 buffers are sent before
 * the close function returns.
 **/
void
iec61883_dv_set_synch(iec61883_dv_t dv, int synch);

/**
 * iec61883_dv_get_speed - get data rate for transmission
 * @dv: pointer to iec61883_dv object
 *
 * Returns:
 * one of enum raw1394_iso_speed (S100, S200, or S400)
 **/
int
iec61883_dv_get_speed(iec61883_dv_t dv);

/**
 * iec61883_dv_set_speed - set data rate for transmission
 * @dv: pointer to iec61883_dv object
 * @speed: one of enum raw1394_iso_speed (S100, S200, or S400)
 **/
void
iec61883_dv_set_speed(iec61883_dv_t dv, int speed);

/**
 * iec61883_dv_get_dropped - get the total number of dropped packets
 * @dv: pointer to iec61883_dv object
 *
 * Returns:
 * The total number of dropped packets since starting transmission or reception.
 **/
unsigned int
iec61883_dv_get_dropped(iec61883_dv_t dv);

/**
 * iec61883_dv_get_callback_data - get the callback data you supplied
 * @dv: pointer to iec61883_dv object
 *
 * This function is useful in bus reset callback routines because only
 * the libraw1394 handle is available, and libiec61883 uses the userdata
 * field of the libraw1394 handle.
 *
 * Returns:
 * An opaque pointer to whatever you supplied when you called init.
 **/
void *
iec61883_dv_get_callback_data(iec61883_dv_t dv);


/*******************************************************************************
 * The DV frame interface 
 **/

typedef struct iec61883_dv_fb* iec61883_dv_fb_t;

typedef int
(*iec61883_dv_fb_recv_t)(unsigned char *data, int len, int complete, 
	void *callback_data);

/**
 * iec61883_dv_fb_init - setup reception of DV frames
 * @handle: raw1394 handle.
 * @put_data: your callback function.
 * @callback_data: an opaque pointer you can send to your callback.
 *
 * This provides a frame-oriented instead of stream-oriented interface as
 * compared to iec61883_dv_init(). This means that starts at the beginning of a
 * frame, and all data is frame-aligned. Your callback function
 * will receive a complete DV frame even if packets are dropped.
 * The callback parameter @complete indicates if a full frame is received.
 *
 * Returns:
 * A pointer to an iec61883_dv_fb object upon success or NULL on failure.
 **/
iec61883_dv_fb_t
iec61883_dv_fb_init (raw1394handle_t handle, 
		iec61883_dv_fb_recv_t put_data,
		void *callback_data);

/**
 * iec61883_dv_fb_start - start receiving DV frames
 * @dvfb: pointer to iec61883_dv_fb object
 * @channel: the isochronous channel number
 *
 * Returns:
 * 0 for success or -1 for failure
 **/
int
iec61883_dv_fb_start(iec61883_dv_fb_t dvfb, int channel);

/**
 * iec61883_dv_fb_stop - stop receiving DV frames
 * @dvfb: pointer to iec61883_dv_fb object
 **/
void
iec61883_dv_fb_stop(iec61883_dv_fb_t dvfb);

/**
 * iec61883_dv_fb_close - stop receiving DV frames and destroy iec61883_dv_fb object
 * @dvfb: pointer to iec61883_dv_fb object
 **/
void
iec61883_dv_fb_close(iec61883_dv_fb_t dvfb);

/**
 * iec61883_dv_fb_get_incomplete - get the total number of incomplete frames
 * @dvfb: point to the iec61883_dv_fb object
 *
 * Returns:
 * The total number of incomplete frames due to dropped packets since starting
 * reception.
 **/
unsigned int
iec61883_dv_fb_get_incomplete(iec61883_dv_fb_t dvfb);

/**
 * iec61883_dv_fb_get_callback_data - get the callback data you supplied
 * @dvfb: pointer to iec61883_dv_fb object
 *
 * This function is useful in bus reset callback routines because only
 * the libraw1394 handle is available, and libiec61883 uses the userdata
 * field of the libraw1394 handle.
 *
 * Returns:
 * An opaque pointer to whatever you supplied when you called init.
 **/
void *
iec61883_dv_fb_get_callback_data(iec61883_dv_fb_t dvfb);


/*******************************************************************************
 * MPEG-2 Transport Stream
 **/

/* MPEG2 Time Shift Flag - not currently used anywhere */
#define IEC61883_FDF_MPEG2_TSF 0x01

/* size of a MPEG-2 Transport Stream packet */
#define IEC61883_MPEG2_TSP_SIZE 188

typedef struct iec61883_mpeg2* iec61883_mpeg2_t;

typedef int 
(*iec61883_mpeg2_recv_t)(unsigned char *data, int len, unsigned int dropped, 
	void *callback_data);

typedef int 
(*iec61883_mpeg2_xmit_t)(unsigned char *data, int n_packets, 
	unsigned int dropped, void *callback_data);

/**
 * iec61883_mpeg2_recv_init - setup reception of MPEG2-TS
 * @handle: the libraw1394 handle to use for all operations
 * @put_data: a function pointer to your callback routine
 * @callback_data: an opaque pointer to provide to your callback function
 *
 * Returns:
 * A pointer to an iec61883_mpeg2 object upon success or NULL for failure.
 **/
iec61883_mpeg2_t
iec61883_mpeg2_recv_init(raw1394handle_t handle,
		iec61883_mpeg2_recv_t put_data,
		void *callback_data);

/**
 * iec61883_mpeg2_xmit_init - setup transmission of MPEG2-TS
 * @handle: the libraw1394 handle to use for all operations
 * @get_data: a function pointer to your callback routine
 * @callback_data: an opaque pointer to provide to your callback function
 *
 * Returns:
 * A pointer to an iec61883_mpeg2 object upon success or NULL for failure.
 **/
iec61883_mpeg2_t
iec61883_mpeg2_xmit_init(raw1394handle_t handle,
		iec61883_mpeg2_xmit_t get_data,
		void *callback_data);

/**
 * iec61883_mpeg2_recv_start - start receiving MPEG2-TS
 * @mpeg2: pointer to iec61883_mpeg2 object
 * @channel: isochronous channel number
 *
 * Returns:
 * 0 for success or -1 for failure
 **/
int
iec61883_mpeg2_recv_start(iec61883_mpeg2_t mpeg2, int channel);

/**
 * iec61883_mpeg2_xmit_start - start transmitting MPEG2-TS
 * @mpeg2: pointer to iec61883_mpeg2 object
 * @pid: the program ID of the transport stream to select
 * @channel: isochronous channel number
 *
 * Returns:
 * 0 for success or -1 for failure
 **/
int
iec61883_mpeg2_xmit_start(iec61883_mpeg2_t mpeg2, int pid, int channel);

/**
 * iec61883_mpeg2_recv_stop - stop receiving MPEG2-TS
 * @mpeg2: pointer to the iec61883_mpeg2 object
 **/
void
iec61883_mpeg2_recv_stop(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_xmit_stop - stop transmitting MPEG2-TS
 * @mpeg2: pointer to the iec61883_mpeg2 object
 **/
void
iec61883_mpeg2_xmit_stop(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_close - stop receiving or transmitting and destroy iec61883_mpeg2 object
 * @mpeg2: pointer to iec61883_mpeg2 object
 **/
void
iec61883_mpeg2_close(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_get_buffers - get the size of the raw1394 buffer
 * @mpeg2: pointer to iec61883_mpeg2 object
 **/
unsigned int
iec61883_mpeg2_get_buffers(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_set_buffers - set the size of the raw1394 buffer
 * @mpeg2: pointer to iec61883_mpeg2 object
 * @packets: the size of the buffer in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_mpeg2_set_buffers(iec61883_mpeg2_t mpeg2, unsigned int packets);

/**
 * iec61883_mpeg2_get_prebuffers - get the size of the pre-transmission buffer
 * @mpeg2: pointer to iec61883_mpeg2 object
 **/
unsigned int
iec61883_mpeg2_get_prebuffers(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_set_prebuffers - set the size of the pre-transmission buffer
 * @mpeg2: pointer to iec61883_mpeg2 object
 * @packets: the size of the buffer in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_mpeg2_set_prebuffers(iec61883_mpeg2_t mpeg2, unsigned int packets);

/**
 * iec61883_mpeg2_get_irq_interval - get the interrupt rate
 * @mpeg2: pointer to iec61883_mpeg2 object
 **/
unsigned int
iec61883_mpeg2_get_irq_interval(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_set_irq_interval - set the interrupt rate
 * @mpeg2: pointer to iec61883_mpeg2 object
 * @packets: the size of the interval in packets
 *
 * This is an advanced option that can only be set after initialization and 
 * before reception or transmission.
 **/
void
iec61883_mpeg2_set_irq_interval(iec61883_mpeg2_t mpeg2, unsigned int packets);

/**
 * iec61883_mpeg2_get_synch - get behavior on close
 * @mpeg2: pointer to iec61883_mpeg2 object
 *
 * If synch is not zero, then when stopping reception all packets in
 * raw1394 buffers are flushed and invoking your callback function and when
 * stopping transmission all packets in the raw1394 buffers are sent before
 * the close function returns.
 **/
int
iec61883_mpeg2_get_synch(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_set_synch - set behavior on close
 * @mpeg2: pointer to iec61883_mpeg2 object
 * @synch: 1 if you want sync behaviour, 0 otherwise
 *
 * If synch is not zero, then when stopping reception all packets in
 * raw1394 buffers are flushed and invoking your callback function and when
 * stopping transmission all packets in the raw1394 buffers are sent before
 * the close function returns.
 **/
void
iec61883_mpeg2_set_synch(iec61883_mpeg2_t mpeg2, int synch);

/**
 * iec61883_mpeg2_get_speed - get data rate for transmission
 * @mpeg2: pointer to iec61883_mpeg2 object
 *
 * Returns:
 * one of enum raw1394_iso_speed (S100, S200, or S400)
 **/
int
iec61883_mpeg2_get_speed(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_set_speed - set data rate for transmission
 * @mpeg2: pointer to iec61883_mepg2 object
 * @speed: one of enum raw1394_iso_speed (S100, S200, or S400)
 **/
void
iec61883_mpeg2_set_speed(iec61883_mpeg2_t mpeg2, int speed);

/**
 * iec61883_mpeg2_get_dropped - get the total number of dropped packets
 * @mpeg2: pointer to iec61883_mpeg2 object
 *
 * Returns:
 * The total number of dropped packets since starting transmission or reception.
 **/
unsigned int
iec61883_mpeg2_get_dropped(iec61883_mpeg2_t mpeg2);

/**
 * iec61883_mpeg2_get_callback_data - get the callback data you supplied
 * @mpeg2: pointer to iec61883_mpeg2 object
 *
 * This function is useful in bus reset callback routines because only
 * the libraw1394 handle is available, and libiec61883 uses the userdata
 * field of the libraw1394 handle.
 *
 * Returns:
 * An opaque pointer to whatever you supplied when you called init.
 **/
void *
iec61883_mpeg2_get_callback_data(iec61883_mpeg2_t mpeg2);


/*******************************************************************************
 * Connection Management Procedures
 **/

enum iec61883_pcr_overhead_id {
	IEC61883_OVERHEAD_512 = 0, 
	IEC61883_OVERHEAD_32, 
	IEC61883_OVERHEAD_64,
	IEC61883_OVERHEAD_96,
	IEC61883_OVERHEAD_128,
	IEC61883_OVERHEAD_160,
	IEC61883_OVERHEAD_192,
	IEC61883_OVERHEAD_224,
	IEC61883_OVERHEAD_256,
	IEC61883_OVERHEAD_288,
	IEC61883_OVERHEAD_320,
	IEC61883_OVERHEAD_352,
	IEC61883_OVERHEAD_384,
	IEC61883_OVERHEAD_416,
	IEC61883_OVERHEAD_448,
	IEC61883_OVERHEAD_480
};

/**
 * iec61883_cmp_calc_bandwidth - calculate bandwidth allocation units
 * @handle: a libraw1394 handle
 * @output: the node id of the transmitter
 * @plug: the index of the output plug
 * @speed: if >= 0 then override the data rate factor
 *
 * Uses parameters of the output plug control register of the transmitter to
 * calculate what its bandwidth requirements are. Parameters inlude payload,
 * overhead_id, and data rate (optionally @speed).
 *
 * Returns:
 * bandwidth allocation units or -1 on failure
 **/
int
iec61883_cmp_calc_bandwidth (raw1394handle_t handle, nodeid_t output, int plug,
	int speed);

/**
 * iec61883_cmp_connect - establish or overlay connection automatically
 * @handle: a libraw1394 handle
 * @output: node id of the transmitter
 * @oplug: the output plug to use. If -1, find the first online plug, and
 * upon return, contains the plug number used.
 * @input: node id of the receiver
 * @iplug: the input plug to use. If -1, find the first online plug, and
 * upon return, contains the plug number used.
 * @bandwidth: an input/output parameter. As an input this is a boolean that
 * indicates whether to perform bandwidth allocation. Supply 0 to skip bandwidth
 * allocation; any other value permits it. Upon return, actual bandwidth allocation
 * units allocated are returned, which you supply to the disconnect routine.
 *
 * This is a high level function that attempts to be as smart as possible, but
 * it gives point-to-point connections higher priority over broadcast connections.
 * It can automatically handle situations where either @input and/or @output does
 * not implement plug control registers. However, if one node implements plug
 * registers it assumes the other node has some sort of manual channel selection
 * (i.e., through software or a control panel).
 *
 * Returns:
 * It returns the isochronous channel number selected or -1 if the function has
 * failed for any reason.
 **/
int
iec61883_cmp_connect (raw1394handle_t handle, nodeid_t output, int *oplug,
	nodeid_t input, int *iplug, int *bandwidth);

/**
 * iec61883_cmp_reconnect - reestablish or overlay connection after a bus reset
 * @handle: a libraw1394 handle
 * @output: node id of the transmitter
 * @oplug: the output plug to use. If -1, find the first online plug, and
 * upon return, contains the plug number used.
 * @input: node id of the receiver
 * @iplug: the input plug to use. If -1, find the first online plug, and
 * upon return, contains the plug number used.
 * @bandwidth: an input/output parameter. As an input this is a boolean that
 * indicates whether to perform bandwidth allocation. Supply 0 to skip bandwidth
 * allocation; any other value permits it. Upon return, actual bandwidth allocation
 * units allocated are returned, which you supply to the disconnect routine.
 * @channel: the channel number on which to reconnect, if possible
 *
 * This is a high level function that attempts to be as smart as possible, but
 * it gives point-to-point connections higher priority over broadcast connections.
 * It can automatically handle situations where either @input and/or @output does
 * not implement plug control registers. However, if one node implements plug
 * registers it assumes the other node has some sort of manual channel selection
 * (i.e., through software or a control panel).
 *
 * This function is useful in bus reset situations where it would be nice to 
 * reconfigure the transmission or reception if the channel does not need to
 * change.
 *
 * Returns:
 * It returns the isochronous channel number selected or -1 if the function has
 * failed for any reason.
 **/
int
iec61883_cmp_reconnect (raw1394handle_t handle, nodeid_t output, int *oplug,
	nodeid_t input, int *iplug, int *bandwidth, int channel);

/**
 * iec61883_cmp_disconnect - break connection automatically
 * @handle: a libraw1394 handle
 * @output: node id of the transmitter
 * @oplug: the output plug to use, or -1 to find the first online plug.
 * @input: node id of the receiver
 * @iplug: the input plug to use, or -1 to find the first online plug.
 * @channel: the isochronous channel in use
 * @bandwidth: the number of bandwidth allocation units to release when needed
 *
 * This high level connection management function automatically locates the
 * appropriate plug on @output and @input based upon the channel number. It 
 * gracefully handles when node plugs exist on @output and/or @input. When there 
 * are no more point-to-point connections on output plug, then the channel and
 * bandwidth are released.
 *
 * Returns:
 * 0 on success or -1 on failure
 **/
int
iec61883_cmp_disconnect (raw1394handle_t handle, nodeid_t output, int oplug,
	nodeid_t input, int iplug, unsigned int channel, unsigned int bandwidth);

/**
 * iec61883_cmp_normalize_output - make IRM channels available consistent with plug
 * @handle: a libraw1394 handle
 * @node: node id of the transmitter
 *
 * Loops over every output plug on the node, and if it has a point-to-point or
 * broadcast connection, then make sure the channel is allocated with the 
 * isochronous resource manager. This is used to help provide bus sanity and prevent
 * a channel from being used on a subsequent connection that might really already be
 * in usage!
 *
 * Returns:
 * 0 on success or -1 ojn failure
 **/
int
iec61883_cmp_normalize_output (raw1394handle_t handle, nodeid_t node);

/*
 * The following CMP functions are lower level routines used by the above 
 * connection procedures exposed here in case the above procedures do not fit 
 * your use cases.
 */
int
iec61883_cmp_create_p2p (raw1394handle_t handle, 
	nodeid_t output_node, int output_plug,
	nodeid_t input_node, int input_plug,	
	unsigned int channel, unsigned int speed);

int
iec61883_cmp_create_p2p_output (raw1394handle_t handle, 
	nodeid_t output_node, int output_plug,
	unsigned int channel, unsigned int speed);

int
iec61883_cmp_create_p2p_input (raw1394handle_t handle,
	nodeid_t input_node, int input_plug,
	unsigned int channel);

int
iec61883_cmp_create_bcast (raw1394handle_t handle, 
	nodeid_t output_node, int output_plug,
	nodeid_t input_node, int input_plug,
	unsigned int channel, unsigned int speed);

int
iec61883_cmp_create_bcast_output (raw1394handle_t handle, 
	nodeid_t output_node, int output_plug,
	unsigned int channel, unsigned int speed);
		
int
iec61883_cmp_create_bcast_input (raw1394handle_t handle,
	nodeid_t input_node, int input_plug,
	unsigned int channel);

int
iec61883_cmp_overlay_p2p (raw1394handle_t handle, 
	nodeid_t output_node, int output_plug,
	nodeid_t input_node, int input_plug);

int
iec61883_cmp_overlay_p2p_output (raw1394handle_t handle, 
	nodeid_t output_node, int output_plug);

int
iec61883_cmp_overlay_p2p_input (raw1394handle_t handle,
	nodeid_t input_node, int input_plug);
		
int
iec61883_cmp_overlay_bcast (raw1394handle_t handle, 
	nodeid_t output_node, int output_plug,
	nodeid_t input_node, int input_plug);

/**
 * iec61883_plug_impr_init - Initialise hosting of local input plug registers.
 * @h: A raw1394 handle.
 * @data_rate: an enum iec61883_datarate
 *
 * Initially, no plugs are available; call iec61883_plug_add_ipcr() to add
 * plugs.
 *
 * Returns: 
 * 0 for success, -EINVAL if data_rate is invalid, or or -1 (errno) on 
 * libraw1394 failure.
 **/
int
iec61883_plug_impr_init (raw1394handle_t h, unsigned int data_rate);

/**
 * ice61883_plug_impr_clear - Set the number of input plugs to zero.
 * @h: A raw1394 handle.
 **/
void
iec61883_plug_impr_clear (raw1394handle_t h);

/** 
 * iec61883_plug_impr_close - Stop hosting local input plugs.
 * @h: A raw1394 handle.
 *
 * Returns: 
 * 0 for success, -1 (errno) on error.
 **/
int
iec61883_plug_impr_close (raw1394handle_t h);

/**
 * iec61883_plug_add_ipcr - Add a local input plug.
 * @h: A raw1394 handle.
 * @online: The initial state of the plug: online (1) or not (0).
 *
 * You must call iec61883_plug_impr_init() before this.
 *
 * Returns: 
 * plug number (>=0) on sucess or -EINVAL if online is not 0 or 1, 
 * -ENOSPC if maximum plugs reached, or -EPERM if iec61883_plug_impr_init() 
 * not called.
 **/
int
iec61883_plug_ipcr_add (raw1394handle_t h, unsigned int online);

/**
 * iec61883_plug_ompr_init - Initialise hosting of local output plug registers.
 * @h: A raw1394 handle
 * @data_rate: one of enum iec61883_datarate
 * @bcast_channel: the broacast channel base (0 - 63)
 *
 * Initially, no plugs are available; call iec61883_plug_add_opcr() to add
 * plugs.
 *
 * Returns: 
 * 0 for success, -EINVAL if data_rate or bcast_channel is invalid, 
 * or or -1 (errno) on libraw1394 failure.
 **/
int
iec61883_plug_ompr_init (raw1394handle_t h, unsigned int data_rate, 
	unsigned int bcast_channel);

/**
 * ice61883_plug_ompr_clear - Set the number of output plugs to zero.
 * @h: A raw1394 handle.
 **/
void
iec61883_plug_ompr_clear (raw1394handle_t h);

/**
 * iec61883_plug_ompr_close - Stop hosting local output plugs.
 * @h: A raw1394 handle.
 *
 * Returns: 
 * 0 for success, -1 (errno) on error.
 **/
int
iec61883_plug_ompr_close (raw1394handle_t h);

/**
 * iec61883_plug_add_opcr - Add a local output plug.
 * @h: A raw1394 handle.
 * @online: The initial state of the plug: online (1) or not (0).
 * @overhead_id: one of enum iec61883_pcr_overhead_id.
 * @payload: the maximum number of quadlets per packet.
 *
 * You must call iec61883_plug_ompr_init() before this.
 *
 * Returns: 
 * plug number (>=0) on sucess, -EINVAL if online is not 0 or 1 or
 * if overhead_id or payload are out of range, -ENOSPC if maximum plugs 
 * reached, or -EPERM if iec61883_plug_ompr_init() not called.
 **/
int
iec61883_plug_opcr_add (raw1394handle_t h, unsigned int online,
	unsigned int overhead_id, unsigned int payload);


#ifdef __cplusplus
}
#endif

#endif /* _IEC61883_H */
