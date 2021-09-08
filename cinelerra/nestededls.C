#include "bcsignals.h"
#include "edl.h"
#include "filexml.h"
#include "indexstate.h"
#include "nestededls.h"


NestedEDLs::NestedEDLs()
{
}

NestedEDLs::~NestedEDLs()
{
	for(int i = 0; i < nested_edls.size(); i++)
	{
    	nested_edls.get(i)->Garbage::remove_user();
	}
    nested_edls.remove_all();
}

void NestedEDLs::dump()
{
	for(int i = 0; i < nested_edls.size(); i++)
    {
        printf("  %s\n", nested_edls.get(i)->path);
    }
}

int NestedEDLs::size()
{
	return nested_edls.size();
}

EDL* NestedEDLs::get(int number)
{
	return nested_edls.get(number);
}

EDL* NestedEDLs::get_copy(EDL *src)
{
	if(!src) return 0;
	for(int i = 0; i < nested_edls.size(); i++)
	{
		EDL *dst = nested_edls.get(i);
		if(!strcmp(dst->path, src->path))
		{
        	return dst;
        }
	}

	EDL *dst = new EDL;
	dst->create_objects();
	dst->copy_all(src);
	nested_edls.append(dst);
	return dst;
}

EDL* NestedEDLs::get(char *path)
{
	for(int i = 0; i < nested_edls.size(); i++)
	{
		EDL *dst = nested_edls.get(i);
		if(!strcmp(dst->path, path))
			return dst;
	}

	EDL *dst = new EDL;
	dst->create_objects();
	FileXML xml_file;
	xml_file.read_from_file(path);
//printf("NestedEDLs::get %d %s\n", __LINE__, path);
	dst->load_xml(&xml_file, LOAD_ALL);

// Override path EDL was saved to with the path it was loaded from.
	dst->set_path(path);
	nested_edls.append(dst);
	return dst;
}

void NestedEDLs::clear()
{
	for(int i = 0; i < nested_edls.size(); i++)
		nested_edls.get(i)->Garbage::remove_user();
	nested_edls.remove_all();
}


void NestedEDLs::update_index(EDL *nested_edl)
{
	for(int i = 0; i < nested_edls.size(); i++)
	{
		EDL *current = nested_edls.get(i);
		if(!strcmp(current->path, nested_edl->path))
		{
// printf("NestedEDLs::update_index %d %p %d\n", 
// __LINE__, 
// current->index_state,
// nested_edl->index_state->index_status);
			current->index_state->copy_from(nested_edl->index_state);
		}
	}
}

void NestedEDLs::remove_edl(EDL *nested_edl)
{
	nested_edls.remove(nested_edl);
	nested_edl->Garbage::remove_user();
}



