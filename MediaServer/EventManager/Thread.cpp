#include "Thread.h"

Thread::Thread() :
    mArg(nullptr),
    mIsStart(false),
    mIsDetach(false)
{

}
Thread::~Thread()
{
    if(mIsStart == true && mIsDetach == false)
        detach();
}
    
bool Thread::start(void *arg)
{
    mArg = arg;

    mThreadId = std::thread(&Thread::threadRun, this);

    //if (pthread_create(&mThreadId, NULL, threadRun, this)) {
    //    return false;
    //}
        
    mIsStart = true;
    return true;
}

bool Thread::detach()
{
    if(mIsStart != true)
        return false;

    if(mIsDetach == true)
        return true;

    mThreadId.detach();
    //if(pthread_detach(mThreadId))
    //    return false;

    mIsDetach = true;

    return true;
}

bool Thread::join()
{
    if(mIsStart != true || mIsDetach == true)
        return false;

    mThreadId.join();
    //if(pthread_join(mThreadId, NULL))
    //    return false;
    
    return true;
}


void *Thread::threadRun(void *arg)
{
    Thread* thread = (Thread*)arg;
    thread->run(thread->mArg);
    return NULL;
}
