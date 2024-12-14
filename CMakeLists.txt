cmake_minimum_required(VERSION 3.10)
project(RTSP_ServerV2)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread ")

set(CMAKE_BUILD_TYPE DEBUG)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(INCLUDE_DIR /usr/local/include)
set(LIB_DIR /usr/local/lib)
include_directories(${INCLUDE_DIR})
link_directories(${LIB_DIR})

add_executable(RTSP_ServerV2
        MediaServer/Stream/Buffer.cpp
        MediaServer/Stream/InetAddress.cpp
        MediaServer/Stream/MediaSessionManager.cpp
        MediaServer/Stream/MediaSession.cpp
        MediaServer/Stream/AACFileMediaSource.cpp
        MediaServer/Stream/H264FileMediaSource.cpp
        MediaServer/Stream/Rtp.cpp
        MediaServer/Stream/MediaSource.cpp
        MediaServer/Stream/H264FileSink.cpp
        MediaServer/Stream/AACFileSink.cpp
        MediaServer/Stream/Sink.cpp
        MediaServer/Stream/RtspConnection.cpp
        MediaServer/Stream/RtspServer.cpp
        MediaServer/Stream/TcpConnection.cpp
        MediaServer/EventManager/Event.cpp
        MediaServer/EventManager/EventScheduler.cpp
        MediaServer/EventManager/Poller.cpp
        MediaServer/EventManager/SelectPoller.cpp
        MediaServer/EventManager/EPollPoller.cpp
        MediaServer/EventManager/PollPoller.cpp
        MediaServer/EventManager/SocketsOps.cpp
        MediaServer/EventManager/Thread.cpp
        MediaServer/EventManager/ThreadPool.cpp
        MediaServer/EventManager/Timer.cpp
        MediaServer/EventManager/UsageEnvironment.cpp
        MediaServer/main.cpp
        )