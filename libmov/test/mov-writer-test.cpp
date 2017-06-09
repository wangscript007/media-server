#include "mov-writer.h"
#include "mov-format.h"
#include "mpeg4-aac.h"
#include "flv-proto.h"
#include "flv-reader.h"
#include "flv-demuxer.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static uint8_t s_buffer[2 * 1024 * 1024];
static int s_width, s_height;

static void onFLV(void* mov, int type, int format, const void* data, size_t bytes, uint32_t pts, uint32_t dts)
{
	static bool s_aac_track = false;
	static bool s_avc_track = false;

	format = (FLV_TYPE_AUDIO == type) ? (format << 8) : format;
	switch(format)
	{
	case (FLV_AUDIO_AAC << 8):
		mov_writer_write_audio(mov, data, bytes, pts, dts);
		break;

	case (FLV_AUDIO_MP3 << 8):
		assert(0);
		break;

	case FLV_VIDEO_H264:
		mov_writer_write_video(mov, data, bytes, pts, dts);
		break;

	case FLV_VIDEO_AVCC:
		if (!s_avc_track)
		{
			s_avc_track = true;
			mov_writer_video_meta(mov, MOV_AVC1, s_width, s_height, data, bytes);
		}
		break;

	case (FLV_AUDIO_ASC << 8):
		if (!s_aac_track)
		{
			s_aac_track = true;
			struct mpeg4_aac_t aac;
			mpeg4_aac_audio_specific_config_load((const uint8_t*)data, bytes, &aac);
			int rate = mpeg4_aac_audio_frequency_to((enum mpeg4_aac_frequency)aac.sampling_frequency_index);
			mov_writer_audio_meta(mov, MOV_MP4A, aac.channel_configuration, 16, rate, data, bytes);
		}
		break;

	default:
		// nothing to do
		assert(0);
	}

	printf("\n");
}

void mov_writer_test(int w, int h, const char* inflv, const char* outmp4)
{
	int r, type;
	uint32_t timestamp;
	void* flv = flv_reader_create(inflv);
	void* mov = mov_writer_create(outmp4);
	void* demuxer = flv_demuxer_create(onFLV, mov);

	s_width = w;
	s_height = h;
	while ((r = flv_reader_read(flv, &type, &timestamp, s_buffer, sizeof(s_buffer))) > 0)
	{
		r = flv_demuxer_input(demuxer, type, s_buffer, r, timestamp);
		if (r < 0)
		{
			assert(0);
		}
	}

	mov_writer_destroy(mov);
	flv_reader_destroy(flv);
	flv_demuxer_destroy(demuxer);
}
