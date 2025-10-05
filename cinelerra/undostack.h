
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

#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include "linklist.h"
#include "stringfile.inc"

#define UNDOLEVELS 500
#define UNDO_KEY_INTERVAL 100

// The undo stack is a series of key undo buffers and
// incremental undo buffers.  The incremental buffers
// store the differences in the most compact way possible:
// a series of offsets, sizes and values.  This should allow
// a huge number of undo updates.


// even numbered entries are the before entries
// odd numbered entries are the after entries
// redo loads an after entry
// undo loads a before entry

class UndoStackItem : public ListItem<UndoStackItem>
{
public:
	UndoStackItem();
	~UndoStackItem();

// Must be inserted into the list before calling this, so it can get the
// previous key buffer.
	void set_data(const char *data);
	void set_description(char *description);
	void set_filename(const char *filename);
	const char* get_description();
	void set_flags(uint64_t flags);
    void set_modified(int modified);

// Decompress the buffers and return them in a newly allocated string.
// The string must be deleted by the user.
	char* get_data();
	char* get_filename();
	int has_data();
	int get_size();
	int is_key();
	uint64_t get_flags();
    int get_modified();
	

// Get pointer to incremental data for use in an apply_difference command.
	char* get_incremental_data();
	int get_incremental_size();

	void set_creator(void *creator);
	void* get_creator();

private:
// command description for the menu item
	char *description;

// key undo buffer or incremental undo buffer
	int key;

// type of modification
	uint64_t load_flags;
// value of changes_made
    int changes_made;
	
// data after the modification for redos
	char *data;
	int data_size;

// pointer to the object which set this undo buffer
	void *creator;

	char *session_filename;
};

class UndoStack : public List<UndoStackItem>
{
public:
	UndoStack();
	~UndoStack();
	
// Create a new undo entry and put on the stack.
// The current pointer points to the new entry.
// delete future undos if in the middle
// delete undos older than UNDOLEVELS if last
	UndoStackItem* push();

// move to the previous undo entry
	void pull();


// move to the next undo entry for a redo
	UndoStackItem* pull_next();

	void dump();
	
	UndoStackItem* current;
};

#endif
