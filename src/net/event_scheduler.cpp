//
// Created by tcy on 2024/5/15.
//

#include "../../include/net/event_scheduler.h"

static int CreateEventFd() {
    auto event_fd =  ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(event_fd < 0) {
        LOG_DEBUG("create event fd error!\n");
        return -1;
    }
    return event_fd;
}

EventScheduler* EventScheduler::CreateNew() {
    auto event_fd = CreateEventFd();
    if(event_fd < 0) {
        return nullptr;
    }
    return new EventScheduler(event_fd);
}

EventScheduler::EventScheduler(int32_t fd) : m_wakeup_fd_(fd) {
    m_poller_ = EpollEpoller::CreateNew();

    m_time_manager_ = TimerManager::CreateNew(m_poller_);
    m_wake_io_event_ = IOEvent::CreateNew(m_wakeup_fd_, this);
    m_wake_io_event_->SetReadCallBack(HandleReadCallback);
    m_wake_io_event_->EnableReadHandle();
    m_poller_->AddIOEvent(m_wake_io_event_);
}

EventScheduler::~EventScheduler() {
    m_poller_->RemoveIOEvent(m_wake_io_event_);
    ::close(m_wakeup_fd_);
    delete m_wake_io_event_;
    delete m_time_manager_;
    delete m_poller_;
}

void EventScheduler::Loop() {
    while(!m_quit_) {
        this->HandleTriggleEvents();
        m_poller_->HandleEvent();
        this->HandleOtherEvent();
    }
}

void EventScheduler::WakeUp() {
    uint64_t one = 1;
    auto ret = ::write(m_wakeup_fd_, &one, sizeof(one));
}

void EventScheduler::HandleTriggleEvents() {
    if(!m_trigger_events_.empty()) {
        for(auto & iter:m_trigger_events_) {
            iter->HandleEvent();
        }
        m_trigger_events_.clear();
    }
}

void EventScheduler::HandleReadCallback(void *arg) {
    if(!arg) {
        return;
    }
    auto scheduler = (EventScheduler*)arg;
    scheduler->HandleRead();
}

void EventScheduler::HandleRead() const {
    uint64_t one;
    while(::read(m_wakeup_fd_, &one, sizeof(one)) > 0);
}

void EventScheduler::RunInLocalThread(EventScheduler::CallBack cb, void *arg) {
    std::lock_guard<std::mutex> locker(m_mtx_);
    m_callback_que_.emplace(cb, arg);
}

void EventScheduler::HandleOtherEvent() {
    std::lock_guard<std::mutex> locker(m_mtx_);
    while(!m_callback_que_.empty()) {
        auto event = m_callback_que_.front();
        event.first(event.second);
    }
}