#include "funcprotos.h"
#include "quicktime.h"



int quicktime_mvhd_init(quicktime_mvhd_t *mvhd)
{
	int i;
	mvhd->version = 0;
	mvhd->flags = 0;
	mvhd->creation_time = quicktime_current_time();
	mvhd->modification_time = quicktime_current_time();
	mvhd->time_scale = 600;
	mvhd->duration = 0;
	mvhd->preferred_rate = 1.0;
	mvhd->preferred_volume = 0.996094;
	for(i = 0; i < 10; i++) mvhd->reserved[i] = 0;
	quicktime_matrix_init(&(mvhd->matrix));
	mvhd->preview_time = 0;
	mvhd->preview_duration = 0;
	mvhd->poster_time = 0;
	mvhd->selection_time = 0;
	mvhd->selection_duration = 0;
	mvhd->current_time = 0;
	mvhd->next_track_id = 1;
	return 0;
}

int quicktime_mvhd_delete(quicktime_mvhd_t *mvhd)
{
	return 0;
}

void quicktime_mvhd_dump(quicktime_mvhd_t *mvhd)
{
	printf(" movie header\n");
	printf("  version %d\n", mvhd->version);
	printf("  flags %ld\n", mvhd->flags);
	printf("  creation_time %u\n", mvhd->creation_time);
	printf("  modification_time %u\n", mvhd->modification_time);
	printf("  time_scale %ld\n", mvhd->time_scale);
	printf("  duration %ld\n", mvhd->duration);
	printf("  preferred_rate %f\n", mvhd->preferred_rate);
	printf("  preferred_volume %f\n", mvhd->preferred_volume);
	quicktime_print_chars("  reserved ", mvhd->reserved, 10);
	quicktime_matrix_dump(&(mvhd->matrix));
	printf("  preview_time %ld\n", mvhd->preview_time);
	printf("  preview_duration %ld\n", mvhd->preview_duration);
	printf("  poster_time %ld\n", mvhd->poster_time);
	printf("  selection_time %ld\n", mvhd->selection_time);
	printf("  selection_duration %ld\n", mvhd->selection_duration);
	printf("  current_time %ld\n", mvhd->current_time);
	printf("  next_track_id %ld\n", mvhd->next_track_id);
}

void quicktime_read_mvhd(quicktime_t *file, quicktime_mvhd_t *mvhd, quicktime_atom_t *parent_atom)
{
	mvhd->version = quicktime_read_char(file);
	mvhd->flags = quicktime_read_int24(file);
	mvhd->creation_time = quicktime_read_int32(file);
	mvhd->modification_time = quicktime_read_int32(file);
	mvhd->time_scale = quicktime_read_int32(file);
	mvhd->duration = quicktime_read_int32(file);
	mvhd->preferred_rate = quicktime_read_fixed32(file);
	mvhd->preferred_volume = quicktime_read_fixed16(file);
	quicktime_read_data(file, mvhd->reserved, 10);
	quicktime_read_matrix(file, &(mvhd->matrix));
	mvhd->preview_time = quicktime_read_int32(file);
	mvhd->preview_duration = quicktime_read_int32(file);
	mvhd->poster_time = quicktime_read_int32(file);
	mvhd->selection_time = quicktime_read_int32(file);
	mvhd->selection_duration = quicktime_read_int32(file);
	mvhd->current_time = quicktime_read_int32(file);
	mvhd->next_track_id = quicktime_read_int32(file);
}

void quicktime_mhvd_init_video(quicktime_t *file, quicktime_mvhd_t *mvhd, double frame_rate)
{
	mvhd->time_scale = quicktime_get_timescale(frame_rate);
}

void quicktime_write_mvhd(quicktime_t *file, quicktime_mvhd_t *mvhd)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "mvhd");

	quicktime_write_char(file, mvhd->version);
	quicktime_write_int24(file, mvhd->flags);
	quicktime_write_int32(file, mvhd->creation_time);
	quicktime_write_int32(file, mvhd->modification_time);
	quicktime_write_int32(file, mvhd->time_scale);
	quicktime_write_int32(file, mvhd->duration);
	quicktime_write_fixed32(file, mvhd->preferred_rate);
	quicktime_write_fixed16(file, mvhd->preferred_volume);
	quicktime_write_data(file, mvhd->reserved, 10);
	quicktime_write_matrix(file, &(mvhd->matrix));
	quicktime_write_int32(file, mvhd->preview_time);
	quicktime_write_int32(file, mvhd->preview_duration);
	quicktime_write_int32(file, mvhd->poster_time);
	quicktime_write_int32(file, mvhd->selection_time);
	quicktime_write_int32(file, mvhd->selection_duration);
	quicktime_write_int32(file, mvhd->current_time);
	quicktime_write_int32(file, mvhd->next_track_id);

	quicktime_atom_write_footer(file, &atom);
}
