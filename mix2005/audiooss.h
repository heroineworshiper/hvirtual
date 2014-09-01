#ifndef AUDIOOSS_H
#define AUDIOOSS_H


#include "audiodriver.h"

class AudioOSS : public AudioDriver
{
public:
	AudioOSS(Mixer *mixer);
	~AudioOSS();

	int construct_tree();
};


#endif
