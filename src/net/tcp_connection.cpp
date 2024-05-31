//
// Created by tcy on 2024/5/27.
//

#include "../../include/net/tcp_connection.h"

TcpConnection::TcpConnection(UsageEnvironment *env, int32_t sockfd):
        m_env_(env), m_socket_(sockfd) {
    m_tcp_conn_io_event_ = IOEvent::CreateNew(sockfd, this);
    m_tcp_conn_io_event_->SetReadCallBack(ReadCallback);
    m_tcp_conn_io_event_->SetWriteCallback(WriteCallback);
    m_tcp_conn_io_event_->SetErrorCallback(ErrorCallback);
    m_tcp_conn_io_event_->EnableReadHandle();
    m_env_->Scheduler()->AddIOEvent(m_tcp_conn_io_event_);

    memset(m_buffer_, 0, sizeof(m_buffer_));
}

TcpConnection::~TcpConnection() {
    m_env_->Scheduler()->RemoveIOEvent(m_tcp_conn_io_event_);
    delete m_tcp_conn_io_event_;
}

void TcpConnection::SetDisconnectionCallback(TcpConnection::DisconnectionCallback cb, void *arg) {
    m_disconnect_cb_ = cb;
    m_arg_ = arg;
}

void TcpConnection::EnableReadHandling() {
    if(m_tcp_conn_io_event_->IsReadHandle()) {
        return;
    }
    m_tcp_conn_io_event_->EnableReadHandle();
    m_env_->Scheduler()->UpdateIOEvent(m_tcp_conn_io_event_);
}

void TcpConnection::EnableWriteHandling() {
    if(m_tcp_conn_io_event_->IsWriteHandle()) {
        return;
    }
    m_tcp_conn_io_event_->EnableWriteHandle();
    m_env_->Scheduler()->UpdateIOEvent(m_tcp_conn_io_event_);
}

void TcpConnection::EnableErrorHandling() {
    if(m_tcp_conn_io_event_->IsErrorHandle()) {
        return;
    }
    m_tcp_conn_io_event_->EnableErrorHandle();
    m_env_->Scheduler()->UpdateIOEvent(m_tcp_conn_io_event_);
}

void TcpConnection::DisableReadHandling() {
    if(!m_tcp_conn_io_event_->IsReadHandle()) {
        return;
    }
    m_tcp_conn_io_event_->DisableReadHandle();
    m_env_->Scheduler()->UpdateIOEvent(m_tcp_conn_io_event_);
}

void TcpConnection::DisableWriteHandling() {
    if(!m_tcp_conn_io_event_->IsWriteHandle()) {
        return;
    }
    m_tcp_conn_io_event_->DisableWriteHandle();
    m_env_->Scheduler()->UpdateIOEvent(m_tcp_conn_io_event_);
}

void TcpConnection::DisableErrorHandling() {
    if(!m_tcp_conn_io_event_->IsErrorHandle()) {
        return;
    }
    m_tcp_conn_io_event_->DisableErrorHandle();
    m_env_->Scheduler()->UpdateIOEvent(m_tcp_conn_io_event_);
}

void TcpConnection::HandleRead() {
    auto ret = m_input_buffer_.Read(m_socket_.GetSockFd());
    if(ret == 0) {
        LOG_DEBUG("client disconnect!\n");
        HandleDisconnection();
        return;
    }
    if(ret < 0) {
        LOG_ERROR("read error!\n");
        HandleDisconnection();
        return;
    }

    HandleReadBytes();
}

void TcpConnection::HandleReadBytes() {
    LOG_DEBUG("default read handle\n");
    m_input_buffer_.RetrieveAll();
}

void TcpConnection::HandleWrite() {
    LOG_DEBUG("default write handle\n");
    m_output_buffer_.RetrieveAll();
}

void TcpConnection::HandleError() {
    LOG_DEBUG("default error handle\n");
}

void TcpConnection::HandleDisconnection() {
    if(m_disconnect_cb_) {
        m_disconnect_cb_(m_arg_, m_socket_.GetSockFd());
    }
}

void TcpConnection::ReadCallback(void *arg) {
    auto tcp_conn = (TcpConnection*)arg;
    tcp_conn->HandleRead();
}

void TcpConnection::WriteCallback(void *arg) {
    auto tcp_conn = (TcpConnection*)arg;
    tcp_conn->HandleWrite();
}

void TcpConnection::ErrorCallback(void *arg) {
    auto tcp_conn = (TcpConnection*)arg;
    tcp_conn->HandleError();
}