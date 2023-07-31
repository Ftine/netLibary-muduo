/*
//
// Created by ftion on 2023/5/29.
//
#include "../ThreadPool.h"
#include "../CountDownlatch.h"
#include "../CurrentThread.h"
#include "../Logging.h"

#include <stdio.h>
#include <unistd.h>  // usleep
#include <future>

void print()
{
    printf("tid=%d\n", muduo::CurrentThread::tid());
}

void printString(const std::string& str)
{
    LOG_INFO << str;
    usleep(100*1000);
}

void test(int maxSize)
{
    LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
    muduo::ThreadPool pool("MainThreadPool");
    pool.setMaxQueueSize(maxSize);
    pool.start(5);

    LOG_WARN << "Adding";
    pool.run(print);
    pool.run(print);
    for (int i = 0; i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.run(std::bind(printString, std::string(buf)));
    }
    LOG_WARN << "Done";

    muduo::CountDownLatch latch(1);
    pool.run(std::bind(&muduo::CountDownLatch::countDown, &latch));
    latch.wait();
    pool.stop();
}

*/
/*
 * Wish we could do this in the future.
void testMove()
{
  muduo::ThreadPool pool;
  pool.start(2);

  std::unique_ptr<int> x(new int(42));
  pool.run([y = std::move(x)]{ printf("%d: %d\n", muduo::CurrentThread::tid(), *y); });
  pool.stop();
}
*//*



class Mytask
{
    int delaySec_;
    std::promise<std::string> prom;
public:
    explicit Mytask(int delaySec) : delaySec_(delaySec) {}
    void run()
    {
        printf("tid: %d, Delay %d run...\n", muduo::CurrentThread::tid(), delaySec_);
        usleep(delaySec_ * 1000 * 1000);
        prom.set_value(std::string("Dealy ") + std::to_string(delaySec_));
    }

    auto getResult() { return prom.get_future(); }
};


int main()
{
    test(0);
    test(1);
    test(5);
    test(10);
    test(50);
}
*/


#include "../ThreadPool.h"
#include "../CountDownlatch.h"
#include "../CurrentThread.h"
#include "../Logging.h"

#include <future>

class Mytask
{
    int delaySec_;
    std::promise<std::string> prom;
public:
    explicit Mytask(int delaySec) : delaySec_(delaySec) {}
    void run()
    {
        printf("tid: %d, Delay %d run...\n", muduo::CurrentThread::tid(), delaySec_);
        usleep(delaySec_ * 1000 * 1000);
        prom.set_value(std::string("Dealy ") + std::to_string(delaySec_));
    }

    std::future<std::string> getResult() { return prom.get_future(); }
};

int main()
{
    muduo::ThreadPool pool("threadpool test");
    pool.start(10); // 保证后续任务都加入到队列中

    std::vector<std::shared_ptr<Mytask>> tasks;
    // 任务入队
    for(int i = 0; i < 5; i++){
        tasks.emplace_back( new Mytask(rand() % 5) ); // 随机0~5秒
        pool.run([=]{
            tasks[i]->run();
        });
    }
    // 异步等待结果
    for(int i = 0; i < 5; i++){
        pool.run([=]{
            printf("tid: %d, Task %s done!\n",
                   muduo::CurrentThread::tid(), tasks[i]->getResult().get().c_str());
        });
    }

    LOG_WARN << "Done";

    {  // 若去掉此处代码，并且任务数量小于队列容量，可能会导致队列中任务未全部执行完毕就关闭线程池退出程序。
        muduo::CountDownLatch latch(1);
        pool.run(std::bind(&muduo::CountDownLatch::countDown, &latch));
        latch.wait();
    }
    pool.stop();

    return 0;
}

