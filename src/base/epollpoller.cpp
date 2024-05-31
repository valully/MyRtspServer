//
// Created by tcy on 2024/5/14.
//

#include <cassert>
#include "../../include/base/epollpoller.h"

bool EpollEpoller::UpdateIOEvent(IOEvent *event) {
    struct epoll_event epoll_evt;
    auto fd = event->GetFd();

    memset(&epoll_evt, 0, sizeof(epoll_evt));
    epoll_evt.data.fd = fd;
    if(event->IsReadHandle()) {
        epoll_evt.events |= EPOLLIN;
    }
    if(event->IsWriteHandle()) {
        epoll_evt.events |= EPOLLOUT;
    }
    if(event->IsErrorHandle()) {
        epoll_evt.events |= EPOLLERR;
    }
    if(m_event_map_.count(fd) == 0) {
        epoll_ctl(m_epoll_fd_, EPOLL_CTL_ADD, fd, &epoll_evt);
        m_event_map_.insert({fd, event});
        if(m_event_map_.size() >= m_epoll_event_lists_.size()) {
            m_epoll_event_lists_.resize(m_epoll_event_lists_.size() * 2);
        }
    } else {
        epoll_ctl(m_epoll_fd_, EPOLL_CTL_MOD, fd, &epoll_evt);
    }
    return true;
}

bool EpollEpoller::RemoveIOEvent(IOEvent *event) {
    auto fd = event->GetFd();
    if(m_event_map_.count(fd) == 0) {
        return false;
    }
    epoll_ctl(m_epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    m_event_map_.erase(fd);
    return true;
}

void EpollEpoller::HandleEvent() {
    int32_t nums, fd;
    uint32_t ret_event, event;
    nums = epoll_wait(m_epoll_fd_, &m_epoll_event_lists_[0], m_epoll_event_lists_.size(), EPOLL_TIMEOUT);
    if(nums < 0) {
        LOG_DEBUG("epoll wait error\n");
        return;
    }
    for(int32_t i = 0; i < nums; i++){
        ret_event = 0;
        fd = m_epoll_event_lists_[i].data.fd;
        event = m_epoll_event_lists_[i].events;
        if((event & EPOLLIN) || (event & EPOLLPRI) || (event & EPOLLRDHUP)) {
            ret_event |= IOEvent::EVENT_READ;
        }
        if(event & EPOLLOUT) {
            ret_event |= IOEvent::EVENT_WRITE;
        }
        if(event & EPOLLERR) {
            ret_event |= IOEvent::EVENT_ERROR;
        }

        auto iter = m_event_map_.find(fd);
        assert(iter != m_event_map_.end());

        iter->second->SetREvent(ret_event);
        iter->second->HandleEvent();
    }
}
