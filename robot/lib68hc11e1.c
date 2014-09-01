#include "lib68hc11e1.h"



#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <linux/serial.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

// The MC68HC11 series varies widely.
// For the MC68HC11E1 you've got 512 bytes of RAM.


// With the MC68HC11E1 and 12Mhz oscillator, these speeds are achieved.
#define SLOW_SPEED 1800
#define FAST_SPEED 23040




// ****************************************************************************
// Functions for interpreting analog data
// ****************************************************************************


limit_buffer_t* new_limit_buffer(int size)
{
	limit_buffer_t *result = calloc(1, sizeof(limit_buffer_t));
	result->size = size;
	result->start = calloc(1, size);
	result->pointer = result->start;
	result->end = result->start + size;
	return result;
}

void delete_limit_buffer(limit_buffer_t *ptr)
{
	free(ptr->start);
	free(ptr);
}

void update_limit_buffer(limit_buffer_t *ptr, unsigned char value)
{
	int need_new_max = 0;
	int need_new_min = 0;
	int i;
	if(*ptr->pointer == ptr->max) need_new_max = 1;
	if(*ptr->pointer == ptr->min) need_new_min = 1;

	*ptr->pointer = value;
	if(need_new_max)
	{
		ptr->max = *ptr->start;
		for(i = 1; i < ptr->size; i++)
		{
			if(ptr->start[i] > ptr->max) ptr->max = ptr->start[i];
		}
	}
	else
	if(value > ptr->max) 
		ptr->max = value;

	if(need_new_min)
	{
		ptr->min = *ptr->start;
		for(i = 1; i < ptr->size; i++)
		{
			if(ptr->start[i] < ptr->min) ptr->min = ptr->start[i];
		}
	}
	else
	if(value < ptr->min) 
		ptr->min = value;

	ptr->pointer++;
	if(ptr->pointer >= ptr->end) ptr->pointer = ptr->start;
}

unsigned char limit_buffer_min(limit_buffer_t *ptr)
{
	return ptr->min;
}

unsigned char limit_buffer_max(limit_buffer_t *ptr)
{
	return ptr->max;
}









// ****************************************************************************
// Functions for interfacing the serial port
// ****************************************************************************


void fputw(int data, FILE *fd)
{
	fputc(data >> 8, fd);
	fputc(data & 0xff, fd);
}

int fgetw(FILE *fd)
{
	int result = 0;
	result = fgetc(fd) << 8;
	result |= fgetc(fd);
	return result;
}

















//  Set the serial port speed to slow and initialize the hardware
static int init_serial(int baud_rate, char *device)
{
	struct termios term;
	struct serial_struct serial_struct;
	int file;
	
	file = open(device, O_RDWR | O_NOCTTY | O_SYNC);
	if (file == -1)
	{
		perror("init_serial");
		return 1;
	}
	
	tcflush(file, TCIOFLUSH);
	
	if (tcgetattr(file, &term))
	{
		perror("init_serial: tcgetattr");
		close(file);
		return 1;
	}

// Set kernel to custom baud and low latency
	if(ioctl(file, TIOCGSERIAL, &serial_struct) < 0)
	{
		perror("init_serial: TIOCGSERIAL");
		close(file);
		return 1;
	}

	serial_struct.flags |= ASYNC_LOW_LATENCY;
	serial_struct.flags |= ASYNC_SPD_CUST;
	serial_struct.custom_divisor = (int)((float)serial_struct.baud_base / (float)baud_rate + 0.5);


	if(ioctl(file, TIOCSSERIAL, &serial_struct) < 0)
	{
		perror("init_serial: TIOCSSERIAL");
		close(file);
		return 1;
	}

// Set C library to custom baud
	cfsetispeed(&term, B38400);
	cfsetospeed(&term, B38400);

// Set the special sauce which gets this microprocessor to work.
// Derived from the settings minicom leaves after it quits.
	term.c_iflag = IGNBRK;
	term.c_oflag = 0;
// Disable echo so next time it's opened, it doesn't go into flash.
// Disable canonincal mode so data isn't buffered by lines.
	term.c_lflag = 0;

// Cause control lines to go negative when the file is closed.
// This creates the reset condition.
	term.c_cflag |= HUPCL;


	if (tcsetattr(file, TCSADRAIN, &term))
	{
		perror("init_serial: tcsetattr");
		close(file);
		return 1;
	}
	
	close(file);
	usleep(10000);
	return 0;
}


