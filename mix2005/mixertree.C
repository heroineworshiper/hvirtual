#include "bchash.h"
#include "mixertree.h"

#include <string.h>

MixerTree::MixerTree(Mixer *mixer)
 : ArrayList<MixerNode*>()
{
	this->mixer = mixer;
}

MixerTree::~MixerTree()
{
	remove_all_objects();
}

MixerNode* MixerTree::add_node(int type,
	int min, 
	int max, 
	char *title, 
	char *title_base,
	int channels,
	int has_mute)
{
	MixerNode *result = new MixerNode(mixer, this, 0);
	result->type = type;
	result->min = min;
	result->max = max;
	strcpy(result->title, title);
	strcpy(result->title_base, title_base);
	result->channels = channels;
	result->has_mute = has_mute;
	append(result);
	return result;
}

void MixerTree::load_defaults(BC_Hash *defaults)
{
	for(int i = 0; i < total; i++)
	{
		char string[BCTEXTLEN];
		MixerNode *node = values[i];
		strcpy(string, node->title);
		char *ptr = string;
		while(*ptr)
		{
			if(*ptr == ' ') *ptr = '_';
			ptr++;
		}

// Get visibility toggle
		char string2[BCTEXTLEN];
		sprintf(string2, "%s_SHOW", string);
		node->show = defaults->get(string2, node->show);

// Get data value
		switch(node->type)
		{
			case MixerNode::TYPE_TOGGLE:
				strcat(string, "_TOGGLE");
				break;
			case MixerNode::TYPE_POT:
				strcat(string, "_POT");
				break;
			case MixerNode::TYPE_MENU:
				strcat(string, "_MENU");
				break;
		}

		for(int j = 0; j < node->channels; j++)
		{
			sprintf(string2, "%s_%d", string, j);
			node->value[j] = defaults->get(string2, node->value[j]);
		}
	}
}

void MixerTree::save_defaults(BC_Hash *defaults)
{
	for(int i = 0; i < total; i++)
	{
		char string[BCTEXTLEN];
		MixerNode *node = values[i];
		strcpy(string, node->title);
		char *ptr = string;
		while(*ptr)
		{
			if(*ptr == ' ') *ptr = '_';
			ptr++;
		}

// Save visibility toggle
		char string2[BCTEXTLEN];
		sprintf(string2, "%s_SHOW", string);
		defaults->update(string2, node->show);

// Save data value
		switch(node->type)
		{
			case MixerNode::TYPE_TOGGLE:
				strcat(string, "_TOGGLE");
				break;
			case MixerNode::TYPE_POT:
				strcat(string, "_POT");
				break;
			case MixerNode::TYPE_MENU:
				strcat(string, "_MENU");
				break;
		}

		for(int j = 0; j < node->channels; j++)
		{
			sprintf(string2, "%s_%d", string, j);
			defaults->update(string2, node->value[j]);
		}
	}
}

void MixerTree::dump()
{
	printf("MixerTree::dump\n");
	printf("    card_name=%s\n", card_name);
	printf("    device_name=%s\n", device_name);
	for(int i = 0; i < total; i++)
	{
		values[i]->dump();
	}
}







MixerNode::MixerNode(Mixer *mixer, 
	MixerTree *parent_tree, 
	MixerNode *parent_node)
{
	this->mixer = mixer;
	this->parent_tree = parent_tree;
	this->parent_node = parent_node;
	title[0] = 0;
	title_base[0] = 0;
	show = 1;
	record = 0;
	has_mute = 0;
	bzero(value, sizeof(int) * MAX_CHANNELS);
//	bzero(mute, sizeof(int) * MAX_CHANNELS);
}

MixerNode::~MixerNode()
{
	menu_items.remove_all_objects();
}

void MixerNode::add_menu_item(char *string)
{
	char *ptr = new char[strlen(string) + 1];
	strcpy(ptr, string);
	menu_items.append(ptr);
}

char* MixerNode::get_menu_text(int channel)
{
	return menu_items.values[value[channel]];
}

char* MixerNode::get_menu_item(int number)
{
	return menu_items.values[number];
}

void MixerNode::dump()
{
	printf("    title=%s\n", title);
	printf("      type=%d channels=%d has_mute=%d min=%d max=%d record=%d\n", 
		type, channels, has_mute, min, max, record);
	printf("      values=");
	for(int i = 0; i < channels; i++)
		printf("%d ", value[i]);
	printf("\n");

// 	if(has_mute)
// 	{
// 		printf("      mute=");
// 		for(int i = 0; i < channels; i++)
// 			printf("%d ", mute[i]);
// 		printf("\n");
// 	}
	if(menu_items.total)
		printf("      ");
	for(int i = 0; i < menu_items.total; i++)
		printf("%s; ", menu_items.values[i]);
	if(menu_items.total)
		printf("\n");
}






