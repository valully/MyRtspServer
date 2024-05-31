//
// Created by tcy on 2024/5/18.
//

#ifndef RTSP_ACCEPTOR_H
#define RTSP_ACCEPTOR_H

#include "usageenvironment.h"
#include "event.h"
#include "inet_address.h"
#include "tcp_socket.h"

class Acceptor {
public:
    typedef void(*NewConnectionCallback)(void* data, int32_t connfd);

    static Acceptor* CreateNew(UsageEnvironment* env, const Ipv4Address& addr) {
        return new Acceptor(env, addr);
    }

    Acceptor(UsageEnvironment* env, const Ipv4Address& addr):
        m_env_(env), m_addr_(addr), m_socket_(Sockets::CreateTcpSock()) {
        m_socket_.SetReuseAddr(1);
        m_socket_.Bind(m_addr_);
        m_accept_io_event_ = IOEvent::CreateNew(m_socket_.GetSockFd(), this);
        m_accept_io_event_->SetReadCallBack(ReadCallback);
        m_accept_io_event_->EnableReadHandle();
    }
    ~Acceptor() {
        if(m_listening_) {
            m_env_->Scheduler()->RemoveIOEvent(m_accept_io_event_);
        }
        delete m_accept_io_event_;
    }

    bool GetListenning() const { return m_listening_; }
    void Listen() {
        m_listening_ = true;
        m_socket_.Listen(1024);
        m_env_->Scheduler()->AddIOEvent(m_accept_io_event_);
    }
    void SetNewConnectionCallback(NewConnectionCallback cb, void* arg) {
        m_new_cb_ = cb;
        m_arg_ = arg;
    }

private:
    static void ReadCallback(void* arg) {
        auto acceptor = (Acceptor*)arg;
        acceptor->HandleRead();
    }
    void HandleRead() {
        auto connfd = m_socket_.Accept();
        LOG_DEBUG("client connect fd: %d\n", connfd);
        if(m_new_cb_) {
            m_new_cb_(m_arg_, connfd);
        }
    }

private:
    UsageEnvironment* m_env_;
    Ipv4Address m_addr_;
    IOEvent* m_accept_io_event_{nullptr};
    TcpSocket m_socket_;
    bool m_listening_{false};
    NewConnectionCallback m_new_cb_{nullptr};
    void* m_arg_;
};

#endif //RTSP_ACCEPTOR_H
