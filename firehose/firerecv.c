#include "firehose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <utime.h>

#define BUFFER_SIZE 0x100000

static void receiver_loop(void *ptr)
{

	firehose_receiver_t *receiver = (firehose_receiver_t*)ptr;
	char *buffer = calloc(1, BUFFER_SIZE);
	int i, result = 0;
	FILE *out;

	result = !firehose_read_data(receiver,
		buffer,
		1);

	if(!result)
	{
		if(buffer[0] != VERSION)
		{
			printf("Invalid version number, you MOR*ON!\n");
			firehose_delete_receiver(receiver);
			free(buffer);
		}
	}

// Read filename up to 0 byte
	i = 0;
	do
	{
		result = !firehose_read_data(receiver, 
			buffer + i, 
			1);
		i++;
	}while(buffer[i - 1] != 0 && !result);

	char *filename = strdup(buffer);

// Read dates
	struct utimbuf utime_buf;
	if(!result)
	{
		result = !firehose_read_data(receiver, 
			buffer, 
			8);
		utime_buf.actime = ((((uint64_t)buffer[0]) & 0xff) << 56) |
			((((uint64_t)buffer[1]) & 0xff) << 48) |
			((((uint64_t)buffer[2]) & 0xff) << 40) |
			((((uint64_t)buffer[3]) & 0xff) << 32) |
			((((uint64_t)buffer[4]) & 0xff) << 24) |
			((((uint64_t)buffer[5]) & 0xff) << 16) |
			((((uint64_t)buffer[6]) & 0xff) << 8) |
			(((uint64_t)buffer[7]) & 0xff);
	}

	if(!result)
	{
		result = !firehose_read_data(receiver, 
			buffer, 
			8);
		utime_buf.modtime = ((((uint64_t)buffer[0]) & 0xff) << 56) |
			((((uint64_t)buffer[1]) & 0xff) << 48) |
			((((uint64_t)buffer[2]) & 0xff) << 40) |
			((((uint64_t)buffer[3]) & 0xff) << 32) |
			((((uint64_t)buffer[4]) & 0xff) << 24) |
			((((uint64_t)buffer[5]) & 0xff) << 16) |
			((((uint64_t)buffer[6]) & 0xff) << 8) |
			(((uint64_t)buffer[7]) & 0xff);
	}


// Open output file
	if(!result && !(out = fopen(filename, "wb")))
	{
		perror("fopen");
		result = 1;
	}

// Read off the pipe
	if(!result)
	{
		printf("Receiving %s...", filename);
		fflush(stdout);
		while(!firehose_eof(receiver) && !result)
		{
			long bytes_read;
			bytes_read = firehose_read_data(receiver, 
				buffer, 
				BUFFER_SIZE);
			result = !fwrite(buffer, bytes_read, 1, out);
		}

		fclose(out);
// Set time
		utime(filename, &utime_buf);
		printf("Done.\n");
	}

// Stop reading
	firehose_delete_receiver(receiver);
	free(buffer);
	free(filename);
}




int main(int argc, char *argv[])
{
	int i;
	firehose_t *hose;
	int ports[argc];
	int total_ports = 0;
	int result = 0;

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-i"))
		{
			if(i + 1 < argc)
			{
				i++;
				ports[total_ports] = atol(argv[i]);
				total_ports++;
			}
			else
			{
				printf("-i needs a port number you Mor*on!\n");
				exit(1);
			}
		}
	}


	if(!total_ports)
	{
		printf(
"Usage: %s -i port1 -i port2 ...\n",
argv[0]
		);
		exit(1);
	}

	hose = firehose_new();


// Set up ports

	for(i = 0; i < total_ports; i++)
	{
		firehose_add_receiver(hose, ports[i]);
	}





	while(!result)
	{
        pthread_attr_t  attr;
		firehose_receiver_t *receiver;
		pthread_t tid;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

// Wait for connection
		result = firehose_open_read(hose, &receiver);

// Start receiving in background
		if(!result) 
			pthread_create(&tid, &attr, (void*)receiver_loop, receiver);
	}




	firehose_delete(hose);
	return 0;
}

