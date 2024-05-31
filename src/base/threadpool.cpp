//
// Created by tcy on 2024/5/16.
//

#include "../../include/base/threadpool.h"
#include "../../include/base/log.h"

ThreadPool::ThreadPool(int32_t num) {
    m_thread_arr_.reserve(num);
    std::lock_guard<std::mutex> locker(m_mtx_);
    for(int32_t i = 0; i < num; i++) {
        m_thread_arr_.emplace_back(&ThreadPool::HandleTask, this);
    }
}

ThreadPool::~ThreadPool() {
    CancelThreads();
}

void ThreadPool::AddTask(ThreadPool::Task &task) {
    std::lock_guard<std::mutex> locker(m_mtx_);
    m_task_que_.push(task);
    m_cond_.notify_one();
}

void ThreadPool::HandleTask() {
    while(!m_quit_) {
        Task task;
        {
            std::unique_lock<std::mutex> locker(m_mtx_);
            if(m_task_que_.empty()) {
                m_cond_.wait(locker);
            }
            if(m_quit_) {
                break;
            }
            if(m_task_que_.empty()) {
                continue;
            }
            task = m_task_que_.front();
            m_task_que_.pop();
            // LOG_DEBUG("m_task_que size: %d\n", m_task_que_.size());
        }
        task.Handle();
    }
}

void ThreadPool::CancelThreads() {
    std::unique_lock<std::mutex> locker(m_mtx_);
    m_quit_ = true;
    locker.unlock();
    m_cond_.notify_all();
    for(auto & iter:m_thread_arr_) {
        if(iter.joinable()) {
            iter.join();
        }
    }
    locker.lock();
    m_thread_arr_.clear();
}

