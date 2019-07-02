#ifndef GUARD_exceptions_20190207153259_
#define GUARD_exceptions_20190207153259_
/*
@file		exceptions.hpp
@author		Webstar
@date		2019-07-02 15:32
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/



#include <string>

using namespace std;

namespace vs
{
	namespace exceptions
	{
		/// @brief Base exception class with a custom message variable.
		/// A custom error message field has been added to the std::exception base class.
		class BaseException : public exception
		{
		protected:
			string m_message;
		public:
			BaseException(string message) : m_message(message) { }
			virtual ~BaseException() throw () {}
			virtual const char* what() const throw () {
				// return custom message
				return m_message.c_str();
			}
		};

		/// Exception for files that can not be found or opened
		class InvalidFile : public BaseException
		{
		public:
			string file_path;
			InvalidFile(string message, string file_path)
				: BaseException(message), file_path(file_path) { }
			virtual ~InvalidFile() throw () {}
		};

		/// Exception when no streams are found in the file
		class NoStreamsFound : public BaseException
		{
		public:
			string file_path;
			NoStreamsFound(string message, string file_path)
				: BaseException(message), file_path(file_path) { }
			virtual ~NoStreamsFound() throw () {}
		};

		/// Exception when no valid codec is found for a file
		class InvalidCodec : public BaseException
		{
		public:
			string file_path;
			InvalidCodec(string message, string file_path)
				: BaseException(message), file_path(file_path) { }
			virtual ~InvalidCodec() throw () {}
		};

		/// Exception when a reader is closed, and a frame is requested
		class ReaderClosed : public BaseException
		{
		public:
			string file_path;
			ReaderClosed(string message, string file_path)
				: BaseException(message), file_path(file_path) { }
			virtual ~ReaderClosed() throw () {}
		};

		/// Exception for frames that are out of bounds.
		class OutOfBoundsFrame : public BaseException
		{
		public:
			int current_frame;
			int video_length;
			OutOfBoundsFrame(string message, int current_frame, int video_length)
				: BaseException(message), current_frame(current_frame), video_length(video_length) { }
			virtual ~OutOfBoundsFrame() throw () {}
		};

	}
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