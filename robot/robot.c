#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "parapin.h"
#include "robot.h"

#define MODEL_NUMBER "Heroine 1120"

// Delay to allow registers to take up values
//#define DELAY usleep(0);
#define DELAY usleep(1000);


#define OFF 0

// Multiplexer values

#define MULTIPLEX_0 0x0
#define MULTIPLEX_1 LP_PIN17
#define MULTIPLEX_2 LP_PIN14
#define MULTIPLEX_4 LP_PIN16
#define DATA_PINS LP_DATA_PINS
#define MULTIPLEX_PINS (LP_PIN14 | LP_PIN16 | LP_PIN17)
#define MULTIPLEX_DISABLE MULTIPLEX_4

// The registers
#define MOTOR_CHANNEL1 MULTIPLEX_2
#define MOTOR_CHANNEL2 (MULTIPLEX_2 | MULTIPLEX_1)
#define VERTICAL_MOTORS MOTOR_CHANNEL1
#define HORIZONTAL_MOTORS MOTOR_CHANNEL2

#define SWITCH_CHANNEL1 MULTIPLEX_1
#define SWITCH_CHANNEL2 MULTIPLEX_0
#define VERTICAL_SWITCHES SWITCH_CHANNEL1
#define HORIZONTAL_SWITCHES SWITCH_CHANNEL2


// Directions
#define STOP      0
#define FORWARD   1
#define BACKWARD  2
#define LEFT      FORWARD
#define RIGHT     BACKWARD
#define UP        FORWARD
#define DOWN      BACKWARD

// Sampling
#define VERTICAL_THRESHOLD 1000
#define VERTICAL_MIN 5000
#define HORIZONTAL_THRESHOLD 1000
#define SWITCH_THRESHOLD 1000

// Delays
#define BALANCE_ACTIVE_FORWARD 100000
#define BALANCE_ACTIVE_BACKWARD 100000
#define BALANCE_INACTIVE 500000
#define AUTOLEVEL_UP 20000
#define AUTOLEVEL_DOWN 10000
#define AUTOLEVEL_UP_INACTIVE 50000
#define AUTOLEVEL_DOWN_INACTIVE 100000
#define VNUDGE_ACTIVE_FORWARD 20000
#define VNUDGE_ACTIVE_BACKWARD 5000
#define VNUDGE_INACTIVE_FORWARD 100000
#define VNUDGE_INACTIVE_BACKWARD 50000
#define HNUDGE_ACTIVE 10000
#define HNUDGE_INACTIVE 50000

// The amount of counter rotation which is given to the horizontal servo to
// keep the cable from tensioning.
//#define HORIZONTAL_SLACK 1000000
#define HORIZONTAL_SLACK 500000
#define INFINITE_ROWS 65536
#define INFINITE_COLUMNS 65536

#define MIN_ROW 0
#define MAX_ROW 200

// Mask sent to parallel port to control motors
static unsigned int motor_mask1 = 0;
static unsigned int motor_mask2 = 0;
static unsigned int power_mask = 0;
static unsigned int switch_mask = 0;
static unsigned int multiplexer = -1;
static int analog1 = 0;
static int analog2 = 0;
static int analog3 = 0;


// Motor pin mask values
#define GRAB_FWD LP_PIN07
#define GRAB_BWD LP_PIN06
#define GRABBER 0

#define CRANE2_FWD LP_PIN08
#define CRANE2_BWD LP_PIN09
#define CRANE2 1

#define CRANE1_FWD LP_PIN05
#define CRANE1_BWD LP_PIN03
#define CRANE1 2

#define SLIDER_FWD LP_PIN04
#define SLIDER_BWD LP_PIN02
#define SLIDER 3

#define HORIZONTAL_FWD LP_PIN08
#define HORIZONTAL_BWD LP_PIN09
#define HORIZONTAL 4

#define FORWARD_TILT 0
#define BACKWARD_TILT 1
#define TILT_SWITCH LP_PIN04
#define TOP_SWITCH LP_PIN06
#define VERTICAL LP_PIN05
#define FRONT_SWITCH LP_PIN07
#define BACK_SWITCH LP_PIN08
#define BOTTOM_SWITCH LP_PIN09
#define LEFT_SWITCH LP_PIN09
#define RIGHT_SWITCH LP_PIN08
#define HORIZ_SWITCH LP_PIN07

// Display macros

#define MOTOR1_TO_CHAR(mask) \
((motor_mask1 & (mask)) ? '*' : ' ')

#define MOTOR2_TO_CHAR(mask) \
((motor_mask2 & (mask)) ? '*' : ' ')

#define NOTMOTOR1_TO_CHAR(mask) \
(!(motor_mask1 & (mask)) ? '*' : ' ')

#define NOTMOTOR2_TO_CHAR(mask) \
(!(motor_mask2 & (mask)) ? '*' : ' ')

#define SWITCH_TO_CHAR(mask) \
((switch_mask & (mask)) ? '*' : '0')

// Noise reduction procedures
typedef struct
{
	char *start;
	char *pointer;
	char *end;
	int threshold;
	int total;
} switch_buffer_t;

switch_buffer_t* new_switch_buffer(int threshold, int default_value)
{
	switch_buffer_t *result = calloc(1, sizeof(switch_buffer_t));
	result->threshold = threshold;
	result->start = malloc(threshold);
	memset(result->start, default_value, threshold);
	result->pointer = result->start;
	result->end = result->start + threshold;
	result->total = threshold * default_value;
	return result;
}

void delete_switch_buffer(switch_buffer_t *ptr)
{
	free(ptr->start);
	free(ptr);
}

