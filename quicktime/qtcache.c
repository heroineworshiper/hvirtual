#include "funcprotos.h"
#include "qtprivate.h"
#include <string.h>


quicktime_cache_t* quicktime_new_cache()
{
	quicktime_cache_t *result = calloc(1, sizeof(quicktime_cache_t));
	return result;
}

void quicktime_delete_cache(quicktime_cache_t *ptr)
{
	if(ptr->frames) 
	{
		int i;
//printf("quicktime_delete_cache 1\n");
		for(i = 0; i < ptr->allocation; i++)
		{
			quicktime_cacheframe_t *frame = &ptr->frames[i];
			if(frame->y) free(frame->y);
			if(frame->u) free(frame->u);
			if(frame->v) free(frame->v);
		}
		free(ptr->frames);
		free(ptr);
	}
}

void quicktime_reset_cache(quicktime_cache_t *ptr)
{
	ptr->total = 0;
}

void quicktime_put_cache(quicktime_cache_t *cache)
{
    if(cache->put_cache)
        cache->put_cache(cache->put_cache_ptr);
}

// void quicktime_put_cache(
//     quicktime_cache_t *ptr,
// 	int64_t frame_number,
//     int track,
// 	unsigned char *y,
// 	unsigned char *u,
// 	unsigned char *v,
// 	int y_size,
// 	int u_size,
// 	int v_size)
// {
// 	quicktime_cacheframe_t *frame = 0;
// 	int i;
// 
// 
// // printf("quicktime_put_frame %d total=%d allocation=%d size=%d %d %d\n", 
// // __LINE__, 
// // ptr->total, 
// // ptr->allocation,
// // y_size,
// // u_size,
// // v_size);
// // Get existing frame
// 	for(i = 0; i < ptr->total; i++)
// 	{
// 		if(ptr->frames[i].frame_number == frame_number &&
//             ptr->frames[i].track == track)
// 		{
// 			frame = &ptr->frames[i];
// 			break;
// 		}
// 	}
// 
// 
// 	if(!frame)
// 	{
// 		if(ptr->total >= ptr->allocation)
// 		{
// 			int new_allocation = ptr->allocation * 2;
// 
// // printf("quicktime_put_frame %d ptr->allocation=%d new_allocation=%d\n", 
// // __LINE__, 
// // ptr->allocation, 
// // new_allocation);
// 
// 			if(!new_allocation) new_allocation = 32;
// 			ptr->frames = realloc(ptr->frames, 
// 				sizeof(quicktime_cacheframe_t) * new_allocation);
// 			bzero(ptr->frames + ptr->total,
// 				sizeof(quicktime_cacheframe_t) * (new_allocation - ptr->allocation));
// 			ptr->allocation = new_allocation;
// 		}
// 
// 		frame = &ptr->frames[ptr->total];
// //printf("quicktime_put_frame 30 %d %p %p %p\n", ptr->total, frame->y, frame->u, frame->v);
// 		ptr->total++;
// 
// // Memcpy is a lot slower than just dropping the seeking frames.
// 		if(y) 
// 		{
// 			frame->y = realloc(frame->y, y_size);
// 			frame->y_size = y_size;
// 			memcpy(frame->y, y, y_size);
// 		}
// 
// 		if(u)
// 		{
// 			frame->u = realloc(frame->u, u_size);
// 			frame->u_size = u_size;
// 			memcpy(frame->u, u, u_size);
// 		}
// 
// 		if(v)
// 		{
// 			frame->v = realloc(frame->v, v_size);
// 			frame->v_size = v_size;
// 			memcpy(frame->v, v, v_size);
// 		}
// 		frame->frame_number = frame_number;
// 		frame->track = track;
// 	}
// 	
// 
// //printf("quicktime_put_frame %d total=%d allocation=%d\n", __LINE__, ptr->total, ptr->allocation);
// // Delete oldest frames
// 	if(ptr->max)
// 	{
// 		while(quicktime_cache_usage(ptr) > ptr->max && ptr->total > 0)
// 		{
//             quicktime_purge_cache2(ptr);
// 		}
// 	}
// 
// 
// // printf("quicktime_put_frame %d total=%d allocation=%d\n", __LINE__, ptr->total, ptr->allocation);
// // printf("quicktime_put_frame %d max=0x%x current=0x%x\n", 
// // __LINE__,
// // ptr->max, 
// // quicktime_cache_usage2(ptr));
// 
// }

int64_t quicktime_purge_cache2(quicktime_cache_t *ptr)
{
    int64_t result = 0;
    int i;

    if(ptr->total > 0)
    {
	    quicktime_cacheframe_t *frame = &ptr->frames[0];

	    result = frame->y_size + frame->u_size + frame->v_size;
	    if(frame->y) free(frame->y);
	    if(frame->u) free(frame->u);
	    if(frame->v) free(frame->v);

	    for(i = 0; i < ptr->total - 1; i++)
	    {
		    quicktime_cacheframe_t *frame1 = &ptr->frames[i];
		    quicktime_cacheframe_t *frame2 = &ptr->frames[i + 1];
		    *frame1 = *frame2;
	    }


	    frame = &ptr->frames[ptr->total - 1];
	    frame->y = 0;
	    frame->u = 0;
	    frame->v = 0;
	    ptr->total--;
    }
    
    return result;
}


int quicktime_get_cache(quicktime_cache_t *ptr,
	int64_t frame_number,
    int track,
	unsigned char **y,
	unsigned char **u,
	unsigned char **v)
{
	int i;

	for(i = 0; i < ptr->total; i++)
	{
		quicktime_cacheframe_t *frame = &ptr->frames[i];
		if(frame->frame_number == frame_number &&
            frame->track == track)
		{
			
			*y = frame->y;
			*u = frame->u;
			*v = frame->v;
			return 1;
			break;
		}
	}

	return 0;
}

int quicktime_has_cache(quicktime_cache_t *ptr,
	int64_t frame_number,
    int track)
{
	int i;

	for(i = 0; i < ptr->total; i++)
	{
		quicktime_cacheframe_t *frame = &ptr->frames[i];
		if(frame->frame_number == frame_number &&
            frame->track == track)
		{
			return 1;
			break;
		}
	}

	return 0;
}

int64_t quicktime_cache_usage(quicktime_cache_t *ptr)
{
	int64_t result = 0;
	int i;
//printf("quicktime_cache_usage %p %d %lld\n", ptr,  ptr->total, result);
	for(i = 0; i < ptr->total; i++)
	{
		quicktime_cacheframe_t *frame = &ptr->frames[i];
		result += frame->y_size + frame->u_size + frame->v_size;
	}
	return result;
}

void quicktime_cache_max(quicktime_cache_t *ptr, int bytes)
{
	ptr->max = bytes;
}




