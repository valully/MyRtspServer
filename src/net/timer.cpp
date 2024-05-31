//
// Created by tcy on 2024/5/15.
//

#include "../../include/net/timer.h"

static int TimerFdCreate(int32_t clockid, int32_t flag) {
    return timerfd_create(clockid, flag);
}

static bool TimerFdSetTime(int32_t fd, Timer::TimeStamp when, Timer::TimeInterval period) {
    struct itimerspec new_val;
    new_val.it_value.tv_sec = when / 1000;
    new_val.it_value.tv_nsec = when % 1000 * 1000000;
    new_val.it_interval.tv_sec = period / 1000;
    new_val.it_interval.tv_nsec = period % 1000 * 1000000;

    if(timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_val, nullptr) < 0) {
        return false;
    }
    return true;
}

TimerManager* TimerManager::CreateNew(Poller *poller) {
    if(!poller) {
        return nullptr;
    }

    auto timefd = TimerFdCreate(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(timefd < 0) {
        LOG_ERROR("timer fd create error!\n");
        return nullptr;
    }
    return new TimerManager(timefd, poller);
}

TimerManager::TimerManager(int32_t timer_fd, Poller *poller):
    m_timer_fd_(timer_fd), m_poller_(poller) {
    m_timer_io_event_ = IOEvent::CreateNew(timer_fd, this);
    m_timer_io_event_->SetReadCallBack(HandleRead);
    m_timer_io_event_->EnableReadHandle();
    ModifyTimeout();
    m_poller_->AddIOEvent(m_timer_io_event_);
}

Timer::TimerId TimerManager::AddTimer(TimerEvent *event, Timer::TimeStamp time_stamp,
                                      Timer::TimeInterval time_interval) {
    Timer timer(event, time_stamp, time_interval);
    ++last_timer_id_;
    m_timers_map_.insert({last_timer_id_, timer});
    m_events_map_.insert({{time_stamp, last_timer_id_}, timer});
    ModifyTimeout();
    return last_timer_id_;
}

bool TimerManager::RemoveTimer(Timer::TimerId time_id) {
    auto iter = m_timers_map_.find(time_id);
    if(iter != m_timers_map_.end()) {
        auto time_stamp = iter->second.m_time_stamp_;
        auto time_id2 = iter->first;
        m_timers_map_.erase(time_id2);
        m_events_map_.erase({time_stamp, time_id2});
    }
    ModifyTimeout();
    return true;
}

void TimerManager::ModifyTimeout() {
    auto iter = m_events_map_.begin();
    if(iter != m_events_map_.end()) {
        TimerFdSetTime(m_timer_fd_, iter->second.m_time_stamp_, iter->second.m_time_interval_);
    } else {
        TimerFdSetTime(m_timer_fd_, 0, 0);
    }
}

void TimerManager::HandleRead(void *arg) {
    if(!arg) {
        return;
    }
    auto timermanager= (TimerManager*)arg;
    timermanager->HandleTimerEvent();
}

void TimerManager::HandleTimerEvent() {
    if(!m_timers_map_.empty()) {
        int64_t time_point = Timer::GetCurTime();

        while(!m_timers_map_.empty() && m_events_map_.begin()->first.first <= time_point) {
            auto timeid = m_events_map_.begin()->first.second;
            auto timer = m_events_map_.begin()->second;

            timer.HandleEvent();
            m_events_map_.erase(m_events_map_.begin());
            if(timer.m_repeat_) {
                timer.m_time_stamp_ = time_point + timer.m_time_interval_;
                m_events_map_.insert({{timer.m_time_stamp_, timeid}, timer});
            } else {
                m_timers_map_.erase(timeid);
            }
        }
    }
    ModifyTimeout();
}
