#ifndef RTSPSERVER_SOCKETSOPS_H
#define RTSPSERVER_SOCKETSOPS_H
#include <string>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif // !WIN32

namespace sockets
{
    int createTcpSock();//默认创建非阻塞的tcp描述符
    int createUdpSock();//默认创建非阻塞的udp描述符
    bool bind(int sockfd, std::string ip, uint16_t port);
    bool listen(int sockfd, int backlog);
    int accept(int sockfd);
    // 通常是向描述符写数据
    int write(int sockfd, const void* buf, int size);// tcp 写入
    int sendto(int sockfd, const void* buf, int len, const struct sockaddr *destAddr); // udp 写入
    int setNonBlock(int sockfd);// 设置非阻塞模式
    int setBlock(int sockfd, int writeTimeout); // 设置阻塞模式
    void setReuseAddr(int sockfd, int on);
    void setReusePort(int sockfd);
    void setNonBlockAndCloseOnExec(int sockfd);
    void ignoreSigPipeOnSocket(int socketfd);
    void setNoDelay(int sockfd);
    void setKeepAlive(int sockfd);
    void setNoSigpipe(int sockfd);
    void setSendBufSize(int sockfd, int size);
    void setRecvBufSize(int sockfd, int size);
    std::string getPeerIp(int sockfd);
    int16_t getPeerPort(int sockfd);
    int getPeerAddr(int sockfd, struct sockaddr_in *addr);
    void close(int sockfd);
    bool connect(int sockfd, std::string ip, uint16_t port, int timeout);
    std::string getLocalIp();
}

#endif //RTSPSERVER_SOCKETSOPS_H