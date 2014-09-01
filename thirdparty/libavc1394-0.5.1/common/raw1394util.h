#ifndef RAW1394UTIL_H
#define RAW1394UTIL_H 1

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* maximum number of retry attempts on raw1394 async transactions */
#define MAXTRIES 20
/* amount of delay in nanoseconds to wait before retrying raw1394 async transaction */
#define RETRY_DELAY 20000

/* Mask and shift macros */
#define RAW1394_MASK_ACK(x) ((x) >> 16)
#define RAW1394_MASK_RCODE(x) ((x) & 0xffff)

/* Ack codes */
#define ACK_COMPLETE		0x1
#define ACK_PENDING		0x2
#define ACK_BUSY_X		0x4
#define ACK_BUSY_A		0x5
#define ACK_BUSY_B		0x6
#define ACK_DATA_ERROR		0xd
#define ACK_TYPE_ERROR		0xe
#define ACK_LOCAL		0x10

/* Return codes */
#define RCODE_COMPLETE		0x0
#define RCODE_CONFLICT_ERROR	0x4
#define RCODE_DATA_ERROR	0x5
#define RCODE_TYPE_ERROR	0x6
#define RCODE_ADDRESS_ERROR	0x7

int
cooked1394_read(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                        size_t length, quadlet_t *buffer);

int
cooked1394_write(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                         size_t length, quadlet_t *data);
#ifdef __cplusplus
}
#endif
#endif
