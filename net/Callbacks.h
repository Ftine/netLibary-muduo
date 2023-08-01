//
// Created by fight on 2023/5/30.
//

#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include "../base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo{
    /*
     * std::placeholders::_1是一个占位符，通常在使用函数对象（Function Object）或函数指针时使用。
     * 函数对象是一种可调用对象，它可以像函数一样被调用，并且可以封装数据或状态。
     * 函数指针是指向函数的指针，它可以作为参数传递给其他函数。
     *
     * 占位符std::placeholders::_1表示函数或函数对象的第一个参数。 _2 第二个 以此类推
     * 通常在使用函数对象或函数指针时，需要将其作为参数传递给其他函数，
     * 但是有时候需要在传递参数时只指定其中某些参数，而另一些参数需要在稍后指定。
     * 这时就可以使用占位符来占位，等到稍后需要时再填充实际的参数。
     */
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;

    template<typename T>
    inline T* get_pointer(const std::shared_ptr<T>& ptr)
    {
        return ptr.get();
    }

    template<typename T>
    inline T* get_pointer(const std::unique_ptr<T>& ptr)
    {
        return ptr.get();
    }

    // 类型转化 上型 转 下型
    // see License in muduo/base/Types.h
    template<typename To, typename From>
    inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
        if (false)
        {
            implicit_cast<From*, To*>(0);
        }

    #ifndef NDEBUG
            assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
    #endif
            return ::std::static_pointer_cast<To>(f);
    }

    namespace net
    {

        // 所有客户端可见的回调都在这里。

        class Buffer;
        class TcpConnection;
        typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
        typedef std::function<void()> TimerCallback;
        typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
        typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
        typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
        typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

        // the data has been read to (buf, len)
        typedef std::function<void (const TcpConnectionPtr&,
                                    Buffer*,
                                    Timestamp)> MessageCallback;

        void defaultConnectionCallback(const TcpConnectionPtr& conn);
        void defaultMessageCallback(const TcpConnectionPtr& conn,
                                    Buffer* buffer,
                                    Timestamp receiveTime);

    }  // namespace net



}



#endif //MUDUO_NET_CALLBACKS_H
