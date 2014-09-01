#ifndef FIREHOSE_H
#define FIREHOSE_H

#include "firehoseprivate.h"




#define VERSION 1




// Create a new firehose object
firehose_t* firehose_new();

// Set UDP mode
void firehose_set_udp(firehose_t *hose, int value);

// Stop writing or reading and
// delete a firehose object
void firehose_delete(firehose_t *hose);










// Define a destination host and port to write to.
// Hostname is of the format "hostname:port"
void firehose_add_destination(firehose_t *hose, char *hostname);

// Open ports for writing.
int firehose_open_write(firehose_t *hose);

// Write a buffer.  Returns the number of bytes written.
// The data_size should be a big multiple of PAYLOAD_SIZE
long firehose_write_data(firehose_t *hose, char *data, long data_size);











// Define a receiving port to read from
void firehose_add_receiver(firehose_t *hose, int port);

// Open ports for reading.
//
// Returns TRUE if error and FALSE if success.
//
// A new receiver is stored in **receiver which must be passed to 
// firehose_read_data.  A server should run firehose_open_read
// over and over in a loop and call firehose_read_data in a thread,
// one thread for every call to firehose_open_read which returns 0.
int firehose_open_read(firehose_t *hose, firehose_receiver_t **receiver);

// Read data_size bytes into *data.  Returns the number of bytes read.
long firehose_read_data(firehose_receiver_t *receiver, 
	char *data, 
	long data_size);

// Returns TRUE if end of stream was reached on the receiver.
int firehose_eof(firehose_receiver_t *receiver);

// Run when finished with every *receiver object
void firehose_delete_receiver(firehose_receiver_t *receiver);












#endif
