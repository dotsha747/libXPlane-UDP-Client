/*
 * XPUtils.cpp
 *
 *  Created on: Jul 27, 2017
 *      Author: shahada
 *
 *  Copyright (c) 2017-2018 Shahada Abubakar.
 *
 *  This file is part of libXPlane-UDP-Client.
 *
 *  This program is free software: you can redistribute it and/or
 *  modify it under the terms of the Lesser GNU General Public
 *  License as  published by the Free Software Foundation, either
 *  version 3 of the  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  See the GNU General Public License for more details.
 *
 */

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
