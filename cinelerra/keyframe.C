
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"

#include <stdio.h>
#include <string.h>



KeyFrame::KeyFrame()
 : Auto()
{
	data[0] = 0;
}

KeyFrame::KeyFrame(EDL *edl, KeyFrames *autos)
 : Auto(edl, (Autos*)autos)
{
	data[0] = 0;
}

KeyFrame::~KeyFrame()
{
}

void KeyFrame::load(FileXML *file)
{
//printf("KeyFrame::load 1\n");
// Shouldn't be necessary
//	position = file->tag.get_property((char*)"POSITION", position);
//printf("KeyFrame::load 1\n");

	file->read_text_until((char*)"/KEYFRAME", data, MESSAGESIZE);
//printf("KeyFrame::load 2 data=\n%s\nend of data\n", data);
}

void KeyFrame::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
//printf("KeyFrame::copy 1 %d\n%s\n", position - start, data);
	file->tag.set_title((char*)"KEYFRAME");
	if(default_auto)
		file->tag.set_property((char*)"POSITION", 0);
	else
		file->tag.set_property((char*)"POSITION", position - start);
// default_auto converts a default auto to a normal auto
	if(is_default && !default_auto)
		file->tag.set_property((char*)"DEFAULT", 1);
	file->append_tag();
// Can't put newlines in because these would be reimported and resaved along
// with new newlines.
//	file->append_newline();

	file->append_text(data);
//	file->append_newline();

	file->tag.set_title((char*)"/KEYFRAME");
	file->append_tag();
	file->append_newline();
}


void KeyFrame::copy_from(Auto *that)
{
	copy_from((KeyFrame*)that);
}

void KeyFrame::copy_from(KeyFrame *that)
{
	Auto::copy_from(that);
	KeyFrame *keyframe = (KeyFrame*)that;
	strcpy(data, keyframe->data);
	position = keyframe->position;
}

void KeyFrame::copy_data(KeyFrame *src)
{
	strcpy(data, src->data);
}

int KeyFrame::identical(KeyFrame *src)
{
	return !strcasecmp(src->data, data);
}


void KeyFrame::get_contents(BC_Hash *ptr, char **text, char **extra)
{
	FileXML input;
	input.set_shared_string(data, strlen(data));
	int result = 0;
	char *this_text = 0;
	char *this_extra = 0;
	while(!result)
	{
		result = input.read_tag();
		if(!result)
		{
			for(int i = 0; i < input.tag.total_properties; i++)
			{
				const char *key = input.tag.get_property_text(i);
				const char *value = input.tag.get_property(key);
				ptr->update(key, value);
			}

// Read any text after tag
			this_text = input.read_text();
			(*text) = strdup(this_text);

// Read remaining data
			this_extra = input.get_ptr();
			(*extra) = strdup(this_extra);
			break;
		}
	}
}

void KeyFrame::update_parameter(BC_Hash *params, 
	char *text,
	char *extra)
{
	FileXML output;
	FileXML input;
	input.set_shared_string(get_data(), strlen(get_data()));
	int result = 0;
	BC_Hash this_params;
	char *this_text = 0;
	char *this_extra = 0;
	int got_it = 0;

// printf("KeyFrame::update_parameter %d %p %p %p \n", 
// __LINE__,
// params,
// text,
// extra);


	get_contents(&this_params, &this_text, &this_extra);


// printf("KeyFrame::update_parameter %d params=%p\n", __LINE__, params);
// if(params) params->dump();
// printf("KeyFrame::update_parameter %d\n", __LINE__);
// this_params.dump();

// Get first tag
	while(!result)
	{
		result = input.read_tag();
		if(!result)
		{
// Replicate first tag
			output.tag.set_title(input.tag.get_title());

// Get each parameter from this keyframe
			for(int i = 0; i < this_params.size(); i++)
			{
				const char *key = this_params.get_key(i);
				const char *value = this_params.get_value(i);

// Get new value from the params argument
				got_it = 1;
				if(params)
				{
					got_it = 0;
					for(int j = 0; j < params->size(); j++)
					{
						if(!strcmp(params->get_key(j), key))
						{
							got_it = 1;
							value = params->get_value(j);
							break;
						}
					}
				}

// Set parameter in output.
				output.tag.set_property(key, value);
			}

// Get each parameter from params argument
			if(params)
			{
				for(int i = 0; i < params->size(); i++)
				{
					const char *key = params->get_key(i);
//printf("KeyFrame::update_parameter %d %s\n", __LINE__, key);
					
					got_it = 0;
					for(int j = 0; j < this_params.size(); j++)
					{
						if(!strcmp(this_params.get_key(j), key))
						{
							got_it = 1;
							break;
						}
					}
//printf("KeyFrame::update_parameter %d %s\n", __LINE__, key);

// If it wasn't found in output, set new parameter in output.
					if(!got_it)
					{
						output.tag.set_property(key, params->get_value(i));
//printf("KeyFrame::update_parameter %d %s\n", __LINE__, key);
					}
				}
			}



// Append parameters to output
			output.append_tag();

// Write anonymous text & duplicate the rest
			if(text)
			{
				output.append_text(text);
			}
			else
			{
				output.append_text(this_text);
			}

// Append remaining previous data
			if(extra)
			{
				output.append_text(extra);
			}
			else
			{
				output.append_text(this_extra);
			}

// Move output to input
			output.terminate_string();
			strcpy(this->data, output.string);
			break;
		}
	}

	delete [] this_text;
	delete [] this_extra;
}


void KeyFrame::get_diff(KeyFrame *src, 
	BC_Hash **params, 
	char **text, 
	char **extra)
{
	const int debug = 0;
	FileXML input;
	input.set_shared_string(data, strlen(data));
	int result = 0;
	BC_Hash this_params;
	char *this_text = 0;
	char *this_extra = 0;
	BC_Hash src_parameters;
	char *src_text = 0;
	char *src_extra = 0;

	get_contents(&this_params, &this_text, &this_extra);
	src->get_contents(&src_parameters, &src_text, &src_extra);

if(debug) printf("KeyFrame::get_diff %d %d %d\n", 
__LINE__, 
this_params.size(), 
src_parameters.size());

// Capture changed parameters
	char this_value[BCTEXTLEN];
	for(int i = 0; i < MIN(this_params.size(), src_parameters.size()); i++)
	{
		const char *src_key = src_parameters.get_key(i);
		const char *src_value = src_parameters.get_value(i);
		this_value[0] = 0;
		this_params.get(src_key, this_value);
if(debug) printf("KeyFrame::get_diff %d %s %s %s\n", 
__LINE__, 
src_key,
src_value, 
this_value);
// Capture values which differ
		if(strcmp(src_value, this_value))
		{
			if(!(*params)) (*params) = new BC_Hash;
			(*params)->update(src_key, src_value);
		}
	}


// Capture text	which differs
	if(strcmp(this_text, src_text)) (*text) = strdup(src_text);

	if(strcmp(this_extra, src_extra)) (*extra) = strdup(src_extra);

	
	delete [] this_text;
	delete [] this_extra;
	delete [] src_text;
	delete [] src_extra;
}

int KeyFrame::operator==(Auto &that)
{
	return identical((KeyFrame*)&that);
}

int KeyFrame::operator==(KeyFrame &that)
{
	return identical(&that);
}

char* KeyFrame::get_data()
{
	return data;
}

void KeyFrame::set_data(char *data)
{
	strcpy(this->data, data);
}


void KeyFrame::dump()
{
	printf("     position: %lld\n", (long long)position);
	printf("     data: %s\n", data);
}
