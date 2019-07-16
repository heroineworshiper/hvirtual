
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "bcwindowbase.inc"
#include "units.h"
#include "language.h"

#include <stdlib.h>
#include <string.h>

float* DB::topower = 0;
int* Freq::freqtable = 0;


DB::DB(float infinitygain)
{
	this->infinitygain = infinitygain;
	if(!topower)
	{
		int i;
		float value;

		// db to power table
		topower = new float[(MAXGAIN - INFINITYGAIN) * 10 + 1];
		topower += -INFINITYGAIN * 10;
		for(i = INFINITYGAIN * 10; i <= MAXGAIN * 10; i++)
		{
			topower[i] = pow(10, (float)i / 10 / 20);
			
//printf("%f %f\n", (float)i/10, topower[i]);
		}
		topower[INFINITYGAIN * 10] = 0;   // infinity gain
	}
	db = 0;
}

float DB::fromdb_table() 
{ 
	return db = topower[(int)(db * 10)]; 
}

float DB::fromdb_table(float db) 
{ 
	if(db > MAXGAIN) db = MAXGAIN;
	if(db <= INFINITYGAIN) return 0;
	return db = topower[(int)(db * 10)]; 
}

float DB::fromdb()
{
	return pow(10, db / 20);
}

float DB::fromdb(float db)
{
	return pow(10, db / 20);
}

// set db to the power given using a formula
float DB::todb(float power)
{
	float db;
	if(power == 0) 
		db = -100;
	else 
	{
		db = (float)(20 * log10(power));
		if(db < -100) db = -100;
	}
	return db;
}


Freq::Freq()
{
	init_table();
	freq = 0;
}

Freq::Freq(const Freq& oldfreq)
{
	this->freq = oldfreq.freq;
}

void Freq::init_table()
{
	if(!freqtable)
	{
		freqtable = new int[TOTALFREQS + 1];

  		for(int i = 0; i <= TOTALFREQS; i++)
  		{
    		freqtable[i] = tofreq_f(i);
//printf("Freq::init_table %d\n", freqtable[i]);
  		}
	}
}

int Freq::fromfreq() 
{
	int i;

  	for(i = 0; i < TOTALFREQS && freqtable[i] < freq; i++)
    	;
  	return(i);
};

int Freq::fromfreq(int index) 
{
	int i;

 	init_table();
 	for(i = 0; i < TOTALFREQS && freqtable[i] < index; i++)
    	;
  	return(i);
};

int Freq::tofreq(int index)
{ 
	init_table();
	if(index >= TOTALFREQS) index = TOTALFREQS - 1;
	int freq = freqtable[index]; 
	return freq; 
}

// frequency doubles for every OCTAVE slots.  OCTAVE must be divisible by 3
// 27.5 is at i=1
// 55 is at i=106
// 110 is at i=211
// 220 is at i=316
// 440 is at i=421
// 880 is at i=526
double Freq::tofreq_f(double index)
{
    if(index < 0.5)
    {
        return 0;
    }

    return 440.0 * pow(2, (double)(index - 421) / OCTAVE);
}

double Freq::fromfreq_f(double f)
{
    if(f < 0.5)
    {
        return 0;
    }

    return log(f / 440) / log(2.0) * OCTAVE + 421;
}


Freq& Freq::operator++() 
{
	if(freq < TOTALFREQS) freq++;
	return *this;
}
	
Freq& Freq::operator--()
{
	if(freq > 0) freq--;
	return *this;
}
	
int Freq::operator>(Freq &newfreq) { return freq > newfreq.freq; }
int Freq::operator<(Freq &newfreq) { return freq < newfreq.freq; }
Freq& Freq::operator=(const Freq &newfreq) { freq = newfreq.freq; return *this; }
int Freq::operator=(const int newfreq) { freq = newfreq; return newfreq; }
int Freq::operator!=(Freq &newfreq) { return freq != newfreq.freq; }
int Freq::operator==(Freq &newfreq) { return freq == newfreq.freq; }
int Freq::operator==(int newfreq) { return freq == newfreq; }

