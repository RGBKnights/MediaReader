#ifndef GUARD_reader_20190207153203_
#define GUARD_reader_20190207153203_
/*
@file		reader.hpp
@author		Webstar
@date		2019-07-02 15:32
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/


// STD
#include <string>
#include <thread>
#include <chrono>

// FFmpeg Setup
#include "utilities.hpp"

#include "exceptions.hpp"
#include "common.hpp"
#include "fraction.hpp"
#include "cache.hpp"
#include "frame.hpp"

using namespace std;
using namespace vs;

namespace vs
{
	/// @brief This class uses the FFmpeg libraries, to open video files and audio files, and return
	/// Frame objects for any frame in the file.
	/// @remark All seeking and caching is handled internally, and the primary public interface is the GetFrame()
	/// method.  To use this reader, simply create an instance of this class, and call the GetFrame method
	/// to start retrieving frames.  Use the <b>info</b> struct to obtain information on the file, such as the length
	/// (# of frames), height, width, bit rate, frames per second (fps), etc...
	class FFmpegReader
	{
	private:
		std::mutex processing_mutex;
		std::mutex get_frames_mutex;

		string path;

		int max_width;
		int max_height;

		bool is_open;
		bool is_duration_known;
		bool check_fps;
		bool has_missing_frames;
		bool check_interlace;

		AVFormatContext *pFormatCtx;
		int i, videoStream, audioStream, subtitleStream;
		AVCodecContext *pCodecCtx, *aCodecCtx;
		AVStream *pStream, *aStream;
		AVPacket *packet;
		AVPicture *pFrame;
		int picture_type;

		FrameCache working_cache;
		FrameCache missing_frames;
		FrameCache final_cache;

		AudioLocation previous_packet_location;

		map<long int, long int> processing_video_frames;
		multimap<long int, long int> processing_audio_frames;
		map<long int, long int> processed_video_frames;
		map<long int, long int> processed_audio_frames;
		multimap<long int, long int> missing_video_frames;
		multimap<long int, long int> missing_video_frames_source;
		multimap<long int, long int> missing_audio_frames;
		multimap<long int, long int> missing_audio_frames_source;
		map<long int, int> checked_frames;

		long int audio_pts_offset;
		long int video_pts_offset;
		long int last_frame;
		long int largest_frame_processed;
		long int current_video_frame;

		bool is_seeking;
		long int seeking_pts;
		long int seeking_frame;
		bool is_video_seek;
		int seek_count;
		long int seek_audio_frame_found;
		long int seek_video_frame_found;

		QSharedPointer<Frame> last_video_frame;

		// Internal methods
		void UpdateAudioInfo();
		void UpdateVideoInfo();

		QSharedPointer<Frame> ReadStream(long int requested_frame);
		bool CheckMissingFrame(long int requested_frame);
		void CheckWorkingFrames(bool end_of_stream, long int requested_frame);
		bool IsPartialFrame(long int requested_frame);

		void UpdatePTSOffset(bool is_video);
		long int GetVideoPTS();
		int GetNextPacket();
		bool GetAVFrame();

		long int ConvertFrameToVideoPTS(long int frame_number);
		long int ConvertVideoPTStoFrame(long int pts);
		long int ConvertFrameToAudioPTS(long int frame_number);
		AudioLocation GetAudioPTSLocation(long int pts);

		void RemoveAVFrame(AVPicture*);
		void RemoveAVPacket(AVPacket*);

		QSharedPointer<Frame> CreateFrame(long int requested_frame);

		void Seek(long int requested_frame);
		bool CheckSeek(bool is_video);

		void ProcessVideoPacket(long int requested_frame);
		void ProcessAudioPacket(long int requested_frame, long int target_frame, int starting_sample);

	public:
		/// Constructor for FFmpegReader.
		FFmpegReader(string filename);

		/// Constructor for FFmpegReader.
		FFmpegReader(QString filename);

		/// Destructor
		~FFmpegReader();

		/// @brief Enable or disable seeking.  
		/// @remark Seeking can more quickly locate the requested frame, 
		/// but some codecs have trouble seeking, and can introduce 
		/// artifacts or blank images into the video.
		bool enable_seek;

		/// returns details of the media file.
		MediaInfo info;

		/// Get the cache object used by this reader
		FrameCache* GetCache() { return &final_cache; };

		/// Open File
		void Open();

		/// Close File
		void Close();

		/// Determine if reader is open or closed
		bool IsOpen() { return is_open; };

		/// Writes to std output the details of the media file.
		void DisplayInfo();

		/// @brief Get a shared pointer to a openshot::Frame object for a specific frame number of this reader.
		/// @returns The requested frame of video
		/// @param requested_frame	The frame number that is requested.
		QSharedPointer<Frame> GetFrame(long int requested_frame);
	};
}

/*
=============================================================
Copyright Venatio Studios 2019
=============================================================
Revision History

0.0.1 : 2019-07-02 15:32
#vNext
=============================================================
*/

#endif