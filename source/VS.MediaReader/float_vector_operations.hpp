#ifndef GUARD_float_vector_operations_20190207153248_
#define GUARD_float_vector_operations_20190207153248_
/*
@file		float_vector_operations.hpp
@author		Webstar
@date		2019-07-02 15:32
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/


namespace vs
{
	///  Fills a block of memory with zeros.
	inline void zeromem(void* memory, size_t numBytes)  { memset(memory, 0, numBytes); }

	/// Returns the larger of two values
	template <typename Type>
	Type typed_max(const Type a, const Type b) { return (a < b) ? b : a; }

	/// Returns the larger of three values
	template <typename Type>
	Type typed_max(const Type a, const Type b, const Type c) { return (a < b) ? ((b < c) ? c : b) : ((a < c) ? c : a); }

	/// Returns the larger of four values
	template <typename Type>
	Type typed_max(const Type a, const Type b, const Type c, const Type d) { return typed_max(a, typed_max(b, c, d)); }

	/// Returns the smaller of two values
	template <typename Type>
	Type typed_min(const Type a, const Type b) { return (b < a) ? b : a; }

	/// Returns the smaller of three values
	template <typename Type>
	Type typed_min(const Type a, const Type b, const Type c) { return (b < a) ? ((c < b) ? c : b) : ((c < a) ? c : a); }

	/// Returns the smaller of four values
	template <typename Type>
	Type typed_min(const Type a, const Type b, const Type c, const Type d) { return typed_min(a, typed_min(b, c, d)); }

	/// Scans an array of values, returning the minimum value that it contains
	template <typename Type>
	Type findMinimum(const Type* data, int numValues)
	{
		if (numValues <= 0)
			return Type();

		Type result(*data++);

		while (--numValues > 0) // (> 0 rather than >= 0 because we've already taken the first sample)
		{
			const Type& v = *data++;
			if (v < result)  result = v;
		}

		return result;
	}

	/// Scans an array of values, returning the maximum value that it contains
	template <typename Type>
	Type findMaximum(const Type* values, int numValues)
	{
		if (numValues <= 0)
			return Type();

		Type result(*values++);

		while (--numValues > 0) // (> 0 rather than >= 0 because we've already taken the first sample)
		{
			const Type& v = *values++;
			if (result < v)  result = v;
		}

		return result;
	}

	/// Scans an array of values, returning the minimum and maximum values that it contains
	template <typename Type>
	void findMinAndMax(const Type* values, int numValues, Type& lowest, Type& highest)
	{
		if (numValues <= 0)
		{
			lowest = Type();
			highest = Type();
		}
		else
		{
			Type mn(*values++);
			Type mx(mn);

			while (--numValues > 0) // (> 0 rather than >= 0 because we've already taken the first sample)
			{
				const Type& v = *values++;

				if (mx < v)  mx = v;
				if (v < mn)  mn = v;
			}

			lowest = mn;
			highest = mx;
		}
	}

	/// @brief Handy function for getting the number of elements in a simple const C array.
	/// @code
	/// static int myArray[] = { 1, 2, 3 };
	/// int numElements = numElementsInArray (myArray) // returns 3
	/// @endcode
	template <typename Type, int N>
	int numElementsInArray(Type(&array)[N])
	{
		(void) sizeof(0[array]); // This line should cause an error if you pass an object with a user-defined subscript operator
		return N;
	}

	/// @brief A collection of simple vector operations on arrays of floats
	class FloatVectorOperations
	{
	public:
		/// Clears a vector of floats
		static void  clear(float* dest, int numValues) ;

		/// Clears a vector of doubles
		static void  clear(double* dest, int numValues) ;

		/// Copies a repeated value into a vector of floats
		static void  fill(float* dest, float valueToFill, int numValues) ;

		/// Copies a repeated value into a vector of doubles
		static void  fill(double* dest, double valueToFill, int numValues) ;

		/// Copies a vector of floats
		static void  copy(float* dest, const float* src, int numValues) ;

		/// Copies a vector of doubles
		static void  copy(double* dest, const double* src, int numValues) ;

		/// Copies a vector of floats, multiplying each value by a given multiplier */
		static void  copyWithMultiply(float* dest, const float* src, float multiplier, int numValues) ;

		/// Copies a vector of doubles, multiplying each value by a given multiplier */
		static void  copyWithMultiply(double* dest, const double* src, double multiplier, int numValues) ;

		/// Adds a fixed value to the destination values
		static void  add(float* dest, float amountToAdd, int numValues) ;

		/// Adds a fixed value to the destination values
		static void  add(double* dest, double amountToAdd, int numValues) ;

		/// Adds a fixed value to each source value and stores it in the destination array
		static void  add(float* dest, const float* src, float amount, int numValues) ;

		/// Adds a fixed value to each source value and stores it in the destination array
		static void  add(double* dest, const double* src, double amount, int numValues) ;

		/// Adds the source values to the destination values
		static void  add(float* dest, const float* src, int numValues) ;

		/// Adds the source values to the destination values
		static void  add(double* dest, const double* src, int numValues) ;

		/// Adds each source1 value to the corresponding source2 value and stores the result in the destination array
		static void  add(float* dest, const float* src1, const float* src2, int num) ;

		/// Adds each source1 value to the corresponding source2 value and stores the result in the destination array
		static void  add(double* dest, const double* src1, const double* src2, int num) ;

		/// Subtracts the source values from the destination values
		static void  subtract(float* dest, const float* src, int numValues) ;

		/// Subtracts the source values from the destination values
		static void  subtract(double* dest, const double* src, int numValues) ;

		/// Subtracts each source2 value from the corresponding source1 value and stores the result in the destination array
		static void  subtract(float* dest, const float* src1, const float* src2, int num) ;

		/// Subtracts each source2 value from the corresponding source1 value and stores the result in the destination array
		static void  subtract(double* dest, const double* src1, const double* src2, int num) ;

		/// Multiplies each source value by the given multiplier, then adds it to the destination value
		static void  addWithMultiply(float* dest, const float* src, float multiplier, int numValues) ;

		/// Multiplies each source value by the given multiplier, then adds it to the destination value
		static void  addWithMultiply(double* dest, const double* src, double multiplier, int numValues) ;

		/// Multiplies each source1 value by the corresponding source2 value, then adds it to the destination value
		static void  addWithMultiply(float* dest, const float* src1, const float* src2, int num) ;

		/// Multiplies each source1 value by the corresponding source2 value, then adds it to the destination value
		static void  addWithMultiply(double* dest, const double* src1, const double* src2, int num) ;

		/// Multiplies each source value by the given multiplier, then subtracts it to the destination value
		static void  subtractWithMultiply(float* dest, const float* src, float multiplier, int numValues) ;

		/// Multiplies each source value by the given multiplier, then subtracts it to the destination value
		static void  subtractWithMultiply(double* dest, const double* src, double multiplier, int numValues) ;

		/// Multiplies each source1 value by the corresponding source2 value, then subtracts it to the destination value
		static void  subtractWithMultiply(float* dest, const float* src1, const float* src2, int num) ;

		/// Multiplies each source1 value by the corresponding source2 value, then subtracts it to the destination value
		static void  subtractWithMultiply(double* dest, const double* src1, const double* src2, int num) ;

		/// Multiplies the destination values by the source values
		static void  multiply(float* dest, const float* src, int numValues) ;

		/// Multiplies the destination values by the source values
		static void  multiply(double* dest, const double* src, int numValues) ;

		/// Multiplies each source1 value by the correspinding source2 value, then stores it in the destination array
		static void  multiply(float* dest, const float* src1, const float* src2, int numValues) ;

		/// Multiplies each source1 value by the correspinding source2 value, then stores it in the destination array
		static void  multiply(double* dest, const double* src1, const double* src2, int numValues) ;

		/// Multiplies each of the destination values by a fixed multiplier
		static void  multiply(float* dest, float multiplier, int numValues) ;

		/// Multiplies each of the destination values by a fixed multiplier
		static void  multiply(double* dest, double multiplier, int numValues) ;

		/// Multiplies each of the source values by a fixed multiplier and stores the result in the destination array
		static void  multiply(float* dest, const float* src, float multiplier, int num) ;

		/// Multiplies each of the source values by a fixed multiplier and stores the result in the destination array
		static void  multiply(double* dest, const double* src, double multiplier, int num) ;

		/// Copies a source vector to a destination, negating each value
		static void  negate(float* dest, const float* src, int numValues) ;

		/// Copies a source vector to a destination, negating each value
		static void  negate(double* dest, const double* src, int numValues) ;

		/// Copies a source vector to a destination, taking the absolute of each value
		static void  abs(float* dest, const float* src, int numValues) ;

		/// Copies a source vector to a destination, taking the absolute of each value
		static void  abs(double* dest, const double* src, int numValues) ;

		/// Converts a stream of integers to floats, multiplying each one by the given multiplier
		static void  convertFixedToFloat(float* dest, const int* src, float multiplier, int numValues) ;

		/// Each element of dest will be the minimum of the corresponding element of the source array and the given comp value
		static void  min(float* dest, const float* src, float comp, int num) ;

		/// Each element of dest will be the minimum of the corresponding element of the source array and the given comp value
		static void  min(double* dest, const double* src, double comp, int num) ;

		/// Each element of dest will be the minimum of the corresponding source1 and source2 values
		static void  min(float* dest, const float* src1, const float* src2, int num) ;

		/// Each element of dest will be the minimum of the corresponding source1 and source2 values
		static void  min(double* dest, const double* src1, const double* src2, int num) ;

		/// Each element of dest will be the maximum of the corresponding element of the source array and the given comp value
		static void  max(float* dest, const float* src, float comp, int num) ;

		/// Each element of dest will be the maximum of the corresponding element of the source array and the given comp value
		static void  max(double* dest, const double* src, double comp, int num) ;

		/// Each element of dest will be the maximum of the corresponding source1 and source2 values
		static void  max(float* dest, const float* src1, const float* src2, int num) ;

		/// Each element of dest will be the maximum of the corresponding source1 and source2 values
		static void  max(double* dest, const double* src1, const double* src2, int num) ;

		/// Each element of dest is calculated by hard clipping the corresponding src element so that it is in the range specified by the arguments low and high
		static void  clip(float* dest, const float* src, float low, float high, int num) ;

		/// Each element of dest is calculated by hard clipping the corresponding src element so that it is in the range specified by the arguments low and high
		static void  clip(double* dest, const double* src, double low, double high, int num) ;

		/// Finds the miniumum and maximum values in the given array
		// static Range<float>  findMinAndMax(const float* src, int numValues) ;

		/// Finds the miniumum and maximum values in the given array
		// static Range<double>  findMinAndMax(const double* src, int numValues) ;

		/// Finds the miniumum value in the given array
		static float  findMinimum(const float* src, int numValues) ;

		/// Finds the miniumum value in the given array
		static double  findMinimum(const double* src, int numValues) ;

		/// Finds the maximum value in the given array
		static float  findMaximum(const float* src, int numValues) ;

		/// Finds the maximum value in the given array
		static double  findMaximum(const double* src, int numValues) ;
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