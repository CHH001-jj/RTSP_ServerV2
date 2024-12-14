#include "RtspConnection.h"
#include "RtspServer.h"
#include <stdio.h>
#include <string.h>
#include "Rtp.h"
#include "MediaSessionManager.h"
#include "MediaSession.h"
#include "../Utils/Log.h"
#include "../Utils/Common.h"
#include <iostream>

static void getPeerIp(int fd, std::string& ip)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    getpeername(fd, (struct sockaddr*)&addr, &addrlen);
    ip = inet_ntoa(addr.sin_addr);
}

RtspConnection* RtspConnection::createNew(RtspServer* rtspServer, int clientFd)
{
    return new RtspConnection(rtspServer, clientFd);
    //    return New<RtspConnection>::allocate(rtspServer, clientFd);
}

RtspConnection::RtspConnection(RtspServer* rtspServer, int clientFd) :
        TcpConnection(rtspServer->env(), clientFd),
        mRtspServer(rtspServer),
        mMethod(RtspConnection::Method::NONE),
        mTrackId(MediaSession::TrackId::TrackIdNone),
        mSessionId(rand()),
        mIsRtpOverTcp(false),
    mStreamPrefix("track")
{
    LOGI("RtspConnection() mClientFd=%d", mClientFd);

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        mRtpInstances[i] = NULL;
        mRtcpInstances[i] = NULL;
    }
    getPeerIp(clientFd, mPeerIp);

}

RtspConnection::~RtspConnection()
{
    LOGI("~RtspConnection() mClientFd=%d", mClientFd);
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if (mRtpInstances[i])
        {

            MediaSession* session = mRtspServer->mSessMgr->getSession(mSuffix);

            if (!session) {
                session->removeRtpInstance(mRtpInstances[i]);
            }
            delete mRtpInstances[i];
        }

        if (mRtcpInstances[i])
        {
            delete mRtcpInstances[i];
        }
    }

}

void RtspConnection::handleReadBytes(){
    if (mIsRtpOverTcp)
    {
        if (mInputBuffer.peek()[0] == '$')
        {
            handleRtpOverTcp();
            return;
        }
    }
    if (!parseRequest())
    {
        LOGE("parseRequest err");
        goto disConnect;
    }
    switch (mMethod)
    {
        case OPTIONS:
            if (!handleCmdOption())
                goto disConnect;
            break;
        case DESCRIBE:
            if (!handleCmdDescribe())
                goto disConnect;
            break;
        case SETUP:
            if (!handleCmdSetup())
                goto disConnect;
            break;
        case PLAY:
            if (!handleCmdPlay())
                goto disConnect;
            break;
        case TEARDOWN:
            if (!handleCmdTeardown())
                goto disConnect;
            break;

        default:
            goto disConnect;
    }

    return;
disConnect:
    handleDisConnect();
}

bool RtspConnection::parseRequest()
{
    //解析第一行
    const char* crlf = mInputBuffer.findCRLF();
    if (crlf == NULL){
        mInputBuffer.retrieveAll();
        return false;
    }
    bool ret = parseRequest1(mInputBuffer.peek(), crlf);
    if (ret == false){
        mInputBuffer.retrieveAll();
        return false;
    }else {
        mInputBuffer.retrieveUntil(crlf + 2);
    }

    //解析第一行之后的所有行
    crlf = mInputBuffer.findLastCrlf();
    if (crlf == NULL)
    {
        mInputBuffer.retrieveAll();
        return false;
    }
    ret = parseRequest2(mInputBuffer.peek(), crlf);

    if (ret == false)
    {
        mInputBuffer.retrieveAll();
        return false;
    }else{
        mInputBuffer.retrieveUntil(crlf + 2);
        return true;
    }
}

