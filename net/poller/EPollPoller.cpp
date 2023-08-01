//
// Created by fight on 2023/6/4.
//

#include "EPollPoller.h"
#include "../Channel.h"
#include "../../base/Logging.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

namespace
{
    const int kNew = -1; // 没在EventList中
    const int kAdded = 1; // 加入到EventList中
    const int kDeleted = 2; // 以前在但是被删了
}

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize){

    if(epollfd_ < 0){
        LOG_SYSFATAL << " EPollPoller::EPollPoller";
    }
}

EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

/*
 * Poller::poll()函数是Poller的核心功能，
 * 调用系统函数::poll()获得当前活动的IO事件，
 * 然后填充调用方传入的activeChannels，并返回return的时刻。
 */
Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_TRACE << "fd total count " << channels_.size();
    int numEvents = ::epoll_wait(epollfd_,          // 当前epoll占用的fd
                                 &*events_.begin(),    // events_数组第一个元素地址
                                 static_cast<int>(events_.size()), // events_数组元素个数
                                 timeoutMs);           // 超时时间
    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0) {
        LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);//把有IO事件的channel加入到activeChannels中, EventLoop统一处理
        if (implicit_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size()*2);
        }
    } else if (numEvents == 0) {
        LOG_TRACE << "nothing happened";
    } else {
        // error happens, log uncommon ones
        if (savedErrno != EINTR){
            errno = savedErrno;
            LOG_SYSERR << "EPollPoller::poll()";
        }
    }
    return now;
}


// 将Channel对应的fd上关注的事件注册、更新到epoll中，
// 用于维护和更新std::map<int, Channel*> channels_。
void EPollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events() << " index = " << index;

    if (index == kNew || index == kDeleted){ //没在或者曾经在epoll队列中，添加到epoll队列中
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew){  //没在epoll队列中
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;    // channels_中添加 fd->channel键值对
        }
        else { // index == kDeleted  // 曾经在epoll队列中
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(kAdded);     //修改index为已在队列中kAdded（1）
        update(EPOLL_CTL_ADD, channel);     // 注册事件
    }
    else{ //如果就在epoll队列中的，若没有关注事件了就暂时删除，如果有关注事件，就修改
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);   // 删除事件
            channel->set_index(kDeleted);     // channel标记kDeleted（2）暂时删除
        }
        else{
            update(EPOLL_CTL_MOD, channel);  // 更新事件
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == kAdded || index == kDeleted); //标志位必须是kAdded或者kDeleted
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    if (index == kAdded){
        update(EPOLL_CTL_DEL, channel); //从epoll队列中删除这个channel
    }
    channel->set_index(kNew);  //设置标志位是kNew(-1) ，相当于完全删除
}

const char* EPollPoller::operationToString(int op)
{
    switch (op)
    {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR op");
            return "Unknown Operation";
    }
}






//注册、删除事件核心
void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memZero(&event, sizeof event);
    event.events = channel->events();

    //data.ptr成员变量通常用于保存与文件描述符关联的数据指针，
    event.data.ptr = channel;         // 注意，event.data.ptr被赋值指向当前指针channel
    int fd = channel->fd();
    LOG_TRACE << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd << " event = { " << channel->eventsToString() << " }";

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL){
            LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        } else{
            LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
    }
}


/*
 * 遍历events_，找出当前有IO事件就绪的fd，把它对应的channel填入到activeChannels中。
 * 复杂度是O(N)，其中N是events_的个数。
 * 为了提前结束遍历，每找到一个活动的fd就递增i，当i等于numEvents时说明活动的fd找完，提前结束遍历。
 *
 * 当前活动事件events保存在channle中，供EvenetLoop中调用Channel::handleEvent()使用。
 *
 * 遍历events_[i]时，通过成员data.ptr得到对应的channel，
 * 是因为在注册调用update()时events_的成员data.ptr被赋值为当前channel。
 */
void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr); // 获取当前就绪IO事件对应的channel
        #ifndef NDEBUG
                int fd = channel->fd();
                ChannelMap::const_iterator it = channels_.find(fd);
                assert(it != channels_.end());
                assert(it->second == channel);
        #endif
        channel->set_revents(events_[i].events);  // 设置channel当前就绪的IO事件
        activeChannels->push_back(channel);       // 就绪IO事件channe放入activeChannels
    }
}
