#ifndef AUDIODRIVER_H
#define AUDIODRIVER_H


#include "mixer.inc"

class AudioDriver
{
public:
	AudioDriver(Mixer *mixer);
	virtual ~AudioDriver();

	void initialize();
	virtual int construct_tree() { return 1; };
// Write parameters to device
	virtual void write_parameters();
// Read parameters from device
	virtual void read_parameters();

	Mixer *mixer;
	int device_open;
};



#endif
