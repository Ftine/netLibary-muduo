#include "../EventLoop.h"
#include "../../base/Thread.h"
#include "../../base/Logging.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <thread>

using namespace muduo;
using namespace muduo::net;

EventLoop* g_loop;

void callback()
{
    printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    EventLoop anotherLoop;
}

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
    loop.runAfter(1.0, callback);
    loop.loop();
}

//int main()
//{
//    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
//
//    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
//    printf("this one\n");
//    EventLoop loop;
//    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
//    printf("this two");
//
//    Thread thread(threadFunc);
//    thread.start();
//
//    loop.loop();
//}


int main(){
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

    // 一个线程只能有一个EventLoop
    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

    loop.runInLoop([]{
        LOG_INFO << "runinloop in IO thread.";
    });

    std::thread([&]{
        CurrentThread::sleepUsec(3*1000*1000); // 3 sec
        loop.runInLoop([]{

            LOG_INFO << "runinloop in other thread.";
        });
    }).detach();

    std::thread([&]{

        loop.runInLoop([]{
            CurrentThread::sleepUsec(7*1000*1000); // 3 sec
            LOG_INFO << "runinloop in other aaaathread.";
        });
    }).detach();

    loop.loop();
    return 0;

}




