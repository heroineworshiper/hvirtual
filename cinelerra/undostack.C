
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

#include "bcsignals.h"
#include "bctimer.h"
#include "clip.h"
#include "stringfile.h"
#include "undostack.h"
#include <string.h>

UndoStack::UndoStack() : List<UndoStackItem>()
{
	current = 0;
}

UndoStack::~UndoStack()
{
}

UndoStackItem* UndoStack::push()
{
// current is only 0 if before first undo
	if(current)
		current = insert_after(current);
	else
		current = insert_before(first);
	
// delete future undos if necessary
	if(current && current->next)
	{
		while(current->next) remove(last);
	}

// delete oldest 2 undos if necessary
	if(total() > UNDOLEVELS)
	{
		for(int i = 0; i < 2; i++)
		{
			UndoStackItem *second = first->next;
			char *temp_data = 0;


			if(!second->is_key())
			{
				temp_data = second->get_data();
			}
			remove(first);

// Convert new first to key buffer.
			if(!second->is_key())
			{
				second->set_data(temp_data);
			}
			delete [] temp_data;
		}
	}
	
	return current;
}

void UndoStack::pull()
{
	if(current) current = PREVIOUS;
}

UndoStackItem* UndoStack::pull_next()
{
// use first entry if none
	if(!current)
		current = first;
	else
// use next entry if there is a next entry
	if(current->next)
		current = NEXT;
// don't change current if there is no next entry
	else
		return 0;
		
	return current;
}


void UndoStack::dump()
{
	printf("UndoStack::dump\n");
	UndoStackItem *current = last;
	int i = 0;
// Dump most recent
	while(current && i < 10)
	{
		printf("  %d %p %s %c\n", 
			i++, 
			current, 
			current->get_description(), 
			current == this->current ? '*' : ' ');
		current = PREVIOUS;
	}
}



















// These difference routines are straight out of the Heroinediff/Heroinepatch 
// utilities.






// We didn't want to use the diff program because that isn't efficient enough
// to compress EDL's.  This program also handles binary data.

// Algorithm 1:
// Search from beginning for first difference.
// Count everything up to first difference as equal.
// Determine length of different block by searching for transposition with most
// similar characters.
// For every byte in different block, search for occurrence of 
// transposed copy longer than a record header in remaining old buffer.
// If no match is found, add 1 to the different block length and try the next
// byte.  Once the most similar match is found, store the changed data leading up to it in the
// difference record.
// This can be real slow if there is a large transposition early in the file.

// Algorithm 2:
// Search backwards from end of both files for last difference.
// Count all data from last same bytes as a transposition.
// Search forwards from start of both files until last same bytes 
// for differences.








// Minimum size of transposed block.
// Smaller transpositions should be included in the same difference record.
#define MIN_TRANS 16

// Format for difference records.
// The first 4 bytes are the offset of the change.
// The next 4 bytes are the number of bytes of new data.
// The next 4 bytes are the number of bytes of old data replaced.
// The new data follows.
static void append_record(unsigned char **result, 
	int *result_size, 
	unsigned char *new_data,
	int new_offset,
	int new_size,
	int old_size)
{
	if(new_size || old_size)
	{
		int record_size = new_size + 12;
		unsigned char *new_result = new unsigned char[(*result_size) + record_size];
		memcpy(new_result, (*result), (*result_size));
		delete [] (*result);

		unsigned char *new_result_ptr = new_result + (*result_size);
		*(int32_t*)new_result_ptr = new_offset;
		new_result_ptr += 4;
		*(int32_t*)new_result_ptr = new_size;
		new_result_ptr += 4;
		*(int32_t*)new_result_ptr = old_size;
		new_result_ptr += 4;

		memcpy(new_result_ptr, new_data, new_size);
		(*result_size) += record_size;
		(*result) = new_result;
	}
}