int update_switch_buffer(switch_buffer_t *ptr, int value)
{
	ptr->total -= *ptr->pointer;
	*ptr->pointer = value;
	ptr->total += *ptr->pointer;
	ptr->pointer++;
	if(ptr->pointer >= ptr->end) ptr->pointer = ptr->start;
	return ptr->total;
}

int switch_buffer_true(switch_buffer_t *ptr)
{
	return ptr->total >= ptr->threshold;
}

int switch_buffer_false(switch_buffer_t *ptr)
{
	return ptr->total <= 0;
}

// Controlling the motors requires first, switching off an enable bit to switch
// the multiplexor to temporary motor registers.  Then switch the 
// multiplexer to the new temporary motor register and the data to the 
// new motor values.  Finally switch the enable bit to the proper motor 
// register.  The motor temporaries can override the switch registers.

static void set_motors(int channel, unsigned int values)
{
	if(multiplexer != channel)
	{
// Switch to temporary registers
		set_pin(MULTIPLEX_DISABLE);
		DELAY

// Set new channel
		clear_pin(MULTIPLEX_PINS & ~MULTIPLEX_DISABLE);
		set_pin(channel);
	}

// Set new motor register
	pin_output_mode(LP_DATA_PINS);
	if(channel == MOTOR_CHANNEL1)
		motor_mask1 = values;
	else
	if(channel == MOTOR_CHANNEL2)
		motor_mask2 = values;
	clear_pin(DATA_PINS & ~values);
	set_pin(values);
	DELAY

	if(multiplexer != channel)
	{
// Switch to real registers
		clear_pin(MULTIPLEX_DISABLE);
		DELAY
	}

	multiplexer = channel;
}

static void get_switches(int channel)
{
	if(multiplexer != channel)
	{
// Switch to temporary registers
		set_pin(MULTIPLEX_DISABLE);
		DELAY

// Set switch channel
		pin_input_mode(LP_DATA_PINS);
		clear_pin(MULTIPLEX_PINS & ~MULTIPLEX_DISABLE);
		set_pin(channel);
		DELAY

		clear_pin(MULTIPLEX_DISABLE);
		DELAY

		multiplexer = channel;
	}

// Get switches
	switch_mask = ~pin_is_set(LP_DATA_PINS);
}

static void spin_motor(int number, int direction)
{
	int channel;
	switch(number)
	{
		case SLIDER:
			motor_mask1 &= ~(SLIDER_FWD | SLIDER_BWD);
			if(direction == FORWARD)
				motor_mask1 |= SLIDER_FWD;
			else
			if(direction == BACKWARD)
				motor_mask1 |= SLIDER_BWD;
			channel = VERTICAL_MOTORS;
			break;

		case GRABBER:
			motor_mask1 &= ~(GRAB_FWD | GRAB_BWD);
			if(direction == FORWARD)
				motor_mask1 |= GRAB_FWD;
			else
			if(direction == BACKWARD)
				motor_mask1 |= GRAB_BWD;
			channel = VERTICAL_MOTORS;
			break;

		case CRANE1:
			motor_mask1 &= ~(CRANE1_FWD | CRANE1_BWD);
			if(direction == FORWARD)
				motor_mask1 |= CRANE1_FWD;
			else
			if(direction == BACKWARD)
				motor_mask1 |= CRANE1_BWD;
			channel = VERTICAL_MOTORS;
			break;

		case CRANE2:
			motor_mask1 &= ~(CRANE2_FWD | CRANE2_BWD);
			if(direction == FORWARD)
				motor_mask1 |= CRANE2_FWD;
			else
			if(direction == BACKWARD)
				motor_mask1 |= CRANE2_BWD;
			channel = VERTICAL_MOTORS;
			break;

		case HORIZONTAL:
			motor_mask2 &= ~(HORIZONTAL_FWD | HORIZONTAL_BWD);
			if(direction == FORWARD)
				motor_mask2 |= HORIZONTAL_FWD;
			else
			if(direction == BACKWARD)
				motor_mask2 |= HORIZONTAL_BWD;
			channel = HORIZONTAL_MOTORS;
			break;
	}

	if(channel == VERTICAL_MOTORS)
		set_motors(channel, motor_mask1);
	else
		set_motors(channel, motor_mask2);
}

static void power_on()
{
	if(!power_mask)
	{
		power_mask = 1;
		clear_pin(LP_PIN01);
// Capacitor charge
		usleep(500000);
// Keep motors from shorting out
		set_motors(MOTOR_CHANNEL2, 0);
		set_motors(MOTOR_CHANNEL1, 0);
// Wait for capacitor
		usleep(1000000);
	}
}

static void power_off()
{
	power_mask = 0;
	set_pin(LP_PIN01);
}

// Special handler for getting a single vertical reading.
// Averages several thousand results.
static int get_average(int channel, int value)
{
	int i;
	int total = 0;

	for(i = 0; i < SWITCH_THRESHOLD; i++)
	{
		get_switches(channel);
		total += ((switch_mask & value) == value);
	}

	return total >= SWITCH_THRESHOLD;
}


static void print_vswitch_title()
{
	printf(
"Switch:   1 2 Tilt vertical top front back bottom\n");
}

static void print_hswitch_title()
{
	printf(
"Switch:   0 1 2 3 4 horiz right left\n");
}

static void print_vswitches()
{
printf(
"          %c %c %c    %c        %c   %c     %c    %c\r",
		SWITCH_TO_CHAR(LP_PIN02),
		SWITCH_TO_CHAR(LP_PIN03),
		SWITCH_TO_CHAR(TILT_SWITCH),
		SWITCH_TO_CHAR(VERTICAL),
		SWITCH_TO_CHAR(TOP_SWITCH),
		SWITCH_TO_CHAR(FRONT_SWITCH),
		SWITCH_TO_CHAR(BACK_SWITCH),
		SWITCH_TO_CHAR(BOTTOM_SWITCH));
}

