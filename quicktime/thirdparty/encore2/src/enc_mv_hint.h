/* mv_hint.h */

/* API version 1 */

#ifndef ENC_MV_HINT_H
#define ENC_MV_HINT_H

#ifdef __cplusplus
extern "C" {
#endif




#define MV_HINT_API_VERSION 1

/* this version of the API is only really suitable for IPPPPPPPP GOVs */


typedef struct {
    int dx;       /* in half-pel units */
    int dy;       /* in half-pel units */
} mv_hint_vector_t;


typedef struct {
    int num_vectors; /* 0 <= num_vectors <= 8 */
    mv_hint_vector_t vectors[8]; 
} mv_hint_mb_t;


typedef struct {
	int version;
    int valid;
	int ref_frame;
    int mb_width;   /* should correspond to encoder's image dimensions */
    int mb_height;  /* should correspond to encoder's image dimensions */
    mv_hint_mb_t *mb;
} mv_hint_frame_t;


#ifdef __cplusplus
}
#endif

#endif
