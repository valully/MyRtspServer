
#include <iostream>

#include "../include/base/log.h"
#include "../include/net/usageenvironment.h"
#include "../include/base/threadpool.h"
#include "../include/net/event_scheduler.h"
#include "../include/net/event.h"
#include "../include/net/rtsp_server.h"
#include "../include/net/media_session.h"
#include "../include/net/inet_address.h"
#include "../include/net/h264media_source.h"
#include "../include/net/h264rtp_sink.h"
#include "../include/net/aac_media_source.h"
#include "../include/net/aac_rtp_sink.h"

int main(int argc, char* argv[])
{
    if(argc !=  3)
    {
        std::cout<<"Usage: "<<argv[0]<<" <h264 file> <aac file>"<<std::endl;
        return -1;
    }

    //Logger::setLogFile();
    Log::LogInstance()->SetLogLevel(Log::LogDebug);

    EventScheduler* scheduler = EventScheduler::CreateNew();
    ThreadPool* thread_pool = ThreadPool::CreateNew(8);
    UsageEnvironment* env = UsageEnvironment::CreateNew(scheduler, thread_pool);

    Ipv4Address ip_addr("0.0.0.0", 8554);
    RtspServer* server = RtspServer::CreateNew(env, ip_addr);
    MediaSession* session = MediaSession::CreateNew("live");
    MediaSource* video_source = H264MediaSource::CreateNew(env, argv[1]);
    RtpSink* video_rtp_sink = H264RtpSink::CreateNew(env, video_source);
    MediaSource* audio_source = AACMediaSource::CreateNew(env, argv[2]);
    RtpSink* audio_rtp_sink = AACRtpSink::CreateNew(env, audio_source);

    session->AddRtpSink(MediaSession::TrackId0, video_rtp_sink);
    session->AddRtpSink(MediaSession::TrackId1, audio_rtp_sink);

    server->AddMediaSession(session);
    server->Start();

    std::cout<<"Play the media using the URL \""<<server->GetUrl(session)<<"\""<<std::endl;

    env->Scheduler()->Loop();

    return 0;
}