//
// Created by tcy on 2024/5/12.
//

#ifndef RTSP_LOGBUFFER_H
#define RTSP_LOGBUFFER_H

#include <string>
#include <cstring>

#define LOG_BUFFER_SIZE 1024*1024

class LogBuffer {
public:
    LogBuffer():m_cur_ptr(m_data_) { bzero(); }
    ~LogBuffer() = default;

    const char* data() { return m_data_; }

    int avail() { return (int)(m_data_ + LOG_BUFFER_SIZE - m_cur_ptr); }

    int length() { return (int)(m_cur_ptr - m_data_); }

    char* current() { return m_cur_ptr; }

    void add(const int& len) { m_cur_ptr += len; }

    void append(const char* buf, const size_t& len) {
        if(avail() > len) {
            memcpy(m_cur_ptr, buf, len);
            m_cur_ptr += len;
        }
    }

    void reset() { m_cur_ptr = m_data_; }

    void bzero() { memset(m_data_, 0, LOG_BUFFER_SIZE); }
private:
    char m_data_[LOG_BUFFER_SIZE];
    char* m_cur_ptr;
};

#endif //RTSP_LOGBUFFER_H
