#ifndef FIREHOSEPRIVATE_H
#define FIREHOSEPRIVATE_H

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define FIREHOSE_MAJOR 0
#define FIREHOSE_MINOR 6
#define FIREHOSE_POINT 0

// Packet sizes.  Packet count increases automatically

#define TOTAL_MEMORY 0x100000
#define PAYLOAD_SIZE 0x40000
#define DEFAULT_PACKETS (TOTAL_MEMORY / PAYLOAD_SIZE)



#define START_CODE 0x47
#define MAX_PORTS 0x10



// firehose_t state
#define STATE_CLOSED 0
#define STATE_TRANSMITTING 1
#define STATE_RECEIVING 2

// firehose_port_receiver_t state
#define PORT_RECEIVING 0                // Receiving packets
#define PORT_DONE      1                // No more data coming in





#include <pthread.h>

typedef struct
{
	int64_t packet_id;
	int32_t payload_size;
// If this packet is an end of file marker there is no payload
	char end_of_file;
} firehose_header_t;

typedef struct
{
	pthread_t tid;
	int done;
// Encoded packet for sending
	char packet[PAYLOAD_SIZE + sizeof(firehose_header_t)];
	int packet_size;
	int socket_fd;
	struct sockaddr_in addr;
	int firewire_port;
	int use_firewire;
	int use_inet;

// firehose_t
	void *hose;
//	raw1394handle_t handle;
} firehose_transmitter_t;


typedef struct
{
	pthread_t tid;
	int socket_fd;
	int number;
	struct sockaddr_in addr;
	int state;
	pthread_mutex_t startup_lock;

// firehose_receiver_t
	void *receiver;
// firehose_t
	void *hose;
} firehose_port_receiver_t;


typedef struct
{
	firehose_port_receiver_t *ports[MAX_PORTS];
	pthread_mutex_t packet_lock;

// Don't fill packet buffer until space becomes available
	pthread_mutex_t input_lock;

// Don't scan packet buffers until a new packet is available.
	pthread_mutex_t output_lock;

// The receiver ports read packets until all packets are filled
// then locks the input_lock until a packet is emptied by the caller.
// Packets fill from the end to the start
	char **packet_buffer;
	int *packet_filled;
	int total_filled;
	int total_allocated;

	int64_t total_bytes_received;
	int64_t total_bytes_read;

// Data not read by the last firehose_read_data but contained in the
// last packet_buffer.
	char spill[PAYLOAD_SIZE];
	long spill_allocated;
	long spill_size;

// Next packet to be read
	int64_t current_id;

// Highest state of all the port receivers
	int loop_state;
	int error;
// State of buffer reader
	int output_state;
// firehose_t
	void *hose;
} firehose_receiver_t;


typedef struct
{
	char *destinations[MAX_PORTS];
	int receivers[MAX_PORTS];
	int total_destinations;
	int total_receivers;
	int error;
	int state;
	int use_udp;

// Transmitting data
	char payload[PAYLOAD_SIZE];
	int64_t packet_id;
	int32_t payload_size;
	int end_of_file;

	firehose_transmitter_t *transmitters[MAX_PORTS];
	int total_transmitters;
	pthread_mutex_t input_lock, output_lock;


	struct sockaddr_in addr;
// In order to not have to close and reopen receiver ports we 
// stick all ports here.
	int socket_fd[MAX_PORTS];
} firehose_t;



#define SWAPBYTES(x, y) \
{ \
	(x) ^= (y); \
	(y) ^= (x); \
	(x) ^= (y); \
}

#define SWAP_HEADER(header) \
{ \
	SWAPBYTES(((char*)header)[0], ((char*)header)[7]); \
	SWAPBYTES(((char*)header)[1], ((char*)header)[6]); \
	SWAPBYTES(((char*)header)[2], ((char*)header)[5]); \
	SWAPBYTES(((char*)header)[3], ((char*)header)[4]); \
 \
	SWAPBYTES(((char*)header)[8], ((char*)header)[11]); \
	SWAPBYTES(((char*)header)[9], ((char*)header)[10]); \
}



#endif
