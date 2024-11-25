#include "audiodriver.h"
#include "bcsignals.h"
#include "mixer.h"
#include <signal.h>
#include <string.h>

typedef struct
{
	int channels;
	char display[BCTEXTLEN];
} Args;

int print_copyright()
{
	printf("%s (C) 2008 V.%d.%d.%d Adam Williams\n", 
		PROGRAM_NAME, 
		VERSION_MAJOR,
		VERSION_MINOR,
		VERSION_RELEASE);
	printf("No warranties of any kind.\n");
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
			printf("mix2000 [-display display]\n");
			return 1;
		}
		else
		{
			if(i + 1 < argc)
			{
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


int main(int argc, char *argv[])
{
	Args args;
	int result;
	sprintf(args.display, "");

	get_base_args(argc, argv, &args);

	Mixer mixer(args.display);
	BC_Resources::set_signals(new MixerResources(&mixer));
	print_copyright();
	mixer.create_objects();
	mixer.do_gui();

// Save defaults
	mixer.save_defaults();

	return 0;
}
