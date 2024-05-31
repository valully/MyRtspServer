//
// Created by tcy on 2024/5/21.
//

#ifndef RTSP_RTSP_SERVER_H
#define RTSP_RTSP_SERVER_H

#include <map>
#include <vector>
#include <string>

#include "acceptor.h"
#include "media_session.h"
#include "rtsp_connection.h"

class RtspConnection;

class RtspServer {
public:
    static RtspServer* CreateNew(UsageEnvironment* env, const Ipv4Address& addr);

    RtspServer(UsageEnvironment* env, const Ipv4Address& addr);
    ~RtspServer();

    UsageEnvironment* Envir() const { return m_env_; }

    bool AddMediaSession(MediaSession* media_session);

    MediaSession* LookupMediaSession(const std::string& name);

    std::string GetUrl(MediaSession* media_session);

    void Start();

protected:
    void HandleNewConnection(int32_t connfd);
    static void DisconnectionCallback(void* arg, int32_t sockfd);
    void HandleDisconnection(int32_t sockfd);
    static void TriggerCallback(void* arg);
    void HandleDisconnectionList();

private:
    static void NewConnectionCallback(void* arg, int32_t connfd);

private:
    UsageEnvironment* m_env_;
    Acceptor* m_acceptor_;
    Ipv4Address m_addr_;

    std::map<std::string, MediaSession*> m_media_sessions_;
    std::map<int32_t, RtspConnection*> m_connections_;
    std::vector<int32_t> m_disconnected_list_;
    TriggerEvent* m_trigger_event_;
    std::mutex m_mtx_;
};


#endif //RTSP_RTSP_SERVER_H