static int set_speed(int speed, FILE *fd)
{
	struct termios term;
	struct serial_struct serial_struct;

	int fd_num = fileno(fd);

	if(tcgetattr(fd_num, &term))
	{
		perror("set_speed: tcgetattr");
		return 1;
	}

// Set kernel to custom baud
	if(ioctl(fd_num, TIOCGSERIAL, &serial_struct))
	{
		perror("set_speed: TIOCGSERIAL");
		return 1;
	}

	serial_struct.flags |= ASYNC_SPD_CUST;
	serial_struct.custom_divisor = (int)((float)serial_struct.baud_base / (float)speed + 0.5);

	if(ioctl(fd_num, TIOCSSERIAL, &serial_struct))
	{
		perror("set_speed: TIOCSSERIAL");
		return 1;
	}

// Set C library to custom baud
	cfsetispeed(&term, B38400);
	cfsetospeed(&term, B38400);
	if(tcsetattr(fd_num, TCSADRAIN, &term))
	{
		perror("set_speed: tcsetattr");
		return 1;
	}
	return 0;
}




// Load the binary file and run it.
// address is the size of the binary.
// If debug is 1 it dumps the serial port to the console.
// If debug is 0 it returns the serial port file descriptor for your use.
// On failure it always returns 0.
FILE* run_68hc11e1(char *serial_path, unsigned char *prog, int size, int debug)
{
	unsigned char c, ret, test[RAM_SIZE];
	int i, count, error;
	unsigned int num;
	FILE *fd;

// Initialize serial port
	error = init_serial(1800, serial_path);
	if(error) return 0;

// Open serial port
	if ((fd = fopen(serial_path, "r+")) == NULL)
	{
		fprintf(stderr, "run_68hc11e1: fopen %s: %s", serial_path, strerror(errno));
		return 0;
	}


// Need time to come out of reset state
	usleep(10000);


	if(debug) fprintf(stderr, "run_68hc11e1: uploading %d bytes\n", size);

// Now in special bootstrap mode, we upload the serial speed calibrator
	fputc(0xff, fd);

	usleep(10000);

// Upload the executable in one shot.
	fwrite(prog, 1, size, fd);

// program begins executing as soon as there is a 4 character delay
 	usleep(10000); 

	if(debug) fprintf(stderr, "run_68hc11e1: verifying\n");

// Verify data integrity
	fgetc(fd);
	fread(test, 1, size, fd);

	for(count = 0; count < size; count++)
	{
//printf("%02x %02x\n", prog[count], test[count]);
		if(prog[count] != test[count])
		{
			fprintf(stderr, "run_68hc11e1: Returned program and uploaded program differ at byte %x.\n", count);
			fclose(fd);
			return 0;
		}
	}


// Change baud rate.  The possible speeds are in registers.s
	set_speed(FAST_SPEED, fd);
	setvbuf(fd, 0, _IONBF, 0);

	sleep(1);
	return fd;
}

void do_serial(FILE *fd, char *serial_path)
{
	fd_set rfds;
	struct termios info;

	tcgetattr(fileno(stdin), &info);
	info.c_lflag &= ~ICANON;
	info.c_lflag &= ~ECHO;
	tcsetattr(fileno(stdin), TCSANOW, &info);

	fprintf(stderr, "do_serial: connecting to serial port %s.\n", serial_path);

// Read from serial port if desired
	while(1)
	{
		FD_ZERO(&rfds);
		FD_SET(fileno(fd), &rfds);
		FD_SET(fileno(stdin), &rfds);
		int result = select(fileno(fd) + 1,
			&rfds,
			0,
			0,
			0);

		if(FD_ISSET(fileno(fd), &rfds))
		{
			printf("%c", fgetc(fd));
			fflush(stdout);
		}

		if(FD_ISSET(fileno(stdin), &rfds))
		{
			fputc(fgetc(stdin), fd);
			fflush(fd);
		}
	}
}

// Convert S19 file into binary
int read_s19(char *prog, int *address, char *path)
{
	FILE *fd = fopen(path, "r");
	if(fd)
	{
#define BUFFER 0x100000
		char *s19_data = calloc(BUFFER, 1);
		int s19_size = fread(s19_data, 1, BUFFER, fd);
		int i, c;
		int start_address = -1;
		fclose(fd);

// Set memory image to full 0xff since the S19 file may contain gaps.
		memset(prog, 0xff, RAM_SIZE);

// Convert S19 text to binary
		for(i = 0; i < s19_size; )
		{
			c = s19_data[i++];
			if(c == 'S')
			{
				c = s19_data[i++];
				if(c == '1')
				{
					int num, count;
// Use the address offset to allow gaps, but offset the offset so the data
// starts inside our buffer.
					sscanf(&s19_data[i], "%2x%4x", &num, address);
					if(start_address < 0) start_address = *address;
					i += 6;

					for(count = 0; count < num - 3; count++)
					{
						sscanf(&s19_data[i], "%2x", &prog[*address - start_address]);
						(*address)++;
						i += 2;
					}
				}
			}
		}
		*address -= start_address;
		free(s19_data);
		return 0;
	}
	else
	{
		fprintf(stderr, "read_s19 %s: %s\n", path, strerror(errno));
		return 1;
	}
}

