#include <stdio.h>
#include <string.h>
#include "arraylist.h"
#include "bcsignals.h"
#include "filesystem.h"
#include "mwindow.h"

int usage()
{
	printf("XMovie %s (C) 2004 Heroine Virtual Ltd.\n",
		VERSION);
	return 0;
}

int main(int argc, char *argv[])
{
	ArrayList<char*> init_playlist;
	char *string;
    FileSystem fs;

	usage();
	for(int i = 1, j = 0; i < argc; i++, j++)
	{
		init_playlist.append(string = new char[1024]);
		strcpy(string, argv[i]);
        fs.complete_path(string);
    }

	{
		MWindow mwindow(&init_playlist);
		mwindow.create_objects();
// Must be done after main window creation or it won't trap anything.
		BC_Signals *signals = new BC_Signals;
		signals->initialize();

// only loading the first movie on the command line now
// Possibly load an audio file for merge later.
		if(init_playlist.total) mwindow.load_file(init_playlist.values[0], 0);
		mwindow.run_program();
	}

	for(int i = 0; i < init_playlist.total; i++) delete init_playlist.values[i];
	init_playlist.remove_all();
	return 0;
}