char* Units::totext(char *text, 
			double seconds, 
			int time_format, 
			int sample_rate, 
			float frame_rate, 
			float frames_per_foot)    // give text representation as time
{
	int hour, minute, second, thousandths;
	int64_t frame, feet;

	switch(time_format)
	{
		case TIME_SECONDS:
			seconds = fabs(seconds);
			sprintf(text, "%04d.%03d", (int)seconds, (int)(seconds * 1000) % 1000);
			return text;
			break;

		case TIME_HMS:
			seconds = fabs(seconds);
  			hour = (int)(seconds / 3600);
  			minute = (int)(seconds / 60 - hour * 60);
  			second = (int)seconds - (int64_t)hour * 3600 - (int64_t)minute * 60;
			thousandths = (int)(seconds * 1000) % 1000;
  			sprintf(text, "%d:%02d:%02d.%03d", 
				hour, 
				minute, 
				second, 
				thousandths);
			return text;
		  break;
		
		case TIME_HMS2:
		{
			float second;
			seconds = fabs(seconds);
  			hour = (int)(seconds / 3600);
  			minute = (int)(seconds / 60 - hour * 60);
  			second = (float)seconds - (int64_t)hour * 3600 - (int64_t)minute * 60;
  			sprintf(text, "%d:%02d:%02d", hour, minute, (int)second);
			return text;
		}
		  break;

		case TIME_HMS3:
		{
			float second;
			seconds = fabs(seconds);
  			hour = (int)(seconds / 3600);
  			minute = (int)(seconds / 60 - hour * 60);
  			second = (float)seconds - (int64_t)hour * 3600 - (int64_t)minute * 60;
  			sprintf(text, "%02d:%02d:%02d", hour, minute, (int)second);
			return text;
		}
		  break;

		case TIME_HMSF:
		{
			int second;
			seconds = fabs(seconds);
  			hour = (int)(seconds / 3600);
  			minute = (int)(seconds / 60 - hour * 60);
  			second = (int)(seconds - hour * 3600 - minute * 60);
//   			frame = (int64_t)round(frame_rate * 
//   	 			 (float)((float)seconds - (int64_t)hour * 3600 - (int64_t)minute * 60 - second));
//   			sprintf(text, "%01d:%02d:%02d:%02ld", hour, minute, second, frame);
			frame = (int64_t)((double)frame_rate * 
					seconds + 
					0.0000001) - 
				(int64_t)((double)frame_rate * 
					(hour * 
					3600 + 
					minute * 
					60 + 
					second) + 
				0.0000001);   
			sprintf(text, "%01d:%02d:%02d:%02ld", hour, minute, second, frame);
			return text;
		}
			break;
			
		case TIME_SAMPLES:
  			sprintf(text, "%09ld", to_int64(seconds * sample_rate));
			break;
		
		case TIME_SAMPLES_HEX:
  			sprintf(text, "%08x", (int)to_int64(seconds * sample_rate));
			break;
		
		case TIME_FRAMES:
			frame = to_int64(seconds * frame_rate);
			sprintf(text, "%06ld", frame);
			return text;
			break;
		
		case TIME_FEET_FRAMES:
			frame = to_int64(seconds * frame_rate);
			feet = (int64_t)(frame / frames_per_foot);
			sprintf(text, "%05ld-%02ld", 
				feet, 
				(int64_t)(frame - feet * frames_per_foot));
			return text;
			break;
	}
	return text;
}


// give text representation as time
char* Units::totext(char *text, 
		int64_t samples, 
		int samplerate, 
		int time_format, 
		float frame_rate,
		float frames_per_foot)
{
	return totext(text, (double)samples / samplerate, time_format, samplerate, frame_rate, frames_per_foot);
}    

