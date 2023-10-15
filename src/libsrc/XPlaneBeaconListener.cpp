/*
 * XPlaneBeaconListener.cpp
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

#define _DEFAULT_SOURCE

#include <iostream>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#define ERRNO WSAGetLastError()
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
//typedef int socklen_t;
#define ERRNO errno
#define WSAETIMEDOUT 10060
#endif
#include <sstream>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#ifdef __linux__
#include <syslog.h>
#else
inline void syslog(int /*prio*/, const char */*fmt*/, ...) {}   // TODO Windows Syslog not supported
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */
#endif

#include "XPlaneBeaconListener.h"

using namespace std;

// initialize statics
XPlaneBeaconListener * XPlaneBeaconListener::instance = NULL;

XPlaneBeaconListener::XPlaneBeaconListener() {
    cout << __PRETTY_FUNCTION__ << endl;
    fflush(stdout);
    debug = 0;
    quitFlag = false;
    isRunning = false;
    worker_thread = new std::thread(&XPlaneBeaconListener::runListener, this);
}

XPlaneBeaconListener::~XPlaneBeaconListener()
{
    setDebug(true);
    quitFlag = true;
    time_t nowTime = time(NULL);
    cerr << "Waiting for XPlaneBeaconListener to stop: " << endl;
    fflush(stderr);

    if(worker_thread!=nullptr && worker_thread->joinable())
        worker_thread->join();
}

void XPlaneBeaconListener::runListener() {

    // https://web.cs.wpi.edu/~claypool/courses/4514-B99/samples/multicast.c
    // http://ntrg.cs.tcd.ie/undergrad/4ba2/multicast/antony/example.html
    cerr << "Start " << __PRETTY_FUNCTION__ << endl;
    fflush(stderr);
    struct sockaddr_in addr;
    socklen_t addrlen;
    int sock, recv_len;
    struct ip_mreq mreq;
    char message[1024];

    /* set up socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    memset((char *) &addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(49707);
    addrlen = sizeof(addr);

    /* allow multiple sockets to use the same ADDR */
#ifdef _WIN32
    char yes = 1;
#else
    u_int yes = 1;
#endif

    if(debug)
    {
        cerr << "1: " << __PRETTY_FUNCTION__ << endl;
        fflush(stderr);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        ostringstream buf;
        buf << "XPlaneBeaconListener: Reusing ADDR failed: " << strerror(ERRNO);
        throw (buf.str());
    }

    if(debug)
    {
        cerr << "2: " << __PRETTY_FUNCTION__ << endl;
        fflush(stderr);
    }
    // SO_REUSEPORT not needed on WIN32
#ifndef _WIN32
    /* allow multiple sockets to use the same ADDR */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0) {
        ostringstream buf;
        buf << "XPlaneBeaconListener: Reusing PORT failed: " << strerror(ERRNO);
        throw runtime_error(buf.str());
    }
#endif

    /* timeout after 1 second if no data received */
#ifdef _WIN32
    DWORD tv;
    tv = 10000;
    cerr << "timeout : " << tv << endl;
    fflush(stderr);
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,(char *) &tv,
                   sizeof(tv))) {
        ostringstream buf;
        buf << "XPlaneBeaconListener: set SO_RCVTIMEO failed: "
            << strerror(ERRNO);
        throw runtime_error(buf.str());
    }
#else
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *) &tv,
                   sizeof(struct timeval))) {
        ostringstream buf;
        buf << "XPlaneBeaconListener: set SO_RCVTIMEO failed: "
            << strerror(ERRNO);
        throw runtime_error(buf.str());
    }