// Get number of consecutive characters from starting point which are
// identical.
static int fastcompare(unsigned char *ptr1, 
	unsigned char *ptr2, 
	unsigned char *end1, 
	unsigned char *end2)
{
	int result = 0;
	while(ptr1 < end1 && ptr2 < end2)
	{
		if(*ptr1++ == *ptr2++) 
			result++;
		else
			break;
	}
	return result;
}





// Create a buffer containing the differences between the two arguments.
// The new buffer must be deleted by the user.
// The first 4 bytes of the buffer are the size of the new buffer, in case it's
// shorter than the previous buffer.
static unsigned char* get_difference(unsigned char *before, 
		int before_len,
		unsigned char *after,
		int after_len,
		int *result_len,
		int verbose)
{
	unsigned char *result = new unsigned char[4];
	*result_len = 4;

// Store size of new buffer
	*(int32_t*)result = after_len;

	unsigned char *before_end = before + before_len;
	unsigned char *after_end = after + after_len;
	unsigned char *before_ptr = before;
	unsigned char *after_ptr = after;
	int done = 0;

// Scan forward for first difference
	while(!done)
	{
		if(before_ptr < before_end && 
			after_ptr < after_end)
		{
// Both characters equal
			if(*before_ptr == *after_ptr)
			{
				before_ptr++;
				after_ptr++;			
			}
			else
// Characters differ
			{
// Get length of difference by calculating the similarity of every possible
// transposition after it.
				int size = 0;
				unsigned char *before_difference_start = before_ptr;
				unsigned char *after_difference_start = after_ptr;
// Start of most similar transpositions
				unsigned char *before_difference_end = before_end;
				unsigned char *after_difference_end = after_end;
				int done2 = 0;

				int most_similar = 0;

// Assume after_ptr is the start of a transposed block.
				for( ;
					after_ptr < after_end - MIN_TRANS &&
					after_end - after_ptr > most_similar; )
				{
// Scan for transposed after text in remaining before text
					for(before_ptr = before_difference_start;
						before_ptr < before_end - MIN_TRANS &&
						before_end - before_ptr > most_similar; )
					{
// Got a match.
// The start of the match is the end of the difference.
						int comparison;
						if((comparison = fastcompare(before_ptr, 
							after_ptr, 
							before_end, 
							after_end)) > most_similar)
						{
							most_similar = comparison;
							before_difference_end = before_ptr;
							after_difference_end = after_ptr;
// printf("after_ptr=%d before_ptr=%d comparison=%d\n",
// after_ptr - after_difference_start,
// before_ptr - before_difference_start,
// comparison);
						}

// Test only lines
						before_ptr++;
						while(before_ptr < before_end - MIN_TRANS &&
							before_end - before_ptr > most_similar &&
							*before_ptr != '\n')
							before_ptr++;
					}

// Test only lines
					after_ptr++;
					while(after_ptr < after_end - MIN_TRANS &&
						after_end - after_ptr > most_similar &&
						*after_ptr != '\n')
						after_ptr++;
				}




// Test every offset in current lines for closest match.
				unsigned char *new_before_difference_end = before_difference_end;
				unsigned char *new_after_difference_end = after_difference_end;
				for(after_ptr = after_difference_start;
					after_ptr < after_difference_end && 
					after_end - after_ptr > most_similar;
					after_ptr++)
				{
					for(before_ptr = before_difference_start;
						before_ptr < before_difference_end &&
						before_end - before_ptr > most_similar; 
						before_ptr++)
					{
						int comparison;
						if((comparison = fastcompare(before_ptr,
							after_ptr,
							before_end,
							after_end)) > most_similar)
						{
							most_similar = comparison;
							new_before_difference_end = before_ptr;
							new_after_difference_end = after_ptr;
						}
					}
				}

				
				after_difference_end = new_after_difference_end;
				before_difference_end = new_before_difference_end;





				int after_start = after_difference_start - after;
				int before_start = before_difference_start - before;
				int after_len = after_difference_end - after_difference_start;
				int before_len = before_difference_end - before_difference_start;

				if(verbose)
				{
					char string[1024];
					memcpy(string, after_difference_start, MIN(after_len, 40));
					string[MIN(after_len, 40)] = 0;
					printf("after_offset=0x%x before_offset=0x%x after_size=%d before_size=%d\n", 
					after_start, 
					before_start,
					after_len,
					before_len);
				}


// Create difference record
				append_record(&result,
					result_len,
					after_difference_start,
					after_start,
					after_len,
					before_len);
				before_ptr = before_difference_end;
				after_ptr = after_difference_end;
			}
		}
		else
		if(after_ptr < after_end)
		{
// All data after this point is different
			append_record(&result, 
				result_len, 
				after_ptr,
				after_ptr - after,
				after_end - after_ptr,
				before_end - before_ptr);
			done = 1;
		}
		else
		{
			done = 1;
		}
	}
	return result;
}









