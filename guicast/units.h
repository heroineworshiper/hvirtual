/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef UNITS_H
#define UNITS_H

#include "sizes.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>


#define INFINITYGAIN -40
#define MAXGAIN 50
// total slots in frequency table
#define TOTALFREQS 1024
// slots per octave
#define OCTAVE 105
#define TOTAL_TIMEFORMATS 7

// h:mm:ss.sss
#define TIME_HMS 0
#define TIME_HMS_TEXT _("Hours:Minutes:Seconds.xxx")
#define TIME_SECONDS 8
#define TIME_SECONDS_TEXT _("Seconds")
// h:mm:ss:ff
#define TIME_HMSF 1
#define TIME_HMSF_TEXT _("Hours:Minutes:Seconds:Frames")
#define TIME_SAMPLES 2
#define TIME_SAMPLES_TEXT _("Samples")
#define TIME_SAMPLES_HEX 3
#define TIME_SAMPLES_HEX_TEXT _("Hex Samples")
#define TIME_FRAMES 4
#define TIME_FRAMES_TEXT _("Frames")
// fffff-ff
#define TIME_FEET_FRAMES 5
#define TIME_FEET_FRAMES_TEXT _("Feet-frames")


// h:mm:ss
#define TIME_HMS2 6
#define TIME_HMS2_TEXT  _("Hours:Minutes:Seconds")
// hh:mm:ss
#define TIME_HMS3 7
#define TIME_HMS3_TEXT _("Hours:Minutes:Seconds")



class DB
{
public:
	DB(float infinitygain = INFINITYGAIN);
	virtual ~DB() {};
	
// return power of db using a table
	float fromdb_table();
	float fromdb_table(float db);
// return power from db using log10
	float fromdb();
	static float fromdb(float db);

// convert db to power using a formula
	static float todb(float power);

	inline DB& operator++() { if(db < MAXGAIN) db += 0.1; return *this; };
	inline DB& operator--() { if(db > INFINITYGAIN) db -= 0.1; return *this; };
	inline DB& operator=(DB &newdb) { db = newdb.db; return *this; };
	inline DB& operator=(int newdb) { db = newdb; return *this; };
	inline int operator==(DB &newdb) { return db == newdb.db; };
	inline int operator==(int newdb) { return db == newdb; };

	static float *topower;
	float db;
	float infinitygain;
};

// Third octave frequency table
class Freq
{
public:
	Freq();
	Freq(const Freq& oldfreq);
	virtual ~Freq() {};

	static void init_table();

// set freq to index given
	static int tofreq(int index);

// return index of frequency
	int fromfreq();
	static int fromfreq(int index);

// floating point frequency conversions
    static double tofreq_f(double index);
    static double fromfreq_f(double freq);


// increment frequency by one
	Freq& operator++();
	Freq& operator--();
	
	int operator>(Freq &newfreq);
	int operator<(Freq &newfreq);
	Freq& operator=(const Freq &newfreq);
	int operator=(const int newfreq);
	int operator!=(Freq &newfreq);
	int operator==(Freq &newfreq);
	int operator==(int newfreq);

	static int *freqtable;
	int freq;
};


class Units
{
public:
	Units() {};

// No rounding.
	static float toframes(int64_t samples, int sample_rate, double framerate);
// Round up if > .5
	static int64_t toframes_round(int64_t samples, int sample_rate, double framerate);
	static double fix_framerate(double value);
	static double atoframerate(const char *text);


// Punctuate with commas
	static void punctuate(char *string);


// separator strings for BC_TextBox::set_separators
// Returns 0 if the format has no separators.
	static const char* format_to_separators(int time_format);
	static int text_to_format(const char *string);
	static const char* print_time_format(int time_format, char *string);

	static int64_t tosamples(float frames, int sample_rate, double framerate);
// give text representation as time
	static char* totext(char *text, 
				int64_t samples, 
				int time_format, 
				int samplerate, 
				double frame_rate = 0, 
				float frames_per_foot = 0);
// give text representation as time
	static char* totext(char *text, 
				double seconds, 
				int time_format, 
				int sample_rate = 0,
				double frame_rate = 0, 
				float frames_per_foot = 0);    
// convert time to samples
	static int64_t fromtext(const char *text, 
				int samplerate, 
				int time_format, 
				double frame_rate, 
				float frames_per_foot);
// Convert text to seconds
	static double text_to_seconds(const char *text, 
				int samplerate, 
				int time_format, 
				double frame_rate, 
				float frames_per_foot);   

	static float xy_to_polar(int x, int y);
	static void polar_to_xy(float angle, int radius, int &x, int &y);

// Numbers < 0 round down if next digit is < 5
// Numbers > 0 round up if next digit is > 5
	static int64_t round(double result);

// Flooring type converter rounded to nearest .001
	static int64_t to_int64(double result);

	static float quantize10(float value);
	static float quantize(float value, float precision);

	static void* int64_to_ptr(uint64_t value);
	static uint64_t ptr_to_int64(void *ptr);

// Comparisons between double seem to work more often when this is called
// on the comparison values.
	static void fix_double(double *x);
};

#endif
