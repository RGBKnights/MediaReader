/*
@file		reader.cpp
@author		Webstar
@date		2019-07-02 15:34
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

// STD
#include <iostream>
#include <iomanip>

// QT
#include <QDebug>
#include <QtGlobal>

#include "reader.hpp"

using namespace vs;
using namespace vs::exceptions;

bool AudioLocation::is_near(AudioLocation location, int samples_per_frame, long int amount)
{
	// Is frame even close to this one?
	if (abs(location.frame - frame) >= 2)
		return false; // This is too far away to be considered

	// Note that samples_per_frame can vary slightly frame to frame when the
	// audio sampling rate is not an integer multiple of the video fps.
	long int diff = samples_per_frame * (location.frame - frame) + location.sample_start - sample_start;
	if (abs(diff) <= amount)
		return true;
	else
		return false;
}

// Public

FFmpegReader::FFmpegReader(string filename)
	: path(filename),
	max_width(0), max_height(0), last_frame(0), is_seeking(0), seeking_pts(0), seeking_frame(0), seek_count(0),
	largest_frame_processed(0), current_video_frame(0), seek_audio_frame_found(0), seek_video_frame_found(0),
	audio_pts_offset(99999), video_pts_offset(99999), 
	is_video_seek(true), check_interlace(false),check_fps(false), enable_seek(true), is_open(false), is_duration_known(false), has_missing_frames(false),
	packet(NULL), 
	picture_type(0)
{
	// Initialize info struct
	info.has_video = false;
	info.has_audio = false;
	info.has_single_image = false;
	info.duration = 0.0;
	info.file_size = 0;
	info.height = 0;
	info.width = 0;
	info.pixel_format = -1;
	info.fps = Fraction();
	info.video_bit_rate = 0;
	info.pixel_ratio = Fraction();
	info.display_ratio = Fraction();
	info.vcodec = "";
	info.acodec = "";
	info.video_length = 0;
	info.video_stream_index = -1;
	info.video_timebase = Fraction();
	info.interlaced_frame = false;
	info.top_field_first = true;
	info.acodec = "";
	info.audio_bit_rate = 0;
	info.sample_rate = 0;
	info.channels = 0;
	info.channel_layout = LAYOUT_MONO;
	info.audio_stream_index = -1;
	info.audio_timebase = Fraction();

	// Initialize FFMpeg, and register all formats and codecs
	av_register_all();
	avcodec_register_all();
}


FFmpegReader::FFmpegReader(QString filename) 
	: FFmpegReader(filename.toStdString())  // Delegate Constructor
{
}

FFmpegReader::~FFmpegReader()
{
	if (is_open)
	{
		// Auto close reader if not already done
		Close();
	}
}

void FFmpegReader::Open()
{
	if (is_open)
	{
		return;
	}

	if (path.empty())
	{
		throw InvalidFile("File could not be opened.", path);
	}

	unsigned int num_threads = std::thread::hardware_concurrency();

	// Initialize format context
	pFormatCtx = NULL;

	int result = 0;

	// Open video file
	result = avformat_open_input(&pFormatCtx, path.c_str(), NULL, NULL);
	if (result < 0)
		throw InvalidFile("File could not be opened.", path);

	// Retrieve stream information
	result = avformat_find_stream_info(pFormatCtx, NULL);
	if (result < 0)
		throw NoStreamsFound("No streams found in file.", path);

	videoStream = -1;
	audioStream = -1;
	subtitleStream = -1;

	// Loop through each stream, and identify the video and audio stream index
	for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		// Is this a video stream?
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStream < 0) {
			videoStream = i;
		}
		// Is this an audio stream?
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStream < 0) {
			audioStream = i;
		}
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE && subtitleStream < 0) {
			subtitleStream = i;
		}
	}

	if (videoStream == -1 && audioStream == -1)
		throw NoStreamsFound("No video or audio streams found in this file.", path);

	// Is there a video stream?
	if (videoStream != -1)
	{
		// Set the stream index
		info.video_stream_index = videoStream;

		// Set the codec and codec context pointers
		pStream = pFormatCtx->streams[videoStream];

		AVCodec *input_codec;
		if (!(input_codec = avcodec_find_decoder(pStream->codecpar->codec_id)))
			throw InvalidCodec("A valid audio codec could not be found for this file.", path);

		pCodecCtx = avcodec_alloc_context3(input_codec);

		if (avcodec_parameters_to_context(pCodecCtx, pStream->codecpar) < 0)
			throw InvalidCodec("A valid video codec could not be found for this file.", path);

		// Set number of threads equal to number of processors + 1
		pCodecCtx->thread_count = num_threads;

		// Find the decoder for the video stream
		AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if (pCodec == NULL) {
			throw InvalidCodec("A valid video codec could not be found for this file.", path);
		}
		// Open video codec
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
			throw InvalidCodec("A video codec was found, but could not be opened.", path);

		// Update the File Info struct with video details (if a video stream is found)
		UpdateVideoInfo();
	}

	// Is there an audio stream?
	if (audioStream != -1)
	{
		// Set the stream index
		info.audio_stream_index = audioStream;

		// Get a pointer to the codec context for the audio stream
		aStream = pFormatCtx->streams[audioStream];

		AVCodec *input_codec;
		if (!(input_codec = avcodec_find_decoder(aStream->codecpar->codec_id)))
			throw InvalidCodec("A valid audio codec could not be found for this file.", path);

		aCodecCtx = avcodec_alloc_context3(input_codec);

		if (avcodec_parameters_to_context(aCodecCtx, aStream->codecpar) < 0)
			throw InvalidCodec("A valid audio codec could not be found for this file.", path);

		// Set number of threads equal to number of processors + 1
		aCodecCtx->thread_count = num_threads;

		// Find the decoder for the audio stream
		AVCodec *aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
		if (aCodec == NULL) {
			throw InvalidCodec("A valid audio codec could not be found for this file.", path);
		}
		// Open audio codec
		if (avcodec_open2(aCodecCtx, aCodec, NULL) < 0)
			throw InvalidCodec("An audio codec was found, but could not be opened.", path);

		// Update the File Info struct with audio details (if an audio stream is found)
		UpdateAudioInfo();
	}

	// Is there an subtitle track
	if (subtitleStream != -1)
	{
		//TODO: implment subtitles
	}

	// Init previous audio location to zero
	previous_packet_location.frame = -1;
	previous_packet_location.sample_start = 0;

	// Adjust cache size based on size of frame and audio
	working_cache.SetMaxBytesFromInfo(num_threads * 30, info.width, info.height, info.sample_rate, info.channels);
	missing_frames.SetMaxBytesFromInfo(num_threads * 2, info.width, info.height, info.sample_rate, info.channels);
	final_cache.SetMaxBytesFromInfo(num_threads * 2, info.width, info.height, info.sample_rate, info.channels);

	// Mark as "open"
	is_open = true;
}

void FFmpegReader::Close()
{
	// Close all objects, if reader is 'open'
	if (is_open)
	{
		// Mark as "closed"
		is_open = false;

		// Close the codec
		if (info.has_video)
		{
			avcodec_flush_buffers(pCodecCtx);
			avcodec_close(pCodecCtx);
		}
		if (info.has_audio)
		{
			avcodec_flush_buffers(aCodecCtx);
			avcodec_close(aCodecCtx);
		}

		// Clear final cache
		final_cache.Clear();
		working_cache.Clear();
		missing_frames.Clear();

		// Clear processed lists
		{
			//std::lock_guard<std::mutex> lock(processing_mutex);

			processed_video_frames.clear();
			processed_audio_frames.clear();
			processing_video_frames.clear();
			processing_audio_frames.clear();
			missing_audio_frames.clear();
			missing_video_frames.clear();
			missing_audio_frames_source.clear();
			missing_video_frames_source.clear();
			checked_frames.clear();
		}

		// Close the video file
		avformat_close_input(&pFormatCtx);
		av_freep(&pFormatCtx);

		// Reset some variables
		last_frame = 0;
		largest_frame_processed = 0;
		seek_audio_frame_found = 0;
		seek_video_frame_found = 0;
		current_video_frame = 0;
		has_missing_frames = false;
	}
}

void FFmpegReader::DisplayInfo()
{
	cout << fixed << setprecision(2) << boolalpha;
	cout << "----------------------------" << endl;
	cout << "----- File Information -----" << endl;
	cout << "----------------------------" << endl;
	cout << "--> Has Video: " << info.has_video << endl;
	cout << "--> Has Audio: " << info.has_audio << endl;
	cout << "--> Has Single Image: " << info.has_single_image << endl;
	cout << "--> Duration: " << info.duration << " Seconds" << endl;
	cout << "--> File Size: " << double(info.file_size) / 1024 / 1024 << " MB" << endl;
	if (info.has_video)
	{
		cout << "----------------------------" << endl;
		cout << "----- Video Attributes -----" << endl;
		cout << "----------------------------" << endl;
		cout << "--> Width: " << info.width << endl;
		cout << "--> Height: " << info.height << endl;
		cout << "--> Pixel Format: " << info.pixel_format << endl;
		cout << "--> Frames Per Second: " << info.fps.ToDouble() << " (" << info.fps.num << "/" << info.fps.den << ")" << endl;
		cout << "--> Video Bit Rate: " << info.video_bit_rate / 1000 << " kb/s" << endl;
		cout << "--> Pixel Ratio: " << info.pixel_ratio.ToDouble() << " (" << info.pixel_ratio.num << "/" << info.pixel_ratio.den << ")" << endl;
		cout << "--> Display Aspect Ratio: " << info.display_ratio.ToDouble() << " (" << info.display_ratio.num << "/" << info.display_ratio.den << ")" << endl;
		cout << "--> Video Codec: " << info.vcodec << endl;
		cout << "--> Video Length: " << info.video_length << " Frames" << endl;
		cout << "--> Video Stream Index: " << info.video_stream_index << endl;
		cout << "--> Video Timebase: " << info.video_timebase.ToDouble() << " (" << info.video_timebase.num << "/" << info.video_timebase.den << ")" << endl;
		cout << "--> Interlaced: " << info.interlaced_frame << endl;
		cout << "--> Interlaced: Top Field First: " << info.top_field_first << endl;
	}
	if (info.has_audio)
	{
		cout << "----------------------------" << endl;
		cout << "----- Audio Attributes -----" << endl;
		cout << "----------------------------" << endl;
		cout << "--> Audio Codec: " << info.acodec << endl;
		cout << "--> Audio Bit Rate: " << info.audio_bit_rate / 1000 << " kb/s" << endl;
		cout << "--> Sample Rate: " << info.sample_rate << " Hz" << endl;
		cout << "--> # of Channels: " << info.channels << endl;
		cout << "--> Channel Layout: " << info.channel_layout << endl;
		cout << "--> Audio Stream Index: " << info.audio_stream_index << endl;
		cout << "--> Audio Timebase: " << info.audio_timebase.ToDouble() << " (" << info.audio_timebase.num << "/" << info.audio_timebase.den << ")" << endl;
	}
	cout << "----------------------------" << endl;
}

QSharedPointer<Frame> FFmpegReader::GetFrame(long int requested_frame)
{
	// Check for open reader (or throw exception)
	if (!is_open)
		throw ReaderClosed("The FFmpegReader is closed.  Call Open() before calling this method.", path);

	// Gruads
	if (requested_frame < 1)
		requested_frame = 1;

	if (requested_frame > info.video_length && is_duration_known)
		requested_frame = info.video_length;

	if (info.has_video && info.video_length == 0)
		throw InvalidFile("Could not detect the duration of the video or audio stream.", path);


	// Check the cache for this frame
	QSharedPointer<Frame> frame = final_cache.GetFrame(requested_frame);
	if (frame)
	{
		return frame; // Return the cached frame
	}
	else
	{
		//std::lock_guard<std::mutex> lock(get_frames_mutex);

		// Check the cache a 2nd time (due to a potential previous lock)
		if (has_missing_frames)
		{
			bool result = CheckMissingFrame(requested_frame);
		}

		frame = final_cache.GetFrame(requested_frame);
		if (frame)
		{
			return frame;
		}

		// Frame is not in cache
		// Reset seek count
		seek_count = 0;

		// Check for first frame (always need to get frame 1 before other frames, to correctly calculate offsets)
		if (last_frame == 0 && requested_frame != 1)
		{
			// Get first frame
			ReadStream(1);
		}

		// Are we within X frames of the requested frame?
		long int diff = requested_frame - last_frame;
		if (diff >= 1 && diff <= 20)
		{
			// Continue walking the stream
			return ReadStream(requested_frame);
		}
		else
		{
			// Greater than 30 frames away, or backwards, we need to seek to the nearest key frame... Only seek if enabled
			if (enable_seek)
			{
				Seek(requested_frame);
			}

			else if (!enable_seek && diff < 0)
			{
				// Start over, since we can't seek, and the requested frame is smaller than our position
				Close();
				Open();
			}

			// Then continue walking the stream
			return ReadStream(requested_frame);
		}

	}
}

// Private

void FFmpegReader::UpdateAudioInfo()
{
	// Set values of FileInfo struct
	info.has_audio = true;
	info.file_size = pFormatCtx->pb ? avio_size(pFormatCtx->pb) : -1;
	info.acodec = aCodecCtx->codec->name;
	info.channels = aCodecCtx->channels;
	if (aCodecCtx->channel_layout == 0) aCodecCtx->channel_layout = av_get_default_channel_layout(aCodecCtx->channels);
	info.channel_layout = (ChannelLayout)aCodecCtx->channel_layout;
	info.sample_rate = aCodecCtx->sample_rate;
	info.audio_bit_rate = aCodecCtx->bit_rate;

	// Set audio timebase
	info.audio_timebase.num = aStream->time_base.num;
	info.audio_timebase.den = aStream->time_base.den;

	// Get timebase of audio stream (if valid) and greater than the current duration
	if (aStream->duration > 0.0f && aStream->duration > info.duration)
		info.duration = aStream->duration * info.audio_timebase.ToDouble();

	// Check for an invalid video length
	if (info.has_video && info.video_length <= 0)
	{
		// Calculate the video length from the audio duration
		info.video_length = info.duration * info.fps.ToDouble();
	}

	// Set video timebase (if no video stream was found)
	if (!info.has_video)
	{
		// Set a few important default video settings (so audio can be divided into frames)
		info.fps.num = 24;
		info.fps.den = 1;
		info.video_timebase.num = 1;
		info.video_timebase.den = 24;
		info.video_length = info.duration * info.fps.ToDouble();
		info.width = 720;
		info.height = 480;

	}
}

void FFmpegReader::UpdateVideoInfo()
{
	// Set values of FileInfo struct
	info.has_video = true;
	info.file_size = pFormatCtx->pb ? avio_size(pFormatCtx->pb) : -1;
	info.height = pCodecCtx->height;
	info.width = pCodecCtx->width;
	info.vcodec = pCodecCtx->codec->name;
	info.video_bit_rate = pFormatCtx->bit_rate;
	if (!check_fps)
	{
		// set frames per second (fps)
		info.fps.num = pStream->avg_frame_rate.num;
		info.fps.den = pStream->avg_frame_rate.den;
	}

	if (pStream->sample_aspect_ratio.num != 0)
	{
		info.pixel_ratio.num = pStream->sample_aspect_ratio.num;
		info.pixel_ratio.den = pStream->sample_aspect_ratio.den;
	}
	else if (pCodecCtx->sample_aspect_ratio.num != 0)
	{
		info.pixel_ratio.num = pCodecCtx->sample_aspect_ratio.num;
		info.pixel_ratio.den = pCodecCtx->sample_aspect_ratio.den;
	}
	else
	{
		info.pixel_ratio.num = 1;
		info.pixel_ratio.den = 1;
	}

	info.pixel_format = pCodecCtx->pix_fmt;

	// Calculate the DAR (display aspect ratio)
	Fraction size(info.width * info.pixel_ratio.num, info.height * info.pixel_ratio.den);

	// Reduce size fraction
	size.Reduce();

	// Set the ratio based on the reduced fraction
	info.display_ratio.num = size.num;
	info.display_ratio.den = size.den;

	// Set the video timebase
	info.video_timebase.num = pStream->time_base.num;
	info.video_timebase.den = pStream->time_base.den;

	// Set the duration in seconds, and video length (# of frames)
	info.duration = pStream->duration * info.video_timebase.ToDouble();

	// Check for valid duration (if found)
	if (info.duration <= 0.0f && pFormatCtx->duration >= 0)
		// Use the format's duration
		info.duration = pFormatCtx->duration / AV_TIME_BASE;

	// Calculate duration from filesize and bitrate (if any)
	if (info.duration <= 0.0f && info.video_bit_rate > 0 && info.file_size > 0)
		// Estimate from bitrate, total bytes, and framerate
		info.duration = (info.file_size / info.video_bit_rate);

	// No duration found in stream of file
	if (info.duration <= 0.0f)
	{
		// No duration is found in the video stream
		info.duration = -1;
		info.video_length = -1;
		is_duration_known = false;
	}
	else
	{
		// Yes, a duration was found
		is_duration_known = true;

		// Calculate number of frames
		info.video_length = round(info.duration * info.fps.ToDouble());
	}

	// Override an invalid framerate
	if (info.fps.ToFloat() > 120.0f || (info.fps.num == 0 || info.fps.den == 0))
	{
		// Set a few important default video settings (so audio can be divided into frames)
		info.fps.num = 24;
		info.fps.den = 1;
		info.video_timebase.num = 1;
		info.video_timebase.den = 24;

		// Calculate number of frames
		info.video_length = round(info.duration * info.fps.ToDouble());
	}

}

// Check if a frame is missing and attempt to replace it's frame image (and
bool FFmpegReader::CheckMissingFrame(long int requested_frame)
{
	// Lock
	//std::lock_guard<std::mutex> lock(processing_mutex);

	// Init # of times this frame has been checked so far
	int checked_count = 0;

	// Increment check count for this frame (or init to 1)
	if (checked_frames.count(requested_frame) == 0)
	{
		checked_frames[requested_frame] = 1;
	}
	else
	{
		checked_frames[requested_frame]++;
	}

	checked_count = checked_frames[requested_frame];

	// Missing frames (sometimes frame #'s are skipped due to invalid or missing timestamps)
	map<long int, long int>::iterator itr;
	bool found_missing_frame = false;

	// Check if requested frame is a missing frame
	if (missing_video_frames.count(requested_frame) || missing_audio_frames.count(requested_frame))
	{
		long int missing_source_frame = -1;
		if (missing_video_frames.count(requested_frame))
		{
			missing_source_frame = missing_video_frames.find(requested_frame)->second;
		}
		else if (missing_audio_frames.count(requested_frame))
		{
			missing_source_frame = missing_audio_frames.find(requested_frame)->second;
		}

		// Increment missing source frame check count (or init to 1)
		if (checked_frames.count(missing_source_frame) == 0)
		{
			checked_frames[missing_source_frame] = 1;
		}
		else
		{
			checked_frames[missing_source_frame]++;
		}

		// Get the previous frame of this missing frame (if it's available in missing cache)
		QSharedPointer<Frame> parent_frame = missing_frames.GetFrame(missing_source_frame);
		if (parent_frame == NULL)
		{
			parent_frame = final_cache.GetFrame(missing_source_frame);
			if (parent_frame != NULL)
			{
				// Add missing final frame to missing cache
				missing_frames.Add(parent_frame);
			}
		}

		// Create blank missing frame
		QSharedPointer<Frame> missing_frame = CreateFrame(requested_frame);

		// If previous frame found, copy image from previous to missing frame (else we'll just wait a bit and try again later)
		if (parent_frame != NULL)
		{
			// Add this frame to the processed map (since it's already done)
			QSharedPointer<QImage> parent_image = parent_frame->GetImage();
			if (parent_image) 
			{
				missing_frame->AddImage(QSharedPointer<QImage>(new QImage(*parent_image)));

				processed_video_frames[missing_frame->number] = missing_frame->number;
				processed_audio_frames[missing_frame->number] = missing_frame->number;

				// Move frame to final cache
				final_cache.Add(missing_frame);

				// Remove frame from working cache
				working_cache.Remove(missing_frame->number);

				// Update last_frame processed
				last_frame = missing_frame->number;
			}
		}
	}

	return found_missing_frame;
}

// Check the working queue, and move finished frames to the finished queue
void FFmpegReader::CheckWorkingFrames(bool end_of_stream, long int requested_frame)
{
	// Loop through all working queue frames
	bool checked_count_tripped = false;
	int max_checked_count = 80;

	while (true)
	{
		// Get the front frame of working cache
		QSharedPointer<Frame> f(working_cache.GetSmallestFrame());

		// Was a frame found?
		if (!f)
		{
			break;
		}

		// Check if this frame is 'missing'
		CheckMissingFrame(f->number);

		// Init # of times this frame has been checked so far
		int checked_count = 0;
		int checked_frames_size = 0;

		bool is_video_ready = false;
		bool is_audio_ready = false;

		// limit scope of next few lines... for locking
		{
			//std::lock_guard<std::mutex> lock(processing_mutex);

			is_video_ready = processed_video_frames.count(f->number);
			is_audio_ready = processed_audio_frames.count(f->number);

			// Get check count for this frame
			checked_frames_size = checked_frames.size();
			if (!checked_count_tripped || f->number >= requested_frame)
			{
				checked_count = checked_frames[f->number];
			}
			else
			{
				// Force checked count over the limit
				checked_count = max_checked_count;
			}
		}

		if (previous_packet_location.frame == f->number && !end_of_stream)
		{
			is_audio_ready = false; // don't finalize the last processed audio frame
		}

		bool is_seek_trash = IsPartialFrame(f->number);

		// Adjust for available streams
		if (!info.has_video) is_video_ready = true;
		if (!info.has_audio) is_audio_ready = true;

		// Make final any frames that get stuck (for whatever reason)
		if (checked_count >= max_checked_count && (!is_video_ready || !is_audio_ready))
		{
			// Trigger checked count tripped mode (clear out all frames before requested frame)
			checked_count_tripped = true;

			if (info.has_video && !is_video_ready && last_video_frame) 
			{
				// Copy image from last frame
				f->AddImage(QSharedPointer<QImage>(new QImage(*last_video_frame->GetImage())));

				is_video_ready = true;
			}

			if (info.has_audio && !is_audio_ready)
			{
				//std::lock_guard<std::mutex> lock(processing_mutex);

				// Mark audio as processed, and indicate the frame has audio data
				is_audio_ready = true;
			}
		}

		// Check if working frame is final
		if ((!end_of_stream && is_video_ready && is_audio_ready) || end_of_stream || is_seek_trash)
		{
			if (!is_seek_trash)
			{
				// Move frame to final cache
				final_cache.Add(f);

				// Add to missing cache (if another frame depends on it)
				{
					//std::lock_guard<std::mutex> lock(processing_mutex);

					if (missing_video_frames_source.count(f->number))
					{
						missing_frames.Add(f);
					}

					// Remove from 'checked' count
					checked_frames.erase(f->number);
				}

				// Remove frame from working cache
				working_cache.Remove(f->number);

				// Update last frame processed
				last_frame = f->number;

			}
			else
			{
				// Seek trash, so delete the frame from the working cache, and never add it to the final cache.
				working_cache.Remove(f->number);
			}
		}
		else
		{
			break;
		}
	}
}

// Read the stream until we find the requested Frame
QSharedPointer<Frame> FFmpegReader::ReadStream(long int requested_frame)
{
	// Allocate video frame
	bool end_of_stream = false;
	bool check_seek = false;
	bool frame_finished = false;
	int packet_error = -1;

	// Minimum number of packets to process (for performance reasons)
	int packets_processed = 0;
	int minimum_packets = std::thread::hardware_concurrency();
	int max_packets = 4096;

	// TODO: Convert to parallel version
	// Loop through the stream until the correct frame is found
	while (true)
	{
		// Get the next packet into a local variable called packet
		packet_error = GetNextPacket();

		int processing_video_frames_size = 0;
		int processing_audio_frames_size = 0;
		{
			//std::lock_guard<std::mutex> lock(processing_mutex);

			processing_video_frames_size = processing_video_frames.size();
			processing_audio_frames_size = processing_audio_frames.size();
		}

		// Wait if too many frames are being processed
		while (processing_video_frames_size + processing_audio_frames_size >= minimum_packets)
		{
			std::this_thread::sleep_for(2.5s);

			//std::lock_guard<std::mutex> lock(processing_mutex);

			processing_video_frames_size = processing_video_frames.size();
			processing_audio_frames_size = processing_audio_frames.size();
		}

		// Get the next packet (if any)
		if (packet_error < 0)
		{
			// Break loop when no more packets found
			end_of_stream = true;
			break;
		}

		if (info.has_video && packet->stream_index == videoStream) // Video packet
		{
			// Check the status of a seek (if any)
			if (is_seeking)
			{
				check_seek = CheckSeek(true);
			}
			else
			{
				check_seek = false;
			}

			if (check_seek) {
				// Jump to the next iteration of this loop
				continue;
			}

			// Get the AVFrame from the current packet
			frame_finished = GetAVFrame();

			// Check if the AVFrame is finished and set it
			if (frame_finished)
			{
				// Update PTS / Frame Offset (if any)
				UpdatePTSOffset(true);

				// Process Video Packet
				ProcessVideoPacket(requested_frame);
			}

		}
		else if (info.has_audio && packet->stream_index == audioStream) // Audio packet
		{
			// Check the status of a seek (if any)
			if (is_seeking)
			{
				check_seek = CheckSeek(false);
			}
			else
			{
				check_seek = false;
			}

			if (check_seek) 
			{
				// Jump to the next iteration of this loop
				continue;
			}

			// Update PTS / Frame Offset (if any)
			UpdatePTSOffset(false);

			// Determine related video frame and starting sample # from audio PTS
			AudioLocation location = GetAudioPTSLocation(packet->pts);

			// Process Audio Packet
			ProcessAudioPacket(requested_frame, location.frame, location.sample_start);
		}

		// Check if working frames are 'finished'
		bool is_cache_found = false;
		if (!is_seeking)
		{
			// Check for any missing frames
			CheckMissingFrame(requested_frame);

			// Check for final frames
			CheckWorkingFrames(false, requested_frame);
		}

		// Check if requested 'final' frame is available
		is_cache_found = (final_cache.GetFrame(requested_frame) != NULL);

		// Increment frames processed
		packets_processed++;

		// Break once the frame is found
		if ((is_cache_found && packets_processed >= minimum_packets) || packets_processed > max_packets)
			break;

	} // end while

	  // End of stream?
	if (end_of_stream)
	{
		// Mark the any other working frames as 'finished'
		CheckWorkingFrames(end_of_stream, requested_frame);
	}

	// Return requested frame (if found)
	QSharedPointer<Frame> frame = final_cache.GetFrame(requested_frame);
	if (frame)
	{
		// Return prepared frame
		return frame;
	}
	else
	{
		// Check if largest frame is still cached
		frame = final_cache.GetFrame(largest_frame_processed);
		if (frame)
		{
			// return the largest processed frame (assuming it was the last in the video file)
			return frame;
		}
		else
		{
			// The largest processed frame is no longer in cache, return a blank frame
			QSharedPointer<Frame> f = CreateFrame(largest_frame_processed);
			f->AddColor(info.width, info.height, "#000");
			return f;
		}
	}
}

// Get the next packet (if any)
int FFmpegReader::GetNextPacket()
{
	int found_packet = 0;
	AVPacket *next_packet = new AVPacket();
	found_packet = av_read_frame(pFormatCtx, next_packet);

	if (packet)
	{
		// Remove previous packet before getting next one
		RemoveAVPacket(packet);
		packet = NULL;
	}

	if (found_packet >= 0)
	{
		// Update current packet pointer
		packet = next_packet;
	}

	// Return if packet was found (or error number)
	return found_packet;
}

// Remove AVPacket from cache (and deallocate it's memory)
void FFmpegReader::RemoveAVPacket(AVPacket* remove_packet)
{
	// deallocate memory for packet
	AV_FREE_PACKET(remove_packet);

	// Delete the object
	delete remove_packet;
}

// Remove AVFrame from cache (and deallocate it's memory)
void FFmpegReader::RemoveAVFrame(AVPicture* remove_frame)
{
	// Remove pFrame (if exists)
	if (remove_frame)
	{
		// Free memory
		avpicture_free(remove_frame);

		// Delete the object
		delete remove_frame;
	}
}

// Get an AVFrame (if any)
bool FFmpegReader::GetAVFrame()
{
	int frameFinished = -1;

	// Decode video frame
	AVFrame *next_frame = AV_ALLOCATE_FRAME();
	avcodec_decode_video2(pCodecCtx, next_frame, &frameFinished, packet);

	// is frame finished
	if (frameFinished)
	{
		// AVFrames are clobbered on the each call to avcodec_decode_video, so we
		// must make a copy of the image data before this method is called again.
		pFrame = new AVPicture();
		avpicture_alloc(pFrame, pCodecCtx->pix_fmt, info.width, info.height);
		av_picture_copy(pFrame, (AVPicture *)next_frame, pCodecCtx->pix_fmt, info.width, info.height);

		// this need for the scene dection so include it for frame details
		picture_type = next_frame->pict_type;
		// next_frame->metadata

		// Detect interlaced frame (only once)
		if (!check_interlace)
		{
			check_interlace = true;
			info.interlaced_frame = next_frame->interlaced_frame;
			info.top_field_first = next_frame->top_field_first;
		}
	}

	// deallocate the frame
	AV_FREE_FRAME(&next_frame);

	// Did we get a video frame?
	return frameFinished;
}

// Determine if frame is partial due to seek
bool FFmpegReader::IsPartialFrame(long int requested_frame) {

	// Sometimes a seek gets partial frames, and we need to remove them
	bool seek_trash = false;
	// determine max seeked frame
	long int max_seeked_frame = seek_audio_frame_found;
	if (seek_video_frame_found > max_seeked_frame)
	{
		max_seeked_frame = seek_video_frame_found;
	}

	if ((info.has_audio && seek_audio_frame_found && max_seeked_frame >= requested_frame) ||
		(info.has_video && seek_video_frame_found && max_seeked_frame >= requested_frame))
	{
		seek_trash = true;
	}

	return seek_trash;
}

// Update PTS Offset (if any)
void FFmpegReader::UpdatePTSOffset(bool is_video)
{
	// Determine the offset between the PTS and Frame number (only for 1st frame)
	if (is_video)
	{
		// VIDEO PACKET
		if (video_pts_offset == 99999) // Has the offset been set yet?
		{
			// Find the difference between PTS and frame number (no more than 10 timebase units allowed)
			video_pts_offset = 0 - max(GetVideoPTS(), (long)info.video_timebase.ToInt() * 10);
		}
	}
	else
	{
		// AUDIO PACKET
		if (audio_pts_offset == 99999) // Has the offset been set yet?
		{
			// Find the difference between PTS and frame number (no more than 10 timebase units allowed)
			audio_pts_offset = 0 - max(packet->pts, (int64_t)info.audio_timebase.ToInt() * 10);
		}
	}
}

// Get the PTS for the current video packet
long int FFmpegReader::GetVideoPTS()
{
	long int current_pts = 0;
	if (packet->dts != AV_NOPTS_VALUE)
		current_pts = packet->dts;

	// Return adjusted PTS
	return current_pts;
}

// Calculate Starting video frame and sample # for an audio PTS
AudioLocation FFmpegReader::GetAudioPTSLocation(long int pts)
{
	// Apply PTS offset
	pts = pts + audio_pts_offset;

	// Get the audio packet start time (in seconds)
	double audio_seconds = double(pts) * info.audio_timebase.ToDouble();

	// Divide by the video timebase, to get the video frame number (frame # is decimal at this point)
	double frame = (audio_seconds * info.fps.ToDouble()) + 1;

	// Frame # as a whole number (no more decimals)
	long int whole_frame = long(frame);

	// Remove the whole number, and only get the decimal of the frame
	double sample_start_percentage = frame - double(whole_frame);

	// Get Samples per frame
	int samples_per_frame = Frame::GetSamplesPerFrame(whole_frame, info.fps, info.sample_rate, info.channels);

	// Calculate the sample # to start on
	int sample_start = round(double(samples_per_frame) * sample_start_percentage);

	// Protect against broken (i.e. negative) timestamps
	if (whole_frame < 1)
		whole_frame = 1;

	if (sample_start < 0)
		sample_start = 0;

	// Prepare final audio packet location
	AudioLocation location = { whole_frame, sample_start };

	// Compare to previous audio packet (and fix small gaps due to varying PTS timestamps)
	if (previous_packet_location.frame != -1)
	{
		if (location.is_near(previous_packet_location, samples_per_frame, samples_per_frame))
		{
			long int orig_frame = location.frame;
			int orig_start = location.sample_start;

			// Update sample start, to prevent gaps in audio
			location.sample_start = previous_packet_location.sample_start;
			location.frame = previous_packet_location.frame;
		}
		else
		{
			//std::lock_guard<std::mutex> lock(processing_mutex);

			for (long int audio_frame = previous_packet_location.frame; audio_frame < location.frame; audio_frame++)
			{
				if (!missing_audio_frames.count(audio_frame))
				{
					missing_audio_frames.insert(pair<long int, long int>(previous_packet_location.frame - 1, audio_frame));
				}
			}
		}
	}

	// Set previous location
	previous_packet_location = location;

	// Return the associated video frame and starting sample #
	return location;
}

// Seek to a specific frame.  This is not always frame accurate, it's more of an estimation on many codecs.
void FFmpegReader::Seek(long int requested_frame)
{
	// Adjust for a requested frame that is too small or too large
	if (requested_frame < 1)
		requested_frame = 1;

	if (requested_frame > info.video_length)
		requested_frame = info.video_length;

	int processing_video_frames_size = 0;
	int processing_audio_frames_size = 0;
	{
		//std::lock_guard<std::mutex> lock(processing_mutex);
		processing_video_frames_size = processing_video_frames.size();
		processing_audio_frames_size = processing_audio_frames.size();
	}

	// Wait for any processing frames to complete
	while (processing_video_frames_size + processing_audio_frames_size > 0)
	{
		std::this_thread::sleep_for(2.5s);
		//std::lock_guard<std::mutex> lock(processing_mutex);
		processing_video_frames_size = processing_video_frames.size();
		processing_audio_frames_size = processing_audio_frames.size();
	}

	// Clear working cache (since we are seeking to another location in the file)
	working_cache.Clear();
	missing_frames.Clear();

	// Clear processed lists
	{
		//std::lock_guard<std::mutex> lock(processing_mutex);
		processing_audio_frames.clear();
		processing_video_frames.clear();
		processed_video_frames.clear();
		processed_audio_frames.clear();
		missing_audio_frames.clear();
		missing_video_frames.clear();
		missing_audio_frames_source.clear();
		missing_video_frames_source.clear();
		checked_frames.clear();
	}

	// Reset the last frame variable
	last_frame = 0;
	current_video_frame = 0;
	largest_frame_processed = 0;
	has_missing_frames = false;
	bool has_audio_override = info.has_audio;
	bool has_video_override = info.has_video;

	// Increment seek count
	seek_count++;

	// If seeking near frame 1, we need to close and re-open the file (this is more reliable than seeking)
	int buffer_amount = 6;
	if (requested_frame - buffer_amount < 20)
	{
		// Close and re-open file (basically seeking to frame 1)
		Close();
		Open();

		// Update overrides (since closing and re-opening might update these)
		info.has_audio = has_audio_override;
		info.has_video = has_video_override;

		// Not actually seeking, so clear these flags
		is_seeking = false;
		if (seek_count == 1)
		{
			// Don't redefine this on multiple seek attempts for a specific frame
			seeking_frame = 1;
			seeking_pts = ConvertFrameToVideoPTS(1);
		}
		seek_audio_frame_found = 0; // used to detect which frames to throw away after a seek
		seek_video_frame_found = 0; // used to detect which frames to throw away after a seek
	}
	else
	{
		// Seek to nearest key-frame (aka, i-frame)
		bool seek_worked = false;
		int64_t seek_target = 0;

		// Seek video stream (if any)
		if (!seek_worked && info.has_video)
		{
			seek_target = ConvertFrameToVideoPTS(requested_frame - buffer_amount);
			if (av_seek_frame(pFormatCtx, info.video_stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0)
			{
				fprintf(stderr, "%s: error while seeking video stream\n", pFormatCtx->filename);
			}
			else
			{
				// VIDEO SEEK
				is_video_seek = true;
				seek_worked = true;
			}
		}

		// Seek audio stream (if not already seeked... and if an audio stream is found)
		if (!seek_worked && info.has_audio)
		{
			seek_target = ConvertFrameToAudioPTS(requested_frame - buffer_amount);
			if (av_seek_frame(pFormatCtx, info.audio_stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0)
			{
				fprintf(stderr, "%s: error while seeking audio stream\n", pFormatCtx->filename);
			}
			else
			{
				// AUDIO SEEK
				is_video_seek = false;
				seek_worked = true;
			}
		}

		// Was the seek successful?
		if (seek_worked)
		{
			// Flush audio buffer
			if (info.has_audio)
				avcodec_flush_buffers(aCodecCtx);

			// Flush video buffer
			if (info.has_video)
				avcodec_flush_buffers(pCodecCtx);

			// Reset previous audio location to zero
			previous_packet_location.frame = -1;
			previous_packet_location.sample_start = 0;

			// init seek flags
			is_seeking = true;
			if (seek_count == 1)
			{
				// Don't redefine this on multiple seek attempts for a specific frame
				seeking_pts = seek_target;
				seeking_frame = requested_frame;
			}
			seek_audio_frame_found = 0; // used to detect which frames to throw away after a seek
			seek_video_frame_found = 0; // used to detect which frames to throw away after a seek

		}
		else
		{
			// seek failed
			is_seeking = false;
			seeking_pts = 0;
			seeking_frame = 0;

			// dislable seeking for this reader (since it failed)
			// TODO: Find a safer way to do this... not sure how common it is for a seek to fail.
			enable_seek = false;

			// Close and re-open file (basically seeking to frame 1)
			Close();
			Open();

			// Update overrides (since closing and re-opening might update these)
			info.has_audio = has_audio_override;
			info.has_video = has_video_override;
		}
	}
}

// Check the current seek position and determine if we need to seek again
bool FFmpegReader::CheckSeek(bool is_video)
{
	// Are we seeking for a specific frame?
	if (is_seeking)
	{
		// Determine if both an audio and video packet have been decoded since the seek happened.
		// If not, allow the ReadStream method to keep looping
		if ((is_video_seek && !seek_video_frame_found) || (!is_video_seek && !seek_audio_frame_found))
			return false;

		// Check for both streams
		if ((info.has_video && !seek_video_frame_found) || (info.has_audio && !seek_audio_frame_found))
			return false;

		// Determine max seeked frame
		long int max_seeked_frame = seek_audio_frame_found; // determine max seeked frame
		if (seek_video_frame_found > max_seeked_frame)
			max_seeked_frame = seek_video_frame_found;

		// determine if we are "before" the requested frame
		if (max_seeked_frame >= seeking_frame)
		{
			// Seek again... to the nearest Keyframe
			Seek(seeking_frame - (20 * seek_count * seek_count));
		}
		else
		{
			// Seek worked, and we are "before" the requested frame
			is_seeking = false;
			seeking_frame = 0;
			seeking_pts = -1;
		}
	}

	// return the pts to seek to (if any)
	return is_seeking;
}

// Process an audio packet
void FFmpegReader::ProcessAudioPacket(long int requested_frame, long int target_frame, int starting_sample)
{
	// Track 1st audio packet after a successful seek
	if (!seek_audio_frame_found && is_seeking)
		seek_audio_frame_found = target_frame;

	// Are we close enough to decode the frame's audio?
	if (target_frame < (requested_frame - 20))
	{
		// Skip to next frame without decoding or caching
		return;
	}

	// Init an AVFrame to hold the decoded audio samples
	int frame_finished = 0;
	AVFrame *audio_frame = AV_ALLOCATE_FRAME();
	AV_RESET_FRAME(audio_frame);

	int packet_samples = 0;
	int data_size = 0;

	// re-initialize buffer size (it gets changed in the avcodec_decode_audio2 method call)
	int buf_size = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;
	int used = avcodec_decode_audio4(aCodecCtx, audio_frame, &frame_finished, packet);

	if (frame_finished) 
	{
		// determine how many samples were decoded
		int planar = av_sample_fmt_is_planar(aCodecCtx->sample_fmt);
		int plane_size = -1;
		data_size = av_samples_get_buffer_size(&plane_size,
			aCodecCtx->channels,
			audio_frame->nb_samples,
			aCodecCtx->sample_fmt, 1);

		// Calculate total number of samples
		packet_samples = audio_frame->nb_samples * aCodecCtx->channels;
	}

	// Estimate the # of samples and the end of this packet's location (to prevent GAPS for the next timestamp)
	int pts_remaining_samples = packet_samples / info.channels; // Adjust for zero based array

	// Add audio frame to list of processing audio frames
	{
		//std::lock_guard<std::mutex> lock(processing_mutex);

		processing_audio_frames.insert(pair<int, int>(previous_packet_location.frame, previous_packet_location.frame));
	}

	while (pts_remaining_samples)
	{
		// Get Samples per frame (for this frame number)
		int samples_per_frame = Frame::GetSamplesPerFrame(previous_packet_location.frame, info.fps, info.sample_rate, info.channels);

		// Calculate # of samples to add to this frame
		int samples = samples_per_frame - previous_packet_location.sample_start;
		if (samples > pts_remaining_samples)
			samples = pts_remaining_samples;

		// Decrement remaining samples
		pts_remaining_samples -= samples;

		if (pts_remaining_samples > 0) 
		{
			// next frame
			previous_packet_location.frame++;
			previous_packet_location.sample_start = 0;

			// Add audio frame to list of processing audio frames
			{
				//std::lock_guard<std::mutex> lock(processing_mutex);

				processing_audio_frames.insert(pair<int, int>(previous_packet_location.frame, previous_packet_location.frame));
			}

		}
		else 
		{
			// Increment sample start
			previous_packet_location.sample_start += samples;
		}
	}


	// Allocate audio buffer
	int16_t *audio_buf = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];

	// Create output frame
	AVFrame *audio_converted = AV_ALLOCATE_FRAME();
	AV_RESET_FRAME(audio_converted);
	audio_converted->nb_samples = audio_frame->nb_samples;
	av_samples_alloc(audio_converted->data, audio_converted->linesize, info.channels, audio_frame->nb_samples, AV_SAMPLE_FMT_S16, 0);

	AVAudioResampleContext *avr = NULL;
	int nb_samples = 0;

	// setup resample context
	avr = avresample_alloc_context();
	av_opt_set_int(avr, "in_channel_layout", aCodecCtx->channel_layout, 0);
	av_opt_set_int(avr, "out_channel_layout", aCodecCtx->channel_layout, 0);
	av_opt_set_int(avr, "in_sample_fmt", aCodecCtx->sample_fmt, 0);
	av_opt_set_int(avr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int(avr, "in_sample_rate", info.sample_rate, 0);
	av_opt_set_int(avr, "out_sample_rate", info.sample_rate, 0);
	av_opt_set_int(avr, "in_channels", info.channels, 0);
	av_opt_set_int(avr, "out_channels", info.channels, 0);
	int r = avresample_open(avr);

	// Convert audio samples
	nb_samples = avresample_convert(avr, 	// audio resample context
		audio_converted->data, 			// output data pointers
		audio_converted->linesize[0], 	// output plane size, in bytes. (0 if unknown)
		audio_converted->nb_samples,	// maximum number of samples that the output buffer can hold
		audio_frame->data,				// input data pointers
		audio_frame->linesize[0],		// input plane size, in bytes (0 if unknown)
		audio_frame->nb_samples);		// number of input samples to convert

										// Copy audio samples over original samples
	memcpy(audio_buf, audio_converted->data[0], audio_converted->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * info.channels);

	// Deallocate resample buffer
	avresample_close(avr);
	avresample_free(&avr);
	avr = NULL;

	// Free AVFrames
	av_free(audio_converted->data[0]);
	AV_FREE_FRAME(&audio_converted);

	long int starting_frame_number = -1;
	for (int channel_filter = 0; channel_filter < info.channels; channel_filter++)
	{
		// Array of floats (to hold samples for each channel)
		starting_frame_number = target_frame;
		int channel_buffer_size = packet_samples / info.channels;
		float *channel_buffer = new float[channel_buffer_size];

		// Init buffer array
		for (int z = 0; z < channel_buffer_size; z++)
			channel_buffer[z] = 0.0f;

		// Loop through all samples and add them to our Frame based on channel.
		// Toggle through each channel number, since channel data is stored like (left right left right)
		int channel = 0;
		int position = 0;
		for (int sample = 0; sample < packet_samples; sample++)
		{
			// Only add samples for current channel
			if (channel_filter == channel)
			{
				// Add sample (convert from (-32768 to 32768)  to (-1.0 to 1.0))
				channel_buffer[position] = audio_buf[sample] * (1.0f / (1 << 15));

				// Increment audio position
				position++;
			}

			// increment channel (if needed)
			if ((channel + 1) < info.channels)
			{
				// move to next channel
				channel++;
			}
			else
			{
				// reset channel
				channel = 0;
			}
		}

	
		// Loop through samples, and add them to the correct frames
		int start = starting_sample;
		int remaining_samples = channel_buffer_size;
		float *iterate_channel_buffer = channel_buffer;	// pointer to channel buffer
		while (remaining_samples > 0)
		{
			// Get Samples per frame (for this frame number)
			int samples_per_frame = Frame::GetSamplesPerFrame(starting_frame_number, info.fps, info.sample_rate, info.channels);

			// Calculate # of samples to add to this frame
			int samples = samples_per_frame - start;
			if (samples > remaining_samples)
				samples = remaining_samples;

			// Create or get the existing frame object
			QSharedPointer<Frame> f = CreateFrame(starting_frame_number);

			// Add samples for current channel to the frame. Reduce the volume to 98%, to prevent
			// some louder samples from maxing out at 1.0 (not sure why this happens)
			f->AddAudio(true, channel_filter, start, iterate_channel_buffer, samples, 0.98f);

			// Add or update cache
			working_cache.Add(f);

			// Decrement remaining samples
			remaining_samples -= samples;

			// Increment buffer (to next set of samples)
			if (remaining_samples > 0)
				iterate_channel_buffer += samples;

			// Increment frame number
			starting_frame_number++;

			// Reset starting sample #
			start = 0;
		}

		// clear channel buffer
		delete[] channel_buffer;
		channel_buffer = NULL;
		iterate_channel_buffer = NULL;
	}

	// Clean up some arrays
	delete[] audio_buf;
	audio_buf = NULL;

	// Remove audio frame from list of processing audio frames
	{
		//std::lock_guard<std::mutex> lock(processing_mutex);

		// Update all frames as completed
		for (long int f = target_frame; f < starting_frame_number; f++) 
		{
			// Remove the frame # from the processing list. NOTE: If more than one thread is
			// processing this frame, the frame # will be in this list multiple times. We are only
			// removing a single instance of it here.
			processing_audio_frames.erase(processing_audio_frames.find(f));

			// Check and see if this frame is also being processed by another thread
			if (processing_audio_frames.count(f) == 0)
			{
				// No other thread is processing it. Mark the audio as processed (final)
				processed_audio_frames[f] = f;
			}
		}

		if (target_frame == starting_frame_number) 
		{
			// This typically never happens, but just in case, remove the currently processing number
			processing_audio_frames.erase(processing_audio_frames.find(target_frame));
		}
	}

	// Free audio frame
	AV_FREE_FRAME(&audio_frame);

}

// Process a video packet
void FFmpegReader::ProcessVideoPacket(long int requested_frame)
{
	// Calculate current frame #
	long int current_frame = ConvertVideoPTStoFrame(GetVideoPTS());

	// Track 1st video packet after a successful seek
	if (!seek_video_frame_found && is_seeking)
		seek_video_frame_found = current_frame;

	// Are we close enough to decode the frame? and is this frame # valid?
	if ((current_frame < (requested_frame - 20)) || (current_frame == -1))
	{
		// Remove frame and packet
		RemoveAVFrame(pFrame);

		// Skip to next frame without decoding or caching
		return;
	}

	// Init some things local
	PixelFormat pix_fmt = pCodecCtx->pix_fmt;
	int height = info.height;
	int width = info.width;
	long int video_length = info.video_length;
	AVPicture *my_frame = pFrame;
	int pict_type = picture_type;

	// Add video frame to list of processing video frames
	//std::lock_guard<std::mutex> lock(processing_mutex);

	processing_video_frames[current_frame] = current_frame;

	// Create variables for a RGB Frame (since most videos are not in RGB, we must convert it)
	AVFrame *pFrameRGB = NULL;
	int numBytes;
	uint8_t *buffer = NULL;

	// Allocate an AVFrame structure
	pFrameRGB = AV_ALLOCATE_FRAME();
	if (pFrameRGB == NULL)
		throw OutOfBoundsFrame("Convert Image Broke!", current_frame, video_length);

	// Determine if video needs to be scaled down (for performance reasons)
	// Timelines pass their size to the clips, which pass their size to the readers (as max size)
	// If a clip is being scaled larger, it will set max_width and max_height = 0 (which means don't down scale)
	int original_height = height;
	if (max_width != 0 && max_height != 0 && max_width < width && max_height < height)
	{
		// Override width and height (but maintain aspect ratio)
		float ratio = float(width) / float(height);
		int possible_width = round(max_height * ratio);
		int possible_height = round(max_width / ratio);

		if (possible_width <= max_width)
		{
			// use calculated width, and max_height
			width = possible_width;
			height = max_height;
		}
		else
		{
			// use max_width, and calculated height
			width = max_width;
			height = possible_height;
		}
	}

	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGBA, width, height);

	buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGBA, width, height);

	SwsContext *img_convert_ctx = sws_getContext(info.width, info.height, pCodecCtx->pix_fmt, width,
		height, PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);

	// Resize / Convert to RGB
	sws_scale(img_convert_ctx, my_frame->data, my_frame->linesize, 0,
		original_height, pFrameRGB->data, pFrameRGB->linesize);

	// Create or get the existing frame object
	QSharedPointer<Frame> f = CreateFrame(current_frame);

	// Add Image data to frame
	f->AddImage(width, height, 4, QImage::Format_RGBA8888, buffer);

	// Set the picture type for the frame. Eg: The frame type
	f->SetPictureType(pict_type);

	// Update working cache
	working_cache.Add(f);

	// Keep track of last last_video_frame
	last_video_frame = f;

	// Free the RGB image
	av_free(buffer);
	AV_FREE_FRAME(&pFrameRGB);

	// Remove frame and packet
	RemoveAVFrame(my_frame);
	sws_freeContext(img_convert_ctx);

	// Remove video frame from list of processing video frames
	{
		//std::lock_guard<std::mutex> lock(processing_mutex);

		processing_video_frames.erase(current_frame);
		processed_video_frames[current_frame] = current_frame;
	}
}

// Convert PTS into Frame Number
long int FFmpegReader::ConvertVideoPTStoFrame(long int pts)
{
	// Apply PTS offset
	pts = pts + video_pts_offset;
	long int previous_video_frame = current_video_frame;

	// Get the video packet start time (in seconds)
	double video_seconds = double(pts) * info.video_timebase.ToDouble();

	// Divide by the video timebase, to get the video frame number (frame # is decimal at this point)
	long int frame = round(video_seconds * info.fps.ToDouble()) + 1;

	// Keep track of the expected video frame #
	if (current_video_frame == 0)
	{
		current_video_frame = frame;
	}
	else
	{
		// Sometimes frames are duplicated due to identical (or similar) timestamps
		if (frame == previous_video_frame)
		{
			// return -1 frame number
			frame = -1;
		}
		else
		{
			// Increment expected frame
			current_video_frame++;
		}

		// Sometimes frames are missing due to varying timestamps, or they were dropped. 
		// Determine if we are missing a video frame.
		//std::lock_guard<std::mutex> lock(processing_mutex);
		while (current_video_frame < frame)
		{
			if (!missing_video_frames.count(current_video_frame))
			{
				missing_video_frames.insert(pair<long int, long int>(current_video_frame, previous_video_frame));
				missing_video_frames_source.insert(pair<long int, long int>(previous_video_frame, current_video_frame));
			}

			// Mark this reader as containing missing frames
			has_missing_frames = true;

			// Increment current frame
			current_video_frame++;
		}
	}

	// Return frame #
	return frame;
}

// Convert Frame Number into Video PTS
long int FFmpegReader::ConvertFrameToVideoPTS(long int frame_number)
{
	// Get timestamp of this frame (in seconds)
	double seconds = double(frame_number) / info.fps.ToDouble();

	// Calculate the # of video packets in this timestamp
	long int video_pts = round(seconds / info.video_timebase.ToDouble());

	// Apply PTS offset (opposite)
	return video_pts - video_pts_offset;
}

long int FFmpegReader::ConvertFrameToAudioPTS(long int frame_number)
{
	// Get timestamp of this frame (in seconds)
	double seconds = double(frame_number) / info.fps.ToDouble();

	// Calculate the # of audio packets in this timestamp
	long int audio_pts = round(seconds / info.audio_timebase.ToDouble());

	// Apply PTS offset (opposite)
	return audio_pts - audio_pts_offset;
}

// Create a new Frame (or return an existing one) and add it to the working queue.
QSharedPointer<Frame> FFmpegReader::CreateFrame(long int requested_frame)
{
	// Check working cache
	QSharedPointer<Frame> output = working_cache.GetFrame(requested_frame);
	if (!output)
	{
		int samples_per_frame = Frame::GetSamplesPerFrame(requested_frame, info.fps, info.sample_rate, info.channels);

		// Create a new frame on the working cache
		output = QSharedPointer<Frame>(new Frame(requested_frame, info.width, info.height, "#000000", samples_per_frame, info.channels));
		// update pixel ratio
		output->SetPixelRatio(info.pixel_ratio.num, info.pixel_ratio.den);
		// update audio channel layout from the parent reader
		output->ChannelsLayout(info.channel_layout);
		// update the frame's sample rate of the parent reader
		output->SampleRate(info.sample_rate);

		working_cache.Add(output);

		// Set the largest processed frame (if this is larger)
		if (requested_frame > largest_frame_processed)
		{
			largest_frame_processed = requested_frame;
		}
	}

	// Return new frame
	return output;
}
/*
=============================================================
Copyright Venatio Studios 2019
=============================================================
Revision History

0.0.1 : 2019-07-02 15:34
#vNext
=============================================================
*/