bool RtspConnection::parseRequest1(const char* begin, const char* end)
{
    std::string message(begin, end);
    char method[64] = { 0 };
    char url[512] = { 0 };
    char version[64] = { 0 };

    if (sscanf(message.c_str(), "%s %s %s", method, url, version) != 3)
    {
        return false;
    }

    if (!strcmp(method, "OPTIONS")) {
        mMethod = OPTIONS;
    }else if (!strcmp(method, "DESCRIBE")) {
        mMethod = DESCRIBE;
    }else if (!strcmp(method, "SETUP")) {
        mMethod = SETUP;
    }else if (!strcmp(method, "PLAY")) {
        mMethod = PLAY;
    }else if (!strcmp(method, "TEARDOWN")) {
        mMethod = TEARDOWN;
    }else{
        mMethod = NONE;
        return false;
    }
    if (strncmp(url, "rtsp://", 7) != 0)
    {
        return false;
    }

    uint16_t port = 0;
    char ip[64] = { 0 };
    char suffix[64] = { 0 };

    if (sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3)
    {

    }
    else if (sscanf(url + 7, "%[^/]/%s", ip, suffix) == 2)
    {
        port = 554;// 如果rtsp请求地址中无端口，默认获取的端口为：554
    }
    else
    {
        return false;
    }

    mUrl = url;
    mSuffix = suffix;

    return true;
}

bool RtspConnection::parseRequest2(const char* begin, const char* end)
{
    std::string message(begin, end);

    if (!parseCSeq(message)) {
        return false;
    }
    if (mMethod == OPTIONS) {
        return true;
    }
    else if (mMethod == DESCRIBE) {
        return parseDescribe(message);
    }
    else if (mMethod == SETUP)
    {
        return parseSetup(message);
    }
    else if (mMethod == PLAY) {
        return parsePlay(message);
    }
    else if (mMethod == TEARDOWN) {
        return true;
    }
    else {
        return false;
    }

}

bool RtspConnection::parseCSeq(std::string& message)
{
    std::size_t pos = message.find("CSeq");
    if (pos != std::string::npos)
    {
        uint32_t cseq = 0;
        sscanf(message.c_str() + pos, "%*[^:]: %u", &cseq);
        mCSeq = cseq;
        return true;
    }

    return false;
}

bool RtspConnection::parseDescribe(std::string& message)
{
    if ((message.rfind("Accept") == std::string::npos)
        || (message.rfind("sdp") == std::string::npos))
    {
        return false;
    }

    return true;
}

bool RtspConnection::parseSetup(std::string& message)
{
    mTrackId = MediaSession::TrackIdNone;
    std::size_t pos;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        pos = mUrl.find(mStreamPrefix + std::to_string(i));
        if (pos != std::string::npos)
        {
            if (i == 0) {
                mTrackId = MediaSession::TrackId0;
            }
            else if(i == 1)
            {
                mTrackId = MediaSession::TrackId1;
            }
            
        }

    }

    if (mTrackId == MediaSession::TrackIdNone) {
        return false;
    }

    pos = message.find("Transport");
    if (pos != std::string::npos)
    {
        if ((pos = message.find("RTP/AVP/TCP")) != std::string::npos)
        {
            uint8_t rtpChannel, rtcpChannel;
            mIsRtpOverTcp = true;

            if (sscanf(message.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hhu-%hhu",
                       &rtpChannel, &rtcpChannel) != 2)
            {
                return false;
            }

            mRtpChannel = rtpChannel;

            return true;
        }
        else if ((pos = message.find("RTP/AVP")) != std::string::npos)
        {
            uint16_t rtpPort = 0, rtcpPort = 0;
            if (((message.find("unicast", pos)) != std::string::npos))
            {
                if (sscanf(message.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu",
                           &rtpPort, &rtcpPort) != 2)
                {
                    return false;
                }
            }
            else if ((message.find("multicast", pos)) != std::string::npos)
            {
                return true;
            }
            else
                return false;

            mPeerRtpPort = rtpPort;
            mPeerRtcpPort = rtcpPort;
        }
        else
        {
            return false;
        }

        return true;
    }

    return false;
}


bool RtspConnection::parsePlay(std::string& message)
{
    std::size_t pos = message.find("Session");
    if (pos != std::string::npos)
    {
        uint32_t sessionId = 0;
        if (sscanf(message.c_str() + pos, "%*[^:]: %u", &sessionId) != 1)
            return false;
        return true;
    }

    return false;
}

bool RtspConnection::handleCmdOption()
{

    snprintf(mBuffer, sizeof(mBuffer),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %u\r\n"
             "Public:OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n"
             "Server: %s\r\n"
             "\r\n", mCSeq,PROJECT_VERSION);

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0)
        return false;

    return true;
}

