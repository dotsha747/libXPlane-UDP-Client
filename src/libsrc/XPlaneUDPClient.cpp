/*
 * XPlaneUDPClient.cpp
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

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <iostream>
#include <thread>
#include <stdexcept>
#include <regex>
#include <string>

#include <stdexcept>
#include "XPlaneUDPClient.h"
#include "XPUtils.h"

using namespace std;

XPlaneUDPClient::SubscribedDataRef::SubscribedDataRef(std::string _dataRef,
		uint32_t _en, int _minFreq, bool _emitable,
		SubscribedCharArray * _sca) {

	dataRef = _dataRef;
	en = _en;
	minFreq = _minFreq;

	// defaults
	emitable = _emitable;
	partOfCharArray = _sca;
	first = true;
	value = -12345;

	// if part of a char array, add the en to the list
	if (_sca != NULL) {
		_sca->addToEnList(_en);
	}

	ts = 0;
	value = 0;

	// extract array index out of dataRef if there is one.
	regex r("^.*\\[(\\d+)\\]$");
	smatch matches;

	if (regex_match(_dataRef, matches, r)) {
		arrIndex = stoi(matches[1]);
	}

}

std::string XPlaneUDPClient::SubscribedDataRef::getDataRefName() {

	return dataRef;
}

uint32_t XPlaneUDPClient::SubscribedDataRef::getEn() {
	return en;
}

uint32_t XPlaneUDPClient::SubscribedDataRef::getFreq() {
	return minFreq;
}

void XPlaneUDPClient::SubscribedDataRef::setFreq(uint32_t _minFreq) {
	minFreq = _minFreq;
}

// =============================================================================

XPlaneUDPClient::XPlaneUDPClient(std::string _server, uint16_t _port,
		std::function<void(std::string, float)> _receiverCallbackFloat,
		std::function<void(std::string, std::string)> _receiverCallbackString) {

	server = _server;
	port = _port;
	receiverCallbackFloat = _receiverCallbackFloat;
	receiverCallbackString = _receiverCallbackString;
	lastUnsubscribeCheck = 0L;
	quitFlag = false;
	isRunning = false;
	debug = 0;

	lastEn = 0;

	// setup socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		throw runtime_error("Unable to create UDP socket");
	}

	struct sockaddr_in srcaddr;
	memset((char *) &srcaddr, 0, sizeof(srcaddr));
	srcaddr.sin_family = AF_INET;
	srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srcaddr.sin_port = htons(0);

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *) &tv,
			sizeof(struct timeval))) {
		perror("setsockopt: rcvtimeo");
		exit(1);
	}

	// bind
	if (bind(sock, (struct sockaddr *) &srcaddr, sizeof(srcaddr)) < 0) {
		throw runtime_error("Unable to bind socket");
	}

	// setup dest address
	memset((void *) &serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(server.c_str());
	serverAddr.sin_port = htons(port);
	slen = sizeof(serverAddr);

	// launch listener thread
	std::thread t(&XPlaneUDPClient::listenerThread, this);
	t.detach();
}

XPlaneUDPClient::~XPlaneUDPClient() {

	quitFlag = true;

	time_t nowTime = time(NULL);

	while (isRunning && time(NULL) < nowTime + 5) {
		if (debug) {
			cerr << "waiting for XPlaneUDPClient to stop" << endl;
		}
		usleep (100000); // 10ms
	}

	if (isRunning) {
		cerr << "... XPlaneUDPClient failed to stop within 5 seconds." << endl;
	} else {

		// unsubscribe from everything
		dataRefMutex.lock();
		for (auto dataRef : dataRefs) {
			char buf[413];
			memset(buf, 0, sizeof(buf));

			uint32_t zero = 0;
			uint32_t en = dataRef->getEn();
			strcpy(buf, "RREF");
			memcpy(buf + 5, &zero, 4);
			memcpy(buf + 9, &en, 4);

			sendto(sock, (void *) buf, sizeof(buf), 0,
					(struct sockaddr *) &serverAddr, slen);
		}

		dataRefs.clear();
		dataRefByNameIndex.clear();
		dataRefByEnIndex.clear();
		dataRefMutex.unlock();
	}

	close(sock);
}

void XPlaneUDPClient::listenerThread() {

	if (debug) {
		cerr << "In XPlaneUDPClient listenerThread" << endl;
	}

	uint8_t buf[1024];
	struct sockaddr remoteAddr;
	socklen_t slen = sizeof(remoteAddr);
	int recv_len;
	isRunning = true;
	while (!quitFlag) {

		// this will timeout after 1 second if nothing is received, due to
		// the socket option set earlier.

		if ((recv_len = recvfrom(sock, buf, sizeof(buf), 0,
				(struct sockaddr *) &remoteAddr, &slen)) < 0) {

			if (errno != EWOULDBLOCK) {
				ostringstream buf;
				buf << "recvfrom returned " << recv_len << " errno is " << errno
						<< endl;
				throw runtime_error(buf.str());
			};

		};

		time_t nowTime = time(NULL);

		// check for RREF

		if (recv_len >= 4 && buf[0] == 'R' && buf[1] == 'R' && buf[2] == 'E'
				&& buf[3] == 'F') {

			// 9, 10, 11, 12 = value
			// 13, 14, 15, 16 = en

			// cerr << nowTime << " Received RREF of " << recv_len << " bytes"
			// 		<< endl;

			for (int idx = 5; idx < recv_len; idx += 8) {

				// get value
				float value = xflt2float(&(buf[idx + 4]));

				// get en
				uint32_t en = xint2uint32(&(buf[idx]));

				// cerr << " idx is " << idx << " en is " << en << " value is "
				//		<< value << endl;

				// is it something we subscribed to?
				auto sdri = dataRefByEnIndex.find(en);
				if (sdri == dataRefByEnIndex.end()) {

					// not subscribed ... let's get rid of it.
					char buf[413];
					memset(buf, 0, sizeof(buf));

					uint32_t zero = 0;
					strcpy(buf, "RREF");
					memcpy(buf + 5, &zero, 4);
					memcpy(buf + 9, &en, 4);

					sendto(sock, (void *) buf, sizeof(buf), 0,
							(struct sockaddr *) &serverAddr, slen);

					if (debug) {
						cerr << "Unsubscribing from non-subscribed dataref en="
								<< en << endl;
					}

				} else {
					// subscribed
					SubscribedDataRef * sdr = (*sdri).second;

					// is it emitable?
					if (sdr->getEmitable()) {

						// is it part of a char array?
						SubscribedCharArray * sca =
								sdr->getSubscribedCharArray();
						if (sca == NULL) {

							// no, so just emit the data as a float
							float oldValue = sdr->getValue();
							sdr->setValue(nowTime, value);

							if (sdr->getFirst() == true || oldValue != value) {
								receiverCallbackFloat(sdr->getDataRefName(),
										value);
								sdr->setFirst(false);

							}

						} else {

							// a bit more work if it is a char array. Iterate through
							// each of the elements of the char array's datarefs,
							// appending the values as chars to a string.

							// save the value and timestamp for this char
							sdr->setValue(nowTime, value);

							ostringstream stringValue;
							int i = sca->getRangeFrom();
							char c;
							do {
								uint32_t subEn = sca->getEn(
										i - sca->getRangeFrom());
								SubscribedDataRef * subSdr =
										dataRefByEnIndex.find(subEn)->second;
								c = (char) subSdr->getValue();
								if (c != 0) {
									stringValue << c;
								}
								i++;
							} while ((i <= sca->getRangeTo()) && (c != '\0'));

							// emit as a string only if first time or value has changed.
							if (sca->getFirst() == true
									|| sca->getValue() != stringValue.str()) {
								receiverCallbackString(sca->getDataRefName(),
										stringValue.str());
								sca->setFirst(false);
								sca->setValue(stringValue.str());
							}

						}
					} else {
						// not emitable, just save it so we update the ts.
						sdr->setValue(nowTime, value);
					}
				}
			}
		};

		// else check for other messages here. Be sure to check recv_len first as
		// we can also receive -1 if socket timeouts.

		// Check for unsubscribed RREFs. These are where we have datarefs that we think
		// are subscribed, but have not received any data. If nothing has been received
		// in more than 5 seconds, we should resend the subscribe message.

		if (nowTime > lastUnsubscribeCheck) {
			for (auto dataRef : dataRefs) {

				// cerr << "CHECK lastUpdate=" << dataRef->getLastUpdate()
				//		<< " and nowTime-1 " << nowTime - 1 << " for en "
				//		<< dataRef->getEn() << endl;

				if (dataRef->getLastUpdate() < nowTime - 2) {

					// not received in last 2 seconds. Send a subscribe request again.
					uint32_t dref_freq = dataRef->getFreq();
					uint32_t dref_en = dataRef->getEn();

					char buf[413];
					memset(buf, 0, sizeof(buf));

					strcpy(buf, "RREF");
					memcpy(buf + 5, &dref_freq, 4);
					memcpy(buf + 9, &dref_en, 4);
					memcpy(buf + 13, dataRef->getDataRefName().c_str(),
							dataRef->getDataRefName().length() + 1);

					sendto(sock, (void *) buf, sizeof(buf), 0,
							(struct sockaddr *) &serverAddr, slen);

					if (1 || debug) {
						cerr << "Sent subscribe datagram RREF for freq="
								<< dref_freq << ", en=" << dref_en << ", name:"
								<< dataRef->getDataRefName().c_str() << " last:"
								<< dataRef->getLastUpdate() << " now:" << nowTime << endl;
					}
				}
			}
			lastUnsubscribeCheck = nowTime;
		}

	}
	if (debug) {
		cerr << "Done with XPlaneUDPClient listenerThread" << endl;
	}
	isRunning = false;
}

void XPlaneUDPClient::subscribeIndividualDataRef(std::string dataRef,
		uint32_t minFreq, bool _emitable, SubscribedCharArray * _sca) {

	// does the subscription already exist?
	bool needToSubscribe = false;
	SubscribedDataRef * sdr;
	uint32_t en;
	auto sdrI = dataRefByNameIndex.find(dataRef);
	if (sdrI != dataRefByNameIndex.end()) {
		ostringstream buf;
		buf << "Dataref \"" << dataRef << "\" is already subscribed." << endl;
	};

	// we need to subscribe to it. First figure out what "en" to use by checking
	// dataRefByEnIndex for next unused value. This can go really bad if you've
	// subscribed to over 2^32 datarefs.

	while (dataRefByEnIndex.find(lastEn) != dataRefByEnIndex.end()) {
		lastEn++;
	}
	en = lastEn++;

	// create the subscribedDataRef record
	sdr = new XPlaneUDPClient::SubscribedDataRef(dataRef, en, minFreq,
			_emitable, _sca);
	dataRefs.push_back(sdr);

	// update indexes
	dataRefByEnIndex.insert(pair<uint32_t, SubscribedDataRef *>(en, sdr));

	needToSubscribe = true;

	// send a UDP to subscribe to dataref. Here we assume that if we re-subscribe
	// to a dataref, it will update with new specs (like freq).

	if (false && needToSubscribe) {

		uint32_t dref_freq = sdr->getFreq();
		uint32_t dref_en = en;

		char buf[413];
		memset(buf, 0, sizeof(buf));

		strcpy(buf, "RREF");
		memcpy(buf + 5, &dref_freq, 4);
		memcpy(buf + 9, &dref_en, 4);
		memcpy(buf + 13, dataRef.c_str(), dataRef.length() + 1);

		ssize_t res = sendto(sock, (void *) buf, sizeof(buf), 0,
				(struct sockaddr *) &serverAddr, slen);

		if (debug) {
			cerr << "Sent subscribe datagram RREF for freq=" << dref_freq
					<< ", en=" << dref_en << ", name:" << dataRef
					<< " emitable=" << _emitable << ", returned [" << res << "]"
					<< endl;
		}
	}

}

void XPlaneUDPClient::unsubscribeDataRef(std::string dataRefName) {

	dataRefMutex.lock();

	auto sdrI = dataRefByNameIndex.find(dataRefName);
	if (sdrI != dataRefByNameIndex.end()) {

		SubscribedDataRef * sdr = sdrI->second;

		// Remove it from our dataref indexes.

		for (auto it = dataRefByNameIndex.cbegin();
				it != dataRefByNameIndex.cend();) {
			if (it->first == dataRefName) {
				it = dataRefByNameIndex.erase(it);
			} else {
				++it;
			}
		}

		for (auto it = dataRefByEnIndex.cbegin(); it != dataRefByEnIndex.cend();
				) {
			if (it->second->getDataRefName() == dataRefName) {
				it = dataRefByEnIndex.erase(it);
			} else {
				++it;
			}
		}

		// Remove it our list of subscribed datarefs.

		dataRefs.remove(sdr);

		// don't worry about unsubscribing it from X-Plane. It will automatically
		// be unsubscribed the next time we receive it as it is not in our
		// index.

	};

	dataRefMutex.unlock();

}

void XPlaneUDPClient::subscribeDataRef(std::string dataRefName,
		uint32_t minFreq) {

// lock the subscribedDataRefs
	dataRefMutex.lock();

// check to see if the dataRefName has a double subscript.
	regex r("^(.*)\\[(\\d+)\\]\\[(\\d+)\\]$");
	smatch matches;

	if (regex_match(dataRefName, matches, r)) {

		// it is a char array

		string baseDataRefName = matches[1];
		int rangeFrom = stoi(matches[2]);
		int rangeTo = stoi(matches[3]);

		if (debug) {
			cerr << "Requesting charArray [" << baseDataRefName << "] from ["
					<< rangeFrom << "] to [" << rangeTo << "]" << endl;
		}

		// create the subscribedCharArray record
		SubscribedCharArray * sca = new SubscribedCharArray(dataRefName,
				rangeFrom, rangeTo);

		// subscribed to each of the individual elements
		for (int idx = rangeFrom; idx <= rangeTo; idx++) {

			ostringstream drfn;
			drfn << baseDataRefName << "[" << idx << "]";

			// link each subscription to the sca, and only emit on the last one.
			subscribeIndividualDataRef(drfn.str(), minFreq, idx == rangeTo,
					sca);

		}

	} else {

		// just a regular dataref
		subscribeIndividualDataRef(dataRefName, minFreq);
	}

// release mutex
	dataRefMutex.unlock();
}

void XPlaneUDPClient::sendCommand(std::string cmd) {

	char buf[cmd.length() + 6];
	memset(buf, 0, sizeof(buf));

	strcpy(buf, "CMND");
	strcpy(buf + 5, cmd.c_str());

	sendto(sock, (void *) buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr,
			slen);

	if (debug) {
		cerr << "Sent datagram CMND with payload \"" << cmd << "\"" << endl;
	}

}

void XPlaneUDPClient::setDataRef(std::string dataRef, float value) {

	char buf[5 + 4 + 500];
	strcpy(buf, "DREF");
	memcpy(buf + 5, &value, 4);
	memset(buf + 9, ' ', sizeof(buf) - 9);
	strcpy(buf + 9, dataRef.c_str());

	sendto(sock, (void *) buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr,
			slen);

	if (debug) {
		cerr << "Sent datagram DREF with value " << value << " dref \""
				<< dataRef << "\"" << endl;
	}

}

void XPlaneUDPClient::setDataRefString(std::string dataRef, std::string value) {

	// extract array index out of dataRef if there is one.
	regex r("^(.*)\\[(\\d+)\\]\\[(\\d+)\\]$");
	smatch matches;

	if (regex_match(dataRef, matches, r)) {

		string baseDataRef = matches[1];
		int startIdx = stoi(matches[2]);
		int stopIdx = stoi(matches[3]);

		for (int i = startIdx; i < stopIdx; i++) {

			unsigned int idx = i - startIdx;
			float data;

			if (idx < value.length()) {
				data = value.at(idx);
			} else {
				data = ' ';
			}

			ostringstream dref;
			dref << baseDataRef << "[" << i << "]";

			char buf[5 + 4 + 500];
			strcpy(buf, "DREF");
			memcpy(buf + 5, &data, 4);
			memset(buf + 9, ' ', sizeof(buf) - 9);
			strcpy(buf + 9, dref.str().c_str());

			sendto(sock, (void *) buf, sizeof(buf), 0,
					(struct sockaddr *) &serverAddr, slen);

			if (debug) {
				cerr << "Sent datagram DREF with value \"" << data
						<< "\" dref \"" << dref.str() << "\"" << endl;
			}

		}

	} else {
		ostringstream buf;
		buf
				<< "In XPlaneUDPClient::setDataRefString (dataRef, value), dataRef is \""
				<< dataRef << "\", expecting \"^.*\\[(\\d+)\\]$\"";
		throw runtime_error(buf.str());
	}
}