static void print_hswitches()
{
printf(
"          %c %c %c %c %c %c     %c     %c\n",
		SWITCH_TO_CHAR(LP_PIN02),
		SWITCH_TO_CHAR(LP_PIN03),
		SWITCH_TO_CHAR(LP_PIN04),
		SWITCH_TO_CHAR(LP_PIN05),
		SWITCH_TO_CHAR(LP_PIN06),
		SWITCH_TO_CHAR(HORIZ_SWITCH),
		SWITCH_TO_CHAR(RIGHT_SWITCH),
		SWITCH_TO_CHAR(LEFT_SWITCH));
}

static void poll_vswitches()
{
	print_vswitch_title();
	while(1)
	{
		get_switches(VERTICAL_SWITCHES);
		print_vswitches();
		fflush(stdout);
		usleep(100000);
	}
}

static void poll_hswitches()
{
	print_hswitch_title();
	while(1)
	{
		get_switches(HORIZONTAL_SWITCHES);
		print_hswitches();
		fflush(stdout);
		usleep(100000);
	}
}




static void slide_forward_stop()
{
	int done = 0;
	power_on();
	spin_motor(SLIDER, FORWARD);
	do
	{
		get_switches(VERTICAL_SWITCHES);
		if(switch_mask & FRONT_SWITCH) done = 1;
	}while(!done);
	spin_motor(SLIDER, STOP);
}

static void slide_backward_stop()
{
	int done = 0;
	power_on();
	spin_motor(SLIDER, BACKWARD);
	do
	{
		get_switches(VERTICAL_SWITCHES);
		if(switch_mask & BACK_SWITCH) done = 1;
	}while(!done);
	spin_motor(SLIDER, STOP);
}

static void auto_level()
{
	power_on();
	
// Tilt status
// Forward tilt -> true
	int done = 0;
	int current_tilt;
	int new_tilt;
	int i;
	get_switches(VERTICAL_SWITCHES);
	current_tilt = ((switch_mask & TILT_SWITCH) == TILT_SWITCH);

	new_tilt = !current_tilt;

	while(1)
	{
		do
		{
			if(current_tilt == FORWARD_TILT)
			{
				spin_motor(CRANE2, BACKWARD);
				usleep(AUTOLEVEL_DOWN);
			}
			else
			{
				spin_motor(CRANE2, FORWARD);
				usleep(AUTOLEVEL_UP);
			}
			set_motors(VERTICAL_MOTORS, 0);

			if(current_tilt == FORWARD_TILT)
				usleep(AUTOLEVEL_DOWN_INACTIVE);
			else
				usleep(AUTOLEVEL_UP_INACTIVE);

			get_switches(VERTICAL_SWITCHES);
			current_tilt = ((switch_mask & TILT_SWITCH) == TILT_SWITCH);
		}while(new_tilt != current_tilt);
		if(current_tilt == FORWARD_TILT) break;
		new_tilt = !current_tilt;
	}


}



// Moving quantization using every alternation as a row

