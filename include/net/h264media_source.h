//
// Created by tcy on 2024/5/23.
//

#ifndef RTSP_H264MEDIA_SOURCE_H
#define RTSP_H264MEDIA_SOURCE_H

#include <string>

#include "usageenvironment.h"
#include "media_source.h"
#include "../base/threadpool.h"

class H264MediaSource : public MediaSource {
public:
    static H264MediaSource* CreateNew(UsageEnvironment* env, const std::string& file) {
        return new H264MediaSource(env, file);
    }

    H264MediaSource(UsageEnvironment* env, const std::string & file);
    ~H264MediaSource() {
        ::close(m_fd_);
    }

protected:
    virtual void ReadFrame();

private:
    int32_t GetFrameFromH264File(int32_t fd, uint8_t *frame, int32_t size);

private:
    std::string m_file_;
    int32_t m_fd_;
};


#endif //RTSP_H264MEDIA_SOURCE_H
