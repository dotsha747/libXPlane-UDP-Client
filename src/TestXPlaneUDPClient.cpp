//============================================================================
// Name        : TestXPlaneUDPClient.cpp
// Author      : Shahada Abubakar
// Version     :
// Copyright   : Copyright (c) 2014, NEXTSense Sdn Bhd
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <unistd.h>
#include <sstream>

#include <functional>

#include "XPlaneBeaconListener.h"
#include "XPlaneUDPClient.h"

#include <string.h>

#include "XPUtils.h"

using namespace std;

// our callback for changed values.

void receiverCallbackFloat(std::string dataref, float value) {

	cout << "receiverCallbackFloat got [" << dataref << "] and [" << value
			<< "]" << endl;
}

void receiverCallbackString(std::string dataref, std::string value) {
	cout << "receiverCallbackString got [" << dataref << "] and [" << value
			<< "]" << endl;
}

void receiverBeaconCallback(XPlaneBeaconListener::XPlaneServer server,
		bool exists) {
	cout << "receiverBeaconCallback got [" << server.toString() << " is "
			<< (exists ? "alive" : "dead") << "]" << endl;
}

int main() {

	XPlaneBeaconListener::getInstance()->registerNotificationCallback(
			std::bind(receiverBeaconCallback, std::placeholders::_1,
					std::placeholders::_2));
	XPlaneBeaconListener::getInstance()->setDebug(0);

	XPlaneUDPClient xp("192.168.1.10", 49000,
			std::bind(receiverCallbackFloat, std::placeholders::_1,
					std::placeholders::_2),
			std::bind(receiverCallbackString, std::placeholders::_1,
					std::placeholders::_2));
	xp.setDebug(0);

	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!

	xp.subscribeDataRef("sim/aircraft/view/acf_descrip[0][40]", 1);
	xp.subscribeDataRef("sim/cockpit2/engine/actuators/throttle_ratio[0]", 10);
	xp.subscribeDataRef("laminar/B738/fmc1/Line00_L[0][25]", 2);
	xp.subscribeDataRef("laminar/B738/fmc1/Line00_S[0][25]", 2);
	xp.subscribeDataRef("laminar/B738/fmc1/Line_entry[0][25]", 2);
	xp.subscribeDataRef("laminar/B738/fmc1/Line_entry_I[0][25]", 2);
	xp.subscribeDataRef("sim/cockpit2/radios/actuators/audio_nav_selection", 2);
	xp.subscribeDataRef("sim/flightmodel2/engines/engine_is_burning_fuel[0]",
			1);

	float r = 0;

	while (1) {
		sleep(5);

		list<XPlaneBeaconListener::XPlaneServer> s;
		XPlaneBeaconListener::getInstance()->get(s);

		int i = 1;
		cout << "Current List of Servers [" << s.size() << "]:" << endl;
		for (auto t : s) {
			cout << i++ << ": " << t.toString() << endl;
		}

		xp.sendCommand("sim/flight_controls/flaps_down");
		xp.setDataRef("sim/multiplayer/controls/engine_throttle_request[0]", r);
		r += 0.1;

	}

	cout << "Main done." << endl;
	return 0;
}

