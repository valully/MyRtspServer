//
// Created by tcy on 2024/5/12.
//

#ifndef RTSP_LOG_H
#define RTSP_LOG_H

#include <queue>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <sys/time.h>
#include <cstdarg>
#include "logbuffer.h"

#define LOG_ERROR(format, ...) \
    do {\
        if(Log::LogLevel::LogError <= Log::LogInstance()->GetLogLevel()) { \
            Log::LogInstance()->Write(Log::LogLevel::LogError, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_WARNING(format, ...) \
    do { \
        if(Log::LogLevel::LogWarning <= Log::LogInstance()->GetLogLevel()) { \
            Log::LogInstance()->Write(Log::LogLevel::LogWarning, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_DEBUG(format, ...) \
    do { \
        if(Log::LogLevel::LogDebug <= Log::LogInstance()->GetLogLevel()) { \
            Log::LogInstance()->Write(Log::LogLevel::LogDebug, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__); \
        } \
    } while(0)

#define BUFFER_NUM 4

class Log {
public:
    enum LogLevel{
        LogError,
        LogWarning,
        LogDebug
    };

    static Log* LogInstance() {
        static Log log;
        return &log;
    }

    void Init(const std::string& file_name);

    void SetLogFile(const std::string& file_name);
    std::string GetLogFile() { return m_log_file_; }

    void SetLogLevel(LogLevel log_level) { this_log_level_ = log_level; }
    LogLevel GetLogLevel() { return this_log_level_; }
    void Write(LogLevel log_level, const char* file, const char* func, int line, const char* format, ...);

    void Append(const char* log_line, const size_t & len);
    void Run();
private:
    Log();
    ~Log();

    //char m_data_[4096];
    //char* m_cur_ptr;

    LogLevel this_log_level_;
    std::string m_log_file_;
    bool is_std_out_{true};

    std::unique_ptr<std::thread> write_thread_;
    std::mutex m_mutex_;
    std::condition_variable m_cond_;
    FILE* m_fp_;
    bool m_run_{true};

    LogBuffer m_buffer_[BUFFER_NUM];
    LogBuffer* m_cur_buffer_;

    std::queue<LogBuffer*> m_free_buffer_que_;
    std::queue<LogBuffer*> m_flush_buffer_que_;
};


#endif //RTSP_LOG_H
