#ifndef MPEG3AUDIO_H
#define MPEG3AUDIO_H

int mpeg3audio_synth_stereo(mpeg3_layer_t *audio, float *bandPtr, int channel, float *out, int *pnt);
int mpeg3audio_doac3(mpeg3_ac3_t *audio, char *frame, int frame_size, float **output, int render);
int mpeg3audio_read_raw(mpeg3audio_t *audio, unsigned char *output, 
			long *size, long max_size);

#endif
