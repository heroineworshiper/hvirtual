#include <stdio.h>
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

#define DEVICE "/dev/ttyS0"
#define RAM_SIZE 512

// With the MC68HC11E1 and 12Mhz oscillator, these speeds are achieved.
#define SLOW_SPEED 1800
#define FAST_SPEED 23040



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
FILE* run_68hc11e1(unsigned char *prog, int size, int debug)
{
	unsigned char c, ret, test[RAM_SIZE];
	int i, count, error;
	unsigned int num;
	FILE *fd;

// Initialize serial port
	error = init_serial(1800, DEVICE);
	if(error) return 0;

// Open serial port
	if ((fd = fopen(DEVICE, "r+")) == NULL)
	{
		perror("run_68hc11e1: fopen" DEVICE);
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

static void do_serial(FILE *fd)
{
	fd_set rfds;
	struct termios info;

	tcgetattr(fileno(stdin), &info);
	info.c_lflag &= ~ICANON;
	info.c_lflag &= ~ECHO;
	tcsetattr(fileno(stdin), TCSANOW, &info);

	fprintf(stderr, "do_serial: connecting to serial port.\n");

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
		else
		if(FD_ISSET(fileno(stdin), &rfds))
		{
			fputc(fgetc(stdin), fd);
			fflush(fd);
		}
	}
}

// Convert S19 file into binary
static int read_s19(char *prog, int *address, char *path)
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

static void fputw(int data, FILE *fd)
{
	fputc(data >> 8, fd);
	fputc(data & 0xff, fd);
}

static int fgetw(FILE *fd)
{
	int result = 0;
	result = fgetc(fd) << 8;
	result |= fgetc(fd);
	return result;
}






#if 1



// Boundary positions
#define FULL_COUNTER 0x800
#define FULL_CLOCK   0x1d00
#define MAX_SWING (FULL_CLOCK - FULL_COUNTER)

#define MAX_VECTOR 0x200
#define MIN_VECTOR 0x1
#define MAX_Y_VECTOR 0x10

// Commands
#define SET_SERVOS  0x01
#define GET_DETECTOR 0x10

// Detector bits
#define IC1F         0x4

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
	{ 4, 400000 },
	{ 8, 300000 },
	{ 16, 200000 },
	{ 32, 100000 },
	{ 64, 100000 }
};
static int total_delays = sizeof(delays) / sizeof(int[2]);

static void do_delay(int distance)
{
	int i;
	if(!distance) return;
//usleep(1000000);
//return;
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

static int send_swing(FILE *fd, 
	int *current_x, 
	int *current_y,
	int new_x,
	int new_y)
{
	int distance_x = labs(*current_x - new_x);
	int distance_y = labs(*current_y - new_y);

	if(*current_x < 0) distance_x = MAX_SWING;
	if(*current_y < 0) distance_y = MAX_SWING;

// Distance used by the delay
	int max_distance = MAX(distance_x, distance_y);
	int error = 0;

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
	int result = fgetc(fd);
	return result;
}

static void angle_search(FILE *fd)
{
	int current_x = -1;
	int current_y = -1;
// Initial search vector for center
	int size_x = MAX_VECTOR;
	int size_y = MIN_VECTOR;
	int next_x = (FULL_COUNTER + FULL_CLOCK) / 2 - size_x / 2;
	int next_y = (FULL_COUNTER + FULL_CLOCK) / 2 - size_y / 2;
	int x_vector = 1;
	int y_vector = 1;

	int prev_result = -1;
	int result = 0;

	fprintf(stderr, "angle_search: doing angle search\n");

// Initial position
	send_swing(fd, &current_x, &current_y, next_x, next_y);

	while(1)
	{
// Get next vector
		if(!result)
		{
// didn't get a signal
			switch(x_vector)
			{
				case 0:
					size_x <<= 1;
					x_vector = 1;
					break;
				case 1:
					size_x <<= 1;
					x_vector = 0;
					break;
				case 2:
					x_vector = 0;
					break;
				case 3:
					x_vector = 1;
					break;
			}

			switch(y_vector)
			{
				case 0:
					size_y <<= 1;
					y_vector = 1;
					break;
				case 1:
					size_y <<= 1;
					y_vector = 0;
					break;
				case 2:
					y_vector = 0;
					break;
				case 3:
					y_vector = 1;
					break;
			}
		}
		else
		{
// Got a signal
			switch(x_vector)
			{
				case 0:
					x_vector = 3;
					break;
				case 1:
					x_vector = 2;
					break;
				case 2:
					x_vector = 3;
					break;
				case 3:
					x_vector = 2;
					break;
			}
			size_x >>= 1;
			switch(y_vector)
			{
				case 0:
					y_vector = 3;
					break;
				case 1:
					y_vector = 2;
					break;
				case 2:
					y_vector = 3;
					break;
				case 3:
					y_vector = 2;
					break;
			}
			size_y >>= 1;
		}

// Update coords based on vector and size
		size_x = MIN(size_x, MAX_VECTOR);
		size_x = MAX(size_x, MIN_VECTOR);
		size_y = MIN(size_y, MAX_Y_VECTOR);
		size_y = MAX(size_y, MIN_VECTOR);


		switch(x_vector)
		{
			case 0:
			case 2:
				next_x = current_x + size_x;
				break;
			case 1:
			case 3:
				next_x = current_x - size_x;
				break;
		}
		CLAMP(next_x, FULL_COUNTER, FULL_CLOCK);

		switch(y_vector)
		{
			case 0:
			case 2:
				next_y = current_y + size_y;
				break;
			case 1:
			case 3:
				next_y = current_y - size_y;
				break;
		}
		CLAMP(next_y, FULL_COUNTER, FULL_CLOCK);


// Test vector
if(current_x > next_x)
	printf("%d,%d <- %d,%d  ", next_x, next_y, current_x, current_y);
else
	printf("%d,%d -> %d,%d  ", current_x, current_y, next_x, next_y);
fflush(stdout);
		result = send_swing(fd, &current_x, &current_y, next_x, next_y);
printf("%s\n", result ? "got it" : "");
	}
}






// Test program
int main(int argc, char *argv[])
{
// User supplied program to either run or burn in.
	char *s19_path;
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
	int do_angle = 0;

	if(argc < 2)
	{
		fprintf(stderr, 
			"Usage: %s <S19 file> === upload and run the program\n" 
			"       %s -a <S19 file> === upload and run angle front end\n", 
			argv[0],
			argv[0]);
		exit(1);
	}

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-a"))
		{
			do_angle = 1;
		}
		else
			s19_path = argv[i];
	}

	result = read_s19(program, &program_size, s19_path);
	if(!result)
		serial_fd = run_68hc11e1(program, program_size, 1);
	if(serial_fd)
	{
		if(do_angle)
		{
			angle_search(serial_fd);
		}
		else
		{
			do_serial(serial_fd);
		}
	}
}

#endif


