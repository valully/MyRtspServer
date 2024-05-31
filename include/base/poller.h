//
// Created by tcy on 2024/5/14.
//

#ifndef RTSP_POLLER_H
#define RTSP_POLLER_H

#include <map>
#include "../net/event.h"

typedef std::map<int32_t , IOEvent*> IOEventMap;

class Poller {
public:
    virtual ~Poller() = default;
    virtual bool AddIOEvent(IOEvent* event) = 0;
    virtual bool UpdateIOEvent(IOEvent* event) = 0;
    virtual bool RemoveIOEvent(IOEvent* event) = 0;
    virtual void HandleEvent() = 0;

protected:
    Poller() = default;
protected:
    IOEventMap m_event_map_;
};


#endif //RTSP_POLLER_H
