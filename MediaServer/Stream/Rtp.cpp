#include "Rtp.h"
#include <string.h>

RtpPacket::RtpPacket() :
    //mBuf(new uint8_t[4 + RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 100]),
    mBuf((uint8_t*)malloc(4 + RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 100)),
    mBuf4(mBuf + 4),
    mRtpHeader((RtpHeader*)mBuf4),
    mSize(0) {
}
RtpPacket::~RtpPacket() {
    //delete[]mBuf;
    free(mBuf);
    mBuf = NULL;
}
void parseRtpHeader(uint8_t* buf, struct RtpHeader* rtpHeader)
{
    memset(rtpHeader, 0, sizeof(*rtpHeader));

    // byte 0
    rtpHeader->version = (buf[0] & 0xC0) >> 6;
    rtpHeader->padding = (buf[0] & 0x20) >> 5;
    rtpHeader->extension = (buf[0] & 0x10) >> 4;
    rtpHeader->csrcLen = (buf[0] & 0x0F);
    // byte 1
    rtpHeader->marker = (buf[1] & 0x80) >> 7;
    rtpHeader->payloadType = (buf[1] & 0x7F);
    // bytes 2,3
    rtpHeader->seq = ((buf[2] & 0xFF) << 8) | (buf[3] & 0xFF);
    // bytes 4-7
    rtpHeader->timestamp = ((buf[4] & 0xFF) << 24) | ((buf[5] & 0xFF) << 16)
        | ((buf[6] & 0xFF) << 8)
        | ((buf[7] & 0xFF));
    // bytes 8-11
    rtpHeader->ssrc = ((buf[8] & 0xFF) << 24) | ((buf[9] & 0xFF) << 16)
        | ((buf[10] & 0xFF) << 8)
        | ((buf[11] & 0xFF));
}

void parseRtcpHeader(uint8_t* buf, struct RtcpHeader* rtcpHeader) {

    memset(rtcpHeader, 0, sizeof(*rtcpHeader));
    // byte 0
    rtcpHeader->version = (buf[0] & 0xC0) >> 6;
    rtcpHeader->padding = (buf[0] & 0x20) >> 5;
    rtcpHeader->rc = (buf[0] & 0x1F);
    // byte 1
    rtcpHeader->packetType = (buf[1] & 0xFF);
    // bytes 2,3
    rtcpHeader->length= ((buf[2] & 0xFF) << 8) | (buf[3] & 0xFF);
}