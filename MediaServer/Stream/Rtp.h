#ifndef RTSPSERVER_RTP_H
#define RTSPSERVER_RTP_H
#include <stdint.h>
#include <stdlib.h>

#define RTP_VESION              2

#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97

#define RTP_HEADER_SIZE         12
#define RTP_MAX_PKT_SIZE        1400

struct RtpHeader{
    // byte 0
    uint8_t csrcLen:4;// contributing source identifiers count
    uint8_t extension:1;
    uint8_t padding:1;
    uint8_t version:2;

    // byte 1
    uint8_t payloadType:7;
    uint8_t marker:1;
    
    // bytes 2,3
    uint16_t seq;
    
    // bytes 4-7
    uint32_t timestamp;
    
    // bytes 8-11
    uint32_t ssrc;

    // data
    uint8_t payload[0];
};
struct RtcpHeader
{    
    // byte 0
    uint8_t rc : 5;// reception report count
    uint8_t padding : 1;
    uint8_t version : 2;
    // byte 1
    uint8_t packetType;

    // bytes 2,3
    uint16_t length;
};
class RtpPacket {
public:
    RtpPacket();
    ~RtpPacket();
public:
    uint8_t* mBuf; // 4+rtpHeader+rtpBody
    uint8_t* mBuf4;// rtpHeader+rtpBody
    RtpHeader* const mRtpHeader;
    int mSize;// rtpHeader+rtpBody
};

void parseRtpHeader(uint8_t* buf, struct RtpHeader* rtpHeader);
void parseRtcpHeader(uint8_t* buf, struct RtcpHeader* rtcpHeader);




#endif //BXC_RTSPSERVER_RTP_H