bool RtspConnection::handleCmdDescribe()
{
    MediaSession* session = mRtspServer->mSessMgr->getSession(mSuffix);

    if (!session) {
        LOGE("can't find session:%s", mSuffix.c_str());
        return false;
    }
    std::string sdp = session->generateSDPDescription();

    memset((void*)mBuffer, 0, sizeof(mBuffer));
    snprintf((char*)mBuffer, sizeof(mBuffer),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %u\r\n"
             "Content-Length: %u\r\n"
             "Content-Type: application/sdp\r\n"
             "\r\n"
             "%s",
             mCSeq,
             (unsigned int)sdp.size(),
             sdp.c_str());

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0)
        return false;
    return true;
}


bool RtspConnection::handleCmdSetup(){
    char sessionName[100];
    if (sscanf(mSuffix.c_str(), "%[^/]/", sessionName) != 1)
    {
        return false;
    }
    MediaSession* session = mRtspServer->mSessMgr->getSession(sessionName);
    if (!session){
        LOGE("can't find session:%s",sessionName);
        return false;
    }

    if (mTrackId >= MEDIA_MAX_TRACK_NUM || mRtpInstances[mTrackId] || mRtcpInstances[mTrackId]) {
        return false;
    }

    if (session->isStartMulticast()) {
        snprintf((char*)mBuffer, sizeof(mBuffer),
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %d\r\n"
                 "Transport: RTP/AVP;multicast;"
                 "destination=%s;source=%s;port=%d-%d;ttl=255\r\n"
                 "Session: %08x\r\n"
                 "\r\n",
                 mCSeq,
                 session->getMulticastDestAddr().c_str(),
                 sockets::getLocalIp().c_str(),
                 session->getMulticastDestRtpPort(mTrackId),
                 session->getMulticastDestRtpPort(mTrackId) + 1,
                 mSessionId);
    }
    else {
        if (mIsRtpOverTcp)
        {
            //创建rtp over tcp
            createRtpOverTcp(mTrackId, mClientFd, mRtpChannel);
            mRtpInstances[mTrackId]->setSessionId(mSessionId);

            session->addRtpInstance(mTrackId, mRtpInstances[mTrackId]);

            snprintf((char*)mBuffer, sizeof(mBuffer),
                     "RTSP/1.0 200 OK\r\n"
                     "CSeq: %d\r\n"
                     "Server: %s\r\n"
                     "Transport: RTP/AVP/TCP;unicast;interleaved=%hhu-%hhu\r\n"
                     "Session: %08x\r\n"
                     "\r\n",
                     mCSeq,PROJECT_VERSION,
                     mRtpChannel,
                     mRtpChannel + 1,
                     mSessionId);
        }
        else 
        {
            //创建 rtp over udp
            if (createRtpRtcpOverUdp(mTrackId, mPeerIp, mPeerRtpPort, mPeerRtcpPort) != true)
            {
                LOGE("failed to createRtpRtcpOverUdp");
                return false;
            }

            mRtpInstances[mTrackId]->setSessionId(mSessionId);
            mRtcpInstances[mTrackId]->setSessionId(mSessionId);

           
            session->addRtpInstance(mTrackId, mRtpInstances[mTrackId]);

            snprintf((char*)mBuffer, sizeof(mBuffer),
                     "RTSP/1.0 200 OK\r\n"
                     "CSeq: %u\r\n"
                     "Server: %s\r\n"
                     "Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                     "Session: %08x\r\n"
                     "\r\n",
                     mCSeq,PROJECT_VERSION,
                     mPeerRtpPort,
                     mPeerRtcpPort,
                     mRtpInstances[mTrackId]->getLocalPort(),
                     mRtcpInstances[mTrackId]->getLocalPort(),
                     mSessionId);
        }

    }

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0)
        return false;

    return true;
}

bool RtspConnection::handleCmdPlay()
{
    snprintf((char*)mBuffer, sizeof(mBuffer),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %d\r\n"
             "Server: %s\r\n"
             "Range: npt=0.000-\r\n"
             "Session: %08x; timeout=60\r\n"
             "\r\n",
             mCSeq, PROJECT_VERSION,
             mSessionId);

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0)
        return false;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if (mRtpInstances[i]) {
            mRtpInstances[i]->setAlive(true);
        }
         
        if (mRtcpInstances[i]) {
            mRtcpInstances[i]->setAlive(true);
        }
    
    }

    return true;
}

