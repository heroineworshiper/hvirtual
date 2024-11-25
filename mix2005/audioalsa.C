#include "audioalsa.h"
#include "clip.h"
#include "guicast.h"
#include "mixer.h"
#include "mixertree.h"



AudioALSA::AudioALSA(Mixer *mixer)
 : AudioDriver(mixer)
{
}

AudioALSA::~AudioALSA()
{
}

static int mixer_event(snd_mixer_t *mixer, 
	unsigned int mask, 
	snd_mixer_elem_t *elem)
{
  	return 0;
}

int AudioALSA::construct_tree()
{
	do_elements(1, 0, 0);
    return 0;
}

void AudioALSA::read_parameters()
{
	do_elements(0, 1, 0);
}

void AudioALSA::write_parameters()
{
	do_elements(0, 0, 1);
}

void AudioALSA::do_elements(int construct_tree, 
	int read_values, 
	int write_values)
{
	int result = 0;
  	snd_ctl_card_info_t *hw_info = 0;
  	snd_ctl_t *ctl_handle = 0;
const int debug = 1;

	char card_id[BCTEXTLEN];
	char string[BCTEXTLEN];
	char string2[BCTEXTLEN];
	snd_mixer_t *mixer_handle;
	int current_node = 0;

	snd_ctl_card_info_alloca(&hw_info);
	strcpy(card_id, "default");
//	strcpy(card_id, "hw:1");
	mixer->defaults->get("ALSA_CARDID", card_id);
	mixer->defaults->update("ALSA_CARDID", card_id);
	if((result = snd_ctl_open(&ctl_handle, card_id, 0)) < 0)
		fprintf(stderr, 
			"AudioALSA::do_elements: snd_ctl_open card_id=%s error=%s\n",
			card_id,
			snd_strerror(result));



	if((result = snd_ctl_card_info(ctl_handle, hw_info)) < 0)
		fprintf(stderr, "AudioALSA::do_elements: snd_ctl_card_info failed\n");
	snd_ctl_close(ctl_handle);

	if((result = snd_mixer_open(&mixer_handle, 0)) < 0)
		fprintf(stderr, "AudioALSA::do_elements: snd_mixer_open failed.\n");
	if((result = snd_mixer_attach(mixer_handle, card_id)) < 0)
		fprintf(stderr, "AudioALSA::do_elements: snd_mixer_attach failed.\n");
	if((result = snd_mixer_selem_register(mixer_handle, NULL, NULL)) < 0)
		fprintf(stderr, "AudioALSA::do_elements: snd_mixer_selem_register failed.\n");
	snd_mixer_set_callback(mixer_handle, mixer_event);
	if((result = snd_mixer_load(mixer_handle)) < 0)
		fprintf(stderr, "AudioALSA::do_elements: snd_mixer_load failed.\n");



	if(construct_tree)
	{
  		strcpy(mixer->tree->card_name, snd_ctl_card_info_get_name(hw_info));
		strcpy(mixer->tree->device_name, snd_ctl_card_info_get_mixername(hw_info));
	}	

  	snd_mixer_elem_t *elem;
  	snd_mixer_selem_id_t *sid;
	int idx;
	int elem_index;

// Get all mixer elements
	mixer_n_selems = 0;
	void *mixer_sid = calloc(snd_mixer_selem_id_sizeof(), 
		snd_mixer_get_count(mixer_handle));
	for(elem = snd_mixer_first_elem(mixer_handle); 
		elem; 
		elem = snd_mixer_elem_next(elem))
	{
		sid = (snd_mixer_selem_id_t*)(((char *)mixer_sid) + 
			snd_mixer_selem_id_sizeof() * mixer_n_selems);
		snd_mixer_selem_get_id(elem, sid);

// Get the title
		char new_title[BCTEXTLEN];
		strcpy(new_title, snd_mixer_selem_id_get_name(sid));
		sprintf(string,
			"%s %d", 
			new_title,
			snd_mixer_selem_id_get_index(sid));

if(debug) printf("title=%s\n", string);

// Get channel count
		int channels = 0;
		for(channels = 0; 1; channels++)
		{
			if(!snd_mixer_selem_has_playback_channel(elem, 
				(snd_mixer_selem_channel_id_t)channels)) break;
		}
		if(!channels)
		{
			for(channels = 0; 1; channels++)
			{
				if(!snd_mixer_selem_has_capture_channel(elem, 
					(snd_mixer_selem_channel_id_t)channels)) break;
			}
		}
		channels = MIN(MAX_CHANNELS, channels);








// Get the type
		if(snd_mixer_selem_is_enumerated(elem))
		{
			MixerNode *node;

if(debug) printf("  enumerated\n");
// Menu
			if(construct_tree)
			{
				int items = snd_mixer_selem_get_enum_items(elem);
				node = mixer->tree->add_node(MixerNode::TYPE_MENU,
					0,
					items,
					string,
					new_title,
					channels);
				for(int i = 0; i < items; i++)
				{
					snd_mixer_selem_get_enum_item_name(elem, 
						(snd_mixer_selem_channel_id_t)i, 
						BCTEXTLEN, 
						string);
					node->add_menu_item(string);
				}
			}
			else
			{
				node = mixer->tree->values[current_node];
				if(write_values)
				{
					for(int i = 0; i < channels; i++)
						snd_mixer_selem_set_enum_item(elem, 
							(snd_mixer_selem_channel_id_t)i, 
							node->value[i]);
				}
				else
				if(read_values)
				{
					for(int i = 0; i < channels; i++)
					{
						int result = snd_mixer_selem_get_enum_item(elem, 
							(snd_mixer_selem_channel_id_t)i, 
							(unsigned int*)&node->value[i]);
					}
				}
			}
			current_node++;


		}
		else
		{

			if(snd_mixer_selem_has_playback_volume(elem))
			{
if(debug) printf("  playback volume\n");
				long min, max, value;
				snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
				MixerNode *node;

				if(construct_tree)
				{
					node = mixer->tree->add_node(MixerNode::TYPE_POT,
						min,
						max,
						string,
						new_title,
						channels);
				}
				else
				{
					node = mixer->tree->values[current_node];
					if(read_values)
						for(int i = 0; i < channels; i++)
						{
							snd_mixer_selem_get_playback_volume(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								&value);
							node->value[i] = value;
						}
					else
					if(write_values)
						for(int i = 0; i < channels; i++)
						{
							value = node->value[i];
							snd_mixer_selem_set_playback_volume(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								value);
						}
				}

// Mute button bound to volume
				if(snd_mixer_selem_has_playback_switch(elem))
				{
					if(construct_tree)
						node->has_mute = 1;
					else
					if(read_values)
					{
// Values are based on volume setting
						for(int i = 0; i < channels; i++)
						{
							int value_i;
							snd_mixer_selem_get_playback_switch(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								&value_i);
							if(!value_i) node->value[i] = 0;
						}
					}
					else
					if(write_values)
					{
						for(int i = 0; i < channels; i++)
						{
// Set based on volume setting
							snd_mixer_selem_set_playback_switch(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								(node->value[i] > 0) ? 1 : 0);
//								node->mute[i]);
						}
					}
				}

				current_node++;
			}
			else
			if(snd_mixer_selem_has_capture_volume(elem))
			{
if(debug) printf("  capture volume\n");
				long min, max, value;
				snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
				MixerNode *node;
				if(construct_tree)
				{
					node = mixer->tree->add_node(MixerNode::TYPE_POT,
						min,
						max,
						string,
						new_title,
						channels);
					node->record = 1;
				}
				else
				{
					node = mixer->tree->values[current_node];
					if(read_values)
						for(int i = 0; i < channels; i++)
						{
							snd_mixer_selem_get_capture_volume(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								&value);
							node->value[i] = value;
						}
					else
					if(write_values)
						for(int i = 0; i < channels; i++)
						{
							value = node->value[i];
							snd_mixer_selem_set_capture_volume(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								value);
						}
				}


// Mute button bound to volume
				if(snd_mixer_selem_has_capture_switch(elem))
				{
					if(construct_tree)
						node->has_mute = 1;
					else
					if(read_values)
					{
						for(int i = 0; i < channels; i++)
						{
// Values are based on volume setting
							int value_i;
							snd_mixer_selem_get_capture_switch(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								&value_i);
							if(!value) node->value[i] = 0;
						}
					}
					else
					if(write_values)
					{
						for(int i = 0; i < channels; i++)
						{
							snd_mixer_selem_set_capture_switch(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								(node->value[i] > 0) ? 1 : 0);
//								node->mute[i]);
						}
					}
				}

				current_node++;	
			}
			else
// Standalone mute button
			if(snd_mixer_selem_has_playback_switch(elem))
			{
				MixerNode *node;
				char *ptr = strrchr(string, ' ');

				memcpy(string2, string, ptr - string);
				strcpy(string2 + (ptr - string), " Out");
				strcat(string2, ptr);

				if(construct_tree)
				{
					node = mixer->tree->add_node(MixerNode::TYPE_TOGGLE,
						0,
						1,
						string2,
						new_title,
						channels);
				}
				else
				{
					node = mixer->tree->values[current_node];
					if(read_values)
						for(int i = 0; i < channels; i++)
						{
							snd_mixer_selem_get_playback_switch(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								&node->value[i]);
						}
					else
					if(write_values)
						for(int i = 0; i < channels; i++)
						{
							snd_mixer_selem_set_playback_switch(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								node->value[i]);
						}
				}
				current_node++;
			}
			else
// Standalone mute button
			if(snd_mixer_selem_has_capture_switch(elem))
			{
				char *ptr = strrchr(string, ' ');
				memcpy(string2, string, ptr - string);
				strcpy(string2 + (ptr - string), " In");
				strcat(string2, ptr);
				MixerNode *node;

				if(construct_tree)
				{
					node = mixer->tree->add_node(MixerNode::TYPE_TOGGLE,
						0,
						1,
						string2,
						new_title,
						channels);
					node->record = 1;
				}
				else
				{
					node = mixer->tree->values[current_node];
					if(read_values)
						for(int i = 0; i < channels; i++)
							snd_mixer_selem_get_capture_switch(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								&node->value[i]);
					else
					if(write_values)
						for(int i = 0; i < channels; i++)
							snd_mixer_selem_set_capture_switch(elem, 
								(snd_mixer_selem_channel_id_t)i, 
								node->value[i]);
				}
				current_node++;
			}
			else
			if(snd_mixer_selem_has_playback_switch_joined(elem))
			{
				printf("snd_mixer_selem_has_playback_switch_joined\n");
			}
		}







		mixer_n_selems++;
	}


// Remove suffixes from entries which aren't duplicated
	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *current_node = mixer->tree->values[i];
		int count = 0;
		for(int j = 0; j < mixer->tree->total; j++)
		{
			MixerNode *dst_node = mixer->tree->values[j];
			if(!strcmp(dst_node->title_base, current_node->title_base) &&
				dst_node->type == current_node->type)
				count++;
		}
		if(count < 2)
		{
			strcpy(current_node->title, current_node->title_base);
		}
	}

// Remove 0 from entries with only one index
	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *current_node = mixer->tree->values[i];
		char *ptr = strrchr(current_node->title, ' ');
// Got a 0 index
		if(ptr && ptr[1] == '0')
		{
			int count = 0;
// Find other entries
			for(int j = 0; j < mixer->tree->total; j++)
			{
				MixerNode *node = mixer->tree->values[j];
				if(!strncmp(node->title, 
					current_node->title, 
					ptr - current_node->title))
				{
					count++;
				}
			}
// No other entries
			if(count < 2)
				*ptr = 0;
		}
	}



	free(mixer_sid);
	snd_mixer_close(mixer_handle);
}






