/*
 * TestXPlaneUDPClient.cpp
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

#include <iostream>
#include <unistd.h>
#include <sstream>

#include <functional>

#include "XPlaneBeaconListener.h"
#include "XPlaneUDPClient.h"

#include <string.h>
#include <signal.h>

#include "XPUtils.h"

using namespace std;

// globals
bool found = false;
string hostname;
string host;
uint16_t port;

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
	host = server.host;
	port = server.receivePort;
    hostname = server.name;
	found = true;
}


static volatile int s_interrupted = 0;
static void s_signal_handler (int signal_value)
{
    s_interrupted = 1;
}

static void s_catch_signals (void)
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

int main() {

	XPlaneBeaconListener::getInstance()->registerNotificationCallback(
			std::bind(receiverBeaconCallback, std::placeholders::_1,
					std::placeholders::_2));
	XPlaneBeaconListener::getInstance()->setDebug(0);

    s_catch_signals();

	cout << "Press Control-C to abort." << endl;

	// wait for a server
    while (!found && s_interrupted == 0) {
		sleep (1);
	}

    cout << "Found server " << hostname << " " << host << ":" << port << endl;

    return 0;

	XPlaneUDPClient xp(host, port,
			std::bind(receiverCallbackFloat, std::placeholders::_1,
					std::placeholders::_2),
			std::bind(receiverCallbackString, std::placeholders::_1,
					std::placeholders::_2));
    xp.setDebug(1);

	xp.subscribeDataRef("sim/aircraft/view/acf_descrip[0][40]", 1);
    xp.subscribeDataRef("sim/cockpit2/engine/actuators/throttle_ratio[0]", 2);

	xp.sendCommand("sim/flight_controls/flaps_down");
	xp.sendCommand("sim/flight_controls/flaps_down");
    xp.unsubscribeDataRef("sim/aircraft/view/acf_descrip[0][40]");

	float r = 0;
	float i = 0.01;

    while (s_interrupted == 0) {
        usleep (1000 * 1000);

		xp.setDataRef("sim/multiplayer/controls/engine_throttle_request[0]", r);
		r += i;

		if (r > 1) {
			i = -0.01;
		} else if (r < 0) {
			i = 0.01;
		}

        if(r>0.05)
            s_interrupted = 1;

	}

    return 0;
}
