#include "Timer.h"
#ifndef WIN32
#include <sys/timerfd.h>
#endif // !WIN32
#include <time.h>
#include <chrono>
#include "Event.h"
#include "EventScheduler.h"
#include "Poller.h"
#include "../Utils/Log.h"



//struct timespec {
//    time_t tv_sec; //Seconds
//    long   tv_nsec;// Nanoseconds
//};
//struct itimerspec {
//    struct timespec it_interval;  //Interval for periodic timer （定时间隔周期）
//    struct timespec it_value;     //Initial expiration (第一次超时时间)
//};
//    it_interval不为0 表示是周期性定时器
//    it_value和it_interval都为0 表示停止定时器


static bool timerFdSetTime(int fd, Timer::Timestamp when, Timer::TimeInterval period) {

#ifndef WIN32
    struct itimerspec newVal;

    newVal.it_value.tv_sec = when / 1000; //ms->s
    newVal.it_value.tv_nsec = when % 1000 * 1000 * 1000; //ms->ns
    newVal.it_interval.tv_sec = period / 1000;
    newVal.it_interval.tv_nsec = period % 1000 * 1000 * 1000;

    int oldValue = timerfd_settime(fd, TFD_TIMER_ABSTIME, &newVal, NULL);
    if (oldValue < 0) {
        return false;
    }
    return true;
#endif // !WIN32

    return true;
}


Timer::Timer(TimerEvent* event, Timestamp timestamp, TimeInterval timeInterval, TimerId timerId) :
        mTimerEvent(event),
        mTimestamp(timestamp),
        mTimeInterval(timeInterval),
        mTimerId(timerId){
    if (timeInterval > 0){
        mRepeat = true;// 循环定时器
    }else{
        mRepeat = false;//一次性定时器
    }
}

Timer::~Timer()
{

}
// 获取系统从启动到目前的毫秒数
Timer::Timestamp Timer::getCurTime(){
#ifndef WIN32
    // Linux系统
    struct timespec now;// tv_sec (s) tv_nsec (ns-纳秒)
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec*1000 + now.tv_nsec/1000000);
#else
    long long now = std::chrono::steady_clock::now().time_since_epoch().count();
    return now / 1000000;
#endif // !WIN32

}
Timer::Timestamp Timer::getCurTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).
            count();

}
bool Timer::handleEvent()
{
    if (!mTimerEvent) {
        return false;
    }
    return  mTimerEvent->handleEvent();

}

TimerManager* TimerManager::createNew(EventScheduler* scheduler){

    if(!scheduler)
        return NULL;
    return new TimerManager(scheduler);
}

TimerManager::TimerManager(EventScheduler* scheduler) :
        mPoller(scheduler->poller()),
        mLastTimerId(0){

#ifndef WIN32
    mTimerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (mTimerFd < 0) {
        LOGE("create TimerFd error");
        return;
    }else{
        LOGI("fd=%d",mTimerFd);
    }
    mTimerIOEvent = IOEvent::createNew(mTimerFd, this);
    mTimerIOEvent->setReadCallback(readCallback);
    mTimerIOEvent->enableReadHandling();
    modifyTimeout();
    mPoller->addIOEvent(mTimerIOEvent);
#else
    scheduler->setTimerManagerReadCallback(readCallback, this);

#endif // !WIN32


}

TimerManager::~TimerManager()
{
#ifndef WIN32
    mPoller->removeIOEvent(mTimerIOEvent);
    delete mTimerIOEvent;
#endif // !WIN32

}

Timer::TimerId TimerManager::addTimer(TimerEvent* event, Timer::Timestamp timestamp,
                                      Timer::TimeInterval timeInterval)
{
    ++mLastTimerId;
    Timer timer(event, timestamp, timeInterval,mLastTimerId);


    mTimers.insert(std::make_pair(mLastTimerId, timer));
    //mEvents.insert(std::make_pair(TimerIndex(timestamp, mLastTimerId), timer));
    mEvents.insert(std::make_pair(timestamp, timer));
    modifyTimeout();

    return mLastTimerId;
}

bool TimerManager::removeTimer(Timer::TimerId timerId)
{
    std::map<Timer::TimerId, Timer>::iterator it = mTimers.find(timerId);
    if(it != mTimers.end())
    {
        mTimers.erase(timerId);
        // TODO 还需要删除mEvents的事件
    }

    modifyTimeout();

    return true;
}

void TimerManager::modifyTimeout()
{
#ifndef WIN32
    std::multimap<Timer::Timestamp, Timer>::iterator it = mEvents.begin();
    if (it != mEvents.end()) {// 存在至少一个定时器
        Timer timer = it->second;
        timerFdSetTime(mTimerFd, timer.mTimestamp, timer.mTimeInterval);
    }
    else {
        timerFdSetTime(mTimerFd, 0, 0);
    }
#endif // WIN32

}

void TimerManager::readCallback(void *arg) {
    TimerManager* timerManager = (TimerManager*)arg;
    timerManager->handleRead();
}

void TimerManager::handleRead() {

    //LOGI("mTimers.size()=%d,mEvents.size()=%d",mTimers.size(),mEvents.size());
    Timer::Timestamp timestamp = Timer::getCurTime();
    if (!mTimers.empty() && !mEvents.empty()) {

        std::multimap<Timer::Timestamp, Timer>::iterator it = mEvents.begin();
        Timer timer = it->second;
        int expire = timer.mTimestamp - timestamp;

        //LOGI("timestamp=%d,mTimestamp=%d,expire=%d,timeInterval=%d", timestamp, timer.mTimestamp, expire, timer.mTimeInterval);

        if (timestamp > timer.mTimestamp || expire == 0) {

            bool timerEventIsStop = timer.handleEvent();
            mEvents.erase(it);
            if (timer.mRepeat) {
                if (timerEventIsStop) {
                    mTimers.erase(timer.mTimerId);
                }else {
                    timer.mTimestamp = timestamp + timer.mTimeInterval;
                    mEvents.insert(std::make_pair(timer.mTimestamp, timer));
                }
            }
            else {
                mTimers.erase(timer.mTimerId);
            }

        }
    }
    modifyTimeout();
}