static int move_crane(int total, int direction)
{
	int error = 0;


// Move finite distance and stop
	if(total)
	{
		int current_tilt;
		int current_vertical;
		int prev_vertical = get_average(VERTICAL_SWITCHES, VERTICAL);
		int current_total;
		int prev_total = 0;
		int change_count = 0;
		int true_count = 0;
		int false_count = 0;
		struct timeval tilt_start;
		struct timeval tilt_current;
		struct timeval timer_start;
		struct timeval timer_current;
		int64_t difference;
		int tilt_active = 0;
		switch_buffer_t *vertical_switch = new_switch_buffer(VERTICAL_THRESHOLD, prev_vertical);
		switch_buffer_t *top_switch = new_switch_buffer(SWITCH_THRESHOLD, 0);
		switch_buffer_t *bottom_switch = new_switch_buffer(SWITCH_THRESHOLD, 0);

int sample = 0;
#ifdef DEBUG1
FILE *debug = fopen("debug", "w");
#endif
		int min_low_duration = 0x7fffffff;
		int max_low_duration = 0x0;
		int min_high_duration = 0x7fffffff;
		int max_high_duration = 0x0;

		prev_vertical = get_average(VERTICAL_SWITCHES, VERTICAL);

		spin_motor(CRANE1, direction);
		spin_motor(CRANE2, direction);
		gettimeofday(&tilt_start, 0);

		while(1)
		{
// Accumulate values
			get_switches(VERTICAL_SWITCHES);
			update_switch_buffer(vertical_switch,
				(switch_mask & VERTICAL) ? 1 : 0);
sample++;

// Test vertical position
			if(total < INFINITE_ROWS)
			{
// Average values
				if(switch_buffer_true(vertical_switch) && !prev_vertical)
				{
					change_count++;
					prev_vertical = 1;
#ifdef DEBUG1
fprintf(debug, "%c %d %d\n", (switch_mask & VERTICAL) ? '*' : ' ', sample, change_count);
#endif
					if(sample < min_low_duration) min_low_duration = sample;
					else
					if(sample > max_low_duration) max_low_duration = sample;
					if(min_low_duration < VERTICAL_MIN)
					{
						fprintf(stderr, " *** Error: durations not long enough.\n");
						error = 1;
						break;
					}
					sample = 0;
				}
				else
				if(switch_buffer_false(vertical_switch) && prev_vertical)
				{
					change_count++;
					prev_vertical = 0;
#ifdef DEBUG1
fprintf(debug, "%c %d %d\n", (switch_mask & VERTICAL) ? '*' : ' ', sample, change_count);
#endif
					if(sample < min_high_duration) min_high_duration = sample;
					else
					if(sample > max_high_duration) max_high_duration = sample;

					if(min_high_duration < VERTICAL_MIN)
					{
						fprintf(stderr, " *** Error: durations not long enough.\n");
						error = 1;
						break;
					}
					sample = 0;
				}

// Count reached
				if(change_count >= total)
				{
					break;
				}
			}

// Update boundaries
			update_switch_buffer(top_switch, (switch_mask & TOP_SWITCH) ? 1 : 0);
			update_switch_buffer(bottom_switch, (switch_mask & BOTTOM_SWITCH) ? 1 : 0);


			if(switch_buffer_true(top_switch) && 
				switch_buffer_false(bottom_switch) &&
				direction == UP)
			{
				if(total < INFINITE_ROWS)
				{
					fprintf(stderr, " *** Error: Top boundary sensor activated during UP crane operation.\n");
					fprintf(stderr, " *** change_count=%d\n", change_count);
					error = 1;
				}
				break;
			}

			if(switch_buffer_true(bottom_switch) && 
				switch_buffer_false(top_switch) &&
				direction == DOWN)
			{
				if(total < INFINITE_ROWS)
				{
					fprintf(stderr, " *** Error: Bottom boundary sensor activated during DOWN crane operation.\n");
					fprintf(stderr, " *** change_count=%d\n", change_count);
					error = 1;
				}
				break;
			}

// Update balance
			gettimeofday(&tilt_current, 0);
			difference = 
				(int64_t)tilt_current.tv_sec * 1000000 +
				(int64_t)tilt_current.tv_usec - 
				(int64_t)tilt_start.tv_sec * 1000000 -
				(int64_t)tilt_start.tv_usec;

// Stop tilt operation
			if(tilt_active && 
				difference > (direction == UP ? BALANCE_ACTIVE_FORWARD : BALANCE_ACTIVE_BACKWARD))
			{
				spin_motor(CRANE1, direction);
				spin_motor(CRANE2, direction);

				tilt_start = tilt_current;
				tilt_active = 0;
			}
			else
// Start tilt operation
			if(!tilt_active && 
				difference > BALANCE_INACTIVE)
			{
				current_tilt = ((switch_mask & TILT_SWITCH) == TILT_SWITCH);

				if(direction == UP && current_tilt == FORWARD_TILT)
				{
					spin_motor(CRANE2, STOP);
				}
				else
/*
 * 				if(direction == UP && current_tilt == BACKWARD_TILT)
 * 				{
 * 					spin_motor(CRANE1, STOP);
 * 				}
 * 				else
 */
				if(direction == DOWN && current_tilt == BACKWARD_TILT)
				{
					spin_motor(CRANE2, STOP);
				}
				tilt_start = tilt_current;
				tilt_active = 1;
			}
		}
		set_motors(VERTICAL_MOTORS, 0);

		delete_switch_buffer(vertical_switch);
		delete_switch_buffer(top_switch);
		delete_switch_buffer(bottom_switch);

#ifdef DEBUG1
fclose(debug);
#endif
		if(total < INFINITE_ROWS)
		printf("move_crane: low = %d - %d  high = %d - %d\n",
		min_low_duration,
		max_low_duration,
		min_high_duration,
		max_high_duration);

	}
	else
// Move indefinitely
	{
		spin_motor(CRANE1, direction);
		spin_motor(CRANE2, direction);
	}
	return error;
}






static void crane_vertical(int direction)
{
	int distance = 0;

	printf("Enter distance. 0 for indefinite.  65536 for boundary: ");
	fflush(stdout);
	scanf("%d", &distance);
	
	power_on();
	move_crane(distance, direction);
}


static int pull_cd_out()
{
	power_on();

// Open grabber
	spin_motor(GRABBER, BACKWARD);


// Let capacitor settle
	usleep(500000);
	slide_forward_stop();
	set_motors(VERTICAL_MOTORS, 0);

// Tilt forward

// Didn't put enough pressure on CD.
//	spin_motor(CRANE2, FORWARD);
//	usleep(100000);

// May have pushed sled too far off the tracks to control horizontal.
//	spin_motor(CRANE1, BACKWARD);
//	usleep(100000);
//	set_motors(VERTICAL_MOTORS, 0);

	spin_motor(GRABBER, FORWARD);
// Let voltage settle
	usleep(500000);

	slide_backward_stop();

	set_motors(VERTICAL_MOTORS, 0);
	return 0;
}

static int push_cd_in()
{
	power_on();
// Open grabber
	spin_motor(GRABBER, BACKWARD);
// Let capacitor settle
	usleep(500000);

// slide forward
	slide_forward_stop();

	slide_backward_stop();
	set_motors(VERTICAL_MOTORS, 0);
	return 0;
}


static int nudge(int direction)
{
	power_on();

// Get existing color
	int prev_vertical = get_average(VERTICAL_SWITCHES, VERTICAL);
	int current_vertical;
	int done = 0;
	int overshoot_direction = BACKWARD;
	int overshoot_compensate = FORWARD;
	int current_tilt;


#define NUDGE(direction) \
done = 0; \
while(!done) \
{ \
	get_switches(VERTICAL_SWITCHES); \
	current_tilt = ((switch_mask & TILT_SWITCH) == TILT_SWITCH); \
	if(direction == UP) \
	{ \
		if(current_tilt == FORWARD_TILT) \
			spin_motor(CRANE1, direction); \
		else \
			spin_motor(CRANE2, direction); \
		usleep(VNUDGE_ACTIVE_FORWARD); \
 \
		set_motors(VERTICAL_MOTORS, 0); \
		usleep(VNUDGE_INACTIVE_FORWARD); \
	} \
	else \
	if(direction == DOWN) \
	{ \
		if(current_tilt == FORWARD_TILT) \
			spin_motor(CRANE2, direction); \
		else \
			spin_motor(CRANE1, direction); \
		usleep(VNUDGE_ACTIVE_BACKWARD); \
 \
		set_motors(VERTICAL_MOTORS, 0); \
		usleep(VNUDGE_INACTIVE_BACKWARD); \
	} \
 \
	current_vertical = get_average(VERTICAL_SWITCHES, VERTICAL); \
	if(current_vertical != prev_vertical) done = 1; \
}

// Slowly move to opposite color
	NUDGE(direction);

// Move back up because it's easier
	prev_vertical = current_vertical;
	if(direction == overshoot_direction)
	{
		NUDGE(overshoot_compensate);
	}

// Level
	auto_level();
	return 0;
}

