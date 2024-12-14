#ifndef RTSPSERVER_H264FILESINK_H
#define RTSPSERVER_H264FILESINK_H

#include <stdint.h>
#include "Sink.h"



class H264FileSink : public Sink
{
public:
    static H264FileSink* createNew(UsageEnvironment* env, MediaSource* mediaSource);

    H264FileSink(UsageEnvironment* env, MediaSource* mediaSource);
    virtual ~H264FileSink();
    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();
    virtual void sendFrame(MediaFrame* frame);

private:
    RtpPacket mRtpPacket;
    int mClockRate;
    int mFps;

};

#endif //RTSPSERVER_H264FILESINK_H