//
// Created by tcy on 2024/5/12.
//

#include "../../include/base/log.h"
#include <cassert>

Log::Log() {
    //memset(m_data_, 0, 4096);
    //m_cur_ptr = m_data_;
    this_log_level_ = LogLevel::LogDebug;
    write_thread_ = nullptr;
    m_fp_ = nullptr;
    m_cur_buffer_ = nullptr;
}

Log::~Log() {
    while(!m_flush_buffer_que_.empty()) {
        LogBuffer* buffer = m_flush_buffer_que_.front();
        fwrite(buffer->data(), 1, buffer->length(), m_fp_);
        m_flush_buffer_que_.pop();
    }
    fwrite(m_cur_buffer_->data(), 1, m_cur_buffer_->length(), m_fp_);
    fflush(m_fp_);
    fclose(m_fp_);

    m_run_ = false;
    m_cond_.notify_all();

}

void Log::SetLogFile(const std::string &file_name) {
    m_log_file_ = file_name;
    if(m_log_file_ == "/dev/stdout") {
        is_std_out_ = true;
    } else {
        is_std_out_ = false;
    }
}

void Log::Init(const std::string& file_name) {
    m_fp_ = fopen(file_name.c_str(), "w");
    assert(m_fp_ != nullptr);

    for(uint32_t i = 0; i < BUFFER_NUM; i++) {
        m_free_buffer_que_.push(&m_buffer_[i]);
    }
    m_cur_buffer_ = m_free_buffer_que_.front();

    std::unique_ptr<std::thread> new_thread(new std::thread(&Log::Run, this));
    write_thread_ = std::move(new_thread);
}

void Log::Append(const char *log_line, const size_t &len) {
    std::unique_lock<std::mutex> locker(m_mutex_);
    if(m_cur_buffer_->avail()) {
        m_cur_buffer_->append(log_line, len);
    } else {
        m_free_buffer_que_.pop();
        m_flush_buffer_que_.push(m_cur_buffer_);

        while(!m_free_buffer_que_.empty()) {
            m_cond_.notify_one();
            m_cond_.wait(locker);
        }

        m_cur_buffer_ = m_free_buffer_que_.front();
        m_cur_buffer_->append(log_line, len);
        m_cond_.notify_one();
    }
}

void Log::Run() {
    while(m_run_) {
        std::unique_lock<std::mutex> locker(m_mutex_);
        auto ret = m_cond_.wait_for(locker, std::chrono::seconds(5));

        if(ret == std::cv_status::timeout) {
            if(m_cur_buffer_->length() == 0) {
                continue;
            }
            fwrite(m_cur_buffer_->data(), 1, m_cur_buffer_->length(), m_fp_);
            m_cur_buffer_->reset();
            fflush(m_fp_);
        } else {
            bool empty = m_free_buffer_que_.empty();
            while(!m_flush_buffer_que_.empty()) {
                LogBuffer* temp_buffer = m_flush_buffer_que_.front();
                fwrite(temp_buffer->data(), 1, temp_buffer->length(), m_fp_);
                m_flush_buffer_que_.pop();
                temp_buffer->reset();
                m_free_buffer_que_.push(temp_buffer);
                fflush(m_fp_);
            }
            if(empty) {
                m_cond_.notify_one();
            }
        }
    }
}

void Log::Write(Log::LogLevel log_level, const char *file, const char *func, int line, const char *format, ...) {
    if(log_level > this_log_level_) {
        return;
    }
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    struct tm* sys_time = localtime(&(now.tv_sec));

    char m_data[4096];
    memset(m_data, 0, 4096);
    char* m_cur_ptr = m_data;

    sprintf(m_cur_ptr, "%d-%02d-%02d %02d:%02d:%02d",
            sys_time->tm_year + 1900, sys_time->tm_mon + 1, sys_time->tm_mday,
            sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec);
    m_cur_ptr += strlen(m_cur_ptr);

    switch (log_level) {
        case LogDebug: {
            sprintf(m_cur_ptr, " <DEBUG> ");
            break;
        }
        case LogWarning: {
            sprintf(m_cur_ptr, " <WARNING> ");
            break;
        }
        case LogError: {
            sprintf(m_cur_ptr, " <ERROR> ");
            break;
        }
        default: {
            return;
        }
    }
    m_cur_ptr += strlen(m_cur_ptr);

    sprintf(m_cur_ptr, "%s:%s:%d ", file, func, line);
    m_cur_ptr += strlen(m_cur_ptr);

    va_list valst;
    va_start(valst, format);

    vsnprintf(m_cur_ptr, sizeof(m_data) - (m_cur_ptr - m_data), format, valst);
    va_end(valst);

    m_cur_ptr += strlen(m_cur_ptr);

    if(is_std_out_) {
        printf("%s", m_data);
    } else {
        Append(m_data, m_cur_ptr - m_data);
    }
}