static int goto_row(int get_it, int put_it, int row_number)
{
	int row = 0;
	int i;
	int current_vertical;
	int rising = 0;
	int falling = 0;
	int error = 0;


// Get row from user
	if(row_number < 0)
	{
		printf("Enter row number: ");
		fflush(stdout);
		scanf("%d", &row);
		if(row < MIN_ROW || row > MAX_ROW)
		{
			printf("Invalid row you Mor*on!\n");
			return;
		}
	}
	else
		row = row_number;

// Offset for crane routines
	row++;

	power_on();


// Return to bottom
	error = move_crane(INFINITE_ROWS, DOWN);


// Rise to row number
	if(row > 0 && !error)
		error = move_crane(row, UP);

	sleep(1);
// Nudge and level
	if(row > 0 && !error)
		error = nudge(DOWN);

// Now get the CD
	if(get_it && !error)
	{
		pull_cd_out();
	}

// Now put the CD
	if(put_it && !error)
	{
		push_cd_in();
	}
	return error;
}

static int move_horizontal(int total_rising, int total_falling, int direction)
{
	int error = 0;
// Get starting state
	int last_horizontal = get_average(HORIZONTAL_SWITCHES, HORIZ_SWITCH);
	int rising_count = 0;
	int falling_count = 0;
	int error_count = -1;
	switch_buffer_t *horizontal_switch = new_switch_buffer(HORIZONTAL_THRESHOLD, last_horizontal);
	switch_buffer_t *left_switch = new_switch_buffer(SWITCH_THRESHOLD, 0);
	switch_buffer_t *right_switch = new_switch_buffer(SWITCH_THRESHOLD, 0);


	if(last_horizontal)
	{
		rising_count++;
	}
	else
	{
		falling_count++;
	}

// Got it without moving
	if(total_rising && rising_count >= total_rising ||
		total_falling && falling_count >= total_falling)
	{
		return error;
	}

// Start the trip
	spin_motor(HORIZONTAL, direction);
	while(1)
	{
		get_switches(HORIZONTAL_SWITCHES);
		update_switch_buffer(horizontal_switch, (switch_mask & HORIZ_SWITCH) ? 1 : 0);
		update_switch_buffer(left_switch, (switch_mask & LEFT_SWITCH) ? 1 : 0);
		update_switch_buffer(right_switch, (switch_mask & RIGHT_SWITCH) ? 1 : 0);

/*
 * if((switch_mask & HORIZ_SWITCH))
 * printf(__FUNCTION__ " %d\n", horizontal_switch->total);
 */

// Test counters
		if(switch_buffer_true(horizontal_switch))
		{
			error_count++;
			if(!last_horizontal)
			{
				rising_count++;
				if(total_rising && rising_count >= total_rising) break;
				last_horizontal = 1;
				error_count = 0;
			}
		}
		else
		if(switch_buffer_false(horizontal_switch))
		{
			error_count++;
			if(last_horizontal)
			{
				falling_count++;
				if(total_falling && falling_count >= total_falling) break;
				last_horizontal = 0;
				error_count = 0;
			}
		}

// Test boundaries
		if(direction == LEFT && switch_buffer_true(left_switch))
		{
			if(total_rising != INFINITE_COLUMNS ||
				total_falling != INFINITE_COLUMNS)
			{
				fprintf(stderr, " *** Error: Left boundary sensor activated during left operation.\n");
				fprintf(stderr, " *** rising_count=%d falling_count=%d\n", rising_count, falling_count);
				error = 1;
			}
			break;
		}
		else
		if(direction == RIGHT && switch_buffer_true(right_switch))
		{
			if(total_rising != INFINITE_COLUMNS ||
				total_falling != INFINITE_COLUMNS)
			{
				fprintf(stderr, " *** Error: Right boundary sensor activated during right operation.\n");
				fprintf(stderr, " *** rising_count=%d falling_count=%d\n", rising_count, falling_count);
				error = 1;
			}
			break;
		}
	}
	set_motors(HORIZONTAL_MOTORS, 0);
	return error;
}

static void crane_horizontal(int direction)
{
	int distance = 0;
	printf("Enter distance or 0 for indefinite: ");
	fflush(stdout);
	scanf("%d", &distance);
	if(!distance) distance = INFINITE_COLUMNS;

	power_on();
// Make sure we're on the crane
	move_crane(INFINITE_ROWS, UP);
// Make sure vertical sensor isn't grinding on the top row
	auto_level();
// Move it
	move_horizontal(distance, 0, direction);
}

