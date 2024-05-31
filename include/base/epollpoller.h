//
// Created by tcy on 2024/5/14.
//

#ifndef RTSP_EPOLLPOLLER_H
#define RTSP_EPOLLPOLLER_H

#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include "poller.h"

#define INIT_EVENT_LIST_SIZE 16
#define EPOLL_TIMEOUT 10000

typedef std::vector<struct epoll_event> EpollEventLists;

class EpollEpoller : public Poller {
public:
    static EpollEpoller* CreateNew() { return new EpollEpoller; }

    EpollEpoller():m_epoll_event_lists_(INIT_EVENT_LIST_SIZE) {
        m_epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    }
    ~EpollEpoller() override { ::close(m_epoll_fd_); }

    bool AddIOEvent(IOEvent* event) override { return UpdateIOEvent(event); }
    bool UpdateIOEvent(IOEvent* event) override;
    bool RemoveIOEvent(IOEvent* event) override;
    void HandleEvent() override;

private:
    int m_epoll_fd_;
    EpollEventLists m_epoll_event_lists_;
    // std::vector<IOEvent*> m_events_;
};


#endif //RTSP_EPOLLPOLLER_H
