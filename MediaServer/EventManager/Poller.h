#ifndef RTSPSERVER_POLLER_H
#define RTSPSERVER_POLLER_H
#include <map>
#include "Event.h"

class Poller
{
public:
    virtual ~Poller();

    virtual bool addIOEvent(IOEvent* event) = 0;
    virtual bool updateIOEvent(IOEvent* event) = 0;
    virtual bool removeIOEvent(IOEvent* event) = 0;
    virtual void handleEvent() = 0;


protected:
    Poller();

protected:
    typedef std::map<int, IOEvent*> IOEventMap;
    IOEventMap mEventMap;

};

#endif //RTSPSERVER_POLLER_H