#endif
    cerr << "3: " << __PRETTY_FUNCTION__ << endl;
    fflush(stderr);

    /* receive */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        ostringstream buf;
        buf << "XPlaneBeaconListener: bind failed. ERRNO: " << ERRNO << " : " << strerror(ERRNO);
        throw runtime_error(buf.str());
    }

    /* subscribe multicast
     *
     * Under systemd on raspbian multicast is sometimes is not available
     * even though "network-online.target" has been satisfied. This
     * retries for up to 5 seconds before giving up.
     */

    mreq.imr_multiaddr.s_addr = inet_addr("239.255.1.1");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    int result;
    int retry = 0;

    do {
        result = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*) &mreq, sizeof(mreq));
        if (result < 0) {
            retry++;
            syslog (LOG_INFO, "Retrying subscribe to multicast (attempt %d)", retry);
            cerr << "Retrying subscribe to multicast (attempt " << retry << ")" << endl;
            fflush(stderr);
            sleep (1);
        }
    } while (result < 0 && retry < 5);

    if (retry < 0) {
        ostringstream buf;
        buf << "XPlaneBeaconListner: setsockopt IP_ADD_MEMBERSHIP failed: "
            << strerror(ERRNO);
        cerr << "XPlaneBeaconListner: setsockopt IP_ADD_MEMBERSHIP failed: "
             << strerror(ERRNO) << " retry: " << retry << endl;
        fflush(stderr);
        throw runtime_error(buf.str());
    }


    time_t lastExpiredCheck = time(NULL);

    isRunning = true;
    while (!quitFlag)
    {
        if(debug)
        {
            cerr << "loop " << __PRETTY_FUNCTION__ << endl;
            fflush(stderr);
        }
        recv_len = recvfrom(sock, message, sizeof(message), 0,
                            (struct sockaddr *) &addr, &addrlen);
        time_t nowTime = time(NULL);

        if(debug)
        {
            cerr << "loop2 " << __PRETTY_FUNCTION__ << endl;
            fflush(stderr);
        }

        if (recv_len < 0)
        {
            if(debug)
            {
                cerr << "loop3 " << __PRETTY_FUNCTION__ << endl;
                fflush(stderr);
            }
            if(ERRNO == EINTR)
                continue;	//see http://250bpm.com/blog:12
            if (ERRNO != EWOULDBLOCK && ERRNO != EAGAIN && ERRNO != WSAETIMEDOUT)
            {
                if(debug)
                {
                    cerr << "loop3.1 " << ERRNO << __PRETTY_FUNCTION__ << endl;
                    cerr << "recvfrom returned " << recv_len << " ERRNO is " << ERRNO << endl;
                }

                fflush(stderr);
                ostringstream buf;
                buf << "recvfrom returned " << recv_len << " errno is " << ERRNO
                    << endl;
                throw runtime_error(buf.str());
            }
        }
        else if (recv_len == 0)
        {
            break;
        }
        else if (recv_len > 5 && memcmp(message, "BECN", 5) == 0)
        {
            if(debug)
            {
                cerr << "loop4 " << __PRETTY_FUNCTION__ << endl;
                fflush(stderr);
            }
            // parse the message

            XPlaneServer s(nowTime, message, inet_ntoa(addr.sin_addr));

            if(debug)
            {
                cerr << "loop5 " << __PRETTY_FUNCTION__ << endl;
                fflush(stderr);
            }
            cachedServersMutex.lock();
            ostringstream key;
            key << s.host << ":" << s.name << ":" << s.receivePort;

            if (cachedServers.find(key.str()) == cachedServers.end()) {
                for (auto callback : callbacks) {
                    callback(s, true);
                }
            }
            cachedServers[key.str()] = s;
            cachedServersMutex.unlock();
            if(debug)
            {
                cerr << "loop6 " << __PRETTY_FUNCTION__ << endl;
                fflush(stderr);
            }
        }

        // Check the list of servers we have for expired servers.
        if (lastExpiredCheck < nowTime)
        {
            if(debug)
            {
                cerr << "loop7 " << __PRETTY_FUNCTION__ << endl;
                fflush(stderr);
            }
            checkForExpiredServers();
            lastExpiredCheck = nowTime;
        }
    }

    // exit requested.
    if(debug)
    {
        cerr << "loop8 " << __PRETTY_FUNCTION__ << endl;
        fflush(stderr);
    }
    if(sock > 0)
    {
        cerr << "Shutdown sock: " << sock << endl;
        fflush(stderr);
        shutdown(sock,2);
    }
    isRunning = false;
    cerr << "End " << __PRETTY_FUNCTION__ << endl;
    fflush(stderr);
}

XPlaneBeaconListener::XPlaneServer::XPlaneServer(time_t time, char * msg,
                                                 char * _host) {

    prologue = msg;

    received = time;
    memcpy(&beaconMajorVersion, msg + 5, 1);
    memcpy(&beaconMinorVersion, msg + 6, 1);
    memcpy(&applicationHostId, msg + 7, 4);
    memcpy(&versionNumber, msg + 11, 4);
    memcpy(&role, msg + 15, 4);
    memcpy(&receivePort, msg + 19, 2);
    name = (char *) msg + 21;
    host = _host;

}

std::string XPlaneBeaconListener::XPlaneServer::toString() {

    ostringstream s;
    s << "XPlaneServer [ prologue: " << prologue << " beaconMajorVersion:"
      << (int) beaconMajorVersion << " beaconMinorVersion:"
      << (int) beaconMinorVersion << " applicationHostID:"
      << applicationHostId << " versionNumber:" << versionNumber
      << " role:" << role << " receivePort: " << receivePort << " name:"
      << name << " host:" << host << "]" << endl;
    return s.str();
}

void XPlaneBeaconListener::get(
        std::list<XPlaneBeaconListener::XPlaneServer> & ret) {

    cachedServersMutex.lock();
    ret.clear();
    for (auto server : cachedServers) {
        ret.push_back(server.second);
    }

    cachedServersMutex.unlock();

}

void XPlaneBeaconListener::registerNotificationCallback(
        std::function<void(XPlaneServer server, bool exists)> callback) {
    callbacks.push_back(callback);
}

void XPlaneBeaconListener::checkForExpiredServers()
{
    if(quitFlag)
    {
        return;
    }
    cachedServersMutex.lock();
    time_t nowTime = time(NULL);
    for (auto it = cachedServers.cbegin(); it != cachedServers.cend();) {
        // see if it has expired, i.e. we haven't received any beacons in last 30 seconds.
        if (it->second.received < nowTime - 30) {
            for (auto callback : callbacks) {
                callback(it->second, false);
            }
            cachedServers.erase(it++); // or "it = m.erase(it)" since C++11
        } else {
            ++it;
        }
    }
    cachedServersMutex.unlock();
}
