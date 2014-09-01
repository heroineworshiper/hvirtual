#include "lib68hc11e1.h"
#include <stdlib.h>

// Track using photodiode array


#define TRACKER_PATH "tracker2.s19"
#define DEVICE "/dev/ttyS0"


// Boundary positions
#define FULL_COUNTER  0x800
#define FULL_CLOCK    0x1d00
#define MAX_SWING (FULL_CLOCK - FULL_COUNTER)

#define MAX_X_VECTOR  0x200
#define MIN_X_VECTOR  0x0
#define INIT_X_VECTOR 0x10

// This depends on distance from servo, probably measurable by second laser.
#define MAX_Y_VECTOR  0x10
#define MIN_Y_VECTOR  0x0
#define INIT_Y_VECTOR 0x4

// Commands
#define SET_SERVOS   0x01
#define GET_DETECTOR 0x10

// Detector bits
#define IC1F         0x4
#define IC2F         0x2
#define IC3F         0x1
#define IC4F         0x8

#define SENSOR1 	 IC2F
#define SENSOR2 	 IC1F
#define SENSOR3 	 IC4F
#define SENSOR4      IC3F

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))





// Delay times for various motion lengths in microseconds.
// Need to be empirically discovered.
// { fraction of MAX_SWING, delay }
static int delays[][2] = 
{
	{ 1, 500000 },
	{ 2, 500000 },
	{ 4, 300000 },
	{ 8, 200000 },
	{ 16, 200000 },
	{ 32, 100000 },
	{ 64, 100000 }
};

static int total_delays = sizeof(delays) / sizeof(int[2]);

static void do_delay(int distance)
{
	int i;
	if(!distance) return;

/*
 * usleep(1000000);
 * return;
 */

	for(i = total_delays - 1; i >= 0; i--)
	{
		if(MAX_SWING / delays[i][0] >= distance)
		{
			usleep(delays[i][1]);
			return;
		}
	}
	return;
}

static void send_swing(FILE *fd, 
	int *current_x, 
	int *current_y,
	int new_x,
	int new_y,
	int *swing_status)
{
	int distance_x = labs(*current_x - new_x);
	int distance_y = labs(*current_y - new_y);

	if(*current_x < 0) distance_x = MAX_SWING;
	if(*current_y < 0) distance_y = MAX_SWING;

// Distance used by the delay
	int max_distance = MAX(distance_x, distance_y);
	int error = 0;


//printf("send_swing 1\n");
// Only send command if a distance exists
	if(max_distance)
	{
		fputc(SET_SERVOS, fd);

// Horizontal
		fputw(new_y, fd);
		fputw(new_x, fd);

		do_delay(max_distance);
		*current_x = new_x;
		*current_y = new_y;
	}
	else
	{
// Keep from locking up the PWM
		usleep(10000);
	}


	fputc(GET_DETECTOR, fd);

// Get detector status
	*swing_status = fgetc(fd);
//printf("send_swing 10\n");
}

static void do_tracking(FILE *fd)
{
	int current_x = -1;
	int current_y = -1;
// Initial search vector for center
	int size_x = MAX_X_VECTOR;
	int size_y = MIN_Y_VECTOR;
	int next_x = (FULL_COUNTER + FULL_CLOCK) / 2 - size_x;
	int next_y = (FULL_COUNTER + FULL_CLOCK) / 2 - size_y;
	int x_vector = 1;
	int y_vector = 1;

	int prev_result = -1;
	int swing_status = 0;

	fprintf(stderr, "do_tracking: doing angle search\n");

// Go to initial position
	send_swing(fd, 
		&current_x, 
		&current_y, 
		next_x, 
		next_y,
		&swing_status);

// X vectors
// 0  <-------------
// 1  ------>
// 2         ------>

// Y vectors
// 0  n    1  |
//   /|\     \|/
//    |       v

// Sensors:
// Sensor 1    |
// Sensor 2    |
// Sensor 3   \|/
// Sensor 4    v

	while(1)
	{
		switch(x_vector)
		{
			case 0:
				x_vector = 1;
				next_x = current_x + size_x;
				break;

			case 1:
				if(swing_status)
				{
					x_vector = 0;
					next_x = current_x - size_x;
					size_x >>= 1;
				}
				else
				{
					x_vector = 2;
					next_x = current_x + size_x;
				}
				break;

			case 2:
				if(swing_status)
				{
					x_vector = 0;
					next_x = current_x - size_x;
					size_x >>= 1;
				}
				else
				{
					x_vector = 0;
					next_x = current_x - size_x;
					if(size_x)
						size_x <<= 1;
					else
						size_x = INIT_X_VECTOR;
					size_x = MIN(size_x, MAX_X_VECTOR);
					next_x -= size_x;
				}
				break;
		}

		switch(y_vector)
		{
			case 0:
				if(swing_status & (SENSOR1 | SENSOR2))
				{
					next_y += INIT_Y_VECTOR;
					y_vector = 1;
				}
				else
				if(swing_status & (SENSOR3 | SENSOR4))
				{
					next_y -= INIT_Y_VECTOR;
					y_vector = 0;
				}
				else
				{
					;
				}
				break;

			case 1:
				if(swing_status & (SENSOR3 | SENSOR4))
				{
					next_y -= INIT_Y_VECTOR;
					y_vector = 0;
				}
				else
				if(swing_status & (SENSOR1 | SENSOR2))
				{
					next_y += INIT_Y_VECTOR;
					y_vector = 1;
				}
				else
				{
					;
				}
				break;
		}

		CLAMP(next_x, FULL_COUNTER, FULL_CLOCK);
		CLAMP(next_y, FULL_COUNTER, FULL_CLOCK);


// Test vector
		int before_x = current_x;
		int before_y = current_y;

		send_swing(fd, 
			&current_x, 
			&current_y, 
			next_x, 
			next_y,
			&swing_status);
if(swing_status)
{
if(current_x > next_x)
	printf("%d,%d <- %d,%d  ", next_x, next_y, before_x, before_y);
else
	printf("%d,%d -> %d,%d  ", before_x, before_y, next_x, next_y);
if((swing_status & SENSOR1)) printf("sensor1 %d ", size_y);
else
if((swing_status & SENSOR2)) printf("sensor2 %d ", size_y);
else
if((swing_status & SENSOR3)) printf("sensor3 %d ", size_y);
else
if((swing_status & SENSOR4)) printf("sensor4 %d ", size_y);
else
printf("%x ", swing_status);
if(swing_status) printf("horiz %d", size_x);
printf("\n");
}
	}
}


