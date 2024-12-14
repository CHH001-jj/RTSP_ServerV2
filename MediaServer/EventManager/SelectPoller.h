#ifndef RTSPSERVER_SELECTPOLLER_H
#define RTSPSERVER_SELECTPOLLER_H
#include "Poller.h"
#include <vector>
#ifndef WIN32
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#else
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif // !WIN32

class SelectPoller : public Poller
{
public:
    SelectPoller();
    virtual ~SelectPoller();

    static SelectPoller* createNew();

    virtual bool addIOEvent(IOEvent* event);
    virtual bool updateIOEvent(IOEvent* event);
    virtual bool removeIOEvent(IOEvent* event);
    virtual void handleEvent();

private:
    fd_set mReadSet;
    fd_set mWriteSet;
    fd_set mExceptionSet;
    int mMaxNumSockets;
    std::vector<IOEvent*> mIOEvents;// 存储临时活跃的IO事件对象

};
#endif //RTSPSERVER_SELECTPOLLER_H