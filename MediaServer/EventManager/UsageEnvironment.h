#ifndef RTSPSERVER_USAGEENVIRONMENT_H
#define RTSPSERVER_USAGEENVIRONMENT_H

#include "ThreadPool.h"
#include "EventScheduler.h"

class UsageEnvironment
{
public:
    static UsageEnvironment* createNew(EventScheduler* scheduler, ThreadPool* threadPool);

    UsageEnvironment(EventScheduler* scheduler, ThreadPool* threadPool);
    ~UsageEnvironment();

    EventScheduler* scheduler();
    ThreadPool* threadPool();

private:
    EventScheduler* mScheduler;
    ThreadPool* mThreadPool;
};

#endif //RTSPSERVER_USAGEENVIRONMENT_H