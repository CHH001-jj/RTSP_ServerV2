#ifndef RTSPSERVER_EPOLLPOLLER_H
#define RTSPSERVER_EPOLLPOLLER_H

#include "Poller.h"
#include <sys/epoll.h>
#include <vector>
#include <unordered_map>

class EpollPoller : public Poller {
public:
    EpollPoller();
    virtual ~EpollPoller();

    static EpollPoller* createNew();

    virtual bool addIOEvent(IOEvent* event) override;
    virtual bool updateIOEvent(IOEvent* event) override;
    virtual bool removeIOEvent(IOEvent* event) override;
    virtual void handleEvent() override;

private:
    int mEpollFd;                                      // epoll 文件描述符
    std::unordered_map<int, IOEvent*> mEventMap;      // 文件描述符到 IOEvent 的映射
    std::vector<struct epoll_event> mEventList;       // 存储活跃的 epoll 事件
};

#endif // RTSPSERVER_EPOLLPOLLER_H
