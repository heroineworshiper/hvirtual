#include "asset.h"
#include "audiodevice.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "mwindow.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#ifndef OTHERAUDIO
#include <sys/soundcard.h>
#endif

#include <unistd.h>

AudioDevice::AudioDevice(MWindow *mwindow, Asset *asset)
 : Thread(1, 0, 0)
{
	this->asset = asset;
	this->mwindow = mwindow;
	software_sync = 0;
	total_samples_written = 0;
	last_samples_written = 0;
	playing_back = 0;
	last_position = 0;
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		input_lock[i] = new Condition(1, "AudioDevice::input_lock");
		output_lock[i] = new Condition(1, "AudioDevice::output_lock");
	}
	timer_lock = new Mutex("AudioDevice::timer_lock");
}

AudioDevice::~AudioDevice()
{
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		delete input_lock[i];
		delete output_lock[i];
	}
	delete timer_lock;
}

int AudioDevice::set_software_sync(int value)
{
	this->software_sync = value;
	return 0;
}

long AudioDevice::get_fmt()
{
#ifndef OTHERAUDIO
	return AFMT_S16_LE;
#endif
	return 0;
}

int AudioDevice::get_channels()
{
	switch(mix_strategy)
	{
		case DOLBY51_TO_STEREO:
			return asset->channels > 2 ? 2 : asset->channels;
			break;
		case DIRECT_COPY:
			return asset->channels;
			break;
		case STEREO_TO_DOLBY51:
			return 6;
			break;
	}
	return asset->channels;
}

int AudioDevice::start_playback()
{
	this->mix_strategy = mwindow->mix_strategy;

#ifndef OTHERAUDIO
	audio_buf_info playinfo;

// Linux 2.4.18 no longer supports allocating the maximum buffer size.
// Need the largest power of 2 buffer size below or equal to the maximum.
	int dsp_test = open("/dev/dsp", O_WRONLY);
	int buffer_info = 0x7fff000f;
	int format = AFMT_S16_LE;
	int channels = 2;
	if(ioctl(dsp_test, SNDCTL_DSP_SETFRAGMENT, &buffer_info)) printf("SNDCTL_DSP_SETFRAGMENT #1 failed.\n");
	if(ioctl(dsp_test, SNDCTL_DSP_SETFMT, &format) < 0) printf("SNDCTL_DSP_SETFMT #1 failed\n");
	if(ioctl(dsp_test, SNDCTL_DSP_CHANNELS, &channels) < 0) printf("SNDCTL_DSP_CHANNELS #1 failed\n");
	if(ioctl(dsp_test, SNDCTL_DSP_SPEED, &asset->rate) < 0) printf("SNDCTL_DSP_SPEED #1 failed\n");
	if(ioctl(dsp_test, SNDCTL_DSP_GETOSPACE, &playinfo) < 0) printf("SNDCTL_DSP_GETOSPACE #1 failed\n");
	for(actual_buffer = 1;
		actual_buffer < playinfo.bytes;
		actual_buffer *= 2)
		;
	if(actual_buffer > playinfo.bytes) actual_buffer /= 2;
	close(dsp_test);

//printf("AudioDevice::start_playback 1 %d\n", playinfo.bytes);
	int testfrag = 2;
	for(buffer_info = 0x7fff0000;
		testfrag < actual_buffer;
		buffer_info++,
		testfrag *= 2)
		;
//printf("AudioDevice::start_playback 2 %08x\n", buffer_info);

	dsp_out = open("/dev/dsp", O_WRONLY);
	if(dsp_out < 0)
	{
		printf("open odevice failed\n");
		software_sync = 1;
	}
	else
	{
// OSS Envy24 hack
		format = get_fmt();
		channels = get_channels();
		if(ioctl(dsp_out, SNDCTL_DSP_SETFRAGMENT, &buffer_info)) printf("SNDCTL_DSP_SETFRAGMENT failed.\n");
		if(ioctl(dsp_out, SNDCTL_DSP_SETFMT, &format) < 0) printf("SNDCTL_DSP_SETFMT failed\n");
		if(ioctl(dsp_out, SNDCTL_DSP_CHANNELS, &channels) < 0) printf("SNDCTL_DSP_CHANNELS failed\n");
		if(ioctl(dsp_out, SNDCTL_DSP_SPEED, &asset->rate) < 0) printf("SNDCTL_DSP_SPEED failed\n");
		ioctl(dsp_out, SNDCTL_DSP_GETOSPACE, &playinfo);
		actual_buffer = playinfo.bytes / (asset->bits / 8) / channels;
//printf("AudioDevice::start_playback %d\n", playinfo.bytes);
	}
#endif

	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		output_lock[i]->lock("AudioDevice::start_playback");
		output_buffers[i] = 0;
		buffer_len[i] = 0;
	}

	playing_back = 1;
	last_position = 0;
	current_inbuffer = 0;
	current_outbuffer = 0;
	done = 0;
	timer.update();
	start();
	return actual_buffer;
}

