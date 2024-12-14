#ifndef RTSPSERVER_POLLPOLLER_H
#define RTSPSERVER_POLLPOLLER_H

#include "Poller.h"
#include <vector>
#include <poll.h>

class PollPoller : public Poller {
public:
    PollPoller();
    virtual ~PollPoller();

    static PollPoller* createNew();

    virtual bool addIOEvent(IOEvent* event);
    virtual bool updateIOEvent(IOEvent* event);
    virtual bool removeIOEvent(IOEvent* event);
    virtual void handleEvent();

private:
    std::vector<struct pollfd> mPollFds;      // pollfd 列表
    std::vector<IOEvent*> mIOEvents;          // IOEvent 对象列表
};

#endif //RTSPSERVER_POLLPOLLER_H
