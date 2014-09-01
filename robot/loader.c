#include "lib68hc11e1.h"
#include <stdlib.h>
#include <termios.h>

// Track using photodiode array


#define LOADER_PATH "loader.s19"

// Boundary positions for hard servos
#define FULL_COUNTER_HARD  0x800
#define FULL_CLOCK_HARD    0x1d00
//#define FULL_COUNTER_HARD  0x1400
//#define FULL_CLOCK_HARD	 0x2b00
#define MAX_SWING_HARD (FULL_CLOCK_HARD - FULL_COUNTER_HARD)

// Boundary conditions for soft servos
#define FULL_COUNTER_SOFT    0x600
#define FULL_CLOCK_SOFT      0x1e00
#define MAX_SWING_SOFT (FULL_CLOCK_SOFT - FULL_COUNTER_SOFT)

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

#define SENSOR1 	 IC2F
#define SENSOR2 	 IC1F
#define SENSOR3 	 IC4F
#define SENSOR4      IC3F

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))



// PWM values
int crane1 = 0;
int crane2 = 0;
int crane3 = 0;
int tracking1 = MAX_SWING_HARD / 2 + FULL_COUNTER_HARD;
int tracking2 = MAX_SWING_HARD / 2 + FULL_COUNTER_HARD;
int tracking3 = MAX_SWING_HARD / 2 + FULL_COUNTER_HARD;
int tracking4 = MAX_SWING_HARD / 2 + FULL_COUNTER_HARD;

// Magnitudes of analog waveforms
unsigned char mag1 = 0;
unsigned char mag2 = 0;
unsigned char mag3 = 0;
unsigned char mag4 = 0;

void usage()
{

	printf(
"-------------------------------------------------------------------------------\n"
"Heroine 1130 interactive\n"
"\n"
"q%c w%c e%c        -> crane 1\n"
"a%c s%c d%c        -> crane 2\n"
"z%c x%c c%c        -> crane 3\n"
"r t y u i  %04x       -> vertical 1 tracking\n"
"f g h j k  %04x       -> horizontal 1 tracking\n"
"R T Y U I  %04x       -> vertical 2 tracking\n"
"F G H J K  %04x       -> horizontal 2 tracking\n",
(crane1 == FULL_COUNTER_SOFT) ? '*' : ' ',
(crane1 == 0x0) ? '*' : ' ',
(crane1 == FULL_CLOCK_SOFT) ? '*' : ' ',
(crane2 == FULL_CLOCK_SOFT) ? '*' : ' ',
(crane2 == 0x0) ? '*' : ' ',
(crane2 == FULL_COUNTER_SOFT) ? '*' : ' ',
(crane3 == FULL_COUNTER_SOFT) ? '*' : ' ',
(crane3 == 0x0) ? '*' : ' ',
(crane3 == FULL_CLOCK_SOFT) ? '*' : ' ',
tracking1,
tracking2,
tracking3,
tracking4
	);


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


static void send_registers(FILE *fd)
{
	fputw(tracking1, fd);
//	printf("%04x ", fgetw(fd));
	fputw(tracking2, fd);
//	printf("%04x ", fgetw(fd));
	fputw(tracking3, fd);
//	printf("%04x ", fgetw(fd));
	fputw(tracking4, fd);
//	printf("%04x ", fgetw(fd));
	fputw(crane1, fd);
//	printf("%04x ", fgetw(fd));
	fputw(crane2, fd);
//	printf("%04x ", fgetw(fd));
	fputw(crane3, fd);
//	printf("%04x\n", fgetw(fd));
}


void do_servo_change(int *result, char *keys, int c, int do_soft)
{
	int min, max;

	if(do_soft)
	{
		if(c == keys[0]) (*result) = FULL_COUNTER_SOFT;
		else
		if(c == keys[1]) (*result) = 0x0;
		else
		if(c == keys[2]) (*result) = FULL_CLOCK_SOFT;
//printf("%c %s %x\n", c, keys, (*result));
	}
	else
	{
		min = FULL_COUNTER_HARD;
		max = FULL_CLOCK_HARD;

		if(c == keys[0]) (*result) -= 0x80;
		else
		if(c == keys[1]) (*result) -= 0x1;
		else
		if(c == keys[2]) (*result) = (min + max) / 2;
		else
		if(c == keys[3]) (*result) += 0x1;
		else
		if(c == keys[4]) (*result) += 0x80;

		CLAMP((*result), min, max);
	}


}


// Interactive program
int main(int argc, char *argv[])
{
// program to be burned in or loaded into RAM
	unsigned char *program = malloc(RAM_SIZE);
	int program_size;
	int result = 0;
	FILE *serial_fd = 0;
	int i;
	struct termios info;
	int do_usage = 1;

	tcgetattr(fileno(stdin), &info);
	info.c_lflag &= ~ICANON;
	info.c_lflag &= ~ECHO;
	tcsetattr(fileno(stdin), TCSANOW, &info);

// Start program
	if(!read_s19(program, &program_size, LOADER_PATH))
	{
		if((serial_fd = run_68hc11e1(program, program_size, 1)))
		{
			send_registers(serial_fd);
			while(1)
			{
				fd_set rfds;

				if(do_usage)
				{
					usage();
					do_usage = 0;
				}

				FD_ZERO(&rfds);
				FD_SET(fileno(serial_fd), &rfds);
				FD_SET(fileno(stdin), &rfds);
				int result = select(fileno(serial_fd) + 1,
					&rfds,
					0,
					0,
					0);

				if(FD_ISSET(fileno(serial_fd), &rfds))
				{
//					printf("%c", fgetc(serial_fd));
fgetc(serial_fd);
				}

				if(FD_ISSET(fileno(stdin), &rfds))
				{
					int c = fgetc(stdin);
					do_servo_change(&crane1, "qwe", c, 1);
					do_servo_change(&crane2, "dsa", c, 1);
					do_servo_change(&crane3, "zxc", c, 1);
					do_servo_change(&tracking1, "rtyui", c, 0);
					do_servo_change(&tracking2, "fghjk;", c, 0);
					do_servo_change(&tracking3, "RTYUI", c, 0);
					do_servo_change(&tracking4, "FGHJK;", c, 0);


					send_registers(serial_fd);
					do_usage = 1;
				}
			}
		}
	}
}

