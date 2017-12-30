#include "XPUtils.h"

#include <iostream>

using namespace std;

uint32_t xint2uint32 (uint8_t * buf) {

	/* cerr <<
		" buf[0] is " << (int) buf[0] <<
		" buf[1] is " << (int) buf[1] <<
		" buf[2] is " << (int) buf[2] <<
		" buf[3] is " << (int) buf[3] <<
		endl; */

	return buf[3] << 24 | buf [2] << 16 | buf [1] << 8 | buf [0];

}


float xflt2float (uint8_t * buf) {

	// treat it as a uint32_t first, so we can flip the bits based on
	// local endianness. Then union it into a float.

	union {
		float f;
		uint32_t i;
	} v;

	v.i = xint2uint32 (buf);
	return v.f;


}