static unsigned char* get_difference_fast(unsigned char *before, 
		int before_len,
		unsigned char *after,
		int after_len,
		int *result_len,
		int verbose)
{
	unsigned char *result = new unsigned char[4];
	*result_len = 4;

// Store size of new buffer
	*(int32_t*)result = after_len;



// Get last different bytes
	unsigned char *last_difference_after = after + after_len - 1;
	unsigned char *last_difference_before = before + before_len - 1;
	while(1)
	{
		if(last_difference_after < after || 
			last_difference_before < before) break;
		if(*last_difference_after != *last_difference_before)
		{
			break;
		}
		last_difference_after--;
		last_difference_before--;
	}
	last_difference_after++;
	last_difference_before++;






	int done = 0;
	unsigned char *before_ptr = before;
	unsigned char *after_ptr = after;
	unsigned char *before_end = before + before_len;
	unsigned char *after_end = after + after_len;

// Scan forward for first difference
	while(!done)
	{
		if(before_ptr < before_end && 
			after_ptr < after_end)
		{
// Both characters equal
			if(*before_ptr == *after_ptr &&
				before_ptr < last_difference_before &&
				after_ptr < last_difference_after)
			{
				before_ptr++;
				after_ptr++;			
			}
			else
// Characters differ
			{
// Get length of difference.
				unsigned char *before_difference_start = before_ptr;
				unsigned char *after_difference_start = after_ptr;


				while(*before_ptr != *after_ptr &&
					before_ptr < last_difference_before &&
					after_ptr < last_difference_after)
				{
					before_ptr++;
					after_ptr++;
				}

// Finished comparing if either pointer hits its last difference
				if(before_ptr >= last_difference_before ||
					after_ptr >= last_difference_after)
				{
					done = 1;
					before_ptr = last_difference_before;
					after_ptr = last_difference_after;
				}



				int after_start = after_difference_start - after;
				int before_start = before_difference_start - before;
				int after_len = after_ptr - after_difference_start;
				int before_len = before_ptr - before_difference_start;

				if(verbose)
				{
					char string[1024];
					memcpy(string, after_difference_start, MIN(after_len, 40));
					string[MIN(after_len, 40)] = 0;
					printf("after_offset=0x%x before_offset=0x%x after_size=%d before_size=%d \"%s\"\n", 
						after_start, 
						before_start,
						after_len,
						before_len,
						string);
				}


// Create difference record
				append_record(&result,
					result_len,
					after_difference_start,
					after_start,
					after_len,
					before_len);
			}
		}
		else
			done = 1;
	}
	return result;
}









