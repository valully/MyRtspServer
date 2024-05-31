//
// Created by tcy on 2024/5/15.
//

#ifndef RTSP_TIMER_H
#define RTSP_TIMER_H

#include <sys/timerfd.h>
#include "../base/epollpoller.h"
#include "event.h"

class Timer {
public:
    typedef uint32_t TimerId;
    typedef int64_t TimeStamp;
    typedef uint32_t TimeInterval;

    ~Timer() = default;

    static TimeStamp GetCurTime() {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec * 1000 + now.tv_nsec / 1000000;
    }

private:
    friend class TimerManager;
    Timer(TimerEvent* timer_event,TimeStamp time_stamp, TimeInterval time_interval) :
        m_timer_event_(timer_event), m_time_stamp_(time_stamp), m_time_interval_(time_interval) {
        if(time_interval > 0) {
            m_repeat_ = true;
        } else {
            m_repeat_ = false;
        }
    }
    void HandleEvent() {
        if(!m_timer_event_) {
            return;
        }
        m_timer_event_->HandleEvent();
    }

private:
    TimerEvent* m_timer_event_;
    TimeStamp m_time_stamp_;
    TimeInterval m_time_interval_;
    bool m_repeat_;

};

class TimerManager {
public:
    static TimerManager* CreateNew(Poller* poller);

    TimerManager(int32_t timer_fd, Poller* poller);
    ~TimerManager() {
        m_poller_->RemoveIOEvent(m_timer_io_event_);
        delete m_timer_io_event_;
    }

    Timer::TimerId AddTimer(TimerEvent* event, Timer::TimeStamp time_stamp,
                            Timer::TimeInterval time_interval);
    bool RemoveTimer(Timer::TimerId  time_id);

private:
    void ModifyTimeout();
    static void HandleRead(void* arg);
    void HandleTimerEvent();

private:
    int32_t m_timer_fd_;
    Poller* m_poller_;
    std::map<Timer::TimerId, Timer> m_timers_map_;

    typedef std::pair<Timer::TimeStamp, Timer::TimerId> TimerIndex;
    std::multimap<TimerIndex, Timer> m_events_map_;
    uint32_t last_timer_id_{0};
    IOEvent* m_timer_io_event_;
};

#endif //RTSP_TIMER_H
