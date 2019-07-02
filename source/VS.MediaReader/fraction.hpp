#ifndef GUARD_fraction_20190207153241_
#define GUARD_fraction_20190207153241_
/*
@file		fraction.hpp
@author		Webstar
@date		2019-07-02 15:32
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

#include <math.h>

namespace vs
{
	/// @brief This class represents a fraction
	/// @remark Fractions are often used in video editing to represent ratios and rates, for example:
	/// pixel ratios, frames per second, timebase, and other common ratios.  Fractions are preferred
	/// over decimals due to their increased precision.
	class Fraction {
	public:
		int num; ///<Numerator for the fraction
		int den; ///<Denominator for the fraction

		/// Default Constructor
		Fraction();

		/// Constructor with numerator and denominator
		Fraction(int num, int den);

		/// Calculate the greatest common denominator
		int GreatestCommonDenominator();

		/// Reduce this fraction (i.e. 640/480 = 4/3)
		void Reduce();

		/// Return this fraction as a float (i.e. 1/2 = 0.5)
		float ToFloat();

		/// Return this fraction as a double (i.e. 1/2 = 0.5)
		double ToDouble();

		/// Return a rounded integer of the fraction (for example 30000/1001 returns 30)
		int ToInt();

		/// Return the reciprocal as a Fraction
		Fraction Reciprocal();
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