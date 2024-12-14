#include "PollPoller.h"
#include "../Utils/Log.h"

PollPoller::PollPoller() {
    // 初始化，mPollFds 和 mIOEvents 初始为空
}

PollPoller::~PollPoller() {
    mPollFds.clear();
    mIOEvents.clear();
}

PollPoller* PollPoller::createNew() {
    return new PollPoller();
}

bool PollPoller::addIOEvent(IOEvent* event) {
    return updateIOEvent(event);
}

bool PollPoller::updateIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) {
        LOGE("Invalid fd=%d", fd);
        return false;
    }

    // 查找是否已存在
    for (auto& pollfd : mPollFds) {
        if (pollfd.fd == fd) {
            pollfd.events = 0;  // 更新事件
            if (event->isReadHandling()) pollfd.events |= POLLIN;
            if (event->isWriteHandling()) pollfd.events |= POLLOUT;
            if (event->isErrorHandling()) pollfd.events |= POLLERR;
            return true;
        }
    }

    // 不存在，则新增
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = 0;
    pfd.revents = 0;
    if (event->isReadHandling()) pfd.events |= POLLIN;
    if (event->isWriteHandling()) pfd.events |= POLLOUT;
    if (event->isErrorHandling()) pfd.events |= POLLERR;

    mPollFds.push_back(pfd);
    mIOEvents.push_back(event);

    return true;
}

bool PollPoller::removeIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) return false;

    // 移除 pollfd 和对应的 IOEvent
    for (size_t i = 0; i < mPollFds.size(); ++i) {
        if (mPollFds[i].fd == fd) {
            mPollFds.erase(mPollFds.begin() + i);
            mIOEvents.erase(mIOEvents.begin() + i);
            return true;
        }
    }

    return false;
}

void PollPoller::handleEvent() {
    int timeout = 1000;  // 超时时间（毫秒）
    int ret = poll(mPollFds.data(), mPollFds.size(), timeout);
    if (ret < 0) {
        LOGE("poll error");
        return;
    }

    for (size_t i = 0; i < mPollFds.size(); ++i) {
        int rEvent = 0;
        if (mPollFds[i].revents & POLLIN) rEvent |= IOEvent::EVENT_READ;
        if (mPollFds[i].revents & POLLOUT) rEvent |= IOEvent::EVENT_WRITE;
        if (mPollFds[i].revents & POLLERR) rEvent |= IOEvent::EVENT_ERROR;

        if (rEvent != 0) {
            mIOEvents[i]->setREvent(rEvent);
            mIOEvents[i]->handleEvent();  // 调用 IOEvent 的事件处理逻辑
        }
    }
}
