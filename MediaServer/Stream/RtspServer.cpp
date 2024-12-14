#include "RtspServer.h"
#include "RtspConnection.h"
#include "../Utils/Log.h"

RtspServer* RtspServer::createNew(UsageEnvironment* env, MediaSessionManager* sessMgr, Ipv4Address& addr) {

    return new RtspServer(env, sessMgr,addr);
}
RtspServer::RtspServer(UsageEnvironment* env, MediaSessionManager* sessMgr, Ipv4Address& addr) :
        mSessMgr(sessMgr),
        mEnv(env),
        mAddr(addr),
        mListen(false)
{

    mFd = sockets::createTcpSock();
    sockets::setReuseAddr(mFd, 1);
    if (!sockets::bind(mFd, addr.getIp(), mAddr.getPort())) {
        return;
    }

    LOGI("rtsp://%s:%d fd=%d",addr.getIp().data(),addr.getPort(), mFd);
    mAcceptIOEvent = IOEvent::createNew(mFd, this);
    mAcceptIOEvent->setReadCallback(readCallback);//设置回调的socket可读 函数指针
    mAcceptIOEvent->enableReadHandling();

    mCloseTriggerEvent = TriggerEvent::createNew(this);
    mCloseTriggerEvent->setTriggerCallback(cbCloseConnect);//设置回调的关闭连接 函数指针

}

RtspServer::~RtspServer()
{
    if (mListen)
        mEnv->scheduler()->removeIOEvent(mAcceptIOEvent);

    delete mAcceptIOEvent;
    delete mCloseTriggerEvent;

    sockets::close(mFd);
}



void RtspServer::start(){
    LOGI("");
    mListen = true;
    sockets::listen(mFd, 60);
    mEnv->scheduler()->addIOEvent(mAcceptIOEvent);
}

void RtspServer::readCallback(void* arg) {
    RtspServer* rtspServer = (RtspServer*)arg;
    rtspServer->handleRead();
}

void RtspServer::handleRead() {
    int clientFd = sockets::accept(mFd);
    if (clientFd < 0)
    {
        LOGE("handleRead error,clientFd=%d",clientFd);
        return;
    }
    RtspConnection* conn = RtspConnection::createNew(this, clientFd);
    conn->setDisConnectCallback(RtspServer::cbDisConnect, this);
    mConnMap.insert(std::make_pair(clientFd, conn));

}
void RtspServer::cbDisConnect(void* arg, int clientFd) {
    RtspServer* server = (RtspServer*)arg;

    server->handleDisConnect(clientFd);
}

void RtspServer::handleDisConnect(int clientFd) {
 
    std::lock_guard <std::mutex> lck(mMtx);
    mDisConnList.push_back(clientFd);

    mEnv->scheduler()->addTriggerEvent(mCloseTriggerEvent);

}

void RtspServer::cbCloseConnect(void* arg) {
    RtspServer* server = (RtspServer*)arg;
    server->handleCloseConnect();
}
void RtspServer::handleCloseConnect() {

    std::lock_guard <std::mutex> lck(mMtx);

    for (std::vector<int>::iterator it = mDisConnList.begin(); it != mDisConnList.end(); ++it) {

        int clientFd = *it;
        std::map<int, RtspConnection*>::iterator _it = mConnMap.find(clientFd);
        assert(_it != mConnMap.end());
        delete _it->second;
        mConnMap.erase(clientFd);
    }

    mDisConnList.clear();
}