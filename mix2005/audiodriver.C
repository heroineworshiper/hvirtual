#include "audiodriver.h"
#include "mixer.h"
#include "mixertree.h"


AudioDriver::AudioDriver(Mixer *mixer)
{
	this->mixer = mixer;
	device_open = 0;
}

AudioDriver::~AudioDriver()
{
}

void AudioDriver::initialize()
{
	construct_tree();
	read_parameters();
}

void AudioDriver::write_parameters()
{
}

void AudioDriver::read_parameters()
{
}


