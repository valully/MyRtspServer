//
// Created by tcy on 2024/5/27.
//

#ifndef RTSP_AAC_MEDIA_SOURCE_H
#define RTSP_AAC_MEDIA_SOURCE_H

#include "media_source.h"

class AACMediaSource : public MediaSource{
private:
    struct AdtsHeader {
        uint32_t syncword_;  //12 bit 同步字 '1111 1111 1111'，说明一个ADTS帧的开始
        uint32_t id_;  //1 bit MPEG 标示符， 0 for MPEG-4，1 for MPEG-2
        uint32_t layer_;  //2 bit 总是'00'
        uint32_t protection_absent_;  //1 bit 1表示没有crc，0表示有crc
        uint32_t profile_;  //1 bit 表示使用哪个级别的AAC
        uint32_t sampling_freq_index_; //4 bit 表示使用的采样频率
        uint32_t private_bit_;  //1 bit
        uint32_t channel_cfg_;  //3 bit 表示声道数
        uint32_t original_copy_;  //1 bit
        uint32_t home_;  //1 bit

        uint32_t copyright_identification_bit_;   //1 bit
        uint32_t copyright_identification_start_;  //1 bit
        uint32_t aac_frame_length_;  //13 bit 一个ADTS帧的长度包括ADTS头和AAC原始流
        uint32_t adts_buffer_fullness_;  //11 bit 0x7FF 说明是码率可变的码流

        /* number_of_raw_data_blocks_in_frame
        * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
        */
        uint32_t number_of_raw_data_block_in_frame_; //2 bit
    };

    bool ParseAdtsHeader(uint8_t* in, AdtsHeader* res);

    int32_t GetFrameFromAACFile(int32_t fd, uint8_t* buf, uint32_t size);

public:
    static AACMediaSource* CreateNew(UsageEnvironment* env, const std::string& file);

    AACMediaSource(UsageEnvironment* env, const std::string& file);

    virtual ~AACMediaSource();

protected:
    virtual void ReadFrame() override;

private:
    std::string m_file_;
    int32_t m_fd_;
    AdtsHeader m_adts_header_;
};


#endif //RTSP_AAC_MEDIA_SOURCE_H
