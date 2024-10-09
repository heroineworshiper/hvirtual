/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef BCHASH_H
#define BCHASH_H



// Key/value table with persistent storage in stringfiles.
// 1 of the very 1st C++ developments.  It was really a dictionary.


#include "bcwindowbase.inc"
#include "stringfile.inc"
#include "units.h"
#include <string>
#include <map>

using std::string;
using std::multimap;


class BC_Hash
{
public:
	BC_Hash();
	BC_Hash(const char *filename);
	virtual ~BC_Hash();

	int load();        // load from disk file
	int save();        // save to disk file
	int load_string(const char *string);        // load from string
	int save_string(char* &string);       // save to new string
	void save_stringfile(StringFile *file);
	void load_stringfile(StringFile *file);


// common setter
// the nth instance of the key is set if instance is > 0
	int update(const char *name, const char *value, int instance = 0);
// overloaded setters
	int update(const char *name, Freq value); // update a value if it exists
	int update(const char *name, double value); // update a value if it exists
	int update(const char *name, float value); // update a value if it exists
	int update(const char *name, int32_t value); // update a value if it exists
	int update(const char *name, int64_t value); // update a value if it exists
	int update(const char *name, string *value);

// common getter
// the nth instance of the key is returned
// if *value is non zero, the value is copied into the argument
// & the pointer in the DB is returned
// returns the value argument if no instance was found
// pass 0 as the value to get 0 if no instance is found
    const char* get(const char *key, char *value, int instance = 0);
// overloaded getters
	double get(const char *name, double default_);
	float get(const char *name, float default_);
	int32_t get(const char *name, int32_t default_);
	int64_t get(const char *name, int64_t default_);
// returns the default_ argument with the new value
	string* get(const char *name, string *default_);

// Update values with values from another table.
// Adds values that don't exist and updates existing values.
	void copy_from(BC_Hash *src);
// Return 1 if the tables are equivalent
	int equivalent(BC_Hash *src);

	void dump();

    void clear();
// all keys & instances of each key
	int size();
// iterate through every key & instance of each key for saving
	const char* get_key(int number);
	const char* get_value(int number);

private:
// replace spaces
    char* fix_spaces(char *dst, const char *src);
    multimap<string, string> db;
	string filename;        // filename the defaults are stored in
};

#endif
