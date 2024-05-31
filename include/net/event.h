//
// Created by tcy on 2024/5/14.
//

#ifndef RTSP_EVENT_H
#define RTSP_EVENT_H

#include "../base/log.h"

typedef void (*EventCallBack)(void*);

class TriggerEvent {
public:
    static TriggerEvent* CreateNew(void* arg) { return new TriggerEvent(arg); }
    static TriggerEvent* CreateNew() { return new TriggerEvent(nullptr); }

    TriggerEvent(void* arg): m_arg_(arg) {}
    ~TriggerEvent() = default;

    void SetArg(void* arg) { m_arg_ = arg; }
    void SetCallBack(EventCallBack cb) { m_trigger_callback_ = cb; }
    void HandleEvent() {
        if(m_trigger_callback_) {
            m_trigger_callback_(m_arg_);
        }
    }

private:
    void* m_arg_;
    EventCallBack m_trigger_callback_{nullptr};
};

class TimerEvent {
public:
    static TimerEvent* CreateNew(void* arg) { return new TimerEvent(arg); }
    static TimerEvent* CreateNew() { return new TimerEvent(nullptr); }

    TimerEvent(void* arg): m_arg_(arg) {}
    TimerEvent() = default;

    void SetArg(void* arg) { m_arg_ = arg; }
    void SetCallBack(EventCallBack cb) { m_timeout_callback_ = cb; }

    void HandleEvent() {
        if(m_timeout_callback_) {
            m_timeout_callback_(m_arg_);
        }
    }

private:
    void* m_arg_;
    EventCallBack m_timeout_callback_{nullptr};
};

class IOEvent {
public:
    enum {
        EVENT_NONE = 0,
        EVENT_READ = 1,
        EVENT_WRITE = 2,
        EVENT_ERROR = 4
    };

    static IOEvent* CreateNew(int32_t fd, void* arg) { return new IOEvent(fd, arg); }
    static IOEvent* CreateNew(int32_t fd) { return new IOEvent(fd, nullptr); }

    IOEvent(int32_t fd, void* arg) : m_fd_(fd), m_arg_(arg), m_event_(EVENT_NONE),
        m_revent_(EVENT_NONE) {}
    ~IOEvent() = default;

    int32_t GetFd() const { return m_fd_; }
    uint32_t GetEvent() const { return m_event_; }

    void SetREvent(const uint32_t& event) { m_revent_ = event; }
    void SetArg(void* arg) { m_arg_ = arg; }

    void SetReadCallBack(EventCallBack cb) { m_read_cb_ = cb; }
    void SetWriteCallback(EventCallBack cb) { m_write_cb_ = cb; };
    void SetErrorCallback(EventCallBack cb) { m_error_cb_ = cb; };

    void EnableReadHandle() { m_event_ |= EVENT_READ; }
    void EnableWriteHandle() { m_event_ |= EVENT_WRITE; }
    void EnableErrorHandle() { m_event_ |= EVENT_ERROR; }
    void DisableReadHandle() { m_event_ &= ~EVENT_READ; }
    void DisableWriteHandle() { m_event_ &= ~EVENT_WRITE; }
    void DisableErrorHandle() { m_event_ &= ~EVENT_ERROR; }

    bool IsNoneHandle() const { return m_event_ == EVENT_NONE; }
    bool IsReadHandle() const { return (m_event_ & EVENT_READ) != 0; }
    bool IsWriteHandle() const { return (m_event_ & EVENT_WRITE) != 0; }
    bool IsErrorHandle() const { return (m_event_ & EVENT_ERROR) != 0; }

    void HandleEvent() {
        if (m_read_cb_ && (m_revent_ & EVENT_READ)) {
            m_read_cb_(m_arg_);
        }
        if (m_write_cb_ && (m_revent_ & EVENT_WRITE)) {
            m_write_cb_(m_arg_);
        }
        if (m_error_cb_ && (m_revent_ & EVENT_ERROR)) {
            m_error_cb_(m_arg_);
        }
    }

private:
    int32_t m_fd_;
    void* m_arg_;
    uint32_t m_event_;
    uint32_t m_revent_;
    EventCallBack m_read_cb_{nullptr};
    EventCallBack m_write_cb_{nullptr};
    EventCallBack m_error_cb_{nullptr};
};

#endif //RTSP_EVENT_H