bool RtspConnection::handleCmdTeardown()
{
    snprintf((char*)mBuffer, sizeof(mBuffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Server: %s\r\n"
        "\r\n",
        mCSeq, PROJECT_VERSION);

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0)
    {
        return false;
    }

    return true;
}

int RtspConnection::sendMessage(void* buf, int size)
{
    LOGI("%s", buf);
    int ret;

    mOutBuffer.append(buf, size);
    ret = mOutBuffer.write(mClientFd);
    mOutBuffer.retrieveAll();

    return ret;
}

int RtspConnection::sendMessage()
{
    int ret = mOutBuffer.write(mClientFd);
    mOutBuffer.retrieveAll();
    return ret;
}

bool RtspConnection::createRtpRtcpOverUdp(MediaSession::TrackId trackId, std::string peerIp,
                                          uint16_t peerRtpPort, uint16_t peerRtcpPort)
{
    int rtpSockfd, rtcpSockfd;
    int16_t rtpPort, rtcpPort;
    bool ret;

    if (mRtpInstances[trackId] || mRtcpInstances[trackId])
        return false;

    int i;
    for (i = 0; i < 10; ++i){// 重试10次
        rtpSockfd = sockets::createUdpSock();
        if (rtpSockfd < 0)
        {
            return false;
        }

        rtcpSockfd = sockets::createUdpSock();
        if (rtcpSockfd < 0)
        {
            sockets::close(rtpSockfd);
            return false;
        }

        uint16_t port = rand() & 0xfffe;
        if (port < 10000)
            port += 10000;

        rtpPort = port;
        rtcpPort = port + 1;

        ret = sockets::bind(rtpSockfd, "0.0.0.0", rtpPort);
        if (ret != true)
        {
            sockets::close(rtpSockfd);
            sockets::close(rtcpSockfd);
            continue;
        }

        ret = sockets::bind(rtcpSockfd, "0.0.0.0", rtcpPort);
        if (ret != true)
        {
            sockets::close(rtpSockfd);
            sockets::close(rtcpSockfd);
            continue;
        }

        break;
    }

    if (i == 10)
        return false;

    mRtpInstances[trackId] = RtpInstance::createNewOverUdp(rtpSockfd, rtpPort,
                                                           peerIp, peerRtpPort);
    mRtcpInstances[trackId] = RtcpInstance::createNew(rtcpSockfd, rtcpPort,
                                                      peerIp, peerRtcpPort);

    return true;
}

bool RtspConnection::createRtpOverTcp(MediaSession::TrackId trackId, int sockfd,
                                      uint8_t rtpChannel)
{
    mRtpInstances[trackId] = RtpInstance::createNewOverTcp(sockfd, rtpChannel);

    return true;
}

void RtspConnection::handleRtpOverTcp()
{
    int num = 0;
    while (true)
    {
        num += 1;
        uint8_t* buf = (uint8_t*)mInputBuffer.peek();
        uint8_t rtpChannel = buf[1];
        int16_t rtpSize = (buf[2] << 8) | buf[3];

        int16_t bufSize = 4 + rtpSize;

        if (mInputBuffer.readableBytes() < bufSize) {
            // 缓存数据小于一个RTP数据包的长度
            return;
        }else {
            if (0x00 == rtpChannel) {
                RtpHeader rtpHeader;
                parseRtpHeader(buf + 4, &rtpHeader);
                LOGI("num=%d,rtpSize=%d", num, rtpSize);
            }
            else if(0x01 == rtpChannel)
            {
                RtcpHeader rtcpHeader;
                parseRtcpHeader(buf + 4, &rtcpHeader);

                LOGI("num=%d,rtcpHeader.packetType=%d,rtpSize=%d", num, rtcpHeader.packetType, rtpSize);
            }
            else if (0x02 == rtpChannel) {
                RtpHeader rtpHeader;
                parseRtpHeader(buf + 4, &rtpHeader);
                LOGI("num=%d,rtpSize=%d", num, rtpSize);
            }
            else if (0x03 == rtpChannel)
            {
                RtcpHeader rtcpHeader;
                parseRtcpHeader(buf + 4, &rtcpHeader);

                LOGI("num=%d,rtcpHeader.packetType=%d,rtpSize=%d", num, rtcpHeader.packetType, rtpSize);
            }

            mInputBuffer.retrieve(bufSize);
        }
    }

}