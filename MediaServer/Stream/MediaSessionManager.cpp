#include "MediaSessionManager.h"
#include "MediaSession.h"

MediaSessionManager* MediaSessionManager::createNew() {
    return new MediaSessionManager();
}
MediaSessionManager::MediaSessionManager()
{
}

MediaSessionManager::~MediaSessionManager()
{
}

bool MediaSessionManager::addSession(MediaSession* session) {

    if (mSessMap.find(session->name()) != mSessMap.end()) {// 已存在
        return false;
    }
    else {
        mSessMap.insert(std::make_pair(session->name(), session));
        return true;
    }
}
bool MediaSessionManager::removeSession(MediaSession* session) {
    std::map<std::string, MediaSession*>::iterator it = mSessMap.find(session->name());
    if (it == mSessMap.end()) {
        return false;
    }
    else {
        mSessMap.erase(it);
        return true;
    }

}
MediaSession* MediaSessionManager::getSession(const std::string& name) {

    std::map<std::string, MediaSession*>::iterator it = mSessMap.find(name);
    if (it == mSessMap.end()) {
        return NULL;
    }
    else {
        return it->second;
    }
}

