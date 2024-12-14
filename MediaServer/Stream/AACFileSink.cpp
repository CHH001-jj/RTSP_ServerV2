#include "AACFileSink.h"
#include <string>
#include <stdio.h>
#include <string.h>
#include "../Utils/Log.h"

AACFileSink* AACFileSink::createNew(UsageEnvironment* env, MediaSource* mediaSource)
{
    return new AACFileSink(env, mediaSource, RTP_PAYLOAD_TYPE_AAC);
}

AACFileSink::AACFileSink(UsageEnvironment* env, MediaSource* mediaSource, int payloadType) :
        Sink(env, mediaSource, payloadType),
        mSampleRate(44100),
        mChannels(2),
        mFps(mediaSource->getFps())
{
    LOGI("AACFileSink()");
    mMarker = 1;
    runEvery(1000/mFps);
}

AACFileSink::~AACFileSink()
{
    LOGI("~AACFileSink()");
}

std::string AACFileSink::getMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=audio %hu RTP/AVP %d", port, mPayloadType);

    return std::string(buf);
}

static uint32_t AACSampleRate[16] =
{
    97000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0 /*reserved */
};

std::string AACFileSink::getAttribute()
{
    char buf[500] = { 0 };
    sprintf(buf, "a=rtpmap:97 mpeg4-generic/%u/%u\r\n", mSampleRate, mChannels);

    uint8_t index = 0;
    for (index = 0; index < 16; index++)
    {
        if (AACSampleRate[index] == mSampleRate)
            break;
    }
    if (index == 16)
        return "";

    uint8_t profile = 1;
    char configStr[10] = {0};
    // 没看懂
    sprintf(configStr, "%02x%02x", (uint8_t)((profile+1) << 3)|(index >> 1),
            (uint8_t)((index << 7)|(mChannels<< 3)));

    sprintf(buf+strlen(buf),
            "a=fmtp:%d profile-level-id=1;"
            "mode=AAC-hbr;"
            "sizelength=13;indexlength=3;indexdeltalength=3;"
            "config=%04u",
            mPayloadType,
            atoi(configStr));

    return std::string(buf);
}

void AACFileSink::sendFrame(MediaFrame* frame)
{
    RtpHeader* rtpHeader = mRtpPacket.mRtpHeader;
    int frameSize = frame->mSize-7; //去掉aac头部
    
    //AU-Header-Length,表示 AU Header 的长度，固定为 16 位（2 字节）
    rtpHeader->payload[0] = 0x00;
    rtpHeader->payload[1] = 0x10;

    //AU Header
    rtpHeader->payload[2] = (frameSize & 0x1FE0) >> 5; //高8位
    rtpHeader->payload[3] = (frameSize & 0x1F) << 3; //低5位

    /* 去掉aac的头部 */
    memcpy(rtpHeader->payload+4, frame->mBuf+7, frameSize);
    mRtpPacket.mSize = RTP_HEADER_SIZE + 4 + frameSize;
    sendRtpPacket(&mRtpPacket);
    mSeq++;

    /* (1000 / mFps) 表示一帧多少毫秒 */
    mTimestamp += mSampleRate * (1000 / mFps) / 1000;
}

