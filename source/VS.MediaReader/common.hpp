#ifndef GUARD_common_20190207153310_
#define GUARD_common_20190207153310_
/*
@file		common.hpp
@author		Webstar
@date		2019-07-02 15:33
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

// STD
#include <string>

// Denyfy
#include "fraction.hpp"
#include "utilities.hpp"

using namespace std;
using namespace vs;

namespace vs
{
	struct Clip
	{
		long starting_frame_number;
		float starting_frame_time;
		int starting_frame_type;
		long ending_frame_number;
		long delta;
		float clip_length;
	};

	struct Cut
	{
		long frame;
		int picture_type;
		int cut_type;
	};

	/// @brief This struct contains info about a media file, such as height, width, frames per second, etc...
	/// @remark Each reader is responsible for updating this struct to reflect accurate information about the streams.
	struct MediaInfo
	{
		bool has_video;					///< Determines if this file has a video stream
		bool has_audio;					///< Determines if this file has an audio stream
		bool has_single_image;			///< Determines if this file only contains a single image
		float duration;					///< Length of time (in seconds)
		long long file_size;			///< Size of file (in bytes)
		int height;						///< The height of the video (in pixels)
		int width;						///< The width of the video (in pixesl)
		int pixel_format;				///< The pixel format (i.e. YUV420P, RGB24, etc...)
		Fraction fps;					///< Frames per second, as a fraction (i.e. 24/1 = 24 fps)
		int video_bit_rate;				///< The bit rate of the video stream (in bytes)
		Fraction pixel_ratio;			///< The pixel ratio of the video stream as a fraction (i.e. some pixels are not square)
		Fraction display_ratio;			///< The ratio of width to height of the video stream (i.e. 640x480 has a ratio of 4/3)
		string vcodec;					///< The name of the video codec used to encode / decode the video stream
		long int video_length;			///< The number of frames in the video stream
		int video_stream_index;			///< The index of the video stream
		Fraction video_timebase;		///< The video timebase determines how long each frame stays on the screen
		bool interlaced_frame;			///< Are the contents of this frame interlaced
		bool top_field_first;			///< Which interlaced field should be displayed first
		string acodec;					///< The name of the audio codec used to encode / decode the video stream
		int audio_bit_rate;				///< The bit rate of the audio stream (in bytes)
		int sample_rate;				///< The number of audio samples per second (44100 is a common sample rate)
		int channels;					///< The number of audio channels used in the audio stream
		ChannelLayout channel_layout;	///< The channel layout (mono, stereo, 5 point surround, etc...)
		int audio_stream_index;			///< The index of the audio stream
		Fraction audio_timebase;		///< The audio timebase determines how long each audio packet should be played
	};

	/// @brief This struct holds the associated video frame and starting sample # for an audio packet.
	/// @remark Because audio packets do not match up with video frames, this helps determine exactly
	/// where the audio packet's samples belong.
	struct AudioLocation
	{
		long int frame;
		int sample_start;
		bool is_near(AudioLocation location, int samples_per_frame, long int amount);
	};
}

/*
=============================================================
Copyright Venatio Studios 2019
=============================================================
Revision History

0.0.1 : 2019-07-02 15:33
#vNext
=============================================================
*/

#endif