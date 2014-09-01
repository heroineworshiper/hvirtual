#include "playlist.h"

#include <stdio.h>
#include <string.h>

PlaylistArray::PlaylistArray()
 : ArrayList<char*>()
{
}

PlaylistArray::~PlaylistArray()
{
}



Playlist::Playlist()
{
}

Playlist::~Playlist()
{
	for(int i = 0; i < array.total; i++)
	{
		delete array.values[i];
	}
}


int Playlist::load(char *path)
{
	FILE *file;
	char string[1024];
	char *new_string;
	int i;

	if(!(file = fopen(path, "r")))
	{
		return 1;
	}
	fread(string, 8, 1, file);
	string[8] = 0;
	if(strncasecmp(string, "PLAYLIST", 8))
	{
		fclose(file);
		return 1;
	}

	while(!feof(file))
	{
		fgets(string, 1024, file);
		if(string[0] == '#') continue;
		
		array.append(new_string = new char[strlen(string)]);
		strcpy(new_string, string);
	}

	fclose(file);
	return 0;
}

int Playlist::save(char *path)
{
	return 0;
}


int Playlist::append_path(char *path)
{
	return 0;
}

int Playlist::delete_path(int path)
{
	return 0;
}

int Playlist::swap_paths(int path1, int path2)
{
	return 0;
}
