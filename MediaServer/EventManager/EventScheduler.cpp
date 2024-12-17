#include "EventScheduler.h"
#include "SocketsOps.h"
#include "SelectPoller.h"
#include "PollPoller.h"
#include "EPollPoller.h"
#include "../Utils/Log.h"

#ifndef WIN32
#include <sys/eventfd.h>
#endif // !WIN32

EventScheduler* EventScheduler::createNew(PollerType type)
{
    if (type != POLLER_SELECT && type != POLLER_POLL && type != POLLER_EPOLL)
        return NULL;

    return new EventScheduler(type);
}

EventScheduler::EventScheduler(PollerType type) : mQuit(false) {

#ifdef WIN32
    WSADATA wdSockMsg;//这是一个结构体
    int s = WSAStartup(MAKEWORD(2, 2), &wdSockMsg);//打开一个套接字

    if (0 != s)
    {
        switch (s)
        {
        case WSASYSNOTREADY: {
            LOGE("重启电脑，或者检查网络库");
            break;
        }
        case WSAVERNOTSUPPORTED: {
            LOGE("请更新网络库");
            break;
        }
        case WSAEINPROGRESS: {
            LOGE("请重新启动");
            break;
        }
        case WSAEPROCLIM: {
            LOGE("请关闭不必要的软件，以确保有足够的网络资源"); break;
            }
        }
    }

    if (2 != HIBYTE(wdSockMsg.wVersion) || 2 != LOBYTE(wdSockMsg.wVersion))
    {
        LOGE("网络库版本错误");
        return;
    }
#endif // WIN32


    switch (type) {

        case POLLER_SELECT:
            mPoller = SelectPoller::createNew();
            break;

        case POLLER_POLL:
            mPoller = PollPoller::createNew();
            break;

        case POLLER_EPOLL:
            mPoller = EpollPoller::createNew();
            break;

        default:
            _exit(-1);
            break;
    }
    mTimerManager = TimerManager::createNew(this);//WIN系统的定时器回调由子线程托管，非WIN系统则通过select网络模型

}

EventScheduler::~EventScheduler()
{

    delete mTimerManager;
    delete mPoller;

#ifdef WIN32
    WSACleanup();
#endif // WIN32

}

bool EventScheduler::addTriggerEvent(TriggerEvent* event)
{
    mTriggerEvents.push_back(event);

    return true;
}

Timer::TimerId EventScheduler::addTimedEventRunAfater(TimerEvent* event, Timer::TimeInterval delay)
{//添加一个定时事件，事件会在指定的延迟后执行。
    Timer::Timestamp timestamp = Timer::getCurTime();
    timestamp += delay;

    return mTimerManager->addTimer(event, timestamp, 0);
}

Timer::TimerId EventScheduler::addTimedEventRunAt(TimerEvent* event, Timer::Timestamp when)
{//添加一个定时事件，指定事件在某个具体时间点执行。
    return mTimerManager->addTimer(event, when, 0);
}

Timer::TimerId EventScheduler::addTimedEventRunEvery(TimerEvent* event, Timer::TimeInterval interval)
{//添加一个定时重复事件，指定事件以固定时间间隔
    Timer::Timestamp timestamp = Timer::getCurTime();
    timestamp += interval;
    return mTimerManager->addTimer(event, timestamp, interval);
}

bool EventScheduler::removeTimedEvent(Timer::TimerId timerId)
{
    return mTimerManager->removeTimer(timerId);
}

bool EventScheduler::addIOEvent(IOEvent* event)
{
    return mPoller->addIOEvent(event);
}

bool EventScheduler::updateIOEvent(IOEvent* event)
{
    return mPoller->updateIOEvent(event);
}

bool EventScheduler::removeIOEvent(IOEvent* event)
{
    return mPoller->removeIOEvent(event);
}

void EventScheduler::loop() {

#ifdef WIN32
    std::thread([](EventScheduler* sch) {
        while (!sch->mQuit) {
            if (sch->mTimerManagerReadCallback) {
                sch->mTimerManagerReadCallback(sch->mTimerManagerArg);
            }
        }
        }, this).detach();
#endif // WIN32

    while (!mQuit) {
        handleTriggerEvents();
        mPoller->handleEvent();
    }
}

void EventScheduler::handleTriggerEvents()
{
    if (!mTriggerEvents.empty())
    {
        for (std::vector<TriggerEvent*>::iterator it = mTriggerEvents.begin();
             it != mTriggerEvents.end(); ++it)
        {
            (*it)->handleEvent();
        }

        mTriggerEvents.clear();
    }
}

Poller* EventScheduler::poller() {
    return mPoller;
}
void EventScheduler::setTimerManagerReadCallback(EventCallback cb, void* arg)
{
    mTimerManagerReadCallback = cb;
    mTimerManagerArg = arg;
}
