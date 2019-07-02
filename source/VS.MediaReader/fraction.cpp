/*
@file		fraction.cpp
@author		Webstar
@date		2019-07-02 15:34
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

#include "fraction.hpp"

using namespace vs;

// Constructor
Fraction::Fraction() :
	num(1), den(1) {
}
Fraction::Fraction(int num, int den) :
	num(num), den(den) {
}

// Return this fraction as a float (i.e. 1/2 = 0.5)
float Fraction::ToFloat() {
	return float(num) / float(den);
}

// Return this fraction as a double (i.e. 1/2 = 0.5)
double Fraction::ToDouble() {
	return double(num) / double(den);
}

// Return a rounded integer of the frame rate (for example 30000/1001 returns 30 fps)
int Fraction::ToInt() {
	return round((double)num / den);
}

// Calculate the greatest common denominator
int Fraction::GreatestCommonDenominator() {
	int first = num;
	int second = den;

	// Find the biggest whole number that will divide into both the numerator
	// and denominator
	int t;
	while (second != 0) {
		t = second;
		second = first % second;
		first = t;
	}
	return first;
}

void Fraction::Reduce() {
	// Get the greatest common denominator
	int GCD = GreatestCommonDenominator();

	// Reduce this fraction to the smallest possible whole numbers
	num = num / GCD;
	den = den / GCD;
}

// Return the reciprocal as a new Fraction
Fraction Fraction::Reciprocal()
{
	// flip the fraction
	return Fraction(den, num);
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
