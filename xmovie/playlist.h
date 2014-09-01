#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "arraylist.h"
#include "playlist.inc"

class PlaylistArray : public ArrayList<char*>
{
public:
	PlaylistArray();
	~PlaylistArray();
};

class Playlist
{
public:
	Playlist();
	~Playlist();

	int load(char *path);
	int save(char *path);
	
	int append_path(char *path);
	int delete_path(int path);
	int swap_paths(int path1, int path2);
	
	PlaylistArray array;
};

#endif
