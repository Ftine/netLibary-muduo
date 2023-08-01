//
// Created by fight on 2023/6/4.
//

#ifndef MUDUO_NET_POLLER_EPOLLPOLLER_H
#define MUDUO_NET_POLLER_EPOLLPOLLER_H

#include "../Poller.h"

#include <vector>

struct epoll_event;

namespace muduo{
    namespace net{
        class EPollPoller : public Poller{
        public:
            EPollPoller(EventLoop* loop);
            ~EPollPoller() override;

            // 三个重写的虚函数

            // 轮询 I/O 事件。
            Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
            // 更改监听 的 I/O 事件。
            void updateChannel(Channel* channel) override;
            // 当通道被销毁时移除它
            void removeChannel(Channel* channel) override;

        private:
            typedef std::vector<struct epoll_event> EventList;
            static const int kInitEventListSize = 16;
            static const char* operationToString(int op);

            // 把有IO事件的channel加入到activeChannels中, EventLoop统一处理
            void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

            void update(int operation,Channel* channel);

            int epollfd_;
            EventList events_;

        };
    }
}

#endif //MUDUO_NET_POLLER_EPOLLPOLLER_H
