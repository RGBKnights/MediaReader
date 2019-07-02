/*
@file		cache.cpp
@author		Webstar
@date		2019-07-02 15:33
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

#include "cache.hpp"

using namespace std;
using namespace vs;

// Default constructor, no max frames
FrameCache::FrameCache()
	: max_bytes(0), needs_range_processing(false)
{
};

// Constructor that sets the max frames to cache
FrameCache::FrameCache(long long int max_bytes)
	: max_bytes(max_bytes), needs_range_processing(false)
{
};

// Default destructor
FrameCache::~FrameCache()
{
	// Clear
	frames.clear();
	frame_numbers.clear();
	ordered_frame_numbers.clear();
}

// Set maximum bytes to a different amount based on a ReaderInfo struct
void FrameCache::SetMaxBytesFromInfo(long int number_of_frames, int width, int height, int sample_rate, int channels)
{
	// n frames X height X width X 4 colors of chars X audio channels X 4 byte floats
	long long int bytes = number_of_frames * (height * width * 4 + (sample_rate * channels * 4));
	SetMaxBytes(bytes);
}


// Calculate ranges of frames
void FrameCache::CalculateRanges()
{
	// Only calculate when something has changed
	if (needs_range_processing)
	{
		//std::lock_guard<std::mutex> lock(cache_mutex);

		// Build up screen ranges as a data structure

		// Reset needs_range_processing
		needs_range_processing = false;
	}
}


// Add a Frame to the cache
void FrameCache::Add(QSharedPointer<Frame> frame)
{
	//std::lock_guard<std::mutex> lock(cache_mutex);

	long int frame_number = frame->number;

	// Freshen frame if it already exists
	if (frames.count(frame_number))
	{
		MoveToFront(frame_number);
	}
	else
	{
		// Add frame
		frames[frame_number] = frame;
		frame_numbers.push_front(frame_number);
		ordered_frame_numbers.push_back(frame_number);
		needs_range_processing = true;

		CleanUp();
	}
}


// Get a frame from the cache (or NULL shared_ptr if no frame is found)
QSharedPointer<Frame> FrameCache::GetFrame(long int frame_number)
{
	//std::lock_guard<std::mutex> lock(cache_mutex);

	if (frames.count(frame_number))
	{
		return frames[frame_number];
	}
	else
	{
		return QSharedPointer<Frame>();
	}
}


// Get the smallest frame number (or NULL shared_ptr if no frame is found)
QSharedPointer<Frame> FrameCache::GetSmallestFrame()
{
	QSharedPointer<Frame> f;
	long int smallest_frame = -1;

	{
		//std::lock_guard<std::mutex> lock(cache_mutex);

		deque<long int>::iterator itr;	
		for (itr = frame_numbers.begin(); itr != frame_numbers.end(); ++itr)
		{
			if (*itr < smallest_frame || smallest_frame == -1)
				smallest_frame = *itr;
		}
	}
	
	// Return frame
	f = GetFrame(smallest_frame);

	return f;
}


// Gets the maximum bytes value
long long int FrameCache::GetBytes()
{
	//std::lock_guard<std::mutex> lock(cache_mutex);

	long long int total_bytes = 0;

	deque<long int>::reverse_iterator itr;
	for (itr = frame_numbers.rbegin(); itr != frame_numbers.rend(); ++itr)
	{
		total_bytes += frames[*itr]->GetBytes();
	}

	return total_bytes;
}

// Remove a specific frame
void FrameCache::Remove(long int frame_number)
{
	Remove(frame_number, frame_number);
}

// Remove range of frames
void FrameCache::Remove(long int start_frame_number, long int end_frame_number)
{
	//std::lock_guard<std::mutex> lock(cache_mutex);

	// Loop through frame numbers
	deque<long int>::iterator itr;
	for (itr = frame_numbers.begin(); itr != frame_numbers.end();)
	{
		if (*itr >= start_frame_number && *itr <= end_frame_number)
		{
			// erase frame number
			itr = frame_numbers.erase(itr);
		}
		else
			itr++;
	}

	// Loop through ordered frame numbers
	vector<long int>::iterator itr_ordered;
	for (itr_ordered = ordered_frame_numbers.begin(); itr_ordered != ordered_frame_numbers.end();)
	{
		if (*itr_ordered >= start_frame_number && *itr_ordered <= end_frame_number)
		{
			// erase frame number
			frames.erase(*itr_ordered);
			itr_ordered = ordered_frame_numbers.erase(itr_ordered);
		}
		else
			itr_ordered++;
	}

	// Needs range processing (since cache has changed)
	needs_range_processing = true;
}


// Move frame to front of queue (so it lasts longer)
void FrameCache::MoveToFront(long int frame_number)
{
	//std::lock_guard<std::mutex> lock(cache_mutex);

	// Does frame exists in cache?
	if (frames.count(frame_number))
	{
		deque<long int>::iterator itr;
		for (itr = frame_numbers.begin(); itr != frame_numbers.end(); ++itr)
		{
			if (*itr == frame_number)
			{
				frame_numbers.erase(itr);

				frame_numbers.push_front(frame_number);
				break;
			}
		}
	}
}

// Clear the cache of all frames
void FrameCache::Clear()
{
	//std::lock_guard<std::mutex> lock(cache_mutex);

	frames.clear();
	frame_numbers.clear();
	ordered_frame_numbers.clear();
	needs_range_processing = true;
}

// Count the frames in the queue
long int FrameCache::Count()
{
	//std::lock_guard<std::mutex> lock(cache_mutex);

	return frames.size();
}

// Clean up cached frames that exceed the number in our max_bytes variable
void FrameCache::CleanUp()
{
	// Do we auto clean up?
	if (max_bytes > 0)
	{
		//std::lock_guard<std::mutex> lock(cache_mutex);

		while (GetBytes() > max_bytes && frame_numbers.size() > 20)
		{
			long int frame_to_remove = frame_numbers.back();
			Remove(frame_to_remove);
		}
	}
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
