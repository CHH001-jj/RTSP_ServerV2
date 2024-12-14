#ifndef RTSPSERVER_SINK_H
#define RTSPSERVER_SINK_H
#include <string>
#include <stdint.h>

#include "Rtp.h"
#include "MediaSource.h"
#include "../EventManager/Event.h"
#include "../EventManager/UsageEnvironment.h"


class Sink
{
public:
    enum PacketType
    {
        UNKNOWN = -1,
        RTPPACKET = 0,
    };

    typedef void (*SessionSendPacketCallback)(void* arg1, void* arg2, void* packet, PacketType packetType);

    Sink(UsageEnvironment* env, MediaSource* mediaSource, int payloadType);
    virtual ~Sink();

    void stopTimerEvent();

    virtual std::string getMediaDescription(uint16_t port) = 0;
    virtual std::string getAttribute() = 0;

    void setSessionCb(SessionSendPacketCallback cb,void* arg1, void* arg2);

protected:

    virtual void sendFrame(MediaFrame* frame) = 0;
    void sendRtpPacket(RtpPacket* packet);

    void runEvery(int interval);
private:

    static void cbTimeout(void* arg);
    void handleTimeout();

protected:
    UsageEnvironment* mEnv;
    MediaSource* mMediaSource;
    SessionSendPacketCallback mSessionSendPacket;
    void* mArg1;
    void* mArg2;

    uint8_t mCsrcLen;
    uint8_t mExtension;
    uint8_t mPadding;
    uint8_t mVersion;
    uint8_t mPayloadType;
    uint8_t mMarker;
    uint16_t mSeq;
    uint32_t mTimestamp;
    uint32_t mSSRC;

private:
    TimerEvent* mTimerEvent;
    Timer::TimerId mTimerId;// runEvery()之后获取
};

#endif //RTSPSERVER_SINK_H