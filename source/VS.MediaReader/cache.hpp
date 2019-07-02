#ifndef GUARD_cache_20190207153324_
#define GUARD_cache_20190207153324_
/*
@file		cache.hpp
@author		Webstar
@date		2019-07-02 15:33
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/


// STD
#include <map>
#include <vector>
#include <deque>
#include <mutex>

#include "frame.hpp"

using namespace vs;

namespace vs
{
	/// @brief This class is a memory-based cache manager for Frame objects.
	/// @remark It is used by readers to cache recently accessed frames. Due to the
	/// high cost of decoding streams, once a frame is decoded, converted to RGB, and a Frame object is created,
	/// it critical to keep these Frames cached for performance reasons.  However, the larger the cache, the more memory
	/// is required.  You can set the max number of bytes to cache.
	class FrameCache {
	private:
		std::mutex cache_mutex;

		long long int max_bytes;

		std::map<long int, QSharedPointer<Frame>> frames;	///< This map holds the frame number and Frame objects
		std::deque<long int> frame_numbers;						///< This queue holds a sequential list of cached Frame numbers

		bool needs_range_processing;							///< Something has changed, and the range data needs to be re-calculated
		std::vector<long int> ordered_frame_numbers;			///< Ordered list of frame numbers used by cache
		std::map<long int, long int> frame_ranges;				///< This map holds the ranges of frames, useful for quickly displaying the contents of the cache

		void CleanUp();

		void CalculateRanges();

	public:
		/// Default constructor, no max bytes
		FrameCache();

		/// @brief Constructor that sets the max bytes to cache
		/// @param max_bytes The maximum bytes to allow in the cache. Once exceeded, the cache will purge the oldest frames.
		FrameCache(long long int max_bytes);

		/// Default destructor
		~FrameCache();

		/// @brief Add a Frame to the cache
		/// @param frame The openshot::Frame object needing to be cached.
		void Add(QSharedPointer<Frame> frame);

		/// Clear the cache of all frames
		void Clear();

		/// Count the frames in the queue
		long int Count();

		/// @brief Get a frame from the cache
		/// @param frame_number The frame number of the cached frame
		QSharedPointer<Frame> GetFrame(long int frame_number);

		/// Gets the maximum bytes value
		long long int GetBytes();

		/// Get the smallest frame number
		QSharedPointer<Frame> GetSmallestFrame();

		/// @brief Move frame to front of queue (so it lasts longer)
		/// @param frame_number The frame number of the cached frame
		void MoveToFront(long int frame_number);

		/// @brief Remove a specific frame
		/// @param frame_number The frame number of the cached frame
		void Remove(long int frame_number);

		/// @brief Remove a range of frames
		/// @param start_frame_number The starting frame number of the cached frame
		/// @param end_frame_number The ending frame number of the cached frame
		void Remove(long int start_frame_number, long int end_frame_number);

		/// @brief Set maximum bytes to a different amount based on a ReaderInfo struct
		/// @param number_of_frames The maximum number of frames to hold in cache
		/// @param width The width of the frame's image
		/// @param height The height of the frame's image
		/// @param sample_rate The sample rate of the frame's audio data
		/// @param channels The number of audio channels in the frame
		void SetMaxBytesFromInfo(long int number_of_frames, int width, int height, int sample_rate, int channels);

		long long int GetMaxBytes() { return max_bytes; };
		void SetMaxBytes(long long int number_of_bytes) { max_bytes = number_of_bytes; };
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