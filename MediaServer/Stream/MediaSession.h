#ifndef RTSPSERVER_MEDIASESSION_H
#define RTSPSERVER_MEDIASESSION_H
#include <string>
#include <list>

#include "RtpInstance.h"
#include "Sink.h"

#define MEDIA_MAX_TRACK_NUM 2

class MediaSession
{
public:
    enum TrackId
    {
        TrackIdNone = -1,
        TrackId0    = 0,
        TrackId1    = 1,
    };

    static MediaSession* createNew(std::string sessionName);
    explicit MediaSession(const std::string& sessionName);
    ~MediaSession();

public:

    std::string name() const { return mSessionName; }
    std::string generateSDPDescription();
    bool addSink(MediaSession::TrackId trackId, Sink* sink);// ��������������

    bool addRtpInstance(MediaSession::TrackId trackId, RtpInstance* rtpInstance);// ��������������
    bool removeRtpInstance(RtpInstance* rtpInstance);// ɾ������������


    bool startMulticast();
    bool isStartMulticast();
    std::string getMulticastDestAddr() const { return mMulticastAddr; }
    uint16_t getMulticastDestRtpPort(TrackId trackId);

private:
    class Track {
    public:
        Sink* mSink;
        int mTrackId;
        bool mIsAlive;
        std::list<RtpInstance*> mRtpInstances;
    };

    Track* getTrack(MediaSession::TrackId trackId);
   
    static void sendPacketCallback(void* arg1, void* arg2, void* packet,Sink::PacketType packetType);
    void handleSendRtpPacket(MediaSession::Track* tarck, RtpPacket* rtpPacket);



private:
    std::string mSessionName;
    std::string mSdp;
    Track mTracks[MEDIA_MAX_TRACK_NUM];
    bool mIsStartMulticast;
    std::string mMulticastAddr;
    RtpInstance* mMulticastRtpInstances[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* mMulticastRtcpInstances[MEDIA_MAX_TRACK_NUM];
};
#endif //RTSPSERVER_MEDIASESSION_H