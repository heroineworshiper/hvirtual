#include "firehose.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define BUFFER_SIZE 0x100000







typedef struct
{
	struct timeval start_time;
	struct timeval last_time;
	int64_t bytes_transferred;
} status_t;


void init_status(status_t *status)
{
	gettimeofday(&status->start_time, 0);
	gettimeofday(&status->last_time, 0);
	status->bytes_transferred = 0;
	fprintf(stderr, "Reading from stdin...\n");
}

void update_status(status_t *status)
{
	struct timeval new_time;
	gettimeofday(&new_time, 0);
	if(new_time.tv_sec - status->last_time.tv_sec > 1)
	{
		fprintf(stderr, "%lld bytes sent.  %lld bytes/sec         \n", 
			status->bytes_transferred,
			(int64_t)status->bytes_transferred / 
			(int64_t)(new_time.tv_sec - status->start_time.tv_sec));
		fflush(stdout);
		status->last_time = new_time;
		status->start_time = new_time;
		status->bytes_transferred = 0;
	};
}

void stop_status()
{
	fprintf(stderr, "\nDone.\n");
}







static void receiver_loop(void *ptr)
{
	firehose_receiver_t *receiver = (firehose_receiver_t*)ptr;
	char *buffer = calloc(1, BUFFER_SIZE);
	int i, result = 0;

// Read off the pipe
	if(!result)
	{
		fprintf(stderr, "Writing to stdout...\n");
		while(!firehose_eof(receiver) && !result)
		{
			long bytes_read;
			bytes_read = firehose_read_data(receiver, 
				buffer, 
				BUFFER_SIZE);
			result = !fwrite(buffer, bytes_read, 1, stdout);
		}

		fflush(stdout);
		fprintf(stderr, "Done.\n");
	}

// Stop reading
	firehose_delete_receiver(receiver);
	free(buffer);
}




int main(int argc, char *argv[])
{
	int i;
	firehose_t *hose;
	int input_ports[argc];
	char *output_ports[argc];
	int total_receivers = 0, total_transmitters = 0;
	int result = 0;
	int do_receive = 0, do_transmit = 0;
	int use_udp = 0;

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-i"))
		{
			if(i + 1 < argc)
			{
				i++;
				input_ports[total_receivers] = atol(argv[i]);
				total_receivers++;
				do_receive = 1;
			}
			else
			{
				fprintf(stderr, "-i needs a port number you Mor*on!\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-o"))
		{
			if(i + 1 < argc)
			{
				i++;
				output_ports[total_transmitters] = argv[i];
				total_transmitters++;
				do_transmit = 1;
			}
			else
			{
				fprintf(stderr, "-o needs a port number you Mor*on!\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-u"))
		{
			use_udp = 1;
		}
		else
		fprintf(stderr, "Unknown argument %s\n", argv[i]);
	}


	if(!total_receivers && !total_transmitters)
	{
		fprintf(stderr, 
"Usage: %s -i port1 -i port2 -o port1 -o port2 ...\n"
"-i invokes receiving end and listens on every port preceeded by -i\n"
"-o invokes transmitting and sends to every host preceeded by -o\n",
argv[0]
		);
		exit(1);
	}

	if(do_receive && do_transmit)
	{
		fprintf(stderr, "Both receive and transmit ports were given!.\n");
		exit(1);
	}


	hose = firehose_new();


	if(do_receive)
	{
// Set up ports
		for(i = 0; i < total_receivers; i++)
		{
			firehose_add_receiver(hose, input_ports[i]);
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
	}





	if(do_transmit)
	{
		char *buffer = calloc(1, BUFFER_SIZE);
		status_t status;
		for(i = 0; i < total_transmitters; i++)
		{
			firehose_add_destination(hose, output_ports[i]);
		}


		if(!result)
		{
			result = firehose_open_write(hose);
		}
		
		
		init_status(&status);
		
		while(!feof(stdin) && !result)
		{
			long bytes_written = fread(buffer, 1, BUFFER_SIZE, stdin);
			result = !firehose_write_data(hose, buffer, bytes_written);
			status.bytes_transferred += bytes_written;
			update_status(&status);
		}

		stop_status();
	}









	firehose_delete(hose);
	return 0;
}

