//
// Created by tcy on 2024/5/19.
//

#ifndef RTSP_TCP_CONNECTION_H
#define RTSP_TCP_CONNECTION_H

#include "usageenvironment.h"
#include "event.h"
#include "tcp_socket.h"
#include "buffer.h"

class TcpConnection {
public:
    typedef void(*DisconnectionCallback)(void*, int);

    TcpConnection(UsageEnvironment* env, int32_t sockfd);
    virtual ~TcpConnection();

    void SetDisconnectionCallback(DisconnectionCallback cb, void* arg);

protected:
    void EnableReadHandling();

    void EnableWriteHandling();

    void EnableErrorHandling();

    void DisableReadHandling();

    void DisableWriteHandling();

    void DisableErrorHandling();

    void HandleRead();

    virtual void HandleReadBytes();

    virtual void HandleWrite();

    virtual void HandleError();

    void HandleDisconnection();

private:
    static void ReadCallback(void* arg);

    static void WriteCallback(void* arg);

    static void ErrorCallback(void* arg);

protected:
    UsageEnvironment* m_env_;
    TcpSocket m_socket_;
    IOEvent* m_tcp_conn_io_event_;
    DisconnectionCallback m_disconnect_cb_{nullptr};
    void* m_arg_{nullptr};
    Buffer m_input_buffer_;
    Buffer m_output_buffer_;
    char m_buffer_[2048];
};

#endif //RTSP_TCP_CONNECTION_H