int AudioDevice::interrupt_playback()
{
	if(playing_back)
	{
		done = 1;
		Thread::end();
		for(int i = 0; i < TOTAL_BUFFERS; i++)
			input_lock[i]->unlock();
		Thread::join();
		playing_back = 0;
	}
	return 0;
}

int AudioDevice::stop_playback()
{
	Thread::join();
	reset();
	playing_back = 0;
	return 0;
}

int AudioDevice::reset()
{
#ifndef OTHERAUDIO
	ioctl(dsp_out, SNDCTL_DSP_RESET, 0);
	close(dsp_out);
#endif

	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		if(output_buffers[i]) delete output_buffers[i];
		output_buffers[i] = 0;
	}
	return 0;
}

long AudioDevice::samples_rendered()
{
	static long result;

#ifndef OTHERAUDIO
	count_info info;
#endif

	if(playing_back)
	{
		if(software_sync)
		{
			timer_lock->lock("AudioDevice::samples_rendered");
			result = total_samples_written - last_samples_written - 
				actual_buffer;
			result += timer.get_scaled_difference(asset->rate);

//printf("AudioDevice::samples_rendered %ld - %ld - %ld + %ld = %ld\n", total_samples_written, last_samples_written, actual_buffer, timer.get_scaled_difference(asset->rate), result);
			if(result < 0) result = 0;
			timer_lock->unlock();

			if(result < last_position) 
			result = last_position;
			else
			last_position = result;

			return result;
		}
		else
		{
#ifndef OTHERAUDIO
			if(!ioctl(dsp_out, SNDCTL_DSP_GETOPTR, &info))
			{
				result = info.bytes / (asset->bits / 8) / get_channels();
				return result;
			}
			return 0;
#endif
		}
	}
	else
		return total_samples_written;
}

