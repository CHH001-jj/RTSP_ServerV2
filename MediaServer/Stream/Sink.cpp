#include "Sink.h"
#include "../EventManager/SocketsOps.h"
#include "../Utils/Log.h"


Sink::Sink(UsageEnvironment* env, MediaSource* mediaSource, int payloadType) :
        mMediaSource(mediaSource),
        mEnv(env),
        mCsrcLen(0),
        mExtension(0),
        mPadding(0),
        mVersion(RTP_VESION),
        mPayloadType(payloadType),
        mMarker(0),
        mSeq(0),
        mSSRC(rand()),
        mTimestamp(0),
        mTimerId(0),
        mSessionSendPacket(NULL),
        mArg1(NULL),
        mArg2(NULL)
{

    LOGI("Sink()");
    mTimerEvent = TimerEvent::createNew(this);
    mTimerEvent->setTimeoutCallback(cbTimeout);  
}

Sink::~Sink(){
    LOGI("~Sink()");

    //mEnv->scheduler()->removeTimedEvent(mTimerId);// 从定时器中删除，由定时器内部自己已实现，这里不再需要调用

    delete mTimerEvent;
    delete mMediaSource;

    //Delete::release(mTimerEvent);
}
void Sink::stopTimerEvent() {

    mTimerEvent->stop();

}
void Sink::setSessionCb(SessionSendPacketCallback cb, void* arg1, void* arg2) {
    // cb 被回调函数
    //arg1 mediaSession 对象指针
    //arg2 mediaSession 被回调track对象指针
    mSessionSendPacket = cb;
    mArg1 = arg1;
    mArg2 = arg2;
}

void Sink::sendRtpPacket(RtpPacket* packet){
    RtpHeader* rtpHeader = packet->mRtpHeader;
    rtpHeader->csrcLen = mCsrcLen;
    rtpHeader->extension = mExtension;
    rtpHeader->padding = mPadding;
    rtpHeader->version = mVersion;
    rtpHeader->payloadType = mPayloadType;
    rtpHeader->marker = mMarker;
    rtpHeader->seq = htons(mSeq);
    rtpHeader->timestamp = htonl(mTimestamp);
    rtpHeader->ssrc = htonl(mSSRC);

    if(mSessionSendPacket){
        //arg1 mediaSession 对象指针
        //arg2 mediaSession 被回调track对象指针
        mSessionSendPacket(mArg1, mArg2, packet, PacketType::RTPPACKET);
    }

}

void Sink::cbTimeout(void *arg) {
    Sink* sink = (Sink*)arg;
    sink->handleTimeout();
}
void Sink::handleTimeout() {
    MediaFrame* frame = mMediaSource->getFrameFromOutputQueue();
    if (!frame) {
        return;
    }
    this->sendFrame(frame);// 由具体子类实现发送逻辑

    mMediaSource->putFrameToInputQueue(frame);//将使用过的frame插入输入队列，插入输入队列以后，加入一个子线程task，从文件中读取数据再次将输入写入到frame
}

void Sink::runEvery(int interval){
    mTimerId = mEnv->scheduler()->addTimedEventRunEvery(mTimerEvent, interval);
}

