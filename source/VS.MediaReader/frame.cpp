/*
@file		frame.cpp
@author		Webstar
@date		2019-07-02 15:34
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

#include "frame.hpp"

#include <QFile>

using namespace std;
using namespace vs;

// Constructor - blank frame (300x200 blank image, 48kHz audio silence)
Frame::Frame() : number(1),  pixel_ratio(1, 1), channels(2), width(1), height(1),
channel_layout(LAYOUT_STEREO), sample_rate(44100), qbuffer(NULL), has_audio_data(false), has_image_data(false)
{
	// Init the image magic and audio buffer
	audio = QSharedPointer<AudioSampleBuffer>(new AudioSampleBuffer(channels, 0));
	audio->clear();
};

// Constructor - image only (48kHz audio silence)
Frame::Frame(long int number, int width, int height, string color)
	: number(number), pixel_ratio(1, 1), channels(2), width(width), height(height),
	channel_layout(LAYOUT_STEREO), sample_rate(44100), qbuffer(NULL), has_audio_data(false), has_image_data(false)
{
	// Init the image magic and audio buffer
	audio = QSharedPointer<AudioSampleBuffer>(new AudioSampleBuffer(channels, 0));
	audio->clear();
};

// Constructor - audio only (300x200 blank image)
Frame::Frame(long int number, int samples, int channels) :
	number(number),  pixel_ratio(1, 1), channels(channels), width(1), height(1),
	channel_layout(LAYOUT_STEREO), sample_rate(44100), qbuffer(NULL), has_audio_data(false), has_image_data(false)
{
	// Init the image magic and audio buffer
	audio = QSharedPointer<AudioSampleBuffer>(new AudioSampleBuffer(channels, 0));
	audio->clear();
};

// Constructor - image & audio
Frame::Frame(long int number, int width, int height, string color, int samples, int channels)
	: number(number),  pixel_ratio(1, 1), channels(channels), width(width), height(height),
	channel_layout(LAYOUT_STEREO), sample_rate(44100), qbuffer(NULL), has_audio_data(false), has_image_data(false)
{
	// Init the image magic and audio buffer
	audio = QSharedPointer<AudioSampleBuffer>(new AudioSampleBuffer(channels, 0));
	audio->clear();
};

// Copy constructor
Frame::Frame(const Frame &other)
{
	// copy pointers and data
	DeepCopy(other);
}

// Constrain a color value from 0 to 255
int Frame::constrain(int color_value)
{
	// Constrain new color from 0 to 255
	if (color_value < 0)
		color_value = 0;
	else if (color_value > 255)
		color_value = 255;

	return color_value;
}

// Copy data and pointers from another Frame instance
void Frame::DeepCopy(const Frame& other)
{
	number = other.number;
	image = QSharedPointer<QImage>(new QImage(*(other.image)));
	audio = QSharedPointer<AudioSampleBuffer>(new AudioSampleBuffer(*(other.audio)));
	pixel_ratio = Fraction(other.pixel_ratio.num, other.pixel_ratio.den);
	channels = other.channels;
	channel_layout = other.channel_layout;
	has_audio_data = other.has_image_data;
	has_image_data = other.has_image_data;
	sample_rate = other.sample_rate;

	if (other.wave_image)
		wave_image = QSharedPointer<QImage>(new QImage(*(other.wave_image)));
}

// Descructor
Frame::~Frame() {
	// Clear all pointers
	image.reset();
}

// Get an audio waveform image
QSharedPointer<QImage> Frame::GetWaveform(int width, int height, int Red, int Green, int Blue, int Alpha)
{
	// Clear any existing waveform image
	ClearWaveform();

	// Init a list of lines
	QVector<QPointF> lines;
	QVector<QPointF> labels;

	// Calculate width of an image based on the # of samples
	int total_samples = audio->getNumSamples();
	if (total_samples > 0)
	{
		// If samples are present...
		int new_height = 200 * audio->getNumChannels();
		int height_padding = 20 * (audio->getNumChannels() - 1);
		int total_height = new_height + height_padding;
		int total_width = 0;

		// Loop through each audio channel
		int Y = 100;
		for (int channel = 0; channel < audio->getNumChannels(); channel++)
		{
			int X = 0;

			// Get audio for this channel
			const float *samples = audio->getReadPointer(channel);

			for (int sample = 0; sample < audio->getNumSamples(); sample++, X++)
			{
				// Sample value (scaled to -100 to 100)
				float value = samples[sample] * 100;

				// Append a line segment for each sample
				if (value != 0.0) {
					// LINE
					lines.push_back(QPointF(X, Y));
					lines.push_back(QPointF(X, Y - value));
				}
				else {
					// DOT
					lines.push_back(QPointF(X, Y));
					lines.push_back(QPointF(X, Y));
				}
			}

			// Add Channel Label Coordinate
			labels.push_back(QPointF(5, Y - 5));

			// Increment Y
			Y += (200 + height_padding);
			total_width = X;
		}

		// Create blank image
		wave_image = QSharedPointer<QImage>(new QImage(total_width, total_height, QImage::Format_RGBA8888));
		wave_image->fill(QColor(0, 0, 0, 0));

		// Load QPainter with wave_image device
		QPainter painter(wave_image.get());

		// Set pen color
		painter.setPen(QColor(Red, Green, Blue, Alpha));

		// Draw the waveform
		painter.drawLines(lines);
		painter.end();

		// Resize Image (if requested)
		if (width != total_width || height != total_height) 
		{
			QImage scaled_wave_image = wave_image->scaled(width, height, Qt::IgnoreAspectRatio, Qt::FastTransformation);
			wave_image = QSharedPointer<QImage>(new QImage(scaled_wave_image));
		}
	}
	else
	{
		// No audio samples present
		wave_image = QSharedPointer<QImage>(new QImage(width, height, QImage::Format_RGBA8888));
		wave_image->fill(QColor(QString::fromStdString("#000000")));
	}

	// Return new image
	return wave_image;
}

// Display the wave form
QSharedPointer<QImage> Frame::GetWaveform()
{
	// Get audio wave form image
	return GetWaveform(720, 480, 0, 123, 255, 255);
}

// Clear the waveform image (and deallocate it's memory)
void Frame::ClearWaveform()
{
	if (wave_image)
		wave_image.reset();
}

// Get the size in bytes of this frame (rough estimate)
long int Frame::GetBytes()
{
	long int total_bytes = 0;
	if (image)
	{
		total_bytes += (width * height * sizeof(char) * 4);
	}

	if (audio) 
	{
		// approximate audio size (sample rate / 24 fps)
		total_bytes += (sample_rate / 24.0) * sizeof(float);
	}

	// return size of this frame
	return total_bytes;
}


// Get pixel data (as packets)
const unsigned char* Frame::GetPixels()
{
	// Check for blank image
	if (!image)
	{
		// Fill with black
		AddColor(width, height, "#000000");
	}

	// Return array of pixel packets
	return image->bits();
}

// Get pixel data (for only a single scan-line)
const unsigned char* Frame::GetPixels(int row)
{
	// Return array of pixel packets
	return image->scanLine(row);
}

// Set Picture Type
void Frame::SetPictureType(int type)
{
	picture_type = type;
}

// Set Pixel Aspect Ratio
void Frame::SetPixelRatio(int num, int den)
{
	pixel_ratio.num = num;
	pixel_ratio.den = den;
}

// Set frame number
void Frame::SetFrameNumber(long int new_number)
{
	number = new_number;
}

// Calculate the # of samples per video frame (for a specific frame number and frame rate)
int Frame::GetSamplesPerFrame(long int number, Fraction fps, int sample_rate, int channels)
{
	// Get the total # of samples for the previous frame, and the current frame (rounded)
	double fps_rate = fps.Reciprocal().ToDouble();

	// Determine previous samples total, and make sure it's evenly divisible by the # of channels
	double previous_samples = (sample_rate * fps_rate) * (number - 1);
	double previous_samples_remainder = fmod(previous_samples, (double)channels); // subtract the remainder to the total (to make it evenly divisible)
	previous_samples -= previous_samples_remainder;

	// Determine the current samples total, and make sure it's evenly divisible by the # of channels
	double total_samples = (sample_rate * fps_rate) * number;
	double total_samples_remainder = fmod(total_samples, (double)channels); // subtract the remainder to the total (to make it evenly divisible)
	total_samples -= total_samples_remainder;

	// Subtract the previous frame's total samples with this frame's total samples.  Not all sample rates can
	// be evenly divided into frames, so each frame can have have different # of samples.
	int samples_per_frame = round(total_samples - previous_samples);
	return samples_per_frame;
}

// Calculate the # of samples per video frame (for the current frame number)
int Frame::GetSamplesPerFrame(Fraction fps, int sample_rate, int channels)
{
	return GetSamplesPerFrame(number, fps, sample_rate, channels);
}


// Save the frame image to the specified path.  The image format is determined from the extension (i.e. image.PNG, image.JPEG)
void Frame::SaveImage(QString path, float scale, string format, int quality)
{
	// Get preview image
	QSharedPointer<QImage> previewImage = GetImage();

	// scale image if needed
	if (abs(scale) > 1.001 || abs(scale) < 0.999)
	{
		int new_width = width;
		int new_height = height;

		// Update the image to reflect the correct pixel aspect ration (i.e. to fix non-squar pixels)
		if (pixel_ratio.num != 1 || pixel_ratio.den != 1)
		{
			// Calculate correct DAR (display aspect ratio)
			int new_width = previewImage->size().width();
			int new_height = previewImage->size().height() * pixel_ratio.Reciprocal().ToDouble();

			// Resize to fix DAR
			previewImage = QSharedPointer<QImage>(new QImage(previewImage->scaled(new_width, new_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
		}

		// Resize image
		previewImage = QSharedPointer<QImage>(new QImage(previewImage->scaled(new_width * scale, new_height * scale, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	}

	// Save image
	previewImage->save(path, format.c_str(), quality);
}

QSharedPointer<QImage> Frame::GetImage()
{
	// Check for blank image
	if (!image)
	{
		// Fill with black
		AddColor(width, height, "#000000");
	}

	return image;
}

// Add (or replace) pixel data to the frame (based on a solid color)
void Frame::AddColor(int new_width, int new_height, string color)
{
	// Create new image object, and fill with pixel data
	//std::lock_guard<std::mutex> lock(adding_image_mutex);

	image = QSharedPointer<QImage>(new QImage(new_width, new_height, QImage::Format_RGBA8888));

	// Fill with solid color
	image->fill(QColor(QString::fromStdString(color)));

	// Update height and width
	width = image->width();
	height = image->height();
	has_image_data = true;
}

// Add (or replace) pixel data to the frame
void Frame::AddImage(int new_width, int new_height, int bytes_per_pixel, QImage::Format format_type, const unsigned char *pixels_)
{
	// Create new buffer
	//std::lock_guard<std::mutex> lock(adding_image_mutex);

	int buffer_size = new_width * new_height * bytes_per_pixel;
	qbuffer = new unsigned char[buffer_size]();

	// Copy buffer data
	memcpy((unsigned char*)qbuffer, pixels_, buffer_size);

	// Create new image object, and fill with pixel data
	image = QSharedPointer<QImage>(new QImage(qbuffer, new_width, new_height, new_width * bytes_per_pixel, format_type, (QImageCleanupFunction)&vs::Frame::CleanUpBuffer, (void*)qbuffer));

	// Always convert to RGBA8888 (if different)
	if (image->format() != QImage::Format_RGBA8888)
		image->convertToFormat(QImage::Format_RGBA8888);

	// Update height and width
	width = image->width();
	height = image->height();
	has_image_data = true;
}

// Add (or replace) pixel data to the frame
void Frame::AddImage(QSharedPointer<QImage> new_image)
{
	// Ignore blank images
	if (!new_image)
		return;

	// assign image data
	//std::lock_guard<std::mutex> lock(adding_image_mutex);

	image = new_image;

	// Always convert to RGBA8888 (if different)
	if (image->format() != QImage::Format_RGBA8888)
	{
		image->convertToFormat(QImage::Format_RGBA8888);
	}

	// Update height and width
	width = image->width();
	height = image->height();
	has_image_data = true;
}

// Clean up buffer after QImage is deleted
void Frame::CleanUpBuffer(void *info)
{
	if (info)
	{
		// Remove buffer since QImage tells us to
		unsigned char* ptr_to_qbuffer = (unsigned char*)info;
		delete[] ptr_to_qbuffer;
	}
}


// Get number of audio channels
int Frame::GetAudioChannelsCount()
{
	//std::lock_guard<std::mutex> lock(adding_audio_mutex);

	if (audio)
		return audio->getNumChannels();
	else
		return 0;
}

// Get number of audio samples
int Frame::GetAudioSamplesCount()
{
	//std::lock_guard<std::mutex> lock(adding_audio_mutex);

	if (audio)
		return audio->getNumSamples();
	else
		return 0;
}

void Frame::AddAudio(bool replaceSamples, int destChannel, int destStartSample, const float* source, int numSamples, float gainToApplyToSource = 1.0f) {
	//std::lock_guard<std::mutex> lock(adding_audio_mutex);

	{
		// Extend audio container to hold more (or less) samples and channels.. if needed
		int new_length = destStartSample + numSamples;
		int new_channel_length = audio->getNumChannels();
		if (destChannel >= new_channel_length)
		{
			new_channel_length = destChannel + 1;
		}

		if (new_length > audio->getNumSamples() || new_channel_length > audio->getNumChannels())
		{
			audio->setSize(new_channel_length, new_length, true, true, false);
		}

		// Clear the range of samples first (if needed)
		if (replaceSamples)
		{
			audio->clear(destChannel, destStartSample, numSamples);
		}

		// Add samples to frame's audio buffer
		audio->addFrom(destChannel, destStartSample, source, numSamples, gainToApplyToSource);
		has_audio_data = true;
	}
}

QSharedPointer<AudioSampleBuffer> Frame::GetAudioBuffer()
{
	return audio;
}

// Add audio silence
void Frame::AddAudioSilence(int numSamples)
{
	//std::lock_guard<std::mutex> lock(adding_audio_mutex);

	// Resize audio container
	audio->setSize(channels, numSamples, false, true, false);
	audio->clear();
	has_audio_data = true;
}

// Get magnitude of range of samples (if channel is -1, return average of all channels for that sample)
float Frame::GetAudioSample(int channel, int sample, int magnitude_range)
{
	if (channel > 0) 
	{
		// return average magnitude for a specific channel/sample range
		return audio->getMagnitude(channel, sample, magnitude_range);
	}
	else 
	{
		// Return average magnitude for all channels
		return audio->getMagnitude(sample, magnitude_range);
	}
}

// Get an array of sample data
float* Frame::GetAudioSamples(int channel)
{
	return audio->getWritePointer(channel); // return audio data for this channel
}

// Resize audio container to hold more (or less) samples and channels
void Frame::ResizeAudio(int channels, int length, int rate, ChannelLayout layout)
{
	//std::lock_guard<std::mutex> lock(adding_audio_mutex);

	// Resize audio buffer
	audio->setSize(channels, length, true, true, false);
	channel_layout = layout;
	sample_rate = rate;
}

// Get a planar array of sample data
float* Frame::GetPlanarAudioSamples()
{
	float *output = NULL;
	AudioSampleBuffer *buffer(audio.get());
	int num_of_channels = audio->getNumChannels();
	int num_of_samples = audio->getNumSamples();

	// INTERLEAVE all samples together (channel 1 + channel 2 + channel 1 + channel 2, etc...)
	output = new float[num_of_channels * num_of_samples];
	int position = 0;

	// Loop through samples in each channel (combining them)
	for (int channel = 0; channel < num_of_channels; channel++)
	{
		for (int sample = 0; sample < num_of_samples; sample++)
		{
			// Add sample to output array
			output[position] = buffer->getReadPointer(channel)[sample];

			// increment position
			position++;
		}
	}

	// return combined array
	return output;
}


// Get an array of sample data (all channels interleaved together)
float* Frame::GetInterleavedAudioSamples()
{
	float *output = NULL;
	AudioSampleBuffer *buffer(audio.get());
	int num_of_channels = audio->getNumChannels();
	int num_of_samples = audio->getNumSamples();

	// INTERLEAVE all samples together (channel 1 + channel 2 + channel 1 + channel 2, etc...)
	output = new float[num_of_channels * num_of_samples];
	int position = 0;

	// Loop through samples in each channel (combining them)
	for (int sample = 0; sample < num_of_samples; sample++)
	{
		for (int channel = 0; channel < num_of_channels; channel++)
		{
			// Add sample to output array
			output[position] = buffer->getReadPointer(channel)[sample];

			// increment position
			position++;
		}
	}


	// return combined array
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
