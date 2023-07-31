//
// Created by ftion on 2023/5/19.
//

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include "Types.h"

namespace muduo{
    // 封装pthread
    namespace CurrentThread{
        // 用于获取当前线程的id，并将线程id保存为C语言风格的字符串
        //
        // 外部声明

        /*
         * 在 C++ 中，`__thread` 是一个线程局部存储（Thread Local Storage，TLS）关键字。
         * 使用 `__thread` 修饰的变量是线程局部的，每个线程都有一份独立的拷贝，互相独立，互不干扰。
         */
        extern __thread int t_cachedTid;
        extern __thread char t_tidString[32];
        extern __thread int t_tidStringLength;
        extern __thread const char* t_threadName;
        void cacheTid();

        inline int tid()
        {
            // __builtin_expect()是GCC编译器中的一个内建函数（built-in function），
            // 用于告诉编译器一个条件语句中哪种情况更有可能发生。
            // __builtin_expect (long exp, long c)
            if (__builtin_expect(t_cachedTid == 0, 0))
            {
                cacheTid();
            }
            return t_cachedTid;
        }

        inline const char* tidString() // for logging
        { return t_tidString;}

        inline int tidStringLength() // for logging
        { return t_tidStringLength;}

        inline const char* name()
        { return t_threadName;}

        bool isMainThread();

        void sleepUsec(int64_t usec);  // for testing

        string stackTrace(bool demangle);


    }// namespace CurrentThread
}// namespace muduo


#endif //MUDUO_BASE_CURRENTTHREAD_H
