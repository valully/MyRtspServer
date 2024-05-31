//
// Created by tcy on 2024/5/16.
//

#ifndef RTSP_USAGEENVIRONMENT_H
#define RTSP_USAGEENVIRONMENT_H

#include "../base/threadpool.h"
#include "event_scheduler.h"

class UsageEnvironment {
public:
    static UsageEnvironment* CreateNew(EventScheduler* scheduler, ThreadPool* thread_pool) {
        if(!scheduler) {
            return nullptr;
        }
        return new UsageEnvironment(scheduler, thread_pool);
    }

    UsageEnvironment(EventScheduler* scheduler, ThreadPool* thread_pool) :
        m_scheduler_(scheduler),m_thread_pool_(thread_pool) {}
    ~UsageEnvironment() = default;

    EventScheduler* Scheduler() { return m_scheduler_; }
    ThreadPool* ThPool() { return m_thread_pool_; }

private:
    EventScheduler* m_scheduler_;
    ThreadPool* m_thread_pool_;
};


#endif //RTSP_USAGEENVIRONMENT_H
