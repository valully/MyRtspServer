//
// Created by tcy on 2024/5/21.
//

#include "../../include/net/rtsp_server.h"

RtspServer* RtspServer::CreateNew(UsageEnvironment *env, const Ipv4Address &addr) {
    return new RtspServer(env, addr);
}

RtspServer::RtspServer(UsageEnvironment *env, const Ipv4Address &addr) : m_env_(env), m_addr_(addr) {
    m_acceptor_ = Acceptor::CreateNew(env, addr);
    assert(m_acceptor_);
    m_acceptor_->SetNewConnectionCallback(NewConnectionCallback, this);

    m_trigger_event_ = TriggerEvent::CreateNew(this);
    m_trigger_event_->SetCallBack(TriggerCallback);
}

RtspServer::~RtspServer() {
    delete m_trigger_event_;
    delete m_acceptor_;
}

void RtspServer::HandleNewConnection(int32_t connfd) {
    RtspConnection* conn = RtspConnection::CreateNew(this, connfd);
    conn->SetDisconnectionCallback(DisconnectionCallback, this);
    m_connections_.insert({connfd, conn});
}

void RtspServer::DisconnectionCallback(void *arg, int32_t sockfd) {
    auto rtsp_server = (RtspServer*)arg;
    rtsp_server->HandleDisconnection(sockfd);
}

void RtspServer::HandleDisconnection(int32_t sockfd) {
    std::lock_guard<std::mutex> locker(m_mtx_);
    m_disconnected_list_.push_back(sockfd);
    m_env_->Scheduler()->AddTriggerEvent(m_trigger_event_);
}

bool RtspServer::AddMediaSession(MediaSession *media_session) {
    if(m_media_sessions_.find(media_session->Name()) != m_media_sessions_.end()) {
        return false;
    }
    m_media_sessions_.insert({media_session->Name(), media_session});
    return true;
}

MediaSession* RtspServer::LookupMediaSession(const std::string &name) {
    auto iter = m_media_sessions_.find(name);
    if(iter == m_media_sessions_.end()) {
        return nullptr;
    }
    return iter->second;
}

std::string RtspServer::GetUrl(MediaSession *media_session) {
    char url[200];
    snprintf(url, sizeof(url), "rtsp://%s:%d/%s",
             Sockets::GetLocalIp().c_str(), m_addr_.GetPort(), media_session->Name().c_str());
    return url;
}

void RtspServer::Start() {
    m_acceptor_->Listen();
}

void RtspServer::TriggerCallback(void *arg) {
    auto rtsp_server = (RtspServer*)arg;
    rtsp_server->HandleDisconnectionList();
}

void RtspServer::HandleDisconnectionList() {
    std::lock_guard<std::mutex> locker(m_mtx_);
    for(auto& iter: m_disconnected_list_) {
        auto it = m_connections_.find(iter);
        assert(it != m_connections_.end());
        delete it->second;
        m_connections_.erase(iter);
    }
    m_disconnected_list_.clear();
}

void RtspServer::NewConnectionCallback(void *arg, int32_t connfd) {
    auto rtsp_server = (RtspServer*)arg;
    rtsp_server->HandleNewConnection(connfd);
}