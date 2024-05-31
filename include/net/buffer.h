//
// Created by tcy on 2024/5/19.
//

#ifndef RTSP_BUFFER_H
#define RTSP_BUFFER_H

#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <sys/uio.h>

class Buffer {
public:
    static const uint32_t INI_BUFFER_SIZE;

    explicit Buffer() : m_buffer_size_(INI_BUFFER_SIZE), m_read_index_(0), m_write_index_(0) {
        m_buffer_ = (char*) malloc(INI_BUFFER_SIZE);
    }

    ~Buffer() {
        free(m_buffer_);
    }

    uint32_t ReadableBytes() const { return m_write_index_ - m_read_index_; }
    uint32_t WritableBytes() const { return m_buffer_size_ - m_write_index_; }
    uint32_t PrependableBytes() const { return m_read_index_; }
    const char* Peek() const { return m_buffer_ + m_read_index_; }

    const char* BeginWrite() const { return m_buffer_ + m_write_index_; }
    void HasWritten(uint32_t len) {
        assert(len <= WritableBytes());
        m_write_index_ += len;
    }

    void UnWrite(uint32_t len) {
        assert(len <= ReadableBytes());
        m_write_index_ -= len;
    }

    const char* FindCRLF() const {
        auto crlf = std::search(Peek(), BeginWrite(), KCRLF, KCRLF + 2);
        return crlf == BeginWrite() ? nullptr : crlf;
    }

    const char* FindCRLF(const char* start) {
        assert(Peek() <= start);
        assert(start <= BeginWrite());
        auto crlf = std::search(start, BeginWrite(), KCRLF, KCRLF + 2);
        return crlf == BeginWrite() ? nullptr : crlf;
    }

    const char* FindLastCRLF() const {
        auto crlf = std::find_end(Peek(), BeginWrite(), KCRLF, KCRLF + 2);
        return crlf == BeginWrite() ? nullptr : crlf;
    }

    void Retrieve(uint32_t len) {
        assert(len <= ReadableBytes());
        if(len < ReadableBytes()) {
            m_read_index_ += len;
        } else {
            RetrieveAll();
        }
    }

    void RetrieveAll() {
        m_read_index_ = 0;
        m_write_index_ = 0;
    }

    void RetrieveUntil(const char* end) {
        assert(Peek() <= end);
        assert(end <= BeginWrite());
        Retrieve(end - Peek());
    }

    void Append(const char* data, uint32_t len) {
        EnsureWritableBytes(len);
        std::copy(data, data + len, m_buffer_ + m_write_index_);
        HasWritten(len);
    }

    void Append(const void* data, uint32_t len) {
        Append((const char*)data, len);
    }

    void EnsureWritableBytes(uint32_t len) {
        if(WritableBytes() < len) {
            MakeSpace(len);
        }
        assert(WritableBytes() >= len);
    }

    void MakeSpace(uint32_t len);

    int32_t Read(int32_t fd);
    int32_t Write(int32_t fd);

private:
    const char* Begin() const { return m_buffer_; }

private:
    char* m_buffer_;
    uint32_t m_buffer_size_;
    uint32_t m_read_index_;
    uint32_t m_write_index_;

    static const char* KCRLF;
};


#endif //RTSP_BUFFER_H