static int goto_column(int distance)
{
	int current_horizontal;
	int last_horizontal;
	int error = 0;

	if(distance < 0)
	{
		printf("Enter column: ");
		fflush(stdout);
		scanf("%d", &distance);
	}

// Offset column for motion routines
	distance++;

	power_on();

// Make sure we're on the track
	slide_backward_stop();
	move_crane(INFINITE_ROWS, UP);
	usleep(500000);
	move_crane(INFINITE_ROWS, UP);

// Make sure vertical sensor isn't grinding on the top row
	auto_level();

// Go to starting point
	move_horizontal(INFINITE_COLUMNS, INFINITE_COLUMNS, LEFT);
// Go to desired column
	move_horizontal(distance, 0, RIGHT);

// Always slips past due to tension in the string.
	spin_motor(HORIZONTAL, LEFT);

	usleep(HORIZONTAL_SLACK);
	set_motors(HORIZONTAL_MOTORS, 0);

// Slowly move left to high-low transition.
// Tension in the string is less here because of motor position.
	last_horizontal = get_average(HORIZONTAL_SWITCHES, HORIZ_SWITCH);
	while(1)
	{
		spin_motor(HORIZONTAL, LEFT);


		usleep(HNUDGE_ACTIVE);



		set_motors(HORIZONTAL_MOTORS, 0);
		usleep(HNUDGE_INACTIVE);
		current_horizontal = get_average(HORIZONTAL_SWITCHES, HORIZ_SWITCH);
//printf(__FUNCTION__ " 1 %d %d\n", current_horizontal, last_horizontal);
		if( /* last_horizontal && */ !current_horizontal)
		{
// Always slips due to tension.
			spin_motor(HORIZONTAL, RIGHT);
			usleep(HORIZONTAL_SLACK);
			set_motors(HORIZONTAL_MOTORS, 0);
			break;
		}
		if(switch_mask & LEFT_SWITCH) break;
		last_horizontal = current_horizontal;
	}
	return error;
}

static int move_cd()
{
	int row1;
	int row2;
	int error = 0;

	printf("Enter row A: ");
	fflush(stdout);
	scanf("%d", &row1);
	if(row1 < MIN_ROW || row1 > MAX_ROW)
	{
		printf("Invalid row you Mor*on!\n");
		return;
	}

	printf("Enter row B: ");
	fflush(stdout);
	scanf("%d", &row2);
	if(row2 < MIN_ROW || row2 > MAX_ROW)
	{
		printf("Invalid row you Mor*on!\n");
		return;
	}

//sleep(10);
	error = goto_row(1, 0, row1);
	if(!error) error = goto_row(0, 1, row2);
	return error;
}


static int test(int row1, int row2)
{
	int error = 0, i = 0;

	if(row1 < 0)
	{
		printf("Enter starting row: ");
		fflush(stdout);
		scanf("%d", &row1);
		if(row1 < MIN_ROW || row1 > MAX_ROW)
		{
			printf("Invalid row you Mor*on!\n");
			return;
		}


		printf("Enter ending row: ");
		fflush(stdout);
		scanf("%d", &row2);
		if(row2 < MIN_ROW || row2 > MAX_ROW)
		{
			printf("Invalid row you Mor*on!\n");
			return;
		}
	}

	power_on();

// Return to bottom
	error = move_crane(INFINITE_ROWS, DOWN);

// Rise to first row if it is greater than minimum
	if(!error && row1 > MIN_ROW)
		error = move_crane(row1 - 1, UP);

	sleep(1);
	for(i = row1; i <= row2 && !error; i++)
	{
printf("test row %d\n", i);
// Rise half a row to seat ourselves in a solid color
		spin_motor(CRANE1, UP);
		spin_motor(CRANE2, UP);
		usleep(100000);
		set_motors(VERTICAL_MOTORS, 0);

		sleep(1);
// Move crane up just one row
		error = move_crane(1, UP);

// Nudge and level
		if(!error) error = nudge(DOWN);

// Push CD in
		if(!error) error = push_cd_in();

// Pull CD out
		if(!error) error = pull_cd_out();
	}


/*
 * 	for(i = row1; i > 2 && !error; i--)
 * 	{
 * 		int src = i;
 * 		int dst = i - 2;
 * 
 * printf("test get row %d\n", src);
 * 		error = goto_row(1, 0, src);
 * 		if(!error)
 * 		{
 * printf("test put row %d\n", dst);
 * 			error = goto_row(0, 1, dst);
 * 		}
 * 	}
 */
	


	return error;
}


void run_command(int operation,
	int src_row,
	int src_column,
	int dst_row,
	int dst_column)
{
// Reset scheduler in case this is a fork
	struct sched_param params;
	params.sched_priority = 1;
	sched_setscheduler(0, SCHED_RR, &params);

	switch(operation)
	{
		case POWER_ON:
			power_on();
			break;
		case POWER_OFF:
			power_off();
			break;
		case MOVE_CD_ROW:
			if(!goto_row(1, 0, src_row))
					goto_row(0, 1, dst_row);
			power_off();
			break;
		case GET_CD_ROW:
			goto_row(1, 0, src_row);
			power_off();
			break;
		case PUT_CD_ROW:
			printf("run_command: PUT_CD_ROW dst_row=%d\n", dst_row);
			goto_row(0, 1, dst_row);
			power_off();
			break;
		case MOVE_CD:
			printf("run_command: MOVE_CD src_row=%d src_column=%d dst_row=%d dst_column=%d\n", 
				src_row,
				src_column,
				dst_row,
				dst_column);
			if(!goto_column(src_column))
			{
				if(!goto_row(1, 0, src_row))
				{
					if(src_column == dst_column)
					{
						goto_row(0, 1, dst_row);
					}
					else
					if(!goto_column(dst_column))
					{
						goto_row(0, 1, dst_row);
					}
				}
			}
			power_off();
			break;
		case GET_CD:
			printf("run_command: GET_CD src_row=%d src_column=%d\n", 
				src_row,
				src_column);
			if(!goto_column(src_column))
				goto_row(1, 0, src_row);
			power_off();
			break;
		case PUT_CD:
			printf("run_command: PUT_CD dst_row=%d dst_column=%d\n", 
				dst_row,
				dst_column);
			if(!goto_column(dst_column))
				goto_row(0, 1, dst_row);
			power_off();
			break;
		case GOTO_COLUMN:
			printf("run_command: GOTO_COLUMN dst_column=%d\n", 
				dst_column);
			goto_column(dst_column);
			power_off();
			break;
		case GOTO_ROW:
			goto_row(0, 0, dst_row);
			power_off();
			break;
		case AUTO_LEVEL:
			auto_level();
			power_off();
			break;
		case DO_TEST:
			test(src_row, dst_row);
			power_off();
			break;
		case SLIDE_FORWARD_STOP:
			slide_forward_stop();
			power_off();
			break;
		case SLIDE_BACKWARD_STOP:
			slide_backward_stop();
			power_off();
			break;
		case PULL_CD_OUT:
			pull_cd_out();
			power_off();
			break;
		case PUSH_CD_IN:
			push_cd_in();
			power_off();
			break;
	}
}


