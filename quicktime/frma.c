#include "funcprotos.h"
#include "quicktime.h"


void quicktime_delete_frma(quicktime_frma_t *frma)
{
	if(frma->data) free(frma->data);
}

int quicktime_read_frma(quicktime_t *file, 
    quicktime_stsd_table_t *table, 
	quicktime_atom_t *parent_atom,
	quicktime_atom_t *leaf_atom,
	quicktime_frma_t *frma)
{
	frma->data_size = leaf_atom->size - 8;
	frma->data = calloc(1, frma->data_size + 1024);
//	quicktime_set_position(file, parent_atom->start + 12);
	quicktime_read_data(file, 
		frma->data, 
		frma->data_size);

// printf("quicktime_read_frma %d format=%s size=%d\n", 
// __LINE__, 
// table->format,
// (int)leaf_atom->size);



/*
 * printf("quicktime_read_frma %02x %02x %02x %02x %02x %02x %02x %02x\n",
 * frma->data[0], 
 * frma->data[1], 
 * frma->data[2], 
 * frma->data[3], 
 * frma->data[4], 
 * frma->data[5], 
 * frma->data[6], 
 * frma->data[7]);
 */
//	quicktime_atom_skip(file, parent_atom);
	return 0;
}

void quicktime_frma_dump(quicktime_frma_t *frma)
{
	int i;
	if(frma->data_size)
	{
		printf("       QDM2/MP4A description\n");
		printf("        data_size=0x%x\n", frma->data_size);
		printf("        data=");
		for(i = 0; i < frma->data_size; i++)
		{
			printf("0x%02x ", (unsigned char)frma->data[i]);
		}
		printf("\n");
	}
}