int64_t Units::fromtext(const char *text, 
			int samplerate, 
			int time_format, 
			float frame_rate,
			float frames_per_foot)
{
	int64_t hours, minutes, frames, total_samples, i, j;
	int64_t feet;
	double seconds;
	char string[BCTEXTLEN];
	
	switch(time_format)
	{
		case TIME_SECONDS:
			seconds = atof(text);
			return (int64_t)(seconds * samplerate);
			break;

		case TIME_HMS:
		case TIME_HMS2:
		case TIME_HMS3:
// get hours
			i = 0;
			j = 0;
			while(text[i] >=48 && text[i] <= 57 && j < 10) string[j++] = text[i++];
			string[j] = 0;
			hours = atol(string);
// get minutes
			j = 0;
// skip separator
			while((text[i] < 48 || text[i] > 57) && text[i] != 0) i++;
			while(text[i] >=48 && text[i] <= 57 && j < 10) string[j++] = text[i++];
			string[j] = 0;
			minutes = atol(string);
			
// get seconds
			j = 0;
// skip separator
			while((text[i] < 48 || text[i] > 57) && text[i] != 0) i++;
			while((text[i] == '.' || (text[i] >=48 && text[i] <= 57)) && j < 10) string[j++] = text[i++];
			string[j] = 0;
			seconds = atof(string);

			total_samples = (uint64_t)(((double)seconds + minutes * 60 + hours * 3600) * samplerate);
			return total_samples;
			break;

		case TIME_HMSF:
// get hours
			i = 0;
			j = 0;
			while(text[i] >=48 && text[i] <= 57 && j < 10) string[j++] = text[i++];
			string[j] = 0;
			hours = atol(string);
			
// get minutes
			j = 0;
// skip separator
			while((text[i] < 48 || text[i] > 57) && text[i] != 0) i++;
			while(text[i] >=48 && text[i] <= 57 && j < 10) string[j++] = text[i++];
			string[j] = 0;
			minutes = atol(string);
			
// get seconds
			j = 0;
// skip separator
			while((text[i] < 48 || text[i] > 57) && text[i] != 0) i++;
			while(text[i] >=48 && text[i] <= 57 && j < 10) string[j++] = text[i++];
			string[j] = 0;
			seconds = atof(string);
			
// skip separator
			while((text[i] < 48 || text[i] > 57) && text[i] != 0) i++;
// get frames
			j = 0;
			while(text[i] >=48 && text[i] <= 57 && j < 10) string[j++] = text[i++];
			string[j] = 0;
			frames = atol(string);
			
			total_samples = (int64_t)(((float)frames / frame_rate + seconds + minutes*60 + hours*3600) * samplerate);
			return total_samples;
			break;

		case TIME_SAMPLES:
			return atol(text);
			break;
		
		case TIME_SAMPLES_HEX:
		{
			int temp;
			sscanf(text, "%x", &temp);
			total_samples = temp;
			return total_samples;
		}
		
		case TIME_FRAMES:
			return (int64_t)(atof(text) / frame_rate * samplerate);
			break;
		
		case TIME_FEET_FRAMES:
// Get feet
			i = 0;
			j = 0;
			while(text[i] >=48 && text[i] <= 57 && text[i] != 0 && j < 10) string[j++] = text[i++];
			string[j] = 0;
			feet = atol(string);

// skip separator
			while((text[i] < 48 || text[i] > 57) && text[i] != 0) i++;

// Get frames
			j = 0;
			while(text[i] >=48 && text[i] <= 57 && text[i] != 0 && j < 10) string[j++] = text[i++];
			string[j] = 0;
			frames = atol(string);
			return (int64_t)(((float)feet * frames_per_foot + frames) / frame_rate * samplerate);
			break;
	}
	return 0;
}

double Units::text_to_seconds(const char *text, 
				int samplerate, 
				int time_format, 
				float frame_rate, 
				float frames_per_foot)
{
	return (double)fromtext(text, 
		samplerate, 
		time_format, 
		frame_rate, 
		frames_per_foot) / samplerate;
}