static void print_menu()
{
	printf(
"----------------------------------------------------------------------------\n"
MODEL_NUMBER " robot debugger\n"
"Motor:     slide  grab   crane1  crane2   horiz\n"
"----------------------------------------------------------------------------\n"
"Forward:   q %c    w %c    e %c     r %c     t %c\n"
"Backward:  a %c    s %c    d %c     f %c     g %c\n"
"Stop:      z %c    x %c    c %c     v %c     b %c\n"
"\n"
"1 - power on %c                2 - power off %c        3 - stop all motors\n"
"4 - poll vswitches            $ - poll hswitches\n"
"5 - slide forward and stop    6 - slide backward and stop\n"
"7 - pull CD out               8 - push CD in\n"
"9 - crane up                  0 - crane down\n"
"o - nudge up                  p - nudge down\n"
"- - auto level                = - go to row #\n"
"[ - Get CD from row @         ] - put CD into row #\n"
"l - Move CD between rows      + - go to column\n"
"n - move left                 m - move right\n"
"\\ - Test\n"
"\n",
MOTOR1_TO_CHAR(SLIDER_FWD),
MOTOR1_TO_CHAR(GRAB_FWD),
MOTOR1_TO_CHAR(CRANE1_FWD),
MOTOR1_TO_CHAR(CRANE2_FWD),
MOTOR2_TO_CHAR(HORIZONTAL_FWD),
MOTOR1_TO_CHAR(SLIDER_BWD),
MOTOR1_TO_CHAR(GRAB_BWD),
MOTOR1_TO_CHAR(CRANE1_BWD),
MOTOR1_TO_CHAR(CRANE2_BWD),
MOTOR2_TO_CHAR(HORIZONTAL_BWD),
NOTMOTOR1_TO_CHAR(SLIDER_FWD | SLIDER_BWD),
NOTMOTOR1_TO_CHAR(GRAB_FWD | GRAB_BWD),
NOTMOTOR1_TO_CHAR(CRANE1_FWD | CRANE1_BWD),
NOTMOTOR1_TO_CHAR(CRANE2_FWD | CRANE2_BWD),
NOTMOTOR2_TO_CHAR(HORIZONTAL_FWD | HORIZONTAL_BWD),
power_mask ? '*' : ' ',
power_mask ? ' ' : '*'
	);
	print_vswitch_title();
	print_vswitches();
	printf("\n");
	print_hswitch_title();
	print_hswitches();
	printf("\nCommand: ");
}



