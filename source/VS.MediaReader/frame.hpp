#ifndef GUARD_frame_20190207153226_
#define GUARD_frame_20190207153226_
/*
@file		frame.hpp
@author		Webstar
@date		2019-07-02 15:32
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

// STD
#include <mutex>
// QT
#include <QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QImage>
#include <QtGui/QColor>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>
#include <QAudioBuffer>
#include <QBuffer>

#include "common.hpp"
#include "utilities.hpp"
#include "audio_buffer.hpp"

using namespace vs;

namespace vs
{
	/// @brief This class represents a single frame of video (i.e. image & audio data)
	/// @remark FileReaders use instances of this class to store the individual frames of video,
	/// which include both the image data (i.e. pixels) and audio samples. An Frame also has many debug
	/// methods, such as the ability to display the image (using X11), play the audio samples (using JUCE), or
	/// display the audio waveform as an image.
	/// FileWriters use instances of this class to create new video files, image files, or
	/// video streams. So, think of these Frame instances as the smallest unit of work in a video
	/// editor.
	/// There are many ways to create an instance of an Frame:
	/// @code
	/// 
	/// // Most basic: a blank frame (300x200 blank image, 48kHz audio silence)
	///  Frame();
	/// 
	///  // Image only settings (48kHz audio silence)
	///  Frame(1, // Frame number
	///        720, // Width of image
	///        480, // Height of image
	///        "#000000" // HTML color code of background color
	///        );
	/// 
	///  // Audio only (300x200 blank image)
	///  Frame(number, // Frame number
	///        44100, // Sample rate of audio stream
	///        2 // Number of audio channels
	///        );
	/// 
	///  // Image and Audio settings (user defines all key settings)
	///  Frame(number, // Frame number
	///        720, // Width of image
	///        480, // Height of image
	///        "#000000" // HTML color code of background color
	///        44100, // Sample rate of audio stream
	///        2 // Number of audio channels
	///      );
	/// 
	///  @endcode
	class Frame {
	private:
		std::mutex adding_image_mutex;
		std::mutex adding_audio_mutex;

		// Image Data
		QSharedPointer<QImage> image;
		const unsigned char *qbuffer;
		Fraction pixel_ratio;
		int picture_type;

		// Audio Data
		QSharedPointer<AudioSampleBuffer> audio;
		int channels;
		ChannelLayout channel_layout;
		int width;
		int height;
		int sample_rate;
		QSharedPointer<QImage> wave_image;
		
		/// Constrain a color value from 0 to 255
		int constrain(int color_value);


		/// Display the wave form
		void DisplayWaveform();
	public:
		long int number;				///< This is the frame number (starting at 1)
		bool has_audio_data;			///< This frame has been loaded with audio data
		bool has_image_data;			///< This frame has been loaded with pixel data

		/// Constructor - Blank
		Frame();

		/// Constructor - image only (48kHz audio silence)
		Frame(long int number, int width, int height, string color);

		/// Constructor - audio only (300x200 blank image)
		Frame(long int number, int samples, int channels);

		/// Constructor - image & audio
		Frame(long int number, int width, int height, string color, int samples, int channels);

		/// Copy constructor
		Frame(const Frame &other);

		/// Destructor
		~Frame();

		/// Add (or replace) pixel data to the frame (based on a solid color)
		void AddColor(int new_width, int new_height, string color);

		/// Add (or replace) pixel data to the frame
		void AddImage(int new_width, int new_height, int bytes_per_pixel, QImage::Format format_type, const unsigned char *pixels_);

		/// Add (or replace) pixel data to the frame
		void AddImage(QSharedPointer<QImage> new_image);

		/// @brief Channel Layout of audio samples.
		/// @remark A frame needs to keep track of this, since Writers do not always
		/// know the original channel layout of a frame's audio samples (i.e. mono, stereo, 5 point surround, etc...)
		ChannelLayout ChannelsLayout() { return channel_layout; }

		/// Set the channel layout of audio samples (i.e. mono, stereo, 5 point surround, etc...)
		void ChannelsLayout(ChannelLayout new_channel_layout) { channel_layout = new_channel_layout; };

		/// Clean up buffer after QImage is deleted
		static void CleanUpBuffer(void *info);

		/// Clear the waveform image (and deallocate it's memory)
		void ClearWaveform();

		/// Copy data and pointers from another Frame instance
		void DeepCopy(const Frame& other);

		/// Calculate the # of samples per video frame (for the current frame number)
		int GetSamplesPerFrame(Fraction fps, int sample_rate, int channels);

		/// Calculate the # of samples per video frame (for a specific frame number and frame rate)
		static int GetSamplesPerFrame(long int frame_number, Fraction fps, int sample_rate, int channels);

		/// Get the original sample rate of this frame's audio data
		int SampleRate() { return sample_rate; }

		/// Set the original sample rate of this frame's audio data
		void SampleRate(int orig_sample_rate) { sample_rate = orig_sample_rate; };

		/// Add audio samples to a specific channel
		void AddAudio(bool replaceSamples, int destChannel, int destStartSample, const float* source, int numSamples, float gainToApplyToSource);

		/// Add audio silence
		void AddAudioSilence(int numSamples);

		QSharedPointer<AudioSampleBuffer> GetAudioBuffer();

		/// Get magnitude of range of samples (if channel is -1, return average of all channels for that sample)
		float GetAudioSample(int channel, int sample, int magnitude_range);

		/// Get an array of sample data
		float* GetAudioSamples(int channel);

		/// Get an array of sample data (all channels interleaved together)
		float* GetInterleavedAudioSamples();

		/// Get a planar array of sample data
		float* GetPlanarAudioSamples();

		/// Resize audio container to hold more (or less) samples and channels
		void ResizeAudio(int channels, int length, int sample_rate, ChannelLayout channel_layout);

		/// Get number of audio channels
		int GetAudioChannelsCount();

		/// Get number of audio samples
		int GetAudioSamplesCount();

		/// Get the size in bytes of this frame (rough estimate)
		long int GetBytes();

		/// Get pointer to Qt QImage image object
		QSharedPointer<QImage> GetImage();

		/// Set Pixel Aspect Ratio
		Fraction GetPixelRatio() { return pixel_ratio; };

		/// Get pixel data (as packets)
		const unsigned char* GetPixels();

		/// Get pixel data (for only a single scan-line)
		const unsigned char* GetPixels(int row);

		/// Get height of image
		int GetHeight() { return height; }

		/// Get an audio waveform image (with formating options)
		QSharedPointer<QImage> GetWaveform(int width, int height, int Red, int Green, int Blue, int Alpha);

		/// Get an audio waveform image (uses default options)
		QSharedPointer<QImage> GetWaveform();

		/// Get height of image
		int GetWidth() { return width; }

		/// Save the frame image to the specified path.  The image format can be BMP, JPG, JPEG, PNG, PPM, XBM, XPM
		void SaveImage(QString path, float scale, string format = "PNG", int quality = 100);

		/// Set frame number
		void SetFrameNumber(long int number);

		/// Set Pixel Aspect Ratio
		void SetPixelRatio(int num, int den);

		/// Set Picture Type
		void SetPictureType(int type);

		/// Set Picture Type
		int GetPictureType() { return picture_type; };

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