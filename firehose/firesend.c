#include "firehose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 0x100000





// Status information

typedef struct
{
	struct timeval start_time;
	struct timeval last_time;
	int64_t bytes_transferred;
	int64_t total_bytes;
	char filename[1024];
} status_t;


void init_status(status_t *status, char *filename)
{
	gettimeofday(&status->start_time, 0);
	gettimeofday(&status->last_time, 0);
	status->bytes_transferred = 0;
	status->total_bytes = 0;
	strcpy(status->filename, filename);
}

void update_status(status_t *status)
{
	struct timeval new_time;
	gettimeofday(&new_time, 0);
	if(new_time.tv_sec - status->last_time.tv_sec > 2)
	{
		printf("Sending %s %lld @ %lld bytes/sec           \r", 
			status->filename,
			status->total_bytes,
			(int64_t)status->bytes_transferred / 
			(int64_t)(new_time.tv_sec - status->start_time.tv_sec));
		fflush(stdout);
		status->bytes_transferred = 0;
		gettimeofday(&status->start_time, 0);
		gettimeofday(&status->last_time, 0);
	};
}

void stop_status(status_t *status)
{
	printf("Sending %s Done.                              \n", status->filename);
}






int main(int argc, char *argv[])
{
	char *filenames[argc];
	char *sockets[argc];
	int result = 0;
	int i, j;
	char *buffer = calloc(1, BUFFER_SIZE);
	int total_sockets = 0;
	int total_filenames = 0;






// Parse args
	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-i"))
		{
			if(i + 1 < argc)
			{
				i++;
				sockets[total_sockets] = malloc(strlen(argv[i]) + 1);
				strcpy(sockets[total_sockets], argv[i]);
				total_sockets++;
			}
			else
			{
				printf("-i needs an address of form hostname:port you Mor*on!\n");
				exit(1);
			}
		}
		else
		{
			filenames[total_filenames] = malloc(strlen(argv[i]) + 1);
			strcpy(filenames[total_filenames], argv[i]);
			total_filenames++;
		}
	}





// Usage
	if(!total_filenames || !total_sockets)
	{
		printf(
"Usage: %s -i host1:port1 -i host2:port2 ... <filename1> <filename2> ...\n",
argv[0]
		);
		exit(1);
	}




// Upload the files one by one
	for(i = 0; i < total_filenames; i++)
	{
		firehose_t *hose = firehose_new();
		FILE *in;

		for(j = 0; j < total_sockets; j++)
		{
			firehose_add_destination(hose, sockets[j]);
		}

		result = firehose_open_write(hose);

		if(!result)
		{
			if(!(in = fopen(filenames[i], "rb")))
			{
				perror("fopen");
				result = 1;
			}
		}

		struct stat64 stat_buf;
		if(!result)
		{
			stat64(filenames[i], &stat_buf);
		}


		if(!result)
		{
			status_t status;
			init_status(&status, filenames[i]);

// Send version
			unsigned char data[9];
			data[0] = VERSION & 0xff;
			result = !firehose_write_data(hose,
				data,
				1);

// Send filename
			if(!result)
				result = !firehose_write_data(hose, 
					filenames[i], 
					strlen(filenames[i]) + 1);

// Send date
			if(!result)
			{
				data[0] = (((uint64_t)stat_buf.st_atime) >> 56) & 0xff;
				data[1] = (((uint64_t)stat_buf.st_atime) >> 48) & 0xff;
				data[2] = (((uint64_t)stat_buf.st_atime) >> 40) & 0xff;
				data[3] = (((uint64_t)stat_buf.st_atime) >> 32) & 0xff;
				data[4] = (((uint64_t)stat_buf.st_atime) >> 24) & 0xff;
				data[5] = (((uint64_t)stat_buf.st_atime) >> 16) & 0xff;
				data[6] = (((uint64_t)stat_buf.st_atime) >> 8) & 0xff;
				data[7] = ((uint64_t)stat_buf.st_atime) & 0xff;
				result = !firehose_write_data(hose,
					data,
					8);
			}

			if(!result)
			{
				data[0] = (((uint64_t)stat_buf.st_mtime) >> 56) & 0xff;
				data[1] = (((uint64_t)stat_buf.st_mtime) >> 48) & 0xff;
				data[2] = (((uint64_t)stat_buf.st_mtime) >> 40) & 0xff;
				data[3] = (((uint64_t)stat_buf.st_mtime) >> 32) & 0xff;
				data[4] = (((uint64_t)stat_buf.st_mtime) >> 24) & 0xff;
				data[5] = (((uint64_t)stat_buf.st_mtime) >> 16) & 0xff;
				data[6] = (((uint64_t)stat_buf.st_mtime) >> 8) & 0xff;
				data[7] = ((uint64_t)stat_buf.st_mtime) & 0xff;
				result = !firehose_write_data(hose,
					data,
					8);
			}



// Send data
			while(!feof(in) && !result)
			{
				long bytes_written = fread(buffer, 1, BUFFER_SIZE, in);
//printf("main 1 %d %02x\n", bytes_written, *(buffer));
				result = !firehose_write_data(hose, buffer, bytes_written);
				status.bytes_transferred += bytes_written;
				status.total_bytes += bytes_written;
				update_status(&status);
			}
			stop_status(&status);
		}

		firehose_delete(hose);
	}


	return 0;
}




