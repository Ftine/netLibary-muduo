//
// Created by fight on 2023/6/4.
//

#ifndef MUDUO_NET_POLLER_POLLPOLLER_H
#define MUDUO_NET_POLLER_POLLPOLLER_H
#include "../Poller.h"

#include <vector>

struct pollfd;

namespace muduo{
    namespace net{
        class PollPoller : public Poller{

        public:

            PollPoller(EventLoop* loop);
            ~PollPoller() override;

            // 轮询 I/O 事件。
            Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
            // 更改监听 的 I/O 事件
            void updateChannel(Channel* channel) override;
            // 当通道被销毁时移除它
            void removeChannel(Channel* channel) override;

        private:
            // 把有IO事件的channel加入到activeChannels中, EventLoop统一处理
            void fillActiveChannels(int numEvents,
                                    ChannelList* activeChannels) const;

            typedef std::vector<struct pollfd> PollFdList;
            PollFdList pollfds_;

        };
    }
}

#endif //MUDUO_NET_POLLER_POLLPOLLER_H
