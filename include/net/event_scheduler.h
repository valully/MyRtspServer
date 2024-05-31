//
// Created by tcy on 2024/5/15.
//

#ifndef RTSP_EVENT_SCHEDULER_H
#define RTSP_EVENT_SCHEDULER_H

#include <sys/eventfd.h>
#include "timer.h"
#include "../base/epollpoller.h"

class EventScheduler {
public:
    typedef void (*CallBack)(void*);
    /*enum PollerType {
        POLLER_SELECT,
        POLLER_POLL,
        POLLER_EPOLL
    };*/

    static EventScheduler* CreateNew();

    EventScheduler(int32_t fd);
    virtual ~EventScheduler();

    bool AddTriggerEvent(TriggerEvent* event) {
        m_trigger_events_.emplace_back(event);
        return true;
    }

    Timer::TimerId AddTimerEventRunAfter(TimerEvent* event, Timer::TimeInterval delay) {
        Timer::TimeStamp when = Timer::GetCurTime();
        when += delay;
        return m_time_manager_->AddTimer(event, when, 0);
    }

    Timer::TimerId AddTimerEventRunAt(TimerEvent* event, Timer::TimeStamp when) {
        return m_time_manager_->AddTimer(event, when, 0);
    }

    Timer::TimerId AddTimerEventRunEvery(TimerEvent* event, Timer::TimeInterval interval) {
        Timer::TimeStamp when = Timer::GetCurTime();
        when += interval;
        return m_time_manager_->AddTimer(event, when, interval);
    }

    bool RemoveTimerEvent(Timer::TimerId timeid) {
        return m_time_manager_->RemoveTimer(timeid);
    }
    bool AddIOEvent(IOEvent* event) {
        return m_poller_->AddIOEvent(event);
    }
    bool UpdateIOEvent(IOEvent* event) {
        return m_poller_->UpdateIOEvent(event);
    }
    bool RemoveIOEvent(IOEvent* event) {
        return m_poller_->RemoveIOEvent(event);
    }

    void Loop();
    void WakeUp();

    void RunInLocalThread(CallBack cb, void* arg);
    void HandleOtherEvent();

private:
    static void HandleReadCallback(void* arg);
    void HandleTriggleEvents();
    void HandleRead() const;

private:
    bool m_quit_{false};
    Poller* m_poller_;
    TimerManager* m_time_manager_;
    std::vector<TriggerEvent*> m_trigger_events_;
    int32_t m_wakeup_fd_;
    IOEvent* m_wake_io_event_;
    std::queue<std::pair<CallBack, void*>> m_callback_que_;
    std::mutex m_mtx_;
};


#endif //RTSP_EVENT_SCHEDULER_H
