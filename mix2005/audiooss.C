#include "audiooss.h"
#include "mixer.h"
#include "mixertree.h"


AudioOSS::AudioOSS(Mixer *mixer)
 : AudioDriver(mixer)
{
}

AudioOSS::~AudioOSS()
{
}

int AudioOSS::construct_tree()
{
	MixerTree *tree = mixer->tree;
	tree->add_node(MixerNode::TYPE_TOGGLE, 0, 1, "Line");
	tree->add_node(MixerNode::TYPE_TOGGLE, 0, 1, "Mic");
	tree->add_node(MixerNode::TYPE_TOGGLE, 0, 1, "CD");

	tree->add_node(MixerNode::TYPE_SLIDER, 0, 100, "Master", mixer->channels);

	tree->add_node(MixerNode::TYPE_POT, 0, 100, "Bass", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "Treble", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "Line", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "DAC", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "FM", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "CD", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "Mic", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "Rec", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "IGain", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "OGain", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "Speaker", mixer->channels);
	tree->add_node(MixerNode::TYPE_POT, 0, 100, "PhoneOut", mixer->channels);
	return 0;
}




