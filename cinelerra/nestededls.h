#ifndef NESTEDEDLS_H
#define NESTEDEDLS_H


#include "arraylist.h"
#include "edl.inc"

class NestedEDLs
{
public:
	NestedEDLs(EDL *edl);
	~NestedEDLs();


    void dump();
    
// Return copy of the src EDL which belongs to the current object.
	EDL* get_copy(EDL *src);
// Return new EDL loaded from path
	EDL* get(char *path, int *error);
// search for an EDL based on path
    EDL* search(const char *path);
	int size();
	EDL* get(int number);
// returns a flag if an EDL is recursive
    int load(FileXML *file);
	void clear();
	void update_index(EDL *nested_edl);
	void remove_edl(EDL *nested_edl);

	ArrayList<EDL*> nested_edls;
// owner of this
    EDL *edl;
};


#endif


