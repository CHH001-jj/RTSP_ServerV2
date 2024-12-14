#include "EPollPoller.h"
#include "../Utils/Log.h"
#include <unistd.h>
#include <assert.h>

EpollPoller::EpollPoller()
    : mEpollFd(epoll_create1(0)), // 创建 epoll 实例
      mEventList(16)             // 初始事件列表大小为 16
{
    if (mEpollFd < 0) {
        LOGE("Failed to create epoll instance");
    }
}

EpollPoller::~EpollPoller() {
    if (mEpollFd >= 0) {
        close(mEpollFd); // 关闭 epoll 实例
    }
}

EpollPoller* EpollPoller::createNew() {
    return new EpollPoller();
}

bool EpollPoller::addIOEvent(IOEvent* event) {
    return updateIOEvent(event);
}

bool EpollPoller::updateIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) {
        LOGE("Invalid file descriptor: %d", fd);
        return false;
    }

    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = 0;

    if (event->isReadHandling()) ev.events |= EPOLLIN;
    if (event->isWriteHandling()) ev.events |= EPOLLOUT;
    if (event->isErrorHandling()) ev.events |= EPOLLERR;

    if (mEventMap.find(fd) == mEventMap.end()) {
        // 新事件，使用 EPOLL_CTL_ADD
        if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            LOGE("Failed to add fd %d to epoll", fd);
            return false;
        }
        mEventMap[fd] = event;
    } else {
        // 已存在事件，使用 EPOLL_CTL_MOD
        if (epoll_ctl(mEpollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
            LOGE("Failed to modify fd %d in epoll", fd);
            return false;
        }
    }
    return true;
}

bool EpollPoller::removeIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) {
        LOGE("Invalid file descriptor: %d", fd);
        return false;
    }

    if (mEventMap.find(fd) != mEventMap.end()) {
        // 从 epoll 实例中移除
        if (epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
            LOGE("Failed to remove fd %d from epoll", fd);
            return false;
        }
        mEventMap.erase(fd);
    }
    return true;
}

void EpollPoller::handleEvent() {
    // 等待事件触发
    int n = epoll_wait(mEpollFd, mEventList.data(), mEventList.size(), 1000); // 超时时间 1000ms
    if (n < 0) {
        LOGE("epoll_wait error");
        return;
    }

    for (int i = 0; i < n; ++i) {
        int fd = mEventList[i].data.fd;
        int events = mEventList[i].events;
        IOEvent* ioEvent = mEventMap[fd];

        int rEvent = 0;
        if (events & EPOLLIN) rEvent |= IOEvent::EVENT_READ;
        if (events & EPOLLOUT) rEvent |= IOEvent::EVENT_WRITE;
        if (events & EPOLLERR || events & EPOLLHUP) rEvent |= IOEvent::EVENT_ERROR;

        if (rEvent != 0) {
            ioEvent->setREvent(rEvent);
            ioEvent->handleEvent(); // 处理事件
        }
    }

    // 如果事件列表太满，增加容量
    if (n == (int)mEventList.size()) {
        mEventList.resize(mEventList.size() * 2);
    }
}
