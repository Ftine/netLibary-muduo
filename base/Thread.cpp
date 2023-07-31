//
// Created by ftion on 2023/5/19.
//

#include "Thread.h"
#include "Timestamp.h"
#include "Exception.h"
#include "Logging.h"

#include <type_traits>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo{
    namespace detail{
        //内部调用系统调用::syscall(SYS_gettid)返回值作为线程id，并将线程id保存了C语言风格字符及其长度。
        pid_t gettid()
        {
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }

        void afterFork()
        {
            muduo::CurrentThread::t_cachedTid = 0;
            muduo::CurrentThread::t_threadName = "main";
            CurrentThread::tid();
            // no need to call pthread_atfork(NULL, NULL, &afterFork);
        }

        class ThreadNameInitializer
        {
        public:
            ThreadNameInitializer()
            {
                muduo::CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                // pthread_atfork()是一个POSIX线程库（pthread库）中的函数，用于在多线程程序中注册一个“fork处理程序”，
                // 以便在父进程fork()创建子进程之前或之后，对父进程、子进程以及子进程的所有线程进行相应的处理。
                // 具体来说，pthread_atfork()函数可以注册三个回调函数，
                // 分别在fork()创建子进程之前、之后以及在子进程中执行，以便在这些时刻执行相应的操作。
                pthread_atfork(NULL, NULL, &afterFork);
            }
        };

        ThreadNameInitializer init;

        struct ThreadData{
            typedef muduo::Thread::ThreadFunc ThreadFunc;
            ThreadFunc func_;
            string name_;
            pid_t* tid_;
            CountDownLatch* latch_;

            ThreadData(ThreadFunc func,
                       const string& name,
                       pid_t* tid,
                       CountDownLatch* latch)
                    : func_(std::move(func)),
                      name_(name),
                      tid_(tid),
                      latch_(latch)
            { }

            void runInThread()
            {
                *tid_ = muduo::CurrentThread::tid();
                tid_ = NULL;
                latch_->countDown();
                latch_ = NULL;

                muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
                ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
                try
                {
                    func_();
                    muduo::CurrentThread::t_threadName = "finished";
                }
                catch (const Exception& ex)
                {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason: %s\n", ex.what());
                    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
                    abort();
                }
                catch (const std::exception& ex)
                {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason: %s\n", ex.what());
                    abort();
                }
                catch (...)
                {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
                    throw; // rethrow
                }
            }
        };

        void* startThread(void* obj)
        {
            ThreadData* data = static_cast<ThreadData*>(obj);
            data->runInThread();
            delete data;
            return NULL;
        }

    } // namespace detail



    void CurrentThread::cacheTid()
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid = detail::gettid();
            t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
        }
    }

    bool CurrentThread::isMainThread()
    {
        return tid() == ::getpid();
    }

    void CurrentThread::sleepUsec(int64_t usec)
    {
        struct timespec ts = { 0, 0 };
        ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
        ::nanosleep(&ts, NULL);
    }

    AtomicInt32 Thread::numCreated_;

    Thread::Thread(ThreadFunc func, const string& n)
            : started_(false),
              joined_(false),
              pthreadId_(0),
              tid_(0),
              func_(std::move(func)),
              name_(n),
              latch_(1)
    {
        setDefaultName();
    }

    Thread::~Thread() {
        if (started_ && !joined_)
        {
            pthread_detach(pthreadId_);
        }

    }

    // 将数据封装在ThreadData类，执行pthread_create创建线程。
    // 实际执行过程在ThreadData类的runInThread()成员函数中。
    void Thread::start() {
        assert(!started_);
        started_ = true;
        // FIXME: move(func_)
        detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_, &latch_);
        if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
        {
            started_ = false;
            delete data; // or no delete?
            LOG_SYSFATAL << "Failed in pthread_create";
        }
        else
        {
            latch_.wait();
            assert(tid_ > 0);
        }

    }

    int Thread::join() {
        assert(started_);
        assert(!joined_);
        joined_ = true;
        return pthread_join(pthreadId_, NULL);
    }

    void Thread::setDefaultName() {
        int num = numCreated_.incrementAndGet();
        if (name_.empty())
        {
            char buf[32];
            snprintf(buf, sizeof buf, "Thread%d", num);
            name_ = buf;
        }

    }

}
