/******************************************************************************
   Copyright (C) 2014 MyanDB Software Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.
*******************************************************************************/

#ifndef OSSOCKET_H
#define OSSOCKET_H

#include "core.h"


namespace myan
{
namespace utils
{

//10ms
#define SOCK_DFT_TIME_OUT 10000

// max hostname
#define OSS_MAX_HOSTNAME NI_MAXHOST
#define OSS_MAX_SERVICENAME NI_MAXSERV
#define OSS_NUMERICHOST NI_NUMERICHOST

#define SOCKET_GETLASTERROR errno
#define OSS_EAGAIN	EAGAIN
#define OSS_EINTR	EINTR
#define closesocket	close

typedef int SOCKET;

class osSocket
{
private:
    SOCKET _fd;
    socklen_t _addressLen;
    socklen_t _peerAddressLen;

    sockaddr_in _sockAddress;
    sockaddr_in _peerAddress;

    bool _init;

    int _timeout;

    unsigned int _getPort(sockaddr_in *pAddr);

    int _getAddress( sockaddr_in *addr, char *pAddress, unsigned int len);

public:
    osSocket();
    osSocket(unsigned int port, int timeout = 0);
    osSocket( SOCKET *sock, int timeout = 0 );

    virtual ~osSocket(){close();}

    int initSocket();

    void setSocket ( SOCKET *sock, int timeout=0 );

    void setAddress(const char* pHostName, unsigned int port);

    int connect();

    int close();

    int bindListen();

    int send(const char *pMsg, int len, int timeout=SOCK_DFT_TIME_OUT, int flags=0);

    int recv(char *pMsg, int len, int timeout=SOCK_DFT_TIME_OUT, int flags=0);

    int accept(SOCKET *sock, struct sockaddr *addr, socklen_t *addrlen,
               int timeout=SOCK_DFT_TIME_OUT);

    unsigned int getPeerPort();

    int getPeerAddress( char *pAddress, unsigned int length );

    unsigned int getLocalPort();

    int getLocalAddress( char *pAddress, unsigned int length );

    int setTimeOut( int seconds );

    int disableNagle ();

    bool isConnected ();

    int setAnsyn();

    static int getHostName( char *pName, int nameLen );

    static int getPort( const char *pServiceName, unsigned short &port );
};

}
}

#endif // OSSOCKET_H
