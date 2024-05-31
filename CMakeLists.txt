cmake_minimum_required(VERSION 3.16.3)
project(rtsp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(rtsp src/main.cpp
        src/base/log.cpp
        include/base/log.h
        include/base/logbuffer.h
        include/net/event.h
        include/base/poller.h
        src/base/epollpoller.cpp
        include/base/epollpoller.h
        src/net/event_scheduler.cpp
        include/net/event_scheduler.h
        src/net/timer.cpp
        include/net/timer.h
        include/net/usageenvironment.h
        src/base/threadpool.cpp
        include/base/threadpool.h
        src/net/socket_ops.cpp
        include/net/socket_ops.h
        include/net/inet_address.h
        include/net/tcp_socket.h
        include/net/acceptor.h
        src/net/tcp_connection.cpp
        include/net/tcp_connection.h
        src/net/buffer.cpp
        include/net/buffer.h
        include/net/rtp.h
        include/net/rtp_instance.h
        include/net/media_source.h
        include/net/rtpsink.h
        src/net/media_session.cpp
        include/net/media_session.h
        src/net/rtsp_connection.cpp
        include/net/rtsp_connection.h
        include/net/rtsp_server.h
        src/net/rtsp_server.cpp
        src/net/h264media_source.cpp
        include/net/h264media_source.h
        src/net/h264rtp_sink.cpp
        include/net/h264rtp_sink.h
        src/net/aac_media_source.cpp
        include/net/aac_media_source.h
        src/net/aac_rtp_sink.cpp
        include/net/aac_rtp_sink.h)
