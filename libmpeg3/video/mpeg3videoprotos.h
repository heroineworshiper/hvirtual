#ifndef MPEG3VIDEOPROTOS_H
#define MPEG3VIDEOPROTOS_H

void mpeg3video_idct_conversion(short* block);
unsigned int mpeg3slice_showbits(mpeg3_slice_buffer_t *slice_buffer, int bits);
int mpeg3video_get_header(mpeg3video_t *video, int dont_repeat);
unsigned int mpeg3bits_next_startcode(mpeg3_bits_t* stream);
int mpeg3video_getslicehdr(mpeg3_slice_t *slice, mpeg3video_t *video);
int mpeg3_new_slice_buffer(mpeg3_slice_buffer_t *slice_buffer);
int mpeg3_delete_slice_buffer(mpeg3_slice_buffer_t *slice_buffer);
int mpeg3_expand_slice_buffer(mpeg3_slice_buffer_t *slice_buffer);
int mpeg3_delete_slice_decoder(mpeg3_slice_t *slice);
int mpeg3_new_slice_decoder(void *video, mpeg3_slice_t *slice);

void mpeg3video_calc_dmv(mpeg3video_t *video, int DMV[][2], int *dmvector, 
		int mvx, int mvy);

void mpeg3video_motion_vector(mpeg3_slice_t *slice, mpeg3video_t *video, 
			      int *PMV, int *dmvector, 
			      int h_r_size, int v_r_size,
			      int dmv, int mvscale, 
			      int full_pel_vector);
int mpeg3video_motion_vectors(mpeg3_slice_t *slice, mpeg3video_t *video, 
			      int PMV[2][2][2], int dmvector[2], 
			      int mv_field_sel[2][2], int s, 
			      int mv_count, int mv_format, 
			      int h_r_size, int v_r_size, 
			      int dmv, int mvscale);


int mpeg3video_get_macroblocks(mpeg3video_t *video, int framenum);
int mpeg3video_get_macroblock_address(mpeg3_slice_t *slice);
int mpeg3video_read_raw(mpeg3video_t *video,  unsigned char *output, 
			long *size, long max_size);
int mpeg3video_read_frame_backend(mpeg3video_t *video, int skip_bframes);
int mpeg3video_read_yuvframe(mpeg3video_t *video,  char *y_output,
			     char *u_output, char *v_output,
			     int in_x, int in_y, int in_w, int in_h);
int mpeg3video_read_yuvframe_ptr(mpeg3video_t *video, char **y_output,
				 char **u_output, char **v_output);
int mpeg3video_display_second_field(mpeg3video_t *video);
int mpeg3video_getpicturehdr(mpeg3video_t *video);
int mpeg3video_getpicture(mpeg3video_t *video, int framenum);
int mpeg3video_getgophdr(mpeg3video_t *video);
int mpeg3video_present_frame(mpeg3video_t *video);
int mpeg3video_seek(mpeg3video_t *video);
int mpeg3video_init_output();
void mpeg3video_calc_dmv(mpeg3video_t *video, 
		int DMV[][2], 
		int *dmvector, 
		int mvx, 
		int mvy);
int mpeg3video_get_firstframe(mpeg3video_t *video);
int mpeg3video_macroblock_modes(mpeg3_slice_t *slice, 
		mpeg3video_t *video, 
		int *pmb_type, 
		int *pstwtype, 
		int *pstwclass, 
		int *pmotion_type, 
		int *pmv_count, 
		int *pmv_format, 
		int *pdmv, 
		int *pmvscale,
		int *pdct_type);
int mpeg3video_get_cbp(mpeg3_slice_t *slice);
int mpeg3video_clearblock(mpeg3_slice_t *slice, int comp, int size);
int mpeg3video_getmpg2intrablock(mpeg3_slice_t *slice, 
		mpeg3video_t *video, 
		int comp, 
		int dc_dct_pred[]);
int mpeg3video_getintrablock(mpeg3_slice_t *slice, 
		mpeg3video_t *video,
		int comp, 
		int dc_dct_pred[]);
int mpeg3video_getmpg2interblock(mpeg3_slice_t *slice, 
		mpeg3video_t *video, 
		int comp);
int mpeg3video_reconstruct(mpeg3video_t *video, 
	int bx, 
	int by, 
	int mb_type, 
	int motion_type,
	int PMV[2][2][2], 
	int mv_field_sel[2][2], 
	int dmvector[2], 
	int stwtype);
int mpeg3video_getinterblock(mpeg3_slice_t *slice, 
		mpeg3video_t *video, 
		int comp);






#endif
