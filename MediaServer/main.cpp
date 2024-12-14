#include "EventManager/EventScheduler.h"
#include "EventManager/ThreadPool.h"
#include "EventManager/UsageEnvironment.h"
#include "Stream/MediaSessionManager.h"
#include "Stream/RtspServer.h"
#include "Stream/H264FileMediaSource.h"
#include "Stream/H264FileSink.h"
#include "Stream/AACFileMediaSource.h"
#include <unistd.h> 
#include "Stream/AACFileSink.h"
#include "Utils/Log.h"
#include <iostream>
#include <dirent.h>
#include <string>
#include <vector>
#include <sys/types.h>
std::string removeExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return filename; // No extension found
    }
    return filename.substr(0, dotPos); // Remove extension
}
std::vector<std::string> getFileNames(const std::string& directory) {
    std::vector<std::string> fileNames;
    DIR* dir = opendir(directory.c_str());
    if (!dir) {
        perror("Failed to open directory");
        return fileNames;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) { // Only regular files
            std::string filename = entry->d_name;
            fileNames.push_back(removeExtension(filename));
        }
    }
    closedir(dir);

    return fileNames;
}
int main() {
    srand(time(NULL));//时间初始化
    EventScheduler* scheduler = EventScheduler::createNew(EventScheduler::POLLER_EPOLL);
    ThreadPool* threadPool = ThreadPool::createNew(1);// 
    MediaSessionManager* sessMgr = MediaSessionManager::createNew();
    UsageEnvironment* env = UsageEnvironment::createNew(scheduler, threadPool);
    Ipv4Address rtspAddr("192.168.6.137", 8554);
    RtspServer* rtspServer = RtspServer::createNew(env, sessMgr,rtspAddr);

    LOGI("----------session init start------");
    {
        const std::string directory = "../data";
        std::vector<std::string> fileNames = getFileNames(directory);

        for(int i = 0;i<fileNames.size();++i){
            std::cout<<fileNames[i]<<std::endl;
            MediaSession* session = MediaSession::createNew(fileNames[i]);
            
            MediaSource* source = H264FileMediaSource::createNew(env, "../data/" + fileNames[i] + ".h264");

            Sink* sink = H264FileSink::createNew(env, source);
            session->addSink(MediaSession::TrackId0, sink);
            source = AACFileMeidaSource::createNew(env, "../data/" + fileNames[i] + ".aac");
            sink = AACFileSink::createNew(env, source);
            session->addSink(MediaSession::TrackId1, sink);
            // session->startMulticast(); //多播
            sessMgr->addSession(session);
        }
    }
    LOGI("----------session init end------");

    rtspServer->start();
    env->scheduler()->loop();
    return 0;

}