#define LIMIT_BUFFER_SIZE 32
static void do_adc(FILE *fd)
{
	int i = 0;
	limit_buffer_t *buffer1 = new_limit_buffer(LIMIT_BUFFER_SIZE);
	limit_buffer_t *buffer2 = new_limit_buffer(LIMIT_BUFFER_SIZE);
	limit_buffer_t *buffer3 = new_limit_buffer(LIMIT_BUFFER_SIZE);
	limit_buffer_t *buffer4 = new_limit_buffer(LIMIT_BUFFER_SIZE);
	int dump = 0;

	while(1)
	{
		unsigned char data[4];
		fread(data, 4, 1,fd);
		update_limit_buffer(buffer1, data[0]);
		update_limit_buffer(buffer2, data[1]);
		update_limit_buffer(buffer3, data[2]);
		update_limit_buffer(buffer4, data[3]);

		

		if(dump)
		{
			fwrite(data, 4, 1, stdout);
		}
		else
		{
			unsigned char mag1 = limit_buffer_max(buffer1) - limit_buffer_min(buffer1);
			unsigned char mag2 = limit_buffer_max(buffer2) - limit_buffer_min(buffer2);
			unsigned char mag3 = limit_buffer_max(buffer3) - limit_buffer_min(buffer3);
			unsigned char mag4 = limit_buffer_max(buffer4) - limit_buffer_min(buffer4);
			int sensor;
			unsigned char sensor_mag;

			if(mag2 > mag1)
			{
				sensor = 2;
				sensor_mag = mag2;
			}
			else
			{
				sensor = 1;
				sensor_mag = mag1;
			}

			if(mag3 > sensor_mag)
			{
				sensor = 3;
				sensor_mag = mag3;
			}

			if(mag4 > sensor_mag)
			{
				sensor = 4;
				sensor_mag = mag4;
			}

			if(sensor_mag > 100)
			{
				printf("|%s|%s|%s|%s|     %4d %4d %4d %4d\n", 
					sensor == 1 ? "*" : " ",
					sensor == 2 ? "*" : " ",
					sensor == 3 ? "*" : " ",
					sensor == 4 ? "*" : " ",
					mag1,
					mag2,
					mag3,
					mag4);
			}
		}

//		printf("%4d %4d %4d %4d\n", mag1, mag2, mag3, mag4);
	}
}


// Test program
int main(int argc, char *argv[])
{
// User supplied program to either run or burn in.
	char *s19_path = TRACKER_PATH;
	char *serial_path = DEVICE;
// program to be burned in or loaded into RAM
	unsigned char *program = malloc(RAM_SIZE);
	int program_size;
	int result = 0;
	FILE *serial_fd = 0;
// operations
	int burn_eeprom = 0;
	int special_bootstrap = 0;
	int just_serial = 0;
	int baud_rate = 0;
	int i;
	int do_tracker = 1;
	int do_adc_ = 0;

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-h"))
		{
			fprintf(stderr, 
				"Usage: %s === upload and run tracker\n" 
				"       %s <path> === upload program and link serial port\n" 
				"       %s -c <device path> === use specified device as serial port\n", 
				argv[0],
				argv[0],
				argv[0]);
			exit(1);
		}
		else
		{
			if(!strcmp(argv[i], "-c"))
			{
				if(i < argc - 1)
				{
					i++;
					serial_path = argv[i];
				}
				else
				{
					fprintf(stderr, 
						"-c needs a serial port device of the form /dev/ttyS0.\n");
					exit(1);
				}
			}
			else
			if(!strcmp(argv[i], "-a"))
			{
				do_adc_ = 1;
				i++;
			}

			if(i < argc)
			{
				do_tracker = 0;
				s19_path = argv[i];
			}
		}
	}

	result = read_s19(program, &program_size, s19_path);
	if(!result)
		serial_fd = run_68hc11e1(serial_path, program, program_size, 1);
	if(serial_fd)
	{
		if(do_tracker)
		{
			do_tracking(serial_fd);
		}
		else
		if(do_adc_)
		{
			do_adc(serial_fd);
		}
		else
		{
			do_serial(serial_fd, serial_path);
		}
	}
}

