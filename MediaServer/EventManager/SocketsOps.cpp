#include "SocketsOps.h"
#include <fcntl.h>
#include <sys/types.h>          /* See NOTES */
#ifndef WIN32
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif // !WIN32
#include "../Utils/Log.h"
int sockets::createTcpSock()
{

#ifndef WIN32
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
 
#else
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    unsigned long ul = 1;
    int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);

    if (ret == SOCKET_ERROR) {
        LOGE("设置非阻塞失败");
    }
#endif

    return sockfd;
}

int sockets::createUdpSock()
{

#ifndef WIN32
    int sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
#else
    int sockfd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    unsigned long ul = 1;
    int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);

    if (ret == SOCKET_ERROR) {
        LOGE("设置非阻塞失败");
    }
#endif

    return sockfd;

}

bool sockets::bind(int sockfd, std::string ip, uint16_t port)
{

    //SOCKADDR_IN server_addr;
    //server_addr.sin_family = AF_INET;
    //server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    ////server_addr.sin_addr.s_addr = inet_addr("192.168.2.61");
    //server_addr.sin_port = htons(port);

    ////s_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //if (::bind(sockfd, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
    //   
    //    return false;
    //}
    //else {
    //    return true;
    //}

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    //    addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOGE("::bind error,fd=%d,ip=%s,port=%d", sockfd, ip.c_str(), port);
        return false;
    }
    return true;
}

bool sockets::listen(int sockfd, int backlog)
{
    if (::listen(sockfd, backlog) < 0) {
        LOGE("::listen error,fd=%d,backlog=%d", sockfd, backlog);
        return false;
    }
    return true;
}

int sockets::accept(int sockfd)
{
    struct sockaddr_in addr = { 0 };
    socklen_t addrlen = sizeof(struct sockaddr_in);

    int connfd = ::accept(sockfd, (struct sockaddr*)&addr, &addrlen);
    setNonBlockAndCloseOnExec(connfd);
    ignoreSigPipeOnSocket(connfd);

    return connfd;
}


int sockets::write(int sockfd, const void* buf, int size)
{

#ifndef WIN32
    return ::write(sockfd, buf, size);
#else
    return ::send(sockfd, (char*)buf, size, 0);
#endif


}

int sockets::sendto(int sockfd, const void* buf, int len,
    const struct sockaddr* destAddr)
{
    socklen_t addrLen = sizeof(struct sockaddr);
    return ::sendto(sockfd, (char*)buf, len, 0, destAddr, addrLen);
}

int sockets::setNonBlock(int sockfd)
{
#ifndef WIN32
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    return 0;
#else

    unsigned long ul = 1;
    int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);//设置非阻塞

    if (ret == SOCKET_ERROR) {
        return -1;
    }
    else {
        return 0;
    }
#endif
}

int sockets::setBlock(int sockfd, int writeTimeout)
{
#ifndef WIN32
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));

    if (writeTimeout > 0)
    {
        struct timeval tv = { writeTimeout / 1000, (writeTimeout % 1000) * 1000 };
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
    }
#endif

    return 0;
}

void sockets::setReuseAddr(int sockfd, int on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
}

void sockets::setReusePort(int sockfd)
{
#ifdef SO_REUSEPORT
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&on, sizeof(on));
#endif
}

void sockets::setNonBlockAndCloseOnExec(int sockfd)
{
#ifndef WIN32
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
#endif

}

void sockets::ignoreSigPipeOnSocket(int socketfd)
{
#ifndef WIN32
    int option = 1;
    setsockopt(socketfd, SOL_SOCKET, MSG_NOSIGNAL, &option, sizeof(option));
#endif
}

void sockets::setNoDelay(int sockfd)
{
#ifdef TCP_NODELAY
    int on = 1;
    int ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
#endif
}

void sockets::setKeepAlive(int sockfd)
{
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));
}

void sockets::setNoSigpipe(int sockfd)
{
#ifdef SO_NOSIGPIPE
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (char*)&on, sizeof(on));
#endif
}

void sockets::setSendBufSize(int sockfd, int size)
{
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size));
}

void sockets::setRecvBufSize(int sockfd, int size)
{
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));
}

std::string sockets::getPeerIp(int sockfd)
{
    struct sockaddr_in addr = { 0 };
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr*)&addr, &addrlen) == 0)
    {
        return inet_ntoa(addr.sin_addr);
    }

    return "0.0.0.0";
}

int16_t sockets::getPeerPort(int sockfd)
{
    struct sockaddr_in addr = { 0 };
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr*)&addr, &addrlen) == 0)
    {
        return ntohs(addr.sin_port);
    }

    return 0;
}

int sockets::getPeerAddr(int sockfd, struct sockaddr_in* addr)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    return getpeername(sockfd, (struct sockaddr*)addr, &addrlen);
}

void sockets::close(int sockfd)
{
#ifndef WIN32
    int ret = ::close(sockfd);
#else
    int ret = ::closesocket(sockfd);
#endif

}

bool sockets::connect(int sockfd, std::string ip, uint16_t port, int timeout)
{
    bool isConnected = true;
    if (timeout > 0)
    {
        sockets::setNonBlock(sockfd);
    }

    struct sockaddr_in addr = { 0 };
    socklen_t addrlen = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (::connect(sockfd, (struct sockaddr*)&addr, addrlen) < 0)
    {
        if (timeout > 0)
        {
            isConnected = false;
            fd_set fdWrite;
            FD_ZERO(&fdWrite);
            FD_SET(sockfd, &fdWrite);
            struct timeval tv = { timeout / 1000, timeout % 1000 * 1000 };
            select(sockfd + 1, NULL, &fdWrite, NULL, &tv);
            if (FD_ISSET(sockfd, &fdWrite))
            {
                isConnected = true;
            }
            sockets::setBlock(sockfd, 0);
        }
        else
        {
            isConnected = false;
        }
    }

    return isConnected;
}

std::string sockets::getLocalIp()
{
    return "0.0.0.0";
}