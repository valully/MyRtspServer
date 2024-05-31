//
// Created by tcy on 2024/5/19.
//

#include "../../include/net/buffer.h"
#include "../../include/net/socket_ops.h"

const uint32_t Buffer::INI_BUFFER_SIZE = 1024;
const char* Buffer::KCRLF = "\r\n";

void Buffer::MakeSpace(uint32_t len) {
    if(WritableBytes() + PrependableBytes() < len) {
        m_buffer_size_ = m_write_index_ + len;
        m_buffer_ = (char*) realloc(m_buffer_, m_buffer_size_);
    } else {
        auto readable = ReadableBytes();
        std::copy(m_buffer_ + m_read_index_, m_buffer_ + m_write_index_, m_buffer_);
        m_read_index_ = 0;
        m_write_index_ = m_read_index_ + readable;
        assert(readable == ReadableBytes());
    }
}

int32_t Buffer::Read(int32_t fd) {
    char extrabuf[65536];
    struct iovec vec[2];
    const uint32_t writable = WritableBytes();
    vec[0].iov_base = m_buffer_ + m_write_index_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    const int32_t iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    auto n = Sockets::Readv(fd, vec, iovcnt);
    if(n < 0) {
        return -1;
    } else if(n < writable) {
        m_write_index_ += n;
    } else {
        m_write_index_ = m_buffer_size_;
        Append(extrabuf, n - writable);
    }
    return n;
}

int32_t Buffer::Write(int32_t fd) {
    return Sockets::Write(fd, Peek(), ReadableBytes());
}