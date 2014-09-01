#include "bcsignals.h"
#include "mixer.h"
#include <signal.h>
#include <string.h>

typedef struct
{
	int channels;
	char display[1024];
} Args;

int print_copyright()
{
	printf("Mix 2000 Version 1.0.1 (C) 2004 Heroine Virtual Ltd.\n");
	printf("No warranties of any kind.\n");
}


int set_value(int value, int parameter, int channel, int mixer_fd, Args *args)
{
	int oldvalues, oldvalue, difference;
	char *values;
	ioctl(mixer_fd, MIXER_READ(parameter), &oldvalues);
	values = (char *)&oldvalues;
	if(channel) 
	{
		oldvalue = values[channel - 1];
		difference = value - oldvalue;
		values[channel - 1] = value;
	}
	else
	{       // affect all values
		for(int i = 0; i < args->channels; i++)
		{
			values[i] = value;
		}
	}
	
	ioctl(mixer_fd, MIXER_WRITE(parameter), (int *)values);
}

int set_argument(char *arg1, char *arg2, Args *args, Mixer *mixer)
{
	int channel;
	int parameter_char;
	int parameter_number;
	int value;
	int i;

	if(arg1[0] != '-')
	{
		printf("Unrecognized command: %s\n", arg1);
	}
	
	value = atol(arg2);
	
	if(strlen(arg1) == 2)       // two characters means channel argument was omitted
	{
		channel = 0;
		parameter_char = arg1[1];
	}
	else
	{
// Only set one channel
		channel = arg1[1] - 48;
		if(channel < 1)
		{
			printf("There is no %d channel\n", channel);
			return 1;
		}
		else if(channel > mixer->channels)
		{
			printf("Channel out of range.\n");
			return 1;
		}
		parameter_char = arg1[2];
	}
	
	switch(parameter_char)
	{
		case 'o': parameter_number = MASTER_NUMBER; break;
		case 'b': parameter_number = BASS_NUMBER; break;
		case 't': parameter_number = TREBLE_NUMBER; break;
		case 'l': parameter_number = LINE_NUMBER; break;
		case 'd': parameter_number = DSP_NUMBER; break;
		case 'f': parameter_number = FM_NUMBER; break;
		case 'c': parameter_number = CD_NUMBER; break;
		case 'm': parameter_number = MIC_NUMBER; break;
		default:
			printf("Unrecognized parameter: %c\n", parameter_char);
			return 1;
			break;
	}

	if(channel == 0)
	{
		for(i = 0; i < mixer->channels; i++)
			mixer->values[parameter_number][i] = value;
	}
	else
		mixer->values[parameter_number][channel] = value;
	mixer->commandline_setting[parameter_number] = 1;
	return 0;
}

int get_base_args(int argc, char *argv[], Args *args)
{
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-h"))
		{
			print_copyright();
			printf("Usage:\n");
			printf("mix2000 [-display display] [-channels channels] [[-line | mic | cd][-[channel]parameter value]]...\n");
			printf("By default mix2000 starts up a GUI.\n\n");
			printf("display - set the display for the GUI\n");
			printf("channels - the total channels on your soundcard mixer (default = 2)\n");
			printf("Enter channels as first argument\n\n");
			printf("Any number of inputs can be specified:\n");
			printf("line - set input to the line\n");
			printf("mic - set input to the mic\n");
			printf("cd - set input to the cd\n\n");
			printf("Any number of channel, parameter, value settings can be specified:\n");
			printf("[channel] the channel from 1 to 9 to affect\n");
			printf("Omitting channel causes it to affect all channels\n\n");
			printf("parameter may be any one of:\n");
			printf("o - main output\n");
			printf("b - bass\n");
			printf("t - treble\n");
			printf("l - line\n");
			printf("d - dsp\n");
			printf("f - fm\n");
			printf("c - cd\n");
			printf("m - mic\n\n");
			printf("value - the value to set it to\n\n");
			printf("Examples:\n");
			printf("mix2000 -channels 4 -o 100 set main output for all channels to 100 on a 4 channel soundcard\n");
			printf("mix2000 -1b 50 -2t 100     set left bass to 50 and right treble to 100\n");
			printf("mix2000 -line              set input to line\n");
			printf("mix2000 -line -mic -l 75   set input to line and mic and set the line level to 75\n\n");
			printf("mix2000 -channels 4        start the GUI with 4 channels\n\n");
			return 1;
		}
		else
		{
			if(i + 1 < argc)
			{
				if(!strcmp(argv[i], "-channels"))
				{
					args->channels = atol(argv[i + 1]);
					if(args->channels < 1)
					{
						printf("No soundcard has less than 1 channel you Mor*on!!!\n");
						return 1;
					}
					else if(args->channels > 4)
					{
						printf("OSS can't support more than 4 channels.\n");
						return 1;
					}
				}
				else 
				if(!strcmp(argv[i], "-display"))
				{
					strcpy(args->display, argv[i+1]);
				}
				i++;
			}
		}
	}
	return 0;
}

int set_arguments(int argc, char *argv[], Args *args, Mixer *mixer)
{
	int input = 0, result = 0;
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-h"))
		{
			;
		}
		else if(!strcmp(argv[i], "-line"))
		{
			mixer->line = 1;
			result = 1;
		}
		else if(!strcmp(argv[i], "-mic"))
		{
			mixer->mic = 1;
			result = 1;
		}
		else if(!strcmp(argv[i], "-cd"))
		{
			mixer->cd = 1;
			result = 1;
		}
		else
		{
			if(i + 1 < argc)
			{
				if(!strcmp(argv[i], "-channels"))
				{
					;
				}
				else 
				if(!strcmp(argv[i], "-display"))
				{
					;
				}
				else 
				{
					if(set_argument(argv[i], argv[i + 1], args, mixer))
					{
// Failed to set
						return 1;
					}
					else
					{
// Set successfully
						result = 1;
					}
				}
				i++;
			}
			else
			{
				printf("%s was not paired with a value\n", argv[i]);
				return 1;
			}
		}
	}
	return result;
}

int main(int argc, char *argv[])
{
	Args args;
	int result;
	args.channels = 2;
	sprintf(args.display, "");

	get_base_args(argc, argv, &args);

	Mixer mixer(args.display, args.channels);

// Doesn't trap signals for some reason.
	BC_Resources::set_signals(new MixerResources(&mixer));
	BC_Resources::get_signals()->initialize();
	print_copyright();
	mixer.create_objects();

// Read command line settings.
	result = set_arguments(argc, argv, &args, &mixer);



// Start a gui if no command line settings.
	if(!result)
	{
// Enter default settings here.
		mixer.device->write_parameters(0, 0);
		mixer.do_gui();
	}
	else
	{
// Enter command line settings here.
		mixer.device->write_parameters(0, 1);
	}

// Save defaults
	mixer.save_defaults();

	return 0;
}
