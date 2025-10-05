
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILEXML_H
#define FILEXML_H

#include "bcwindowbase.inc"
#include "sizes.h"
#include <stdio.h>
#include <string>
#include <map>

#define MAX_TITLE 1024
#define MAX_PROPERTIES 1024
#define MAX_LENGTH 4096
using std::string;
using std::multimap;


class XMLTag
{
public:
	XMLTag();
	~XMLTag();

	int set_delimiters(char left_delimiter, char right_delimiter);
	int reset_tag();     // clear all structures

	int read_tag(const char *input, int &position, int length);

	int title_is(const char *title);        // test against title and return 1 if they match
	char *get_title();
	int get_title(char *value);
//	int test_property(char *property, char *value);
//	const char *get_property_text(int number);
// iterate through the raw map data
    int total_properties();
	const char *get_key(int number);
    const char *get_value(int number);
// get the formatted map data
	int get_property_int(int number);
	float get_property_float(int number);
//	char *get_property(const char *property);
	const char* get_property(const char *property, char *value);
	int32_t get_property(const char *property, int32_t default_);
	int64_t get_property(const char *property, int64_t default_);
//	int get_property(const char *property, int default_);
	float get_property(const char *property, float default_);
	double get_property(const char *property, double default_);
	const char* get_value(const char *key);
	const char* get_property(const char *name, string *value);

	void set_title(const char *text);       // set the title field
	void set_property(const char *text, const char *value);
	void set_property(const char *text, int32_t value);
	void set_property(const char *text, int64_t value);
//	int set_property(const char *text, int value);
	void set_property(const char *text, float value);
	void set_property(const char *text, double value);
    int has_property(const char *text);
	int write_tag();

// encode the special character at the head of the string
// TODO: move to FileXML
 	static const char* encode_char(char *temp_string, char c);

// convert all the encodings to special characters
    void decode_text(char *text);

	char tag_title[MAX_TITLE];       // title of this tag


// key, value list of properties for this tag
    multimap<string, string> properties;

    std::string text;
	char temp1[BCTEXTLEN];
	char temp2[BCTEXTLEN];
	char left_delimiter, right_delimiter;
};


class FileXML
{
public:
	FileXML();
	FileXML(char left_delimiter, char right_delimiter);
	~FileXML();

	void dump();
	int terminate_string();         // append the terminal 0
	int append_newline();       // append a newline to string
	int append_tag();           // append tag object
// add generic text to the string
	int append_text(const char *text);
// append text with special characters
    void encode_text(const char *text);

// read text, put it in *output, and return it
// decode - decode special characters.
// Text array is dynamically allocated and deleted when FileXML is deleted
	const char* read_text(int decode = 1);
	void read_text_until(const char *tag_end, std::string *output);     // store text in output until the tag is reached
	int read_tag();          // read next tag from file, ignoring any text, and put it in tag
	// return 1 on failure

	int write_to_file(const char *filename);           // write the file to disk
	int write_to_file(FILE *file);           // write the file to disk
	int read_from_file(const char *filename, int ignore_error = 0);          // read an entire file from disk
	int read_from_string(char *string);          // read from a string

// use text object owned by someone else
    void set_shared_string(std::string *shared_string);
	int rewind();
// Get current pointer in the string
	const char* get_ptr();
// get the string
    const char* get_text();
    int get_len();

	XMLTag tag;
	std::string filename;  // Filename used in the last read_from_file or write_to_file

private:
	std::string *text;      // string that contains the actual file
	int shared;      // text object is owned by someone else so don't delete
	int position;    // current position in text object
//	long length;      // length of string file for reading - terminating 0
//	long available;    // possible length before reallocation

    std::string output; // temporary for decoding
//	char *output;       
//	long output_length;
	char left_delimiter, right_delimiter;
};

#endif