static void get_record(unsigned char **patch_ptr,
	unsigned char *patch_end,
	unsigned char **after_data,
	int *after_offset,
	int *after_size,
	int *before_size)
{
	(*after_data) = 0;
	if((*patch_ptr) < patch_end)
	{
		(*after_offset) = *(int32_t*)(*patch_ptr);
		(*patch_ptr) += 4;
		(*after_size) = *(int32_t*)(*patch_ptr);
		(*patch_ptr) += 4;
		(*before_size) = *(int32_t*)(*patch_ptr);
		(*patch_ptr) += 4;

		(*after_data) = (*patch_ptr);
		(*patch_ptr) += (*after_size);
	}
}





static unsigned char* apply_difference(unsigned char *before, 
		int before_len,
		unsigned char *patch,
		int patch_len,
		int *result_len)
{
	unsigned char *patch_ptr = patch;
	*result_len = *(int32_t*)patch_ptr;
	patch_ptr += 4;
	unsigned char *patch_end = patch + patch_len;
	unsigned char *before_end = before + before_len;

	unsigned char *result = new unsigned char[*result_len];
	unsigned char *result_ptr = result;
	unsigned char *result_end = result + *result_len;
	unsigned char *before_ptr = before;

	int done = 0;
	while(!done)
	{
		unsigned char *after_data;
		int after_offset;
		int after_size;
		int before_size;

		get_record(&patch_ptr,
			patch_end,
			&after_data,
			&after_offset,
			&after_size,
			&before_size);

		if(after_data)
		{
			int result_offset = result_ptr - result;
			if(after_offset > result_offset)
			{
				int skip_size = after_offset - result_offset;
				memcpy(result_ptr, before_ptr, skip_size);
				result_ptr += skip_size;
				before_ptr += skip_size;
			}
			memcpy(result_ptr, after_data, after_size);
			result_ptr += after_size;
			before_ptr += before_size;
		}
		else
		{
// All data from before_ptr to end of result buffer is identical
			if(result_end - result_ptr > 0)
				memcpy(result_ptr, before_ptr, result_end - result_ptr);
			done = 1;
		}
	}

	return result;
}










UndoStackItem::UndoStackItem()
 : ListItem<UndoStackItem>()
{
	description = data = 0;
	data_size = 0;
	key = 0;
	creator = 0;
	session_filename = 0;
}

UndoStackItem::~UndoStackItem()
{
	delete [] description;
	delete [] data;
	delete [] session_filename;
}

void UndoStackItem::set_description(char *description)
{
	delete [] this->description;
	this->description= 0;
	this->description = new char[strlen(description) + 1];
	strcpy(this->description, description);
}

void UndoStackItem::set_filename(char *filename)
{
	delete [] this->session_filename;
	this->session_filename = strdup(filename);
}

char* UndoStackItem::get_filename()
{
	return session_filename;
}


const char* UndoStackItem::get_description()
{
	if(description)
		return description;
	else
		return "";
}

int UndoStackItem::has_data()
{
	return data_size ? 1 : 0;
}