int AudioDevice::write_audio(char *buffer, long samples)
{
	if(done) return 1;
	input_lock[current_inbuffer]->lock("AudioDevice::write_audio");
	if(done) return 1;
	long bytes = samples_to_bytes(samples);
	int out_channels = get_channels();
// Correct mix strategy for asset channel count
	int corrected_mix_strategy = mix_strategy;

	if(asset->channels < 5 && mix_strategy == DOLBY51_TO_STEREO ||
		asset->channels < 2 && mix_strategy == STEREO_TO_DOLBY51) 
		corrected_mix_strategy = DIRECT_COPY;


// Allocate output buffer of proper size
	allocate_buffer(current_inbuffer, samples);
	buffer_len[current_inbuffer] = samples;
	bzero(output_buffers[current_inbuffer], bytes);

// Downmix into output buffer
	if(buffer)
	{
		int16_t *output_buffer = (int16_t*)output_buffers[current_inbuffer];
		int16_t *input_buffer = (int16_t*)buffer;

//printf("AudioDevice::write_audio 1 %d %d\n", mix_strategy, asset->channels);
		switch(corrected_mix_strategy)
		{
			case DOLBY51_TO_STEREO:
//printf("AudioDevice::write_audio 2\n");
				for(int i = 0; i < asset->channels; i++)
				{
					int out_channel = 0;
					switch(i)
					{
						case 0: out_channel = 0; break;
						case 1: out_channel = 0; break;
						case 2: out_channel = 1; break;
						case 3: out_channel = 0; break;
						case 4: out_channel = 1; break;
						case 5: out_channel = 0; break;
					}

// Split center channel
					if(i == 0 || i == 5)
					{
						for(int j = 0; j < samples; j++)
						{
							int result = output_buffer[1 + j * out_channels] +
								input_buffer[i + j * asset->channels];
							CLAMP(result, -0x7fff, 0x7fff);
							output_buffer[1 + j * out_channels] = result;
						}
					}

					for(int j = 0; j < samples; j++)
					{
						int result = output_buffer[out_channel + j * out_channels] +
							input_buffer[i + j * asset->channels];
						CLAMP(result, -0x7fff, 0x7fff);
						output_buffer[out_channel + j * out_channels] = result;
					}
				}
				break;

			case DIRECT_COPY:
//printf("AudioDevice::write_audio 3  %d %d\n", asset->channels, samples);
				for(int i = 0; i < asset->channels; i++)
				{
					for(int j = 0; j < samples; j++)
					{
						output_buffer[i + j * out_channels] = input_buffer[i + j * out_channels];
					}
				}
				break;

			case STEREO_TO_DOLBY51:
//printf("AudioDevice::write_audio 3  %d %d\n", asset->channels, samples);
				for(int i = 0; i < asset->channels; i++)
				{
					int out_channel = 0;
					switch(i)
					{
						case 0: out_channel = 0; break;
						case 1: out_channel = 2; break;
					}

					for(int j = 0; j < samples; j++)
					{
						output_buffer[out_channel + j * out_channels] = input_buffer[i + j * asset->channels];
					}
				}
				break;
		}
	}

	output_lock[current_inbuffer]->unlock();
	current_inbuffer = next_buffer(current_inbuffer);
	return 0;
}

void AudioDevice::run()
{
	while(!done && playing_back)
	{
		output_lock[current_outbuffer]->lock("AudioDevice::run");
		if(buffer_len[current_outbuffer] && !done)
		{
			total_samples_written += buffer_len[current_outbuffer];
			last_samples_written = buffer_len[current_outbuffer];
			timer_lock->lock("AudioDevice::run");
			timer.update();
			timer_lock->unlock();

#ifndef OTHERAUDIO
// printf("AudioDevice::run %d %d\n", 
// samples_to_bytes(buffer_len[current_outbuffer]), 
// actual_buffer);
			Thread::enable_cancel();
			if(write(dsp_out, output_buffers[current_outbuffer], 
				samples_to_bytes(buffer_len[current_outbuffer])) < 0)
					sleep(1);
			Thread::disable_cancel();
#else
			sleep(1);
#endif
		}
		else
			done = 1;  // No more buffers

		input_lock[current_outbuffer]->unlock();
		current_outbuffer = next_buffer(current_outbuffer);
	}

#ifndef OTHERAUDIO
	ioctl(dsp_out, SNDCTL_DSP_SYNC, 0);
#endif
}

int AudioDevice::next_buffer(int buffer_num)
{
	buffer_num++;
	if(buffer_num == TOTAL_BUFFERS) buffer_num = 0;
	return buffer_num;
}

int AudioDevice::allocate_buffer(int buffer_num, long size)
{
	if(size > buffer_len[buffer_num] && output_buffers[buffer_num])
	{
		delete output_buffers[buffer_num];
		output_buffers[buffer_num] = 0;
	}
	
	if(!output_buffers[buffer_num])
	{
		output_buffers[buffer_num] = new char[samples_to_bytes(size)];
	}
	return 0;
}

long AudioDevice::samples_to_bytes(long samples)
{
	return samples * asset->bits / 8 * get_channels();
}
