#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct 
{
	int64_t cpu_user, cpu_nice, cpu_system;
	int64_t disk_in, disk_out;
	struct timeval sample_time;
} sample_t;

#define INTERVAL 5

void get_sample(sample_t *result)
{
	FILE *fd = fopen("/proc/stat", "r");
	char string[1024];
	while(fd && !feof(fd))
	{
		fgets(string, 1024, fd);
		if(!strncmp(string, "cpu  ", 5))
		{
			char *ptr = string;
			ptr = strchr(ptr, ' ');
			while(*ptr == ' ') ptr++;
			result->cpu_user = atol(ptr);
			ptr = strchr(ptr, ' ');
			while(*ptr == ' ') ptr++;
			result->cpu_nice = atol(ptr);
			ptr = strchr(ptr, ' ');
			while(*ptr == ' ') ptr++;
			result->cpu_system = atol(ptr);
		}
#if 0
		else
		if(!strncmp(string, "disk_io", 7))
		{
			char *ptr = string;
			ptr = strstr(ptr, "):(");
			ptr += 3;
// dk_drive
			ptr = strchr(ptr, ',');
			while(*ptr == ',') ptr++;
// dk_drive_rio
			ptr = strchr(ptr, ',');
			while(*ptr == ',') ptr++;
// dk_drive_rblk
			result->disk_in = atol(ptr);
			ptr = strchr(ptr, ',');
			while(*ptr == ',') ptr++;
// dk_drive_wio
			ptr = strchr(ptr, ',');
			while(*ptr == ',') ptr++;
// dk_drive_wblk
			result->disk_out = atol(ptr);
		}
#endif
	}
	if(fd) fclose(fd);


	fd = fopen("/proc/diskstats", "r");
	result->disk_in = 0;
	result->disk_out = 0;
	while(fd && !feof(fd))
	{
		fgets(string, 1024, fd);
// Number
		char *ptr = string;
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;
// Number
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;
// Device name
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;
// If device ends in a number, ignore it.
		ptr = strchr(ptr, ' ');
		ptr--;
		if(isdigit(*ptr)) continue;

// Reads issued
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;

// Reads merged
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;

// Sectors read
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;
		result->disk_in += atol(ptr) * 512;
printf("read %d\n", atol(ptr));

// ms spend reading
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;

// writes completed
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;

// sectors written
		ptr = strchr(ptr, ' ');
		while(*ptr == ' ') ptr++;
		result->disk_out += atol(ptr) * 512;
printf("written %d\n", atol(ptr));
	}
	if(fd) fclose(fd);

	gettimeofday(&result->sample_time, 0);
}

void print_difference(sample_t *next_sample, sample_t *prev_sample)
{
	sample_t result;
	result.cpu_user = next_sample->cpu_user - prev_sample->cpu_user;
	result.cpu_nice = next_sample->cpu_nice - prev_sample->cpu_nice;
	result.cpu_system = next_sample->cpu_system - prev_sample->cpu_system;
	result.disk_in = next_sample->disk_in - prev_sample->disk_in;
	result.disk_out = next_sample->disk_out - prev_sample->disk_out;

	printf("CPU: user: %lld%% nice: %lld%% system: %lld%% total: %lld%%\n",
		result.cpu_user / INTERVAL, 
		result.cpu_nice / INTERVAL,
		result.cpu_system / INTERVAL,
		(result.cpu_user + result.cpu_nice + result.cpu_system) / INTERVAL);
	printf("DISK: read: %lld K/sec write: %lld K/sec\n",
		(result.disk_in / 2) / INTERVAL,
		(result.disk_out / 2) / INTERVAL);
}


int main(int argc, char *argv[])
{
	sample_t prev_sample, next_sample;
	get_sample(&prev_sample);

	while(1)
	{
// Take sample
		sleep(INTERVAL);
		get_sample(&next_sample);
		print_difference(&next_sample, &prev_sample);
		prev_sample = next_sample;
	}
}
