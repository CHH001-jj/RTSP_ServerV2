#ifndef RTSPSERVER_RTSPSERVER_H
#define RTSPSERVER_RTSPSERVER_H
#include <mutex>
#include "../EventManager/UsageEnvironment.h"
#include "../EventManager/Event.h"
#include "MediaSession.h"
#include "InetAddress.h"
class MediaSessionManager;
class RtspConnection;
class RtspServer {

public:
    static RtspServer* createNew(UsageEnvironment* env,MediaSessionManager * sessMgr,Ipv4Address& addr);

    RtspServer(UsageEnvironment* env, MediaSessionManager* sessMgr,Ipv4Address& addr);
    ~RtspServer();

public:
    MediaSessionManager* mSessMgr;
    void start();
    UsageEnvironment* env() const {
        return mEnv;
    }
private:
    static void readCallback(void*);
    void handleRead();
    static void cbDisConnect(void* arg, int clientFd);
    void handleDisConnect(int clientFd);
    static void cbCloseConnect(void* arg);
    void handleCloseConnect();

private:
    
    UsageEnvironment* mEnv;
    int  mFd;
    Ipv4Address mAddr;
    bool mListen;
    IOEvent* mAcceptIOEvent;
    std::mutex mMtx;

    std::map<int, RtspConnection*> mConnMap; // <clientFd,conn> 维护所有被创建的连接
    std::vector<int> mDisConnList;//所有被取消的连接 clientFd
    TriggerEvent* mCloseTriggerEvent;// 关闭连接的触发事件

};
#endif //RTSPSERVER_RTSPSERVER_H
