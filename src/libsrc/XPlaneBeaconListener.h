/*
 * XPlaneBeaconListener.h
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

#ifndef XPLANEUDPCLIENT_SRC_XPLANEBEACONLISTENER_H_
#define XPLANEUDPCLIENT_SRC_XPLANEBEACONLISTENER_H_

#include <cstdint>
#include <set>
#include <time.h>
#include <map>
#include <list>
#include <mutex>
#include <functional>
#include <thread>


/** All running X-Plane instances broadcast their presence on the LANs they
 * 	are connected to, using UDP Multicast.
 *
 * 	This class lets you see what X-Plane instances are running on your LAN,
 * 	and probe their details such as IP address, version number and so on.
 * 	It can be used to offer users a choice of which X-Plane they wish to
 * 	connect to, instead of forcing them to configure hard-coded IP
 * 	addresses.
 *
 * 	Accessing the class will launch a background thread that will continuously
 * 	monitor for broadcast messages. The current set of data can be queried at
 * 	any time.
 *
 * 	It is also possible to configure callbacks so that your code can be
 * 	notified whenever a new client appears or disappears.
 *
 * 	This service is compatible with X-Plane 10 and 11. Details on the beacon
 * 	protocol are documented in X-Plane 11/Instructions/X-Plane SPECS from
 * 	Austin/Sending Data to X-Plane.rtfd/TXT.rtf.
 *
 * 	The class is based around the Singleton design pattern, as you're likely
 * 	to only ever need one instance and it's easier to call it from anywhere
 * 	in your code.
 *
 */

class XPlaneBeaconListener {

private:
	XPlaneBeaconListener(); // private so it cannot be called
	XPlaneBeaconListener(XPlaneBeaconListener const &) {}; // private so it cannot be called
	XPlaneBeaconListener & operator= (XPlaneBeaconListener const &) { abort();};
    std::thread * worker_thread;

public:

	void setDebug (int _debug) {
		debug = _debug;
	}

	class XPlaneServer {
	public:
		time_t received;
		std::string prologue;
		uint8_t beaconMajorVersion;
		uint8_t beaconMinorVersion;
		int32_t applicationHostId;
		int32_t versionNumber;
		int32_t role;
		uint16_t receivePort;
		std::string name;
		std::string host;
		XPlaneServer () {};
		XPlaneServer (time_t time, char * mesg, char * host);
		std::string toString();
	};

	static XPlaneBeaconListener * getInstance() {
		if (!instance) {
			instance = new XPlaneBeaconListener ();
		}
		return instance;
	}

	virtual ~XPlaneBeaconListener();
	void get (std::list<XPlaneBeaconListener::XPlaneServer> & ret);
	void registerNotificationCallback (std::function<void(XPlaneServer server, bool exists)> callback);

protected:

	bool quitFlag;
	bool isRunning;
	int debug;

	static XPlaneBeaconListener * instance;

	void runListener ();

	std::map<std::string, XPlaneServer> cachedServers;
	std::mutex cachedServersMutex;

	std::list<std::function<void(XPlaneServer server, bool exists)>> callbacks;

	void checkForExpiredServers ();

};

#endif /* XPLANEUDPCLIENT_SRC_XPLANEBEACONLISTENER_H_ */
