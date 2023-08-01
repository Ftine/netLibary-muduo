//
// Created by fight on 2023/5/31.
//

#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include "../base/Timestamp.h"
#include "EventLoop.h"

#include <map>
#include <vector>

namespace muduo{
    namespace net{
        class Channel;

        class Poller : noncopyable{
        public:
            typedef std::vector<Channel*> ChannelList;

            Poller(EventLoop* loop);
            virtual ~Poller();

            // 轮询 I/O 事件。
            // 在循环线程中调用。
            virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

            // 更改感兴趣的 I/O 事件。
            // 在循环线程中调用。
            virtual void updateChannel(Channel* channel) = 0;

            // 当通道被销毁时移除它
            // 在循环线程中调用
            virtual void removeChannel(Channel* channel) = 0;

            virtual bool hasChannel(Channel* channel) const;

            static Poller* newDefaultPoller(EventLoop* loop);

            void assertInLoopThread() const;


        protected:
            typedef std::map<int, Channel*> ChannelMap;
            ChannelMap channels_; // 当前管理的channel

        private:
            EventLoop* ownerLoop_;
        };
    }
}


#endif //MUDUO_NET_POLLER_H