float Units::toframes(int64_t samples, int sample_rate, float framerate) 
{ 
	return (float)samples / sample_rate * framerate; 
} // give position in frames

int64_t Units::toframes_round(int64_t samples, int sample_rate, float framerate) 
{
// used in editing
	float result_f = (float)samples / sample_rate * framerate; 
	int64_t result_l = (int64_t)(result_f + 0.5);
	return result_l;
}

double Units::fix_framerate(double value)
{
	if(value > 29.5 && value < 30) 
		value = (double)30000 / (double)1001;
	else
	if(value > 59.5 && value < 60) 
		value = (double)60000 / (double)1001;
	else
	if(value > 23.5 && value < 24) 
		value = (double)24000 / (double)1001;
	
	return value;
}

double Units::atoframerate(const char *text)
{
	double result = atof(text);
	return fix_framerate(result);
}


int64_t Units::tosamples(float frames, int sample_rate, float framerate) 
{ 
	float result = (frames / framerate * sample_rate);
	
	if(result - (int)result) result += 1;
	return (int64_t)result;
} // give position in samples


float Units::xy_to_polar(int x, int y)
{
	float angle;
	if(x > 0 && y <= 0)
	{
		angle = atan((float)-y / x) / (2 * M_PI) * 360;
	}
	else
	if(x < 0 && y <= 0)
	{
		angle = 180 - atan((float)-y / -x) / (2 * M_PI) * 360;
	}
	else
	if(x < 0 && y > 0)
	{
		angle = 180 - atan((float)-y / -x) / (2 * M_PI) * 360;
	}
	else
	if(x > 0 && y > 0)
	{
		angle = 360 + atan((float)-y / x) / (2 * M_PI) * 360;
	}
	else
	if(x == 0 && y < 0)
	{
		angle = 90;
	}
	else
	if(x == 0 && y > 0)
	{
		angle = 270;
	}
	else
	if(x == 0 && y == 0)
	{
		angle = 0;
	}

	return angle;
}

void Units::polar_to_xy(float angle, int radius, int &x, int &y)
{
	while(angle < 0) angle += 360;

	x = (int)(cos(angle / 360 * (2 * M_PI)) * radius);
	y = (int)(-sin(angle / 360 * (2 * M_PI)) * radius);
}

int64_t Units::round(double result)
{
	return (int64_t)(result < 0 ? result - 0.5 : result + 0.5);
}

float Units::quantize10(float value)
{
	int64_t temp = (int64_t)(value * 10 + 0.5);
	value = (float)temp / 10;
	return value;
}

float Units::quantize(float value, float precision)
{
	int64_t temp = (int64_t)(value / precision + 0.5);
	value = (float)temp * precision;
	return value;
}

int64_t Units::to_int64(double result)
{
// This must round up if result is one sample within cieling.
// Sampling rates below 48000 may cause more problems.
	return (int64_t)(result < 0 ? (result - 0.005) : (result + 0.005));
}

const char* Units::print_time_format(int time_format, char *string)
{
	switch(time_format)
	{
		case TIME_HMS: sprintf(string, TIME_HMS_TEXT); break;
		case TIME_HMSF: sprintf(string, TIME_HMSF_TEXT); break;
		case TIME_SAMPLES: sprintf(string, TIME_SAMPLES_TEXT); break;
		case TIME_SAMPLES_HEX: sprintf(string, TIME_SAMPLES_HEX_TEXT); break;
		case TIME_FRAMES: sprintf(string, TIME_FRAMES_TEXT); break;
		case TIME_FEET_FRAMES: sprintf(string, TIME_FEET_FRAMES_TEXT); break;
		case TIME_SECONDS: sprintf(string, TIME_SECONDS_TEXT); break;
	}
	
	return string;
}

