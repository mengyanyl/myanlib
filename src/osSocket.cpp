/*******************************************************************************
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

#include "osSocket.h"
#include "logger.h"

namespace myan
{
namespace utils
{

osSocket::osSocket():_init(false), _fd(0),_timeout(0)
{
    memset(&_sockAddress, 0, sizeof(sockaddr_in));
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _addressLen = sizeof(sockaddr_in);
}

osSocket::osSocket(unsigned int port, int timeout):_init(false), _fd(0),_timeout(timeout)
{
    memset(&_sockAddress, 0, sizeof(sockaddr_in));
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _peerAddressLen = sizeof(_peerAddress);
    _sockAddress.sin_family = AF_INET;
    _sockAddress.sin_addr.s_addr = htonl( INADDR_ANY );
    _sockAddress.sin_port = htons( port );
    _addressLen = sizeof(sockaddr_in);
}

osSocket::osSocket ( SOCKET *sock, int timeout )
{
   int rc = EDB_OK ;
   _fd = *sock ;
   _init = true ;
   _timeout = timeout ;
   _addressLen = sizeof( _sockAddress ) ;
   memset ( &_peerAddress, 0, sizeof(sockaddr_in) ) ;
   _peerAddressLen = sizeof(_peerAddress) ;
   rc = getsockname ( _fd, (sockaddr*)&_sockAddress, &_addressLen ) ;
   if ( rc )
   {
      Logger::getLogger().error("Failed to get sock name, error = %d",
               SOCKET_GETLASTERROR) ;
      _init = false ;
   }
   else
   {
      rc = getpeername ( _fd, (sockaddr*)&_peerAddress, &_peerAddressLen ) ;
      if (rc!=EDB_OK)
        Logger::getLogger().error("Failed to get peer name, error = %d",
                    SOCKET_GETLASTERROR ) ;
   }
}

void osSocket::setSocket ( SOCKET *sock, int timeout )
{
   int rc = EDB_OK ;
   _fd = *sock ;
   _init = true ;
   _timeout = timeout ;
   _addressLen = sizeof( _sockAddress ) ;
   memset ( &_peerAddress, 0, sizeof(sockaddr_in) ) ;
   _peerAddressLen = sizeof(_peerAddress) ;
   rc = getsockname ( _fd, (sockaddr*)&_sockAddress, &_addressLen ) ;
   if ( rc )
   {
      Logger::getLogger().error("Failed to get sock name, error = %d",
               SOCKET_GETLASTERROR) ;
      _init = false ;
   }
   else
   {
      rc = getpeername ( _fd, (sockaddr*)&_peerAddress, &_peerAddressLen ) ;
      if (rc!=EDB_OK)
        Logger::getLogger().error("Failed to get peer name, error = %d",
                    SOCKET_GETLASTERROR ) ;
   }
}

int osSocket::initSocket()
{
    int rc = EDB_OK;
    if (_init)
    {
        return EDB_OK;
    }
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _peerAddressLen = sizeof(_peerAddress);
    _fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( -1 == _fd )
    {
        rc = EDB_NETWORK;
        //todo log error
    }
    _init = true;
    setTimeOut(_timeout);
    return rc;
}

void osSocket::setAddress(const char* pHostName, unsigned int port)
{
    struct hostent *hp;
    memset(&_sockAddress, 0, sizeof(sockaddr_in));
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _peerAddressLen = sizeof(sockaddr_in);
    _sockAddress.sin_family = AF_INET;
    if ( ( hp = gethostbyname( pHostName ) ) )
        _sockAddress.sin_addr.s_addr =  *((int*)hp->h_addr_list[0]);
    else
        _sockAddress.sin_addr.s_addr = inet_addr( pHostName );
    _sockAddress.sin_port = htons(port);
    _addressLen = sizeof(_sockAddress);
}

int osSocket::bindListen()
{
    int rc = EDB_OK;
    int temp = 1;
    rc = setsockopt( _fd, SOL_SOCKET, SO_REUSEADDR, (char*)&temp, sizeof(int) );
    if (rc)
    {
        this->close();
        //todo error log
    }
    rc = ::bind( _fd, (struct sockaddr*)&_sockAddress, _addressLen);
    if (rc)
    {
        this->close();
        // todo error log
    }
    rc = listen( _fd, SOMAXCONN );
    if (rc)
    {
        this->close();
        // todo error log
    }
    return rc;
}

int osSocket::send(const char* pMsg, int len, int timeout, int flags)
{
    int rc = EDB_OK;
    SOCKET maxFD = _fd;

    struct timeval maxSelectTime;
    fd_set fds;
    maxSelectTime.tv_sec = timeout / 1000000;
    maxSelectTime.tv_usec = timeout % 1000000;

    if ( 0==len )
        return EDB_OK;

    while(true)
    {
        FD_ZERO( &fds );
        FD_SET ( _fd, &fds );
        rc = select ( (int)(maxFD + 1), NULL, &fds, NULL, timeout>=0?&maxSelectTime:NULL);
        if ( 0==rc )
        {
            return EDB_TIMEOUT;
        }

        if ( 0>rc )
        {
            rc = SOCKET_GETLASTERROR;
            if ( OSS_EINTR==rc )
            {
                continue;
            }
            //todo error log
        }

        if ( FD_ISSET(_fd, &fds) )
        {
            break;
        }
    }

    while (len>0)
    {
        rc = ::send(_fd, pMsg, len, MSG_NOSIGNAL | flags);
        if (-1 == rc)
        {
            Logger::getLogger().error("failed to send, errorCode=%d", SOCKET_GETLASTERROR);
            return -1;
        }
        len -= rc;
        pMsg += rc;
    }
    return rc;
}

int osSocket::recv(char *pMsg, int len, int timeout, int flags)
{
    int rc = EDB_OK;
    int retries = 0;
    SOCKET maxFD = _fd;
    struct timeval maxSelectTime;
    fd_set fds;

    if (0==len)
    {
        return EDB_OK;
    }
    maxSelectTime.tv_sec = timeout/100000;
    maxSelectTime.tv_usec = timeout%100000;
    for (;;)
    {
        FD_ZERO(&fds);
        FD_SET( _fd, &fds );
        rc = select( maxFD+1, &fds, NULL, NULL, timeout>=0?&maxSelectTime:NULL );

        if (rc==0)
        {
            rc = EDB_TIMEOUT;
            return rc;
        }
        if ( 0>rc )
        {
            rc = SOCKET_GETLASTERROR;
            if (rc==OSS_EINTR) continue;
            Logger::getLogger().error("failed to select, errorCode=%d", rc);
            return rc;
        }
        if ( FD_SET(_fd, &fds) )
        {
            break;
        }
    }

    while (len>0)
    {
        rc = ::recv(_fd, pMsg, len, MSG_NOSIGNAL | flags);
        if ( rc > 0 )
        {
            if (flags & MSG_PEEK)
            {
                return EDB_OK;
            }
            len -= rc;
            pMsg += rc;
        }
        else if ( rc == 0)
        {
            Logger::getLogger().warn("peer unexcepted shutdown");
            this->close();
            break;
        }
        else
        {
            rc = SOCKET_GETLASTERROR;
            if ( ( OSS_EAGAIN == rc || EWOULDBLOCK == rc ) &&
                    _timeout > 0 )
            {
                Logger::getLogger().error("recv timeout errorCode=%d", rc);
            }
            if ( ( OSS_EINTR == rc ) && ( retries < 5) ) //#definde MAX_RECV_RETRIES 5
            {
                retries++;
                continue;
            }
            else
            {
                break;
            }
            Logger::getLogger().error("failed to recv, errorCode=%d", rc);
        }
    }
    return rc;
}

int osSocket::connect()
{
    int rc = EDB_OK;
    rc = ::connect( _fd, (struct sockaddr *)&_sockAddress, _addressLen);
    if (rc)
    {
        Logger::getLogger().error("failed to connect, errorCode=%d", rc);
    }
    rc = getsockname( _fd, (sockaddr*)&_sockAddress, &_addressLen);
    if (rc)
    {
        Logger::getLogger().error("failed to get localaddress, errorCode=%d", rc);
    }
    rc = getpeername( _fd, (sockaddr*)&_peerAddress, &_peerAddressLen);
    if (rc)
    {
        Logger::getLogger().error("failed to get peer address, errorCode=%d", rc);
    }
    return rc;
}

int osSocket::close()
{
    int rc = EDB_OK;

    if (_init)
    {
        rc = ::closesocket(_fd);
        if (rc < 0)
        {
            Logger::getLogger().error("socket close error, errorCode=%d", rc);
        }
        _init = false;
    }
    return rc;
}

bool osSocket::isConnected ()
{
   int rc = EDB_OK ;
   rc = ::send ( _fd, "", 0, MSG_NOSIGNAL ) ;
   if ( 0 > rc )
      return false ;
   return true ;
}

int osSocket::accept( SOCKET *sock, struct sockaddr *addr, socklen_t *addrlen, int timeout )
{
    int rc = EDB_OK;
    int maxFD = _fd;
    struct timeval maxSelectTime;
    fd_set fds;
    maxSelectTime.tv_sec = timeout/1000000;
    maxSelectTime.tv_usec = timeout%1000000;

    while (true)
    {
        FD_ZERO(&fds);
        FD_SET( _fd, &fds );
        rc = ::select( (maxFD + 1), &fds, NULL, NULL, timeout>=0?&maxSelectTime:NULL);

        if ( 0==rc )
        {
            return EDB_TIMEOUT;
        }
        else if ( 0>rc )
        {
            rc = SOCKET_GETLASTERROR;
            if ( rc==OSS_EINTR )
            {
                continue;
            }
            Logger::getLogger().error("socket accept error, errorCode=%d", rc);
            return rc;
        }

        if ( FD_ISSET( _fd, &fds ) )
        {
            break;
        }
    }

    rc = EDB_OK;
    *sock = ::accept( _fd, addr, addrlen);
    if ( -1==*sock )
    {
        Logger::getLogger().error("failed to accept aocket error, errorCode=%d", rc);
    }

    return rc;
}

int osSocket::setTimeOut(int timeout)
{
    int rc = EDB_OK;
    struct timeval tv;
    tv.tv_sec = timeout / 1000000;
    tv.tv_usec = timeout % 1000000;

    rc = setsockopt( _fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv) );

    if (rc)
    {
        Logger::getLogger().error("failed to set recv timeout, errorCode=%d", SOCKET_GETLASTERROR);
        return rc;
    }

    rc = setsockopt( _fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv) );
    if (rc)
    {
        Logger::getLogger().error("failed to set send timeout, errorCode=%d", SOCKET_GETLASTERROR);
        return rc;
    }

    return rc;
}

int osSocket::disableNagle ()
{
   int rc = EDB_OK ;
   int temp = 1 ;
   rc = setsockopt ( _fd, IPPROTO_TCP, TCP_NODELAY, (char *) &temp,
                     sizeof ( int ) ) ;
   if ( rc )
   {
      Logger::getLogger().error ( "Failed to setsockopt, rc = %d", SOCKET_GETLASTERROR ) ;
   }

   rc = setsockopt ( _fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &temp,
                     sizeof ( int ) ) ;
   if ( rc )
   {
      Logger::getLogger().error ( "Failed to setsockopt, rc = %d", SOCKET_GETLASTERROR ) ;
   }
   return rc ;
}

int osSocket::setAnsyn()
{
    return fcntl( _fd, F_SETFL, O_NONBLOCK | fcntl(_fd, F_GETFL, 0) );
}

unsigned int osSocket::_getPort(sockaddr_in *pAddr)
{
    return ntohs( pAddr->sin_port );
}

int osSocket::_getAddress(sockaddr_in *addr, char *pAddress, unsigned int len)
{
    len = len < OSS_MAX_HOSTNAME ? len : OSS_MAX_HOSTNAME;
    int rc = getnameinfo( (struct sockaddr *)addr, sizeof(sockaddr), pAddress,
                          len , NULL, 0, OSS_NUMERICHOST);
    if (rc)
    {
        Logger::getLogger().error("failed to getname, errorCode=%d", rc);
        return rc;
    }
    return EDB_OK;
}

unsigned int osSocket::getLocalPort()
{
    return this->_getPort(&_sockAddress);
}

unsigned int osSocket::getPeerPort()
{
    return this->_getPort(&_peerAddress);
}

int osSocket::getLocalAddress(char *pAddress, unsigned int len)
{
    return this->_getAddress( &_sockAddress, pAddress, len);
}

int osSocket::getPeerAddress(char *pAddress, unsigned int len)
{
    return this->_getAddress( &_peerAddress, pAddress, len);
}

int osSocket::getHostName(char *pName, int len)
{
    return gethostname( pName, len);
}

int osSocket::getPort( const char *pServName, unsigned short &port)
{
    struct servent *servinfo;
    servinfo = getservbyname( pServName, "tcp" );
    if ( !servinfo )
    {
        port = atoi( pServName );
    }
    else
    {
        port = (unsigned short)ntohs(servinfo->s_port);
    }
    return EDB_OK;
}

}
}

