//
// Created by tcy on 2024/5/16.
//

#ifndef RTSP_THREADPOOL_H
#define RTSP_THREADPOOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

class ThreadPool {
public:
    class Task{
    public:
        typedef void (*TaskCallback)(void*);

        Task() = default;

        void SetTaskCallback(TaskCallback cb, void* arg){
            m_cb_ = cb;
            m_arg_ = arg;
        }

        void Handle() {
            if(m_cb_) {
                m_cb_(m_arg_);
            }
        }

        Task& operator=(const Task& task) {
            if(this != &task) {
                this->m_cb_ = task.m_cb_;
                this->m_arg_ = task.m_arg_;
            }
            return *this;
        }

    private:
        TaskCallback m_cb_;
        void* m_arg_;
    };

    static ThreadPool* CreateNew(int32_t num) {
        return new ThreadPool(num);
    }

    ThreadPool(int32_t num);
    ~ThreadPool();

    void AddTask(Task& task);

private:
    //void Run(void* arg);
    //void CreateThreads();
    void CancelThreads();
    void HandleTask();

private:
    std::queue<Task> m_task_que_;
    std::mutex m_mtx_;
    std::condition_variable m_cond_;
    std::vector<std::thread> m_thread_arr_;
    bool m_quit_{false};
};


#endif //RTSP_THREADPOOL_H
