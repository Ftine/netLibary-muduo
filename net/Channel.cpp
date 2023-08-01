//
// Created by ftion on 2023/5/31.
//

#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"

#include <sstream>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

// 监听的文件描述符 和 属于的EventLoop
Channel::Channel(EventLoop* loop, int fd__)
        : loop_(loop),
          fd_(fd__),
          events_(0),
          revents_(0),
          index_(-1),
          logHup_(true),
          tied_(false),
          eventHandling_(false),
          addedToLoop_(false){
}


Channel::~Channel()
{
    assert(!eventHandling_);
    assert(!addedToLoop_);
    if (loop_->isInLoopThread())
    {
        assert(!loop_->hasChannel(this));
    }
}

void Channel::update()
{
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::remove() {
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::tie(const std::shared_ptr<void> & obj) {
    tie_ = obj;
    tied_ = true;
}

void Channel::handleEvent(muduo::Timestamp receiveTime) {
    std::shared_ptr<void> guard;
    if(tied_){
        guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(muduo::Timestamp receiveTime) {
    eventHandling_ = true;
    LOG_TRACE << reventsToString();

    if((revents_ & POLLHUP) && !(revents_ & POLLIN) ){
        if (logHup_){
            LOG_WARN << "fd =" << fd_ << "Channel::handle_event() POLLHUP";
        }
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & POLLNVAL){ //描述字不是一个打开的文件描述符
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL)){  //发生错误或者描述符不可打开
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)){ //关于读的事件
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT){   //关于写的事件
        if (writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}



string Channel::reventsToString() const
{
    return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
    return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN)
        oss << "IN ";
    if (ev & POLLPRI)
        oss << "PRI ";
    if (ev & POLLOUT)
        oss << "OUT ";
    if (ev & POLLHUP)
        oss << "HUP ";
    if (ev & POLLRDHUP)
        oss << "RDHUP ";
    if (ev & POLLERR)
        oss << "ERR ";
    if (ev & POLLNVAL)
        oss << "NVAL ";

    return oss.str();
}