int main(int argc, char *argv[])
{
	int i;
	int operation = 0;
	int src_row = -1, src_column = -1;
	int dst_row = -1, dst_column = -1;
	int port = 500;


	struct sched_param params;
	params.sched_priority = 1;
	sched_setscheduler(0, SCHED_RR, &params);


	if(argc > 1)
	{
		for(i = 1; i < argc; i++)
		{
			if(!strcmp(argv[i], "-d"))
			{
				operation = SERVER;
				if(i < argc - 1)
				{
					port = atol(argv[i + 1]);
				}
			}
			else
			if(!strcmp(argv[i], "i"))
			{
				if(!operation)
					operation = GET_CD;
				else
					operation = MOVE_CD;
				if(i > argc - 3)
				{
					printf("No src row or column given you Mor*on!\n");
					exit(1);
				}
				src_row = atol(argv[i + 2]);
				src_column = atol(argv[i + 1]);
				i += 2;
			}
			else
			if(!strcmp(argv[i], "o"))
			{
				if(!operation)
					operation = PUT_CD;
				else
					operation = MOVE_CD;
				if(i > argc - 3)
				{
					printf("No dst row or column given you Mor*on!\n");
					exit(1);
				}
				dst_row = atol(argv[i + 2]);
				dst_column = atol(argv[i + 1]);
				i += 2;
			}
			else
			if(!strcmp(argv[i], "l"))
			{
				operation = MOVE_CD_ROW;
				if(i > argc - 3)
				{
					printf("No src or dst row given you Mor*on!\n");
					exit(1);
				}
				src_row = atol(argv[i + 1]);
				dst_row = atol(argv[i + 2]);
				i += 2;
			}
			else
			if(!strcmp(argv[i], "t"))
			{
				operation = DO_TEST;
				if(i > argc - 3)
				{
					printf("No src or dst row given you Mor*on!\n");
					exit(1);
				}
				src_row = atol(argv[i + 1]);
				dst_row = atol(argv[i + 2]);
				i += 2;
			}
			else
			if(!strcmp(argv[i], "+"))
			{
				if(i > argc - 2)
				{
					printf("No dst column given you Mor*on!\n");
					exit(1);
				}
				dst_column = atol(argv[i + 1]);
				operation = GOTO_COLUMN;
				i += 1;
			}
			else
			if(!strcmp(argv[i], "["))
			{
				if(i > argc - 2)
				{
					printf("No row given you Mor*on!\n");
					exit(1);
				}
				src_row = atol(argv[i + 1]);
				operation = GET_CD_ROW;
				i += 1;
			}
			else
			if(!strcmp(argv[i], "]"))
			{
				if(i > argc - 2)
				{
					printf("No row given you Mor*on!\n");
					exit(1);
				}
				dst_row = atol(argv[i + 1]);
				operation = PUT_CD_ROW;
				i += 1;
			}
			else
			if(!strcmp(argv[i], "="))
			{
				if(i > argc - 2)
				{
					printf("No row given you Mor*on!\n");
					exit(1);
				}
				dst_row = atol(argv[i + 1]);
				operation = GOTO_ROW;
				i += 1;
			}
			else
			if(!strcmp(argv[i], "1"))
			{
				operation = POWER_ON;
			}
			else
			if(!strcmp(argv[i], "2"))
			{
				operation = POWER_OFF;
			}
			else
			if(!strcmp(argv[i], "5"))
			{
				operation = SLIDE_FORWARD_STOP;
			}
			else
			if(!strcmp(argv[i], "6"))
			{
				operation = SLIDE_BACKWARD_STOP;
			}
			else
			if(!strcmp(argv[i], "7"))
			{
				operation = PULL_CD_OUT;
			}
			else
			if(!strcmp(argv[i], "8"))
			{
				operation = PUSH_CD_IN;
			}
			else
			if(!strcmp(argv[i], "-"))
			{
				operation = AUTO_LEVEL;
			}
			else
			if(!strcmp(argv[i], "-h"))
			{
				printf(MODEL_NUMBER " robot controller.\n");
				printf(
"robot\n"
"-d [port]     -> run in server mode using the desired port (500)\n"
"1             -> power on\n"
"2             -> power off\n"
"5             -> slide forward and stop\n"
"6             -> slide backward and stop\n"
"7             -> pull CD out\n"
"8             -> push CD in\n"
"i column row  -> get CD from slot\n"
"o column row  -> put CD in slot\n"
"+ column      -> goto column\n"
"-             -> auto level\n"
"= row         -> goto row\n"
"[ row         -> get CD from row\n"
"] row         -> put CD in row\n"
"l row1 row2   -> move CD from row1 to row2\n"
"t row1 row2   -> perform test between row1 and row2."
"Need pancakes in row1 before starting.\n"
);
				exit(0);
			}
		}
		if(operation == MOVE_CD && 
			(src_row < 0 || src_column < 0 || dst_row < 0 || dst_column < 0))
		{
			printf("No src or destination given for MOVE_CD operation.\n");
			exit(1);
		}
	}


// Initialize parallel port
  	if(pin_init_user(LPT1) < 0)
	{
		fprintf(stderr, "Failed to initialize parallel port\n");
		exit(1);
	}
	pin_input_mode(LP_DATA_PINS);
	pin_output_mode(LP_SWITCHABLE_PINS); 

	power_off();



// Perform operation if desired
	if(operation != NO_OPERATION)
	{
		switch(operation)
		{
			case SERVER:
				run_server(port);
				break;

			default:
				run_command(operation, 
					src_row,
					src_column,
					dst_row,
					dst_column);
				break;
		}
	}
	else
// Go into interactive mode
	while(1)
	{
		print_menu();
		int command = 0;
		char string[1024];

		fscanf(stdin, "%s", string);
		command = string[0];

		switch(command)
		{
			case '1': power_on();     break;
			case '2': power_off();    break;
			case '3':
				set_motors(VERTICAL_MOTORS, 0);
				set_motors(HORIZONTAL_MOTORS, 0);
				break;
			case '4': poll_vswitches(); break;
			case '$': poll_hswitches(); break;
			case '5': slide_forward_stop(); break;
			case '6': slide_backward_stop(); break;
			case '7': pull_cd_out(); break;
			case '8': push_cd_in(); break;
			case '9': crane_vertical(UP); break;
			case '0': crane_vertical(DOWN); break;
			case 'q': spin_motor(SLIDER, FORWARD);  break;
			case 'a': spin_motor(SLIDER, BACKWARD); break;
			case 'z': spin_motor(SLIDER, STOP);     break;
			case 'w': spin_motor(GRABBER, FORWARD);  break;
			case 's': spin_motor(GRABBER, BACKWARD); break;
			case 'x': spin_motor(GRABBER, STOP);     break;
			case 'e': spin_motor(CRANE1, FORWARD);  break;
			case 'd': spin_motor(CRANE1, BACKWARD); break;
			case 'c': spin_motor(CRANE1, STOP);     break;
			case 'r': spin_motor(CRANE2, FORWARD);  break;
			case 'f': spin_motor(CRANE2, BACKWARD); break;
			case 'v': spin_motor(CRANE2, STOP);     break;
			case 't': spin_motor(HORIZONTAL, FORWARD);  break;
			case 'g': spin_motor(HORIZONTAL, BACKWARD); break;
			case 'b': spin_motor(HORIZONTAL, STOP);     break;
			case '-': auto_level();                 break;
			case '=': goto_row(0, 0, -1);             break;
			case '[': goto_row(1, 0, -1);             break;
			case ']': goto_row(0, 1, -1);             break; 
			case '\\': test(-1, -1);                      break; 
			case 'o': nudge(UP);               break; 
			case 'p': nudge(DOWN);              break; 
			case 'l': move_cd();                    break;
			case 'n': crane_horizontal(LEFT);     break;
			case 'm': crane_horizontal(RIGHT);    break;
			case '+': goto_column(-1);              break;
		}
	}

	return 0;
}
