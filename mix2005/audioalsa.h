#ifndef AUDIOALSA_H
#define AUDIOALSA_H

#include "arraylist.h"
#include "audiodriver.h"
#include "alsa/asoundlib.h"




class AudioALSA : public AudioDriver
{
public:
	AudioALSA(Mixer *mixer);
	~AudioALSA();

	int construct_tree();
	void read_parameters();
	void write_parameters();
// Iterate through the tree, performing the operation
	void do_elements(int construct_tree, int read_values, int write_values);
	void append_title(ArrayList<char*> *titles, 
		const char *title);

	int mixer_n_selems;
};





#endif
