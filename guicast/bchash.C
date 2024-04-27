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

#include <stdlib.h>
#include <string.h>
#include "bchash.h"
#include "bcsignals.h"
#include "filesystem.h"
#include "stringfile.h"


BC_Hash::BC_Hash()
{
}

BC_Hash::BC_Hash(const char *filename)
{
	FileSystem directory;
    char string[BCTEXTLEN];
    strcpy(string, filename);
	
	directory.parse_tildas(string);
	this->filename.assign(string);
}

BC_Hash::~BC_Hash()
{
}

void BC_Hash::clear()
{
	db.clear();
}

int BC_Hash::load()
{
	StringFile stringfile(filename.c_str());
	load_stringfile(&stringfile);
	return 0;
}

void BC_Hash::load_stringfile(StringFile *file)
{
	char key[BCTEXTLEN], value[BCTEXTLEN];
// keys previously loaded by this function with the number of occurrances
    std::map<string, int> instances;

	while(file->get_pointer() < file->get_length())
	{
		file->readline(key, value);
        int instance = -1;
        if(instances.find(key) != instances.end()) instance = instances[key];
//             printf("BC_Hash::load_stringfile %d key=%s instance=%d\n",
//                 __LINE__,
//                 key,
//                 instance);
        instance++;
        instances[key] = instance;


		update(key, value, instance);
	}
}

void BC_Hash::save_stringfile(StringFile *file)
{
    int total = size();
	for(int i = 0; i < total; i++)
	{
//printf("BC_Hash::save_stringfile %d %s %s\n", __LINE__, names[i], values[i]);
		file->writeline(get_key(i), get_value(i), 0);
	}
}

int BC_Hash::save()
{
	StringFile stringfile;
	save_stringfile(&stringfile);
	stringfile.write_to_file(filename.c_str());
	return 0;
}

int BC_Hash::load_string(const char *string)
{
	StringFile stringfile;
	stringfile.read_from_string(string);
	load_stringfile(&stringfile);
	return 0;
}

int BC_Hash::save_string(char* &string)
{
	StringFile stringfile;
	save_stringfile(&stringfile);
	string = new char[stringfile.get_length() + 1];
	memcpy(string, stringfile.string, stringfile.get_length() + 1);
	return 0;
}


const char* BC_Hash::get(const char *key, char *value, int instance)
{
    auto range = db.equal_range(key);
    int count = 0;
    for(auto i = range.first; i != range.second; i++)
    {
        if(count == instance)
        {
            string *src = &i->second;
            if(value) strcpy(value, src->c_str());
            return src->c_str();
            break;
        }
        count++;
    }
	return value;  // failed
}


int32_t BC_Hash::get(const char *name, int32_t default_)
{
    const char *value = get(name, 0, 0);
    if(value) return atoi(value);
	return default_;  // failed
}

int64_t BC_Hash::get(const char *name, int64_t default_)
{
    const char *value = get(name, 0, 0);
    if(value) 
    {
        long long temp;
        sscanf(value, "%lld", &temp);
        return temp;
    }
	return default_;  // failed
}

double BC_Hash::get(const char *name, double default_)
{
    const char *value = get(name, 0, 0);
    if(value) return atof(value);
	return default_;  // failed
}

float BC_Hash::get(const char *name, float default_)
{
    const char *value = get(name, 0, 0);
    if(value) return atof(value);
	return default_;  // failed
}

string* BC_Hash::get(const char *name, string *default_)
{
    char temp[BCTEXTLEN];
    strcpy(temp, default_->c_str());
    get(name, temp, 0);
    default_->assign(temp);
    return default_;
}



int BC_Hash::update(const char *key, const char *value, int instance)
{
    string *dst = 0;

    if(key[0] == 0 && value[0] == 0)
    {
        printf("BC_Hash::update %d called with empty key value\n", __LINE__);
        BC_Signals::dump_stack();
        return 0;
    }
// find existing instance of the key
    auto range = db.equal_range(key);
    int count = 0;
    for(auto i = range.first; i != range.second; i++)
    {
        if(count == instance)
        {
            dst = &i->second;
            break;
        }
        count++;
    }

// existing instance found.  Replace it
    if(dst)
    {
        dst->assign(value);
    }
    else
// append a new instance of the key
    {
        db.insert(std::make_pair(key, value));
    }
	return 1;
}




int BC_Hash::update(const char *name, double value) // update a value if it exists
{
	char string[BCTEXTLEN];
	sprintf(string, "%.16e", value);
	return update(name, string);
}

int BC_Hash::update(const char *name, float value) // update a value if it exists
{
	char string[BCTEXTLEN];
	sprintf(string, "%.6e", value);
	return update(name, string);
}

int32_t BC_Hash::update(const char *name, int32_t value) // update a value if it exists
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", value);
	return update(name, string);
}

int BC_Hash::update(const char *name, int64_t value) // update a value if it exists
{
	char string[BCTEXTLEN];
	sprintf(string, "%lld", (long long)value);
	return update(name, string);
}

int BC_Hash::update(const char *name, string *value)
{
    return update(name, value->c_str());
}


void BC_Hash::copy_from(BC_Hash *src)
{
    db = src->db;
}

int BC_Hash::equivalent(BC_Hash *src)
{
    auto i = db.begin();
    auto j = src->db.begin();
	while(i != db.end() && j != src->db.end())
	{
        if(i->first.compare(j->first) ||     // compare keys
            i->second.compare(j->second))    // compare values
            return 0;
	}
    if(i != db.end() || j != src->db.end()) return 0;

	return 1;
}

int BC_Hash::size()
{
	return db.size();
}

const char* BC_Hash::get_key(int number)
{
    int count = 0;
    for(auto i = db.begin(); i != db.end(); i++)
    {
// return the key text
        if(count == number) return i->first.c_str();
        count++;
    }
	return 0;
}

const char* BC_Hash::get_value(int number)
{
    int count = 0;
    for(auto i = db.begin(); i != db.end(); i++)
    {
// return the value text
        if(count == number) return i->second.c_str();
        count++;
    }
	return 0;
}




void BC_Hash::dump()
{
	printf("BC_Hash::dump\n");
    int total = size();
	for(int i = 0; i < total; i++)
		printf("	key=%s value=%s\n", get_key(i), get_value(i));
}
