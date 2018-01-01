/*
 * XPlaneUDPClient.h
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


#ifndef XPLANEUDPCLIENT_SRC_XPLANEUDPCLIENT_H_
#define XPLANEUDPCLIENT_SRC_XPLANEUDPCLIENT_H_

#include <map>
#include <string>
#include <cstdint>
#include <netinet/in.h>
#include <list>
#include <functional>
#include <string>
#include <mutex>
#include <vector>

/** @brief Utility class for getting dataref values from X-Plane over a network.
 *
 * 	It uses X-Plane's UDP interface. First create an instance specifying the host IP
 * 	and port of your X-Plane server.
 *
 * 	Then call subscribeDataRef, passing in the dataRef name, a frequency to with which
 * 	to get updates from the server, and expected data type, and a callback reference.
 * 	The callback
 *
 * 	can we override the subscribe function by return type?
 *
 * 	expectedType = Int, Float, String
 *
 * 	a special case occurs for strings, which are represented as character arrays.
 * 	The callback only occurs [when] and the entire string is passed in at a time.
 *
 * 	callbacks are only performed when there is a change in the data from the last
 * 	time a callback was done.
 *
 * 	The class will launch a background thread when started.
 *
 */

class XPlaneUDPClient {

	std::string server;
	uint16_t port;
	int sock;
	struct sockaddr_in serverAddr;
	socklen_t slen;
	time_t lastUnsubscribeCheck;
	int debug;

	// receiver callback
	std::function<void(std::string, float)> receiverCallbackFloat;
	std::function<void(std::string, std::string)> receiverCallbackString;

	class SubscribedCharArray {
		std::string dataRefName;
		uint16_t rangeFrom;
		uint16_t rangeTo;
		std::vector<uint32_t> enList;
		bool first;

		std::string value;

	public:
		SubscribedCharArray(std::string _dataRefName, uint16_t _rangeFrom,
				uint16_t _rangeTo) {
			dataRefName = _dataRefName;
			rangeFrom = _rangeFrom;
			rangeTo = _rangeTo;
			first = true;
			value = "impossible!#$!@#";
		}

		void addToEnList(uint32_t en) {
			enList.push_back(en);
		}

		std::string getDataRefName() { return dataRefName;};
		uint16_t getRangeFrom () { return rangeFrom;};
		uint16_t getRangeTo () { return rangeTo;};
		uint16_t getEn(int idx) { return enList[idx];};
		std::string getValue() { return value;};
		void setValue (std::string _value) { value = _value;};
		bool getFirst() { return first;};
		void setFirst (bool _first) { first = _first;};


	};
	std::list<SubscribedCharArray> subscribedCharArrayList;

	class SubscribedDataRef {

		// set when created
		std::string dataRef;
		uint32_t arrIndex; // extracted from dataRefName, if present.
		uint32_t en;
		uint32_t minFreq;

		bool emitable;
		SubscribedCharArray * partOfCharArray;


		// updated whenever we get a new value fro mserver
		time_t ts;
		bool first;
		float value;

	public:
		// constructor for subscribedDataRef
		SubscribedDataRef(std::string dataRef, uint32_t en, int minFreq, bool _emitable, SubscribedCharArray * _sca);

		// returns the name of the dataRef
		std::string getDataRefName();

		uint32_t getFreq();
		void setFreq(uint32_t freq);

		uint32_t getEn();

		time_t getLastUpdate() {
			return ts;
		}

		bool getEmitable () { return emitable;};
		SubscribedCharArray * getSubscribedCharArray() { return partOfCharArray;};

		void setValue (time_t _ts, float _data) {
			ts = _ts;
			value = _data;
		}

		float getValue () {
			return value;
		}

		void setFirst (bool _first) {
			first = _first;
		}

		bool getFirst () {
			return first;
		}

	};

	// store subscribedDatrefs and indexes.
	std::mutex dataRefMutex;
	std::list<SubscribedDataRef *> dataRefs;
	std::map<uint32_t, SubscribedDataRef *> dataRefByEnIndex;
	std::map<std::string, SubscribedDataRef *> dataRefByNameIndex;
	uint32_t lastEn;

protected:
	void subscribeIndividualDataRef(std::string dataRef, uint32_t minFreq,
			bool _emitable = true, SubscribedCharArray * _sca = NULL);

	void listenerThread();

	bool isRunning;
	bool quitFlag;

public:

	XPlaneUDPClient(std::string _server, uint16_t _port,
			std::function<void(std::string, float)> _receiverCallbackFloat,
			std::function<void(std::string, std::string)> _receiverCallbackString
			);
	virtual ~XPlaneUDPClient();

	void setDebug (int _debug) {
		debug = _debug;
	}

	void subscribeDataRef(std::string dataRef, uint32_t minFreq);
	void unsubscribeDataRef(std::string dataRef);

	void sendCommand (std::string cmd);

	void setDataRef (std::string dataRef, float value);
	void setDataRefString (std::string dataRef, std::string);


};

#endif /* XPLANEUDPCLIENT_SRC_XPLANEUDPCLIENT_H_ */