void UndoStackItem::set_data(char *data)
{
	delete [] this->data;
	this->data = 0;
	this->data_size = 0;

// Search for key buffer within interval
	int need_key = 1;
	UndoStackItem *current = this;
	this->key = 0;
	for(int i = 0; i < UNDO_KEY_INTERVAL && current; i++)
	{
		if(current->key && current->has_data())
		{
			need_key = 0;
			break;
		}
		else
			current = PREVIOUS;
	}

	int new_size = strlen(data) + 1;

	if(!need_key)
	{
// Reconstruct previous data for difference
		char *prev_buffer = previous->get_data();
		int prev_size = prev_buffer ? strlen(prev_buffer) + 1 : 0;
// Timer timer;
// timer.update();
// printf("UndoStackItem::set_data 1\n");
		this->data = (char*)get_difference_fast((unsigned char*)prev_buffer,
			prev_size,
			(unsigned char*)data,
			new_size,
			&this->data_size,
			0);
//printf("UndoStackItem::set_data 2 %lld\n", timer.get_difference());

// Testing
// FILE *test1 = fopen("/tmp/undo1", "w");
// fwrite(prev_buffer, prev_size, 1, test1);
// fclose(test1);
// FILE *test2 = fopen("/tmp/undo2", "w");
// fwrite(data, new_size, 1, test2);
// fclose(test2);
// FILE *test3 = fopen("/tmp/undo2.diff", "w");
// fwrite(this->data, this->data_size, 1, test3);
// fclose(test3);
// 
//printf("UndoStackItem::set_data 3 %d %d\n", new_size, this->data_size);

// Diff was bigger than original.
// Happens if a lot of tiny changes happened and the record headers
// took more space than the changes.
		if(this->data_size > new_size)
		{
			delete [] this->data;
			this->data_size = 0;
			need_key = 1;
		}
		else
		{
// Reconstruct current data from difference
			int test_size;
			char *test_buffer = (char*)apply_difference((unsigned char*)prev_buffer,
				prev_size,
				(unsigned char*)this->data,
				this->data_size,
				&test_size);
			if(test_size != new_size ||
				memcmp(test_buffer, data, test_size))
			{
// FILE *test1 = fopen("/tmp/undo1", "w");
// fwrite(prev_buffer, prev_size, 1, test1);
// fclose(test1);
// FILE *test2 = fopen("/tmp/undo2", "w");
// fwrite(data, new_size, 1, test2);
// fclose(test2);
// FILE *test3 = fopen("/tmp/undo2.diff", "w");
// fwrite(this->data, this->data_size, 1, test3);
// fclose(test3);
// FILE *test4 = fopen("/tmp/undo3", "w");
// fwrite(test_buffer, test_size, 1, test4);
// fclose(test4);

				printf("UndoStackItem::set_data: incremental undo failed!\n");
				need_key = 1;
				delete [] this->data;
				this->data_size = 0;
				this->data = 0;
			}
			delete [] test_buffer;

		}


		delete [] prev_buffer;
	}

	if(need_key)
	{
		this->key = 1;
		this->data_size = new_size;
		this->data = new char[this->data_size];
		memcpy(this->data, data, this->data_size);
	}
	return;
}



char* UndoStackItem::get_incremental_data()
{
	return data;
}

int UndoStackItem::get_incremental_size()
{
	return data_size;
}

int UndoStackItem::get_size()
{
	return data_size;
}

char* UndoStackItem::get_data()
{
// Find latest key buffer
	UndoStackItem *current = this;
	while(current && !current->key)
		current = PREVIOUS;
	if(!current)
	{
		printf("UndoStackItem::get_data: no key buffer found!\n");
		return 0;
	}

// This is the key buffer
	if(current == this)
	{
		char *result = new char[data_size];
		memcpy(result, data, data_size);
		return result;
	}

// Get key buffer
	char *current_data = current->get_data();
	int current_size = current->get_size();
	current = NEXT;
	while(current)
	{
// Do incremental updates
		int new_size;
		char *new_data = (char*)apply_difference((unsigned char*)current_data,
			current_size,
			(unsigned char*)current->get_incremental_data(),
			current->get_incremental_size(),
			&new_size);
		delete [] current_data;

		if(current == this)
			return new_data;
		else
		{
			current_data = new_data;
			current_size = new_size;
			current = NEXT;
		}
	}

// Never hit this object.
	delete [] current_data;
	printf("UndoStackItem::get_data: lost starting object!\n");
	return 0;
}




int UndoStackItem::is_key()
{
	return key;
}

void UndoStackItem::set_flags(uint64_t flags)
{
	this->load_flags = flags;
}

uint64_t UndoStackItem::get_flags()
{
	return load_flags;
}

void UndoStackItem::set_creator(void *creator)
{
	this->creator = creator;
}

void* UndoStackItem::get_creator()
{
	return creator;
}

void UndoStackItem::set_modified(int modified)
{
    this->changes_made = modified;
}

int UndoStackItem::get_modified()
{
    return changes_made;
}




