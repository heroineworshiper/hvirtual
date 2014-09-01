#ifndef MIXERTREE_H
#define MIXERTREE_H


#include "arraylist.h"
#include "guicast.h"
#include "mixer.inc"
#include "mixertree.inc"


// Stores values of all mixer parameters.  The top row of nodes are each
// mixer parameter.  Each mixer parameter has an array for the channel values.
//
// When the mixer is drawn on the screen, the widgets are ordered according to
// the order of nodes in the tree.
// Toggles are stacked on top of each other.


class MixerTree : public ArrayList<MixerNode*>
{
public:
	MixerTree(Mixer *mixer);
	~MixerTree();

	MixerNode* add_node(int type,
		int min, 
		int max, 
		char *title, 
		char *title_base,
		int channels = 0,
		int has_mute = 0);
	void dump();
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);

	Mixer *mixer;
	char card_name[BCTEXTLEN];
	char device_name[BCTEXTLEN];
};

class MixerNode
{
public:
	MixerNode(Mixer *mixer, MixerTree *parent_tree, MixerNode *parent_node);
	~MixerNode();


	void add_menu_item(char *string);
	char* get_menu_text(int channel);
	char* get_menu_item(int number);
	void dump();

	int record;
	int min;
	int max;
// Value applied to volume and standalone toggles
	int value[MAX_CHANNELS];
// Value applied to toggles attached to volume controls
//	int mute[MAX_CHANNELS];
// Merged mute value with value for pots to simplify the interface.
	int type;
	int channels;
	int show;
// Set when a toggle is attached to a volume control
	int has_mute;
	char title[BCTEXTLEN];
	char title_base[BCTEXTLEN];

	enum
	{
		TYPE_NONE,
		TYPE_TOGGLE,
		TYPE_POT,
		TYPE_MENU,
		TYPE_LEVEL
	};

	Mixer *mixer;
	MixerTree *parent_tree;
	MixerNode *parent_node;
	ArrayList<char*> menu_items;
};


#endif
