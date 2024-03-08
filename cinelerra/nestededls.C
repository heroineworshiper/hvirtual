#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "indexstate.h"
#include "nestededls.h"

#include <inttypes.h>

NestedEDLs::NestedEDLs(EDL *edl)
{
    this->edl = edl;
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
        printf("    nested_sample_rate=%" PRId64 "\n", nested_edls.get(i)->session->nested_sample_rate);
        printf("    nested_frame_rate=%f\n", nested_edls.get(i)->session->nested_frame_rate);
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
// can go no further
    if(edl->nested_depth >= NESTED_DEPTH)
    {
        printf("NestedEDLs::get_copy %d: %s -> %s hit recursive dependency\n", 
            __LINE__, 
            edl->path,
            src->path);
        return 0;
    }

// return an existing one 
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
// overwrite src nested_depth
    dst->nested_depth = edl->nested_depth + 1;
	nested_edls.append(dst);
	return dst;
}

EDL* NestedEDLs::get(char *path, int *error)
{
// can go no further
    if(edl->nested_depth >= NESTED_DEPTH)
    {
        printf("NestedEDLs::get %d: %s -> %s hit recursive dependency\n", 
            __LINE__, 
            edl->path,
            path);
        *error |= IS_RECURSIVE;
        return 0;
    }

	for(int i = 0; i < nested_edls.size(); i++)
	{
		EDL *dst = nested_edls.get(i);
		if(!strcmp(dst->path, path))
		{
        	return dst;
        }
	}

	EDL *dst = new EDL;
    dst->nested_depth = edl->nested_depth + 1;
	dst->create_objects();
	FileXML xml_file;
	xml_file.read_from_file(path);
	*error |= dst->load_xml(&xml_file, LOAD_ALL);
//if(*error) BC_Signals::dump_stack();

// Override path EDL was saved to with the path it was loaded from.
	dst->set_path(path);
	nested_edls.append(dst);
	return dst;
}

EDL* NestedEDLs::search(const char *path)
{
	for(int i = 0; i < nested_edls.size(); i++)
	{
		EDL *dst = nested_edls.get(i);
		if(!strcmp(dst->path, path))
			return dst;
	}
    return 0;
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

int NestedEDLs::load(FileXML *file)
{
	int result = 0;
    int error = 0;

	while(!result)
	{
		result = file->read_tag();
        if(result) break;
		if(file->tag.title_is("/NESTED_EDLS"))
		{
			result = 1;
		}
		else
		if(file->tag.title_is("NESTED_EDL"))
		{
			char *path = file->tag.get_property("SRC");
//printf("NestedEDLs::load %d %s\n", __LINE__, path);
            EDL *edl = 0;
			if(path && path[0] != 0)
			{
				edl = get(path, &error);
			}

            if(edl)
            {
                edl->session->nested_sample_rate = 
                    file->tag.get_property("SAMPLE_RATE", (int64_t)-1);
                edl->session->nested_frame_rate = 
                    file->tag.get_property("FRAME_RATE", (double)-1);
// printf("NestedEDLs::load %d %s %f %" PRId64 "\n", 
//     __LINE__, 
//     path, 
//     edl->session->nested_frame_rate, 
//     edl->session->nested_sample_rate);
            }
		}
	}
	return error;
}




