#include "MediaSource.h"
#include "../Utils/Log.h"
MediaSource::MediaSource(UsageEnvironment* env) :
    mEnv(env),mFps(0)
{

    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i){
        mFrameInputQueue.push(&mFrames[i]);
    }
    
    mTask.setTaskCallback(taskCallback, this);
}

MediaSource::~MediaSource()
{
    LOGI("~MediaSource()");
}

MediaFrame* MediaSource::getFrameFromOutputQueue() {
    std::lock_guard <std::mutex> lck(mMtx);
    if(mFrameOutputQueue.empty()){
        return NULL;
    }
    MediaFrame* frame = mFrameOutputQueue.front();
    mFrameOutputQueue.pop();
    return frame;
}

void MediaSource::putFrameToInputQueue(MediaFrame *frame) {
    //读取原始帧文件，通过任务线程进行处理后放入mFrameInputQueue以供发送
    std::lock_guard <std::mutex> lck(mMtx);
    mFrameInputQueue.push(frame);
    mEnv->threadPool()->addTask(mTask);
}


void MediaSource::taskCallback(void* arg){
    MediaSource* source = (MediaSource*)arg;
    source->handleTask();
}
