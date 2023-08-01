//
// Created by fight on 2023/5/31.
//

#include "Poller.h"
#include "Channel.h"

using namespace muduo;
using namespace muduo::net;

Poller::Poller(EventLoop* loop)
        : ownerLoop_(loop)
{
}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

void Poller::assertInLoopThread() const {
    ownerLoop_->assertInLoopThread();
}

