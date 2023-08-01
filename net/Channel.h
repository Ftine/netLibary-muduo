//
// Created by ftion on 2023/5/31.
//

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"


#include <functional>
#include <memory>

namespace muduo{
    namespace net{
        class EventLoop;

        class Channel : noncopyable{
        public:
            typedef std::function<void()> EventCallback;  // 事件回调函数定义
            typedef std::function<void(Timestamp)> ReadEventCallback; // 读事件回调函数定义


            Channel(EventLoop* loop, int fd);
            ~Channel();

            void handleEvent(Timestamp receiveTime);

            //设置可读、可写、关闭、出错的事件回调函数
            void setReadCallback(ReadEventCallback cb)
            { readCallback_ = std::move(cb); }
            void setWriteCallback(EventCallback cb)
            { writeCallback_ = std::move(cb); }
            void setCloseCallback(EventCallback cb)
            { closeCallback_ = std::move(cb); }
            void setErrorCallback(EventCallback cb)
            { errorCallback_ = std::move(cb); }

            // 将此通道绑定到由 shared_ptr 管理的所有者对象，
            // 防止所有者对象在 handleEvent 中被销毁。
            void tie(const std::shared_ptr<void>&);

            int fd() const { return fd_; }
            int events() const { return events_; }   //返回注册的事件
            void set_revents(int revt) { revents_ = revt; }  // 设置发生的事件:poller中调用

            bool isNoneEvent() const { return events_ == kNoneEvent; } // 判断是否注册了事件

            void enableReading()  { events_ |=  kReadEvent;  update(); } // 注册可读事件
            void disableReading() { events_ &= ~kReadEvent;  update(); } // 注销可读事件
            void enableWriting()  { events_ |=  kWriteEvent; update(); } // 注册可写事件
            void disableWriting() { events_ &= ~kWriteEvent; update(); } // 注销可写事件
            void disableAll()     { events_ =   kNoneEvent;  update(); } // 注销所有事件


             // 是否有可写事件
            bool isWriting() const { return events_ & kWriteEvent; }
             // 是否有可读事件
            bool isReading() const { return events_ & kReadEvent; }

            // for Poller
            int index() { return index_; }
            void set_index(int idx) { index_ = idx; }


            // for debug
            string reventsToString() const;
            string eventsToString() const;

            void doNotLogHup() { logHup_ = false; }

            EventLoop* ownerLoop() { return loop_; }
            void remove();

        private:
            static string eventsToString(int fd, int ev);

            void update();    // 注册事件后更新到EventLoop
            void handleEventWithGuard(Timestamp receiveTime);  // 加锁的事件处理

            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            EventLoop* loop_;     // channel所属的EventLoop
            const int  fd_;       // channel负责的文件描述符，但不负责关闭该文件描述符
            int        events_;   // 注册的事件
            int        revents_;  // epoller || poller返回接收到的就绪的事件
            int        index_;    // 表示在Poll的事件数组中的序号
            bool       logHup_;

            /*
             * tie_变量是一个std::shared_ptr<EventLoop>类型的智能指针，用于存储通道所属的事件循环对象。
             * 当调用Channel类的成员函数处理事件时，通常需要将事件分发给对应的事件循环对象进行处理。
             * 通过使用tie_变量，可以方便地获取通道所属的事件循环对象，从而实现事件的分发和处理。
             */
            std::weak_ptr<void> tie_;

            //tied_变量是一个布尔类型的标志变量，用于标识通道是否关联了EventLoop对象。
            bool tied_;

            bool eventHandling_;                // 是否处于处理事件中
            bool addedToLoop_;
            ReadEventCallback readCallback_;  // 读事件回调
            EventCallback writeCallback_;     // 写事件回调
            EventCallback closeCallback_;     // 关闭事件回调
            EventCallback errorCallback_;     // 出错事件回调

        };

    } // net
} // muduo
#endif //MUDUO_NET_CHANNEL_H
