#ifndef RTSPSERVER_EVENTSCHEDULER_H
#define RTSPSERVER_EVENTSCHEDULER_H

#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>
#include "Timer.h"
#include "Event.h"
class Poller;

class EventScheduler
{
public:
    enum PollerType
    {
        POLLER_SELECT,
        POLLER_POLL,
        POLLER_EPOLL
    };
    static EventScheduler* createNew(PollerType type);

    explicit EventScheduler(PollerType type);
    virtual ~EventScheduler();
public:
    bool addTriggerEvent(TriggerEvent* event);
    Timer::TimerId addTimedEventRunAfater(TimerEvent* event, Timer::TimeInterval delay);
    Timer::TimerId addTimedEventRunAt(TimerEvent* event, Timer::Timestamp when);
    Timer::TimerId addTimedEventRunEvery(TimerEvent* event, Timer::TimeInterval interval);
    bool removeTimedEvent(Timer::TimerId timerId);
    bool addIOEvent(IOEvent* event);
    bool updateIOEvent(IOEvent* event);
    bool removeIOEvent(IOEvent* event);

    void loop();
    //    void wakeup();
    Poller* poller();
    void setTimerManagerReadCallback(EventCallback cb, void* arg);

private:
    void handleTriggerEvents();

private:
    bool mQuit;
    Poller* mPoller;
    TimerManager* mTimerManager;
    std::vector<TriggerEvent*> mTriggerEvents;
    std::mutex mMtx;

    // WIN系统专用的定时器回调start
    EventCallback mTimerManagerReadCallback;
    void* mTimerManagerArg;
    // WIN系统专用的定时器回调end
};

#endif //RTSPSERVER_EVENTSCHEDULER_H