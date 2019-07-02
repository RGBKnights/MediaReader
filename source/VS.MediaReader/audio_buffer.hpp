#ifndef GUARD_audio_buffer_20190207153333_
#define GUARD_audio_buffer_20190207153333_
/*
@file		audio_buffer.hpp
@author		Webstar
@date		2019-07-02 15:33
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

// QT
#include <QByteArray>

// densyfy
#include "heap_block.hpp"
#include "float_vector_operations.hpp"

using namespace vs;

namespace vs
{
	/// @brief A buffer with a specified number of channels and samples.
	/// @see AudioSampleBuffer
	template <typename Type>
	class AudioBuffer
	{
	public:
		AudioBuffer() 
			: numChannels(0), size(0), allocatedBytes(0),
			channels(static_cast<Type**> (preallocatedChannelSpace)),
			isClear(false)
		{
		}

		/// @brief Creates a buffer with a specified number of channels and samples.
		/// @remark The contents of the buffer will initially be undefined, so use clear() to
		/// set all the samples to zero.
		/// The buffer will allocate its memory internally, and this will be released
		/// when the buffer is deleted. If the memory can't be allocated, this will
		/// throw a std::bad_alloc exception.
		
		AudioBuffer(int numChannelsToAllocate,
			int numSamplesToAllocate)
			: numChannels(numChannelsToAllocate),
			size(numSamplesToAllocate)
		{
			Q_ASSERT(size >= 0 && numChannels >= 0);
			allocateData();
		}

		/// @brief Creates a buffer using a pre-allocated block of memory.
		/// @remark Note that if the buffer is resized or its number of channels is changed, it
		/// will re-allocate memory internally and copy the existing data to this new area,
		/// so it will then stop directly addressing this memory.
		AudioBuffer(Type* const* dataToReferTo,
			int numChannelsToUse,
			int numSamples)
			: numChannels(numChannelsToUse),
			size(numSamples),
			allocatedBytes(0)
		{
			Q_ASSERT(dataToReferTo != nullptr);
			Q_ASSERT(numChannelsToUse >= 0 && numSamples >= 0);
			allocateChannels(dataToReferTo, 0);
		}

		/// @brief Creates a buffer using a pre-allocated block of memory.
		/// @remark Note that if the buffer is resized or its number of channels is changed, it
		/// will re-allocate memory internally and copy the existing data to this new area,
		/// so it will then stop directly addressing this memory.
		AudioBuffer(Type* const* dataToReferTo,
			int numChannelsToUse,
			int startSample,
			int numSamples)
			: numChannels(numChannelsToUse),
			size(numSamples),
			allocatedBytes(0),
			isClear(false)
		{
			Q_ASSERT(dataToReferTo != nullptr);
			Q_ASSERT(numChannelsToUse >= 0 && startSample >= 0 && numSamples >= 0);
			allocateChannels(dataToReferTo, startSample);
		}

		/// @brief Copies another buffer.
		/// @remark This buffer will make its own copy of the other's data, unless the buffer was created
		/// using an external data buffer, in which case boths buffers will just point to the same
		/// shared block of data.
		AudioBuffer(const AudioBuffer& other)
			: numChannels(other.numChannels),
			size(other.size),
			allocatedBytes(other.allocatedBytes)
		{
			if (allocatedBytes == 0)
			{
				allocateChannels(other.channels, 0);
			}
			else
			{
				allocateData();

				if (other.isClear)
				{
					clear();
				}
				else
				{
					for (int i = 0; i < numChannels; ++i)
						FloatVectorOperations::copy(channels[i], other.channels[i], size);
				}
			}
		}

		/// @brief Copies another buffer onto this one.
		/// @remark This buffer's size will be changed to that of the other buffer.
		AudioBuffer& operator= (const AudioBuffer& other)
		{
			if (this != &other)
			{
				setSize(other.getNumChannels(), other.getNumSamples(), false, false, false);

				if (other.isClear)
				{
					clear();
				}
				else
				{
					isClear = false;

					for (int i = 0; i < numChannels; ++i)
						FloatVectorOperations::copy(channels[i], other.channels[i], size);
				}
			}

			return *this;
		}

		/// Defualt Destructor
		~AudioBuffer()  {}

		/// Copy Constructor
		AudioBuffer(AudioBuffer&& other) 
			: numChannels(other.numChannels),
			size(other.size),
			allocatedBytes(other.allocatedBytes),
			allocatedData(static_cast<HeapBlock<char, true>&&> (other.allocatedData)),
			isClear(other.isClear)
		{
			if (numChannels < (int)numElementsInArray(preallocatedChannelSpace))
			{
				channels = preallocatedChannelSpace;
				memcpy(preallocatedChannelSpace, other.channels, sizeof(preallocatedChannelSpace));
			}
			else
			{
				channels = other.channels;
			}

			other.numChannels = 0;
			other.size = 0;
			other.allocatedBytes = 0;
		}

		/// Move assignment
		AudioBuffer& operator= (AudioBuffer&& other) 
		{
			numChannels = other.numChannels;
			size = other.size;
			allocatedBytes = other.allocatedBytes;
			allocatedData = static_cast<HeapBlock<char, true>&&> (other.allocatedData);
			isClear = other.isClear;

			if (numChannels < (int)numElementsInArray(preallocatedChannelSpace))
			{
				channels = preallocatedChannelSpace;
				memcpy(preallocatedChannelSpace, other.channels, sizeof(preallocatedChannelSpace));
			}
			else
			{
				channels = other.channels;
			}

			other.numChannels = 0;
			other.size = 0;
			other.allocatedBytes = 0;
			return *this;
		}

		/// @brief Returns the number of channels of audio data that this buffer contains.
		/// @see getNumSamples, getReadPointer, getWritePointer
		int getNumChannels()  { return numChannels; }

		/// @brief Returns the number of samples allocated in each of the buffer's channels.
		/// @see getNumChannels, getReadPointer, getWritePointer
		int getNumSamples()  { return size; }

		/// @brief Returns a pointer to an array of read-only samples in one of the buffer's channels.
		/// @remark For speed, this doesn't check whether the channel number is out of range,
		/// so be careful when using it!
		/// If you need to write to the data, do NOT call this method and const_cast the
		/// result! Instead, you must call getWritePointer so that the buffer knows you're
		/// planning on modifying the data.
		const Type* getReadPointer(int channelNumber) 
		{
			return channels[channelNumber];
		}

		/// @brief Returns a pointer to an array of read-only samples in one of the buffer's channels.
		/// @remark For speed, this doesn't check whether the channel number or index are out of range,
		/// so be careful when using it!
		/// If you need to write to the data, do NOT call this method and const_cast the
		/// result! Instead, you must call getWritePointer so that the buffer knows you're
		/// planning on modifying the data.
		const Type* getReadPointer(int channelNumber, int sampleIndex) 
		{
			return channels[channelNumber] + sampleIndex;
		}

		/// @brief Returns a writeable pointer to one of the buffer's channels.
		/// @remark For speed, this doesn't check whether the channel number is out of range,
		/// so be careful when using it!
		/// Note that if you're not planning on writing to the data, you should always
		/// use getReadPointer instead.
		Type* getWritePointer(int channelNumber) 
		{
			isClear = false;
			return channels[channelNumber];
		}

		/// @brief Returns a writeable pointer to one of the buffer's channels.
		/// @remark For speed, this doesn't check whether the channel number or index are out of range,
		/// so be careful when using it!
		/// Note that if you're not planning on writing to the data, you should
		/// use getReadPointer instead.
		Type* getWritePointer(int channelNumber, int sampleIndex) 
		{
			isClear = false;
			return channels[channelNumber] + sampleIndex;
		}

		/// @briefReturns an array of pointers to the channels in the buffer.
		/// @remark Don't modify any of the pointers that are returned, and bear in mind that
		/// these will become invalid if the buffer is resized.
		const Type** getArrayOfReadPointers()  { return const_cast<const Type**> (channels); }

		/// @brief Returns an array of pointers to the channels in the buffer.
		/// @remark Don't modify any of the pointers that are returned, and bear in mind that
		/// these will become invalid if the buffer is resized.
		Type** getArrayOfWritePointers()  { isClear = false; return channels; }

		/// @brief Returns ByteArray in 16 bit LittleEndian interlaced format for use with QAudioOutput
		/// @remark For speed use the getReadPointer or getArrayOfWritePointers. 
		/// The QAudioFormat options that use be used with this ByteArray:
		/// @li SampleSize = 16
		/// @li SampleType = QAudioFormat::SignedInt
		/// @li ByteOrder = QAudioFormat::LittleEndian
		/// @li Sample and Chancels should be supplied by the reader
		QByteArray getByteArray() 
		{
			QByteArray data;

			for (int i = 0; i < getNumSamples(); i++)
			{
				for (int channel = 0; channel < getNumChannels(); channel++)
				{
					float sample = getSample(channel, i);
					int16_t sampleAsInt = (int16_t)(sample * (float)32768.);

					uint8_t bytes[2];

					bytes[1] = (sampleAsInt >> 8) & 0xFF;
					bytes[0] = sampleAsInt & 0xFF;

					data.append((const char *)&bytes[0], sizeof(uint8_t));
					data.append((const char *)&bytes[1], sizeof(uint8_t));
				}
			}

			return data;
		}


		/// @brief Changes the buffer's size or number of channels.
		/// @remark This can expand or contract the buffer's length, and add or remove channels.
		/// If keepExistingContent is true, it will try to preserve as much of the
		/// old data as it can in the new buffer.
		/// If clearExtraSpace is true, then any extra channels or space that is
		/// allocated will be also be cleared. If false, then this space is left
		/// uninitialised.
		/// If avoidReallocating is true, then changing the buffer's size won't reduce the
		/// amount of memory that is currently allocated (but it will still increase it if
		/// the new size is bigger than the amount it currently has). If this is false, then
		/// a new allocation will be done so that the buffer uses takes up the minimum amount
		/// of memory that it needs.
		/// If the required memory can't be allocated, this will throw a std::bad_alloc exception.
		void setSize(int newNumChannels,
			int newNumSamples,
			bool keepExistingContent = false,
			bool clearExtraSpace = false,
			bool avoidReallocating = false)
		{
			Q_ASSERT(newNumChannels >= 0);
			Q_ASSERT(newNumSamples >= 0);

			if (newNumSamples != size || newNumChannels != numChannels)
			{
				const auto allocatedSamplesPerChannel = ((size_t)newNumSamples + 3) & ~3u;
				const auto channelListSize = ((sizeof(Type*) * (size_t)(newNumChannels + 1)) + 15) & ~15u;
				const auto newTotalBytes = ((size_t)newNumChannels * (size_t)allocatedSamplesPerChannel * sizeof(Type))
					+ channelListSize + 32;

				if (keepExistingContent)
				{
					HeapBlock<char, true> newData;
					newData.allocate(newTotalBytes, clearExtraSpace || isClear);

					auto numSamplesToCopy = (size_t)typed_min(newNumSamples, size);

					auto newChannels = reinterpret_cast<Type**> (newData.get());
					auto newChan = reinterpret_cast<Type*> (newData + channelListSize);

					for (int j = 0; j < newNumChannels; ++j)
					{
						newChannels[j] = newChan;
						newChan += allocatedSamplesPerChannel;
					}

					if (!isClear)
					{
						auto numChansToCopy = typed_min(numChannels, newNumChannels);

						for (int i = 0; i < numChansToCopy; ++i)
							FloatVectorOperations::copy(newChannels[i], channels[i], (int)numSamplesToCopy);
					}

					allocatedData.swapWith(newData);
					allocatedBytes = newTotalBytes;
					channels = newChannels;
				}
				else
				{
					if (avoidReallocating && allocatedBytes >= newTotalBytes)
					{
						if (clearExtraSpace || isClear)
							allocatedData.clear(newTotalBytes);
					}
					else
					{
						allocatedBytes = newTotalBytes;
						allocatedData.allocate(newTotalBytes, clearExtraSpace || isClear);
						channels = reinterpret_cast<Type**> (allocatedData.get());
					}

					auto* chan = reinterpret_cast<Type*> (allocatedData + channelListSize);

					for (int i = 0; i < newNumChannels; ++i)
					{
						channels[i] = chan;
						chan += allocatedSamplesPerChannel;
					}
				}

				channels[newNumChannels] = 0;
				size = newNumSamples;
				numChannels = newNumChannels;
			}
		}

		/// @brief Makes this buffer point to a pre-allocated set of channel data arrays.
		/// @remark There's also a constructor that lets you specify arrays like this, but this
		/// lets you change the channels dynamically.
		/// Note that if the buffer is resized or its number of channels is changed, it
		/// will re-allocate memory internally and copy the existing data to this new area,
		/// so it will then stop directly addressing this memory.
		void setDataToReferTo(Type** dataToReferTo,
			int newNumChannels,
			int newStartSample,
			int newNumSamples)
		{
			Q_ASSERT(dataToReferTo != nullptr);
			Q_ASSERT(newNumChannels >= 0 && newNumSamples >= 0);

			if (allocatedBytes != 0)
			{
				allocatedBytes = 0;
				allocatedData.free();
			}

			numChannels = newNumChannels;
			size = newNumSamples;

			allocateChannels(dataToReferTo, newStartSample);
			Q_ASSERT(!isClear);
		}

		/// @brief Makes this buffer point to a pre-allocated set of channel data arrays.
		/// @remark There's also a constructor that lets you specify arrays like this, but this
		/// lets you change the channels dynamically.
		/// Note that if the buffer is resized or its number of channels is changed, it
		/// will re-allocate memory internally and copy the existing data to this new area,
		/// so it will then stop directly addressing this memory.
		void setDataToReferTo(Type** dataToReferTo,
			int newNumChannels,
			int newNumSamples)
		{
			setDataToReferTo(dataToReferTo, newNumChannels, 0, newNumSamples);
		}

		/// @brief Resizes this buffer to match the given one, and copies all of its content across.
		/// @remark The source buffer can contain a different floating point type, so this can be used to
		/// convert between 32 and 64 bit float buffer types.
		template <typename OtherType>
		void makeCopyOf(const AudioBuffer<OtherType>& other, bool avoidReallocating = false)
		{
			setSize(other.getNumChannels(), other.getNumSamples(), false, false, avoidReallocating);

			if (other.hasBeenCleared())
			{
				clear();
			}
			else
			{
				isClear = false;

				for (int chan = 0; chan < numChannels; ++chan)
				{
					auto* dest = channels[chan];
					auto* src = other.getReadPointer(chan);

					for (int i = 0; i < size; ++i)
						dest[i] = static_cast<Type> (src[i]);
				}
			}
		}

		/// Clears all the samples in all channels
		void clear() 
		{
			if (!isClear)
			{
				for (int i = 0; i < numChannels; ++i)
				{
					FloatVectorOperations::clear(channels[i], size);
				}

				isClear = true;
			}
		}

		/// @brief Clears a specified region of all the channels.
		/// @remark For speed, this doesn't check whether the channel and sample number
		/// are in-range, so be careful!
		void clear(int startSample, int numSamples) 
		{
			if (!isClear)
			{
				if (startSample == 0 && numSamples == size)
					isClear = true;

				for (int i = 0; i < numChannels; ++i)
					FloatVectorOperations::clear(channels[i] + startSample, numSamples);
			}
		}

		/// @brief Clears a specified region of just one channel.
		/// @remark For speed, this doesn't check whether the channel and sample number
		/// are in-range, so be careful!
		void clear(int channel, int startSample, int numSamples) 
		{
			if (!isClear)
				FloatVectorOperations::clear(channels[channel] + startSample, numSamples);
		}

		/// @brief Returns true if the buffer has been entirely cleared.
		/// @remark Note that this does not actually measure the contents of the buffer - it simply
		/// returns a flag that is set when the buffer is cleared, and which is reset whenever
		/// functions like getWritePointer() are invoked. That means the method does not take
		/// any time, but it may return false negatives when in fact the buffer is still empty.
		bool hasBeenCleared()  { return isClear; }

		/// @brief Returns a sample from the buffer.
		/// @remark The channel and index are not checked - they are expected to be in-range. If not,
		/// an assertion will be thrown, but in a release build, you're into 'undefined behaviour'
		/// territory.
		Type getSample(int channel, int sampleIndex) 
		{
			return *(channels[channel] + sampleIndex);
		}

		/// @brief Sets a sample in the buffer.
		/// @remark The channel and index are not checked - they are expected to be in-range. If not,
		/// an assertion will be thrown, but in a release build, you're into 'undefined behaviour'
		/// territory.
		void setSample(int destChannel, int destSample, Type newValue) 
		{
			*(channels[destChannel] + destSample) = newValue;
			isClear = false;
		}

		/// @brief Adds a value to a sample in the buffer.
		/// @remark The channel and index are not checked - they are expected to be in-range. If not,
		/// an assertion will be thrown, but in a release build, you're into 'undefined behaviour'
		/// territory.
		void addSample(int destChannel, int destSample, Type valueToAdd) 
		{
			*(channels[destChannel] + destSample) += valueToAdd;
			isClear = false;
		}

		/// @brief Applies a gain multiple to a region of one channel.
		/// @remark For speed, this doesn't check whether the channel and sample number
		/// are in-range, so be careful!
		void applyGain(int channel, int startSample, int numSamples, Type gain) 
		{
			if (gain != (Type)1 && !isClear)
			{
				auto* d = channels[channel] + startSample;

				if (gain == 0)
					FloatVectorOperations::clear(d, numSamples);
				else
					FloatVectorOperations::multiply(d, gain, numSamples);
			}
		}

		/// @brief Applies a gain multiple to a region of all the channels.
		/// @remark For speed, this doesn't check whether the sample numbers
		/// are in-range, so be careful!
		void applyGain(int startSample, int numSamples, Type gain) 
		{
			for (int i = 0; i < numChannels; ++i)
				applyGain(i, startSample, numSamples, gain);
		}

		/// Applies a gain multiple to all the audio data.
		void applyGain(Type gain) 
		{
			applyGain(0, size, gain);
		}

		/// @brief Applies a range of gains to a region of a channel.
		/// @remark The gain that is applied to each sample will vary from
		/// startGain on the first sample to endGain on the last Sample,
		/// so it can be used to do basic fades.
		/// For speed, this doesn't check whether the sample numbers
		/// are in-range, so be careful!
		void applyGainRamp(int channel, int startSample, int numSamples,
			Type startGain, Type endGain) 
		{
			if (!isClear)
			{
				if (startGain == endGain)
				{
					applyGain(channel, startSample, numSamples, startGain);
				}
				else
				{
					//Q_ASSERT(isPositiveAndBelow(channel, numChannels));
					Q_ASSERT(startSample >= 0 && numSamples >= 0 && startSample + numSamples <= size);

					const auto increment = (endGain - startGain) / (float)numSamples;
					auto* d = channels[channel] + startSample;

					while (--numSamples >= 0)
					{
						*d++ *= startGain;
						startGain += increment;
					}
				}
			}
		}

		/// @brief  Applies a range of gains to a region of all channels.
		/// @remark The gain that is applied to each sample will vary from
		/// startGain on the first sample to endGain on the last Sample,
		/// so it can be used to do basic fades.
		/// For speed, this doesn't check whether the sample numbers
		/// are in-range, so be careful!
		void applyGainRamp(int startSample, int numSamples,
			Type startGain, Type endGain) 
		{
			for (int i = 0; i < numChannels; ++i)
				applyGainRamp(i, startSample, numSamples, startGain, endGain);
		}

		/** Adds samples from another buffer to this one.
		*/
		void addFrom(int destChannel,
			int destStartSample,
			const AudioBuffer& source,
			int sourceChannel,
			int sourceStartSample,
			int numSamples,
			Type gainToApplyToSource = (Type)1) 
		{
			if (gainToApplyToSource != 0 && numSamples > 0 && !source.isClear)
			{
				auto* d = channels[destChannel] + destStartSample;
				auto* s = source.channels[sourceChannel] + sourceStartSample;

				if (isClear)
				{
					isClear = false;

					if (gainToApplyToSource != (Type)1)
						FloatVectorOperations::copyWithMultiply(d, s, gainToApplyToSource, numSamples);
					else
						FloatVectorOperations::copy(d, s, numSamples);
				}
				else
				{
					if (gainToApplyToSource != (Type)1)
						FloatVectorOperations::addWithMultiply(d, s, gainToApplyToSource, numSamples);
					else
						FloatVectorOperations::add(d, s, numSamples);
				}
			}
		}


		/// Adds samples from an array of floats to one of the channels.
		void addFrom(int destChannel,
			int destStartSample,
			const Type* source,
			int numSamples,
			Type gainToApplyToSource = (Type)1) 
		{
			if (gainToApplyToSource != 0 && numSamples > 0)
			{
				auto* d = channels[destChannel] + destStartSample;

				if (isClear)
				{
					isClear = false;

					if (gainToApplyToSource != (Type)1)
						FloatVectorOperations::copyWithMultiply(d, source, gainToApplyToSource, numSamples);
					else
						FloatVectorOperations::copy(d, source, numSamples);
				}
				else
				{
					if (gainToApplyToSource != (Type)1)
						FloatVectorOperations::addWithMultiply(d, source, gainToApplyToSource, numSamples);
					else
						FloatVectorOperations::add(d, source, numSamples);
				}
			}
		}


		/// Adds samples from an array of floats, applying a gain ramp to them.
		void addFromWithRamp(int destChannel,
			int destStartSample,
			const Type* source,
			int numSamples,
			Type startGain,
			Type endGain) 
		{
			if (startGain == endGain)
			{
				addFrom(destChannel, destStartSample, source, numSamples, startGain);
			}
			else
			{
				if (numSamples > 0)
				{
					isClear = false;
					const auto increment = (endGain - startGain) / numSamples;
					auto* d = channels[destChannel] + destStartSample;

					while (--numSamples >= 0)
					{
						*d++ += startGain * *source++;
						startGain += increment;
					}
				}
			}
		}

		/// Copies samples from another buffer to this one
		void copyFrom(int destChannel,
			int destStartSample,
			const AudioBuffer& source,
			int sourceChannel,
			int sourceStartSample,
			int numSamples) 
		{
			if (numSamples > 0)
			{
				if (source.isClear)
				{
					if (!isClear)
						FloatVectorOperations::clear(channels[destChannel] + destStartSample, numSamples);
				}
				else
				{
					isClear = false;
					FloatVectorOperations::copy(channels[destChannel] + destStartSample,
						source.channels[sourceChannel] + sourceStartSample,
						numSamples);
				}
			}
		}

		/// Copies samples from an array of floats into one of the channels.
		void copyFrom(int destChannel,
			int destStartSample,
			const Type* source,
			int numSamples) 
		{
			if (numSamples > 0)
			{
				isClear = false;
				FloatVectorOperations::copy(channels[destChannel] + destStartSample, source, numSamples);
			}
		}

		/// Copies samples from an array of floats into one of the channels, applying a gain to it.
		void copyFrom(int destChannel,
			int destStartSample,
			const Type* source,
			int numSamples,
			Type gain) 
		{
			if (numSamples > 0)
			{
				auto* d = channels[destChannel] + destStartSample;

				if (gain != (Type)1)
				{
					if (gain == 0)
					{
						if (!isClear)
							FloatVectorOperations::clear(d, numSamples);
					}
					else
					{
						isClear = false;
						FloatVectorOperations::copyWithMultiply(d, source, gain, numSamples);
					}
				}
				else
				{
					isClear = false;
					FloatVectorOperations::copy(d, source, numSamples);
				}
			}
		}

		/// Copies samples from an array of floats into one of the channels, applying a gain ramp.
		void copyFromWithRamp(int destChannel,
			int destStartSample,
			const Type* source,
			int numSamples,
			Type startGain,
			Type endGain) 
		{
			if (startGain == endGain)
			{
				copyFrom(destChannel, destStartSample, source, numSamples, startGain);
			}
			else
			{
				if (numSamples > 0)
				{
					isClear = false;
					const auto increment = (endGain - startGain) / numSamples;
					auto* d = channels[destChannel] + destStartSample;

					while (--numSamples >= 0)
					{
						*d++ = startGain * *source++;
						startGain += increment;
					}
				}
			}
		}

		/// Finds the highest absolute sample value within a region of a channel.
		Type getMagnitude(int channel, int startSample, int numSamples) 
		{
			if (isClear)
				return {};

			auto start = FloatVectorOperations::findMinimum(channels[channel] + startSample, numSamples);
			auto end = FloatVectorOperations::findMaximum(channels[channel] + startSample, numSamples);

			return typed_max(start, -start, end, -end);
		}

		/// Finds the highest absolute sample value within a region on all channels.
		Type getMagnitude(int startSample, int numSamples) 
		{
			Type mag = 0;

			if (!isClear)
				for (int i = 0; i < numChannels; ++i)
					mag = max(mag, getMagnitude(i, startSample, numSamples));

			return mag;
		}

		/// Returns the root mean squared level for a region of a channel.
		Type getRMSLevel(int channel, int startSample, int numSamples) 
		{
			if (numSamples <= 0 || channel < 0 || channel >= numChannels || isClear)
				return {};

			auto* data = channels[channel] + startSample;
			double sum = 0.0;

			for (int i = 0; i < numSamples; ++i)
			{
				const Type sample = data[i];
				sum += sample * sample;
			}

			return (Type)std::sqrt(sum / numSamples);
		}

		/// Reverses a part of a channel.
		void reverse(int channel, int startSample, int numSamples) 
		{
			if (!isClear)
				std::reverse(channels[channel] + startSample,
					channels[channel] + startSample + numSamples);
		}

		/// Reverses a part of the buffer.
		void reverse(int startSample, int numSamples) 
		{
			for (int i = 0; i < numChannels; ++i)
				reverse(i, startSample, numSamples);
		}


	private:
		int numChannels, size;
		size_t allocatedBytes;
		Type** channels;
		HeapBlock<char, true> allocatedData;
		Type* preallocatedChannelSpace[32];
		bool isClear;

		void allocateData()
		{
			auto channelListSize = sizeof(Type*) * (size_t)(numChannels + 1);
			allocatedBytes = (size_t)numChannels * (size_t)size * sizeof(Type) + channelListSize + 32;
			allocatedData.malloc(allocatedBytes);
			channels = reinterpret_cast<Type**> (allocatedData.get());
			auto* chan = (Type*)(allocatedData + channelListSize);

			for (int i = 0; i < numChannels; ++i)
			{
				channels[i] = chan;
				chan += size;
			}

			channels[numChannels] = nullptr;
			isClear = false;
		}

		void allocateChannels(Type* const* dataToReferTo, int offset)
		{
			// (try to avoid doing a malloc here, as that'll blow up things like Pro-Tools)
			if (numChannels < (int)numElementsInArray(preallocatedChannelSpace))
			{
				channels = static_cast<Type**> (preallocatedChannelSpace);
			}
			else
			{
				allocatedData.malloc((size_t)numChannels + 1, sizeof(Type*));
				channels = reinterpret_cast<Type**> (allocatedData.get());
			}

			for (int i = 0; i < numChannels; ++i)
			{
				// you have to pass in the same number of valid pointers as numChannels
				channels[i] = dataToReferTo[i] + offset;
			}

			channels[numChannels] = nullptr;
			isClear = false;
		}
	};

	/// @brief A multi-channel buffer of 32-bit floating point audio samples.
	/// @remark This typedef is here for backwards compatibility with the older AudioSampleBuffer
	/// class, which was fixed for 32-bit data, but is otherwise the same as the new
	/// templated AudioBuffer class.
	/// @see AudioBuffer
	typedef AudioBuffer<float> AudioSampleBuffer;

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