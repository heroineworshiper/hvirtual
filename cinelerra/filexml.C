/*
 * CINELERRA
 * Copyright (C) 2008-2025 Adam Williams <broadcast at earthling dot net>
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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcsignals.h"
#include "filexml.h"




// Precision in base 10
// for float is 6 significant figures
// for double is 16 significant figures

FileXML::FileXML()
{
	left_delimiter = '<';
	right_delimiter = '>';
	tag.set_delimiters(left_delimiter, right_delimiter);
    text = new std::string;
    shared = 0;
	position = 0;
}

FileXML::FileXML(char left_delimiter, char right_delimiter)
{
	tag.set_delimiters(left_delimiter, right_delimiter);
	this->left_delimiter = left_delimiter;
	this->right_delimiter = right_delimiter;
    text = new std::string;
    shared = 0;
	position = 0;
}

FileXML::~FileXML()
{
    if(!shared) delete text;
}

void FileXML::dump()
{
	printf("FileXML::dump:\n%s\n", text->c_str());
}

int FileXML::terminate_string()
{
	append_text("");
	return 0;
}

int FileXML::rewind()
{
	terminate_string();
	position = 0;
	return 0;
}


int FileXML::append_newline()
{
	append_text("\n");
	return 0;
}

int FileXML::append_tag()
{
	tag.write_tag();
	append_text(tag.text.c_str());
	tag.reset_tag();
	return 0;
}

int FileXML::append_text(const char *text)
{
// started writing to a string that wasn't empty
    if(!this->text->empty() && position == 0) this->text->clear();

    this->text->append(text);
    position += strlen(text);
    
	return 0;
}

void FileXML::encode_text(const char *src)
{
    char temp_string[32];       // for converting numbers
    int length = strlen(src);
    for(int i = 0; i < length; i++)
    {
        const char *encoded = XMLTag::encode_char(temp_string, src[i]);
        append_text(encoded);
    }
}


const char* FileXML::get_ptr()
{
	return text->c_str() + position;
}

const char* FileXML::read_text(int decode)
{
	int text_position = position;
	int i;

// use < to mark end of text and start of tag

// find end of text
    int length = text->length();
	for(; position < length && text->at(position) != left_delimiter; position++)
	{
		;
	}

// allocate enough space
//	if(output_length) delete [] output;
    int output_length = position - text_position;
    output.clear();
//	output = new char[output_length + 1];

//printf("FileXML::read_text %d %c\n", text_position, string[text_position]);
	for(i = 0; text_position < position; text_position++)
	{
// filter out leading newlines
		if((i > 0 && i < output_length - 1) || text->at(text_position) != '\n') 
		{
// check if we have to decode special characters
// but try to be most backward compatible possible
			int character = text->at(text_position);
			if (character == '&' && decode)
			{
				if (text_position + 3 < length)
				{
                    if(text->at(text_position + 1) == '#' && 
                        text->at(text_position + 2) == 'x' && 
                        text->at(text_position + 3) == 'A')
                    {
                        character = '\n';
						text_position += 3;
                    }
                    else
					if (text->at(text_position + 1) == 'l' && 
                        text->at(text_position + 2) == 't' && 
                        text->at(text_position + 3) == ';')
					{
						character = '<';
						text_position += 3;
					}
                    else
					if (text->at(text_position + 1) == 'g' && 
                        text->at(text_position + 2) == 't' && 
                        text->at(text_position + 3) == ';')
					{
						character = '>';
						text_position += 3;
					}
				}
				if (text_position + 4 < length)
				{
					if (text->at(text_position + 1) == 'a' && 
                        text->at(text_position + 2) == 'm' && 
                        text->at(text_position + 3) == 'p' && 
                        text->at(text_position + 4) == ';')
					{
						character = '&';
						text_position += 4;
					}		
				}
                if(text_position + 5 < length)
                {
                    if (text->at(text_position + 1) == 'q' && 
                        text->at(text_position + 2) == 'u' && 
                        text->at(text_position + 3) == 'o' && 
                        text->at(text_position + 4) == 't' && 
                        text->at(text_position + 5) == ';')
					{
						character = '\"';
						text_position += 5;
					}
                }
			}
//			output[i] = character;
            output.push_back(character);
			i++;
		}
	}
//	output[i] = 0;

	return output.c_str();
}

int FileXML::read_tag()
{
// scan to next tag
    int length = text->length();
//printf("FileXML::read_tag %d length=%d position=%d left_delimiter=%c text=%s\n", 
//__LINE__, length, position, left_delimiter, text->c_str());
	while(position < length && text->at(position) != left_delimiter)
	{
		position++;
	}
	tag.reset_tag();
	if(position >= length) return 1;
//printf("FileXML::read_tag %d length=%d position=%d\n", __LINE__, length, position);
	return tag.read_tag(text->c_str(), position, length);
}

void FileXML::read_text_until(const char *tag_end, std::string *output)
{
// read to next tag
	int out_position = 0;
	int test_position1, test_position2;
	int result = 0;
	
    int length = text->length();
    output->clear();
	while(!result && position < length)
	{
		while(position < length && text->at(position) != left_delimiter)
		{
//printf("FileXML::read_text_until 1 %c\n", string[position]);
			output->push_back(text->at(position++));
		}
		
		if(position < length && text->at(position) == left_delimiter)
		{
// tag reached
// test for tag_end
			result = 1;         // assume end
			
			for(test_position1 = 0, test_position2 = position + 1;   // skip < 
				test_position2 < length &&
				tag_end[test_position1] != 0 &&
				result; 
				test_position1++, test_position2++)
			{
// null result when first wrong character is reached
//printf("FileXML::read_text_until 2 %c\n", string[test_position2]);
				if(tag_end[test_position1] != text->at(test_position2)) 
                    result = 0;
			}

// no end tag reached to copy <
			if(!result)
			{
				output->push_back(text->at(position++));
			}
		}
	}
// if end tag is reached, position is left on the < of the end tag
}


int FileXML::write_to_file(const char *filename)
{
	FILE *out;
	this->filename.assign(filename);
	if(out = fopen(filename, "wb"))
	{
		fprintf(out, "<?xml version=\"1.0\"?>\n");
// Position may have been rewound after storing so we use a strlen
        int length = text->length();
		if(length > 0 && !fwrite(text->c_str(), length, 1, out))
		{
			fprintf(stderr, "FileXML::write_to_file %d \"%s\": %s\n",
				__LINE__,
				filename,
				strerror(errno));
			fclose(out);
			return 1;
		}
		else
		{
		}
	}
	else
	{
		fprintf(stderr, "FileXML::write_to_file %d \"%s\": %s\n",
			__LINE__,
			filename,
			strerror(errno));
		return 1;
	}
	fclose(out);
	return 0;
}

int FileXML::write_to_file(FILE *file)
{
	filename.assign("");
	fprintf(file, "<?xml version=\"1.0\"?>\n");
// Position may have been rewound after storing
    int length = text->length();
	if((length == 0) || fwrite(text->c_str(), length, 1, file))
	{
		return 0;
	}
	else
	{
		fprintf(stderr, "FileXML::write_to_file \"%s\": %s\n",
			filename.c_str(),
			strerror(errno));
		return 1;
	}
	return 0;
}

int FileXML::read_from_file(const char *filename, int ignore_error)
{
	FILE *in;
	
	this->filename.assign(filename);
	if(in = fopen(filename, "rb"))
	{
		fseek(in, 0, SEEK_END);
		int new_length = ftell(in);
		fseek(in, 0, SEEK_SET);
        text->reserve(new_length + 1);
        text->resize(new_length);
		int temp = fread(&(*text)[0], 1, new_length, in);
		(*text)[new_length] = 0;
// printf("FileXML::read_from_file %d filename=%s length=%d temp=%d text=%s\n", 
// __LINE__, filename, (int)text->length(), temp, text->c_str());
		position = 0;
	    fclose(in);
	}
	else
	{
		if(!ignore_error) 
			fprintf(stderr, "FileXML::read_from_file \"%s\" %s\n",
				filename,
				strerror(errno));
		return 1;
	}
	return 0;
}

int FileXML::read_from_string(char *string)
{
	this->filename.assign("");
    this->text->assign(string);
	position = 0;
	return 0;
}

void FileXML::set_shared_string(std::string *shared_string)
{
	if(!this->shared)
	    delete text;

	this->filename.assign("");
    shared = 1;
    this->text = shared_string;
	position = 0;
}

const char* FileXML::get_text()
{
    return text->c_str();
}

int FileXML::get_len()
{
    return text->length();
}


// ================================ XML tag


XMLTag::XMLTag()
{
}

XMLTag::~XMLTag()
{
	reset_tag();
}

int XMLTag::set_delimiters(char left_delimiter, char right_delimiter)
{
	this->left_delimiter = left_delimiter;
	this->right_delimiter = right_delimiter;
	return 0;
}

int XMLTag::reset_tag()     // clear all structures
{
    text.clear();
    properties.clear();
	return 0;
}

int XMLTag::write_tag()
{
	int need_quote = 0;

// opening bracket
	text.push_back(left_delimiter);

// title
    text.append(tag_title);

// iterate through the properties
    for(auto i = properties.begin(); i != properties.end(); i++)
	{
		text.push_back(' '); // add a space before every property
// write the key		
        text.append(i->first);
		text.push_back('=');

// write the value
        int len = i->second.length();
		if(len == 0 ||
            i->second.find(' ') != std::string::npos) need_quote = 1;

// add a quote
		if(need_quote) text.push_back('\"');
// write the value
		for(int j = 0; j < len; j++)
		{
            const char *encoded = encode_char(temp1, i->second.at(j));
//if(encoded[0] == '&') 
//    printf("XMLTag::write_tag %d %s\n", __LINE__, encoded);
            text.append(encoded);
		}
//printf("XMLTag::write_tag %d %s %s\n", 
//__LINE__, i->second.c_str(), text.c_str());


// add a quote
		if(need_quote) text.push_back('\"');
	}     // next property
	
	text.push_back(right_delimiter);   // terminating bracket
	return 0;
}

int XMLTag::read_tag(const char *input, int &position, int length)
{
	int tag_start;
	int i, j, terminating_char;
//printf("XMLTag::read_tag %d position=%d length=%d left_delimiter=%d\n", 
//__LINE__, position, length, left_delimiter);

// search for beginning of a tag
	while(position < length && input[position] != left_delimiter) position++;
//printf("XMLTag::read_tag %d position=%d length=%d\n", 
//__LINE__, position, length);

	if(position >= length) return 1;

// find the start
	while(position < length &&
		(input[position] == ' ' ||         // skip spaces
		input[position] == '\n' ||	 // also skip new lines
		input[position] == left_delimiter))           // skip <
		position++;
//printf("XMLTag::read_tag %d position=%d length=%d\n", 
//__LINE__, position, length);

	if(position >= length) return 1;

	tag_start = position;

// read title
    tag_title.clear();
	for(i = 0; 
		position < length && 
		input[position] != '=' && 
		input[position] != ' ' &&       // space ends title
		input[position] != right_delimiter;
		position++, i++)
	{
		tag_title.push_back(input[position]);
	}

//printf("XMLTag::read_tag %d %s\n", __LINE__, tag_title.c_str());
	if(position >= length) return 1;

	if(input[position] == '=')
	{
// no title but first key
		tag_title.clear();
		position = tag_start;       // rewind
	}

// read properties
	for(i = 0;
		position < length &&
		input[position] != right_delimiter;
		i++)
	{
// read a tag
// find the start
		while(position < length &&
			(input[position] == ' ' ||         // skip spaces
			input[position] == '\n' ||         // also skip new lines
			input[position] == left_delimiter))           // skip <
			position++;

// read the key
		for(j = 0; 
			j < BCTEXTLEN &&
			position < length &&
			input[position] != right_delimiter &&
			input[position] != ' ' &&
			input[position] != '\n' &&	// also new line ends it
			input[position] != '=';
			j++, position++)
		{
			temp1[j] = input[position];
		}
		temp1[j] = 0;


// find the start of the value
		while(position < length &&
			(input[position] == ' ' ||         // skip spaces
			input[position] == '\n' ||         // also skip new lines
			input[position] == '='))           // skip =
			position++;

// find the terminating char
		if(position < length && input[position] == '\"')
		{
			terminating_char = '\"';     // use quotes to terminate
			if(position < length) position++;   // don't store the quote itself
		}
		else 
			terminating_char = ' ';         // use space to terminate

// read until the terminating char
		for(j = 0;
			j < BCTEXTLEN &&
			position < length &&
			input[position] != right_delimiter &&
			input[position] != '\n' &&
			input[position] != terminating_char;
			j++, position++)
		{
			temp2[j] = input[position];
		}
		temp2[j] = 0;

// store the value in a property array
        decode_text(temp2);

// store property if it had a key
        if(temp1[0])
            properties.insert(std::make_pair(temp1, temp2));

// get the terminating char
		if(position < length && input[position] != right_delimiter) position++;
	}

// skip the >
	if(position < length && input[position] == right_delimiter) position++;

	if(properties.size() || !tag_title.empty()) 
		return 0; 
	else 
		return 1;
	return 0;
}

int XMLTag::title_is(const char *title)
{
	if(!strcasecmp(title, tag_title.c_str())) return 1;
	else return 0;
}

const char* XMLTag::get_title()
{
	return tag_title.c_str();
}

int XMLTag::get_title(char *value)
{
	if(!tag_title.empty()) strcpy(value, tag_title.c_str());
	return 0;
}

int XMLTag::has_property(const char *key)
{
    return properties.find(key) != properties.end();
}

// int XMLTag::test_property(char *property, char *value)
// {
// 	int i, result;
// 	for(i = 0, result = 0; i < total_properties && !result; i++)
// 	{
// 		if(!strcasecmp(tag_properties[i], property) && 
//             !strcasecmp(value, tag_property_values[i]))
// 		{
// 			return 1;
// 		}
// 	}
// 	return 0;
// }

const char* XMLTag::get_property(const char *key, char *value)
{
    auto i = properties.find(key);
    if(i != properties.end())
        strcpy((char*)value, i->second.c_str());
	return value;
}

const char* XMLTag::get_property(const char *key, std::string *value)
{
    auto i = properties.find(key);
    if(i != properties.end())
        *value = i->second;
	return value->c_str();
}

const char* XMLTag::get_value(const char *key)
{
    auto i = properties.find(key);
    if(i != properties.end())
        return i->second.c_str();
	return "";
}

int XMLTag::total_properties()
{
    int count = 0;
    for(auto i = properties.begin(); i != properties.end(); i++)
        count++;
    return count;
}

const char* XMLTag::get_key(int number)
{
    int count = 0;
    for(auto i = properties.begin(); i != properties.end(); i++)
    {
// return the key text
        if(count == number) return i->first.c_str();
        count++;
    }
	return "";
}

const char* XMLTag::get_value(int number)
{
    int count = 0;
    for(auto i = properties.begin(); i != properties.end(); i++)
    {
// return the value text
        if(count == number) return i->second.c_str();
        count++;
    }
	return "";
}

int XMLTag::get_property_int(int number)
{
// 0 if value is ""
	return atoi(get_value(number));
}

float XMLTag::get_property_float(int number)
{
	return atof(get_value(number));
}

// char* XMLTag::get_property(const char *property)
// {
//     
// 	int i, result;
// 	for(i = 0, result = 0; i < total_properties && !result; i++)
// 	{
// 		if(!strcasecmp(tag_properties[i], property))
// 		{
// 			return tag_property_values[i];
// 		}
// 	}
// 	return 0;
// }


int32_t XMLTag::get_property(const char *property, int32_t default_)
{
	temp1[0] = 0;
	get_property(property, temp1);
	if(temp1[0] == 0) 
		return default_;
	else 
		return atoi(temp1);
}

int64_t XMLTag::get_property(const char *property, int64_t default_)
{
	int64_t result;
	temp1[0] = 0;
	get_property(property, temp1);
	if(temp1[0] == 0) 
		result = default_;
	else 
	{
		long long temp;
		sscanf(temp1, "%lld", &temp);
		result = temp;
	}
	return result;
}
// 
// int XMLTag::get_property(char *property, int default_)
// {
// 	temp_string[0] = 0;
// 	get_property(property, temp_string);
// 	if(temp_string[0] == 0) return default_;
// 	else return atol(temp_string);
// }
// 
float XMLTag::get_property(const char *property, float default_)
{
	temp1[0] = 0;
	get_property(property, temp1);
	if(temp1[0] == 0) 
		return default_;
	else 
		return atof(temp1);
}

double XMLTag::get_property(const char *property, double default_)
{
	temp1[0] = 0;
	get_property(property, temp1);
	if(temp1[0] == 0) 
		return default_;
	else 
		return atof(temp1);
}

void XMLTag::set_title(const char *text)       // set the title field
{
	tag_title.assign(text);
}

void XMLTag::set_property(const char *text, int32_t value)
{
	sprintf(temp1, "%d", value);
	set_property(text, temp1);
}

void XMLTag::set_property(const char *text, int64_t value)
{
	sprintf(temp1, "%lld", (long long)value);
	set_property(text, temp1);
}

void XMLTag::set_property(const char *text, float value)
{
	if (value - (float)((int64_t)value) == 0)
		sprintf(temp1, "%lld", (long long)value);
	else
		sprintf(temp1, "%.6e", value);
	set_property(text, temp1);
}

void XMLTag::set_property(const char *text, double value)
{
	if (value - (double)((int64_t)value) == 0)
		sprintf(temp1, "%lld", (long long)value);
	else
		sprintf(temp1, "%.16e", value);
	set_property(text, temp1);
}

void XMLTag::set_property(const char *key, const char *value)
{
    auto i = properties.find(key);
    if(i != properties.end())
        i->second.assign(value);
    else
        properties.insert(std::make_pair(key, value));
}



const char* XMLTag::encode_char(char *temp_string, char c)
{
// const without static doesn't guarantee it's available for a return value
    static const char newline[] = "&#xA";
	static const char leftb[] = "&lt;";
	static const char rightb[] = "&gt;";
	static const char amp[] = "&amp;";
    static const char quote[] = "&quot;";
	const char *replacement = 0;

	switch (c) {
        case '\n': replacement = newline; break;
		case '<': replacement = leftb; break;
		case '>': replacement = rightb; break;
		case '&': replacement = amp; break;
		case '\"': replacement = quote; break;
		default: replacement = 0; break;
	}

	if(replacement) return replacement;
    
    
    temp_string[0] = c;
    temp_string[1] = 0;
	return temp_string;
}

void XMLTag::decode_text(char *text)
{
    int len = strlen(text);
    int out = 0;
    for(int in = 0; in < len; in++, out++)
    {
        int c = text[in];
        if(c == '&')
        {
            if(in + 3 < len && 
                text[in + 1] == 'l' && 
                text[in + 2] == 't' && 
                text[in + 3] == ';')
            {
                c = '<';
                in += 3;
            }
            else
            if(in + 3 < len && 
                text[in + 1] == 'g' && 
                text[in + 2] == 't' && 
                text[in + 3] == ';')
            {
                c = '>';
                in += 3;
            }
            else
            if(in + 4 < len && 
                text[in + 1] == 'a' && 
                text[in + 2] == 'm' && 
                text[in + 3] == 'p' && 
                text[in + 4] == ';')
            {
                c = '&';
                in += 4;
            }
            else
            if(in + 5 < len && 
                text[in + 1] == 'q' && 
                text[in + 2] == 'u' && 
                text[in + 3] == 'o' && 
                text[in + 4] == 't' && 
                text[in + 5] == ';')
            {
                c = '\"';
                in += 5;
            }
        }
        text[out] = c;
    }
    text[out] = 0;
}
