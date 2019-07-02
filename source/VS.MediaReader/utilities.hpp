#ifndef GUARD_utilities_20190207153147_
#define GUARD_utilities_20190207153147_
/*
@file		utilities.hpp
@author		Webstar
@date		2019-07-02 15:31
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

// STD
#include <string>

// Required for libavformat to build on Windows
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

// Include the FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavresample/avresample.h>
#include <libswresample/swresample.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>

// libavutil changed folders at some point
#include <libavutil/opt.h>

// channel header refactored
#include <libavutil/channel_layout.h>
}

// This was removed from newer versions of FFmpeg (but still used in libopenshot)
#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#endif
#ifndef AV_ERROR_MAX_STRING_SIZE
#define AV_ERROR_MAX_STRING_SIZE 64
#endif
#ifndef AUDIO_PACKET_ENCODING_SIZE
#define AUDIO_PACKET_ENCODING_SIZE 768000		// 48khz * S16 (2 bytes) * max channels (8)
#endif

// This wraps an unsafe C macro to be C++ compatible function
static const std::string av_make_error_string(int errnum)
{
	char errbuf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
	std::string errstring(errbuf);
	return errstring;
}

// Redefine the C macro to use our new C++ function
#undef av_err2str
#define av_err2str(errnum) av_make_error_string(errnum).c_str()

// Define this for compatibility
#ifndef PixelFormat
#define PixelFormat AVPixelFormat
#endif
#ifndef PIX_FMT_RGBA
#define PIX_FMT_RGBA AV_PIX_FMT_RGBA
#endif
#ifndef PIX_FMT_NONE
#define PIX_FMT_NONE AV_PIX_FMT_NONE
#endif
#ifndef PIX_FMT_RGB24
#define PIX_FMT_RGB24 AV_PIX_FMT_RGB24
#endif
#ifndef PIX_FMT_YUV420P
#define PIX_FMT_YUV420P AV_PIX_FMT_YUV420P
#endif

#define AV_ALLOCATE_FRAME() av_frame_alloc()
#define AV_RESET_FRAME(av_frame) av_frame_unref(av_frame)
#define AV_FREE_FRAME(av_frame) av_frame_free(av_frame)
#define AV_FREE_PACKET(av_packet) av_packet_unref(av_packet)

enum ChannelLayout
{
	LAYOUT_MONO = AV_CH_LAYOUT_MONO,
	LAYOUT_STEREO = AV_CH_LAYOUT_STEREO,
	LAYOUT_2POINT1 = AV_CH_LAYOUT_2POINT1,
	LAYOUT_2_1 = AV_CH_LAYOUT_2_1,
	LAYOUT_SURROUND = AV_CH_LAYOUT_SURROUND,
	LAYOUT_3POINT1 = AV_CH_LAYOUT_3POINT1,
	LAYOUT_4POINT0 = AV_CH_LAYOUT_4POINT0,
	LAYOUT_4POINT1 = AV_CH_LAYOUT_4POINT1,
	LAYOUT_2_2 = AV_CH_LAYOUT_2_2,
	LAYOUT_QUAD = AV_CH_LAYOUT_QUAD,
	LAYOUT_5POINT0 = AV_CH_LAYOUT_5POINT0,
	LAYOUT_5POINT1 = AV_CH_LAYOUT_5POINT1,
	LAYOUT_5POINT0_BACK = AV_CH_LAYOUT_5POINT0_BACK,
	LAYOUT_5POINT1_BACK = AV_CH_LAYOUT_5POINT1_BACK,
	LAYOUT_6POINT0 = AV_CH_LAYOUT_6POINT0,
	LAYOUT_6POINT0_FRONT = AV_CH_LAYOUT_6POINT0_FRONT,
	LAYOUT_HEXAGONAL = AV_CH_LAYOUT_HEXAGONAL,
	LAYOUT_6POINT1 = AV_CH_LAYOUT_6POINT1,
	LAYOUT_6POINT1_BACK = AV_CH_LAYOUT_6POINT1_BACK,
	LAYOUT_6POINT1_FRONT = AV_CH_LAYOUT_6POINT1_FRONT,
	LAYOUT_7POINT0 = AV_CH_LAYOUT_7POINT0,
	LAYOUT_7POINT0_FRONT = AV_CH_LAYOUT_7POINT0_FRONT,
	LAYOUT_7POINT1 = AV_CH_LAYOUT_7POINT1,
	LAYOUT_7POINT1_WIDE = AV_CH_LAYOUT_7POINT1_WIDE,
	LAYOUT_7POINT1_WIDE_BACK = AV_CH_LAYOUT_7POINT1_WIDE_BACK,
	LAYOUT_OCTAGONAL = AV_CH_LAYOUT_OCTAGONAL,
	LAYOUT_STEREO_DOWNMIX = AV_CH_LAYOUT_STEREO_DOWNMIX
};

enum PictureType
{
	FRAME_NONE = AVPictureType::AV_PICTURE_TYPE_NONE,
	FRAME_INTRA = AVPictureType::AV_PICTURE_TYPE_I,
	FRAME_PREDICTED = AVPictureType::AV_PICTURE_TYPE_P,
	FRAME_BI_DIR_PREDICTED = AVPictureType::AV_PICTURE_TYPE_B,
	FRAME_S_GMC_VOP = AVPictureType::AV_PICTURE_TYPE_S,
	FRAME_SWITCHING_INTRA = AVPictureType::AV_PICTURE_TYPE_SI,
	FRAME_SWITCHING_PREDICTED = AVPictureType::AV_PICTURE_TYPE_SP,
	FRAME_BI = AVPictureType::AV_PICTURE_TYPE_BI
};

/*
=============================================================
Copyright Venatio Studios 2019
=============================================================
Revision History

0.0.1 : 2019-07-02 15:31
#vNext
=============================================================
*/
#endif