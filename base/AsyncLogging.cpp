//
// Created by ftion on 2023/5/29.
//

#include "AsyncLogging.h"
#include "LogFile.h"
#include "Timestamp.h"

#include <cstdio>

using namespace muduo;

AsyncLogging::AsyncLogging(const string& basename,
                           off_t rollSize,
                           int flushInterval)
        : flushInterval_(flushInterval),
          running_(false),
          basename_(basename),
          rollSize_(rollSize),
          thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
          latch_(1),
          mutex_(),
          cond_(mutex_),
          currentBuffer_(new Buffer),
          nextBuffer_(new Buffer),
          buffers_()
{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}


/*前端生成一条日志消息调用AsyncLogging::append()函数。
 * 如果当前缓冲（currentBuffer_）剩余空间足够，直接将日志消息最佳到当前缓冲中，这是最常见的情况；
 * 否则，将它移到buffers_队列中，并试图将另一块预备的缓冲（nextBuffer_）移用（std::move）为当前缓冲，
 * 然后追加日志消息并通知后端写入日志数据。以上情况均在临界区之内没有耗时操作，主要是交换指针，运行时间为常数。
 * 如果前端日志写入过快，两块缓冲区都已用完，只能再分配一块新的buffer，作为当前缓冲，这是极少发生的情况。
 */
void AsyncLogging::append(const char* logline, int len)
{
    muduo::MutexLockGuard lock(mutex_);
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        buffers_.push_back(std::move(currentBuffer_));

        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            currentBuffer_.reset(new Buffer); // Rarely happens
        }
        currentBuffer_->append(logline, len);
        cond_.notify();
    }
}
/*
*在消费者内部运行前，先准备两块缓冲区、一个缓冲队列。
*在临界区等待条件触发，条件是超时、前端写满一个或多个buffer。
 * 注意，这里是非常规的条件变量通知用于，这里没有使用while循，而且等待时间有上限。条件触发后，
 * 主要是对缓冲区的移动、交换操作，从而减少临界区的操作时间，另外还准备好待写入文件的缓冲队列。
*进入非临界区的后，能够正常操作待写入文件的缓冲队列中的所有日志数据。
 * 处理日志消息堆积的异常情况，写入日志后端，重新填充缓冲区等。
*/
void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_, rollSize_, false);

    // 内部缓冲区、缓冲队列
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    //缓冲队列
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        // 一、等待唤醒，临界区操作。准备待写入日志的缓冲队列
        {
            muduo::MutexLockGuard lock(mutex_);
            if (buffers_.empty())  // unusual usage!
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        // 声明队列有数据
        assert(!buffersToWrite.empty());

        //  (1) 日志待写入文件buffersToWrite队列超限（短时间堆积，多为异常情况）
        if (buffersToWrite.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toFormattedString().c_str(),
                     buffersToWrite.size()-2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

        // (2) buffersToWrited队列中的日志消息交给后端写入
        for (const auto& buffer : buffersToWrite)
        {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffer->data(), buffer->length());
        }

        // // (3) 将buffersToWrited队列中的buffer重新填充newBuffer1、newBuffer2
        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