int Units::text_to_format(const char *string)
{
	if(!strcmp(string, TIME_HMS_TEXT)) return TIME_HMS;
	if(!strcmp(string, TIME_HMSF_TEXT)) return TIME_HMSF;
	if(!strcmp(string, TIME_SAMPLES_TEXT)) return TIME_SAMPLES;
	if(!strcmp(string, TIME_SAMPLES_HEX_TEXT)) return TIME_SAMPLES_HEX;
	if(!strcmp(string, TIME_FRAMES_TEXT)) return TIME_FRAMES;
	if(!strcmp(string, TIME_FEET_FRAMES_TEXT)) return TIME_FEET_FRAMES;
	if(!strcmp(string, TIME_SECONDS_TEXT)) return TIME_SECONDS;
	return TIME_HMS;
}


#undef BYTE_ORDER
#define BYTE_ORDER ((*(u_int32_t*)"a   ") & 0x00000001)

void* Units::int64_to_ptr(uint64_t value)
{
	unsigned char *value_dissected = (unsigned char*)&value;
	void *result;
	unsigned char *data = (unsigned char*)&result;

// Must be done behind the compiler's back
	if(sizeof(void*) == 4)
	{
		if(!BYTE_ORDER)
		{
			data[0] = value_dissected[4];
			data[1] = value_dissected[5];
			data[2] = value_dissected[6];
			data[3] = value_dissected[7];
		}
		else
		{
			data[0] = value_dissected[0];
			data[1] = value_dissected[1];
			data[2] = value_dissected[2];
			data[3] = value_dissected[3];
		}
	}
	else
	{
		data[0] = value_dissected[0];
		data[1] = value_dissected[1];
		data[2] = value_dissected[2];
		data[3] = value_dissected[3];
		data[4] = value_dissected[4];
		data[5] = value_dissected[5];
		data[6] = value_dissected[6];
		data[7] = value_dissected[7];
	}
	return result;
}

uint64_t Units::ptr_to_int64(void *ptr)
{
	unsigned char *ptr_dissected = (unsigned char*)&ptr;
	int64_t result = 0;
	unsigned char *data = (unsigned char*)&result;
// Don't do this at home.
	if(sizeof(void*) == 4)
	{
		if(!BYTE_ORDER)
		{
			data[4] = ptr_dissected[0];
			data[5] = ptr_dissected[1];
			data[6] = ptr_dissected[2];
			data[7] = ptr_dissected[3];
		}
		else
		{
			data[0] = ptr_dissected[0];
			data[1] = ptr_dissected[1];
			data[2] = ptr_dissected[2];
			data[3] = ptr_dissected[3];
		}
	}
	else
	{
		data[0] = ptr_dissected[0];
		data[1] = ptr_dissected[1];
		data[2] = ptr_dissected[2];
		data[3] = ptr_dissected[3];
		data[4] = ptr_dissected[4];
		data[5] = ptr_dissected[5];
		data[6] = ptr_dissected[6];
		data[7] = ptr_dissected[7];
	}
	return result;
}

const char* Units::format_to_separators(int time_format)
{
	switch(time_format)
	{
		case TIME_SECONDS:     return "0000.000";
		case TIME_HMS:         return "0:00:00.000";
		case TIME_HMS2:        return "0:00:00";
		case TIME_HMS3:        return "00:00:00";
		case TIME_HMSF:        return "0:00:00:00";
		case TIME_SAMPLES:     return 0;
		case TIME_SAMPLES_HEX: return 0;
		case TIME_FRAMES:      return 0;
		case TIME_FEET_FRAMES: return "00000-00";
	}
	return 0;
}

void Units::punctuate(char *string)
{
	int len = strlen(string);
	int commas = (len - 1) / 3;
	for(int i = len + commas, j = len, k; j >= 0 && i >= 0; i--, j--)
	{
		k = (len - j - 1) / 3;
		if(k * 3 == len - j - 1 && j != len - 1 && string[j] != 0)
		{
			string[i--] = ',';
		}

		string[i] = string[j];
	}
}

void Units::fix_double(double *x)
{
	*x = *x;
}



