//
// Created by ftion on 2023/5/25.
//

#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H

#include "noncopyable.h"
#include "StringPiece.h"
#include "Types.h"
#include <cassert>
#include <cstring> // memcpy

namespace muduo{
    namespace detail{
        const int kSmallBuffer = 4000;
        const int kLargeBuffer = 4000*1000;

        /*
         * FixedBuffer类模板对象是一个固定大小的缓存区，用于存储需要输出的日志字符串.
         * FixedBuffer的实现为一个非类型参数的模板类（对于非类型参数而言，目前C++的标准仅支持整型、枚举、指针类型和引用类型），
         * 传入一个非类型参数SIZE表示缓冲区的大小。
         * 通过成员 data_首地址、cur_指针、end()完成对缓冲区的各项操作。
         * 通过.append()接口把日志内容添加到缓冲区来。
         */

        template<int SIZE>
        class FixedBuffer : muduo::noncopyable{
        public:
            FixedBuffer()
                    : cur_(data_)
            {
                setCookie(cookieStart);
            }

            ~FixedBuffer()
            {
                setCookie(cookieEnd);
            }
            void append(const char* /*restrict*/ buf, size_t len)
            {
                // FIXME: append partially
                if (implicit_cast<size_t>(avail()) > len)
                {
                    memcpy(cur_, buf, len);
                    cur_ += len;
                }
            }

            const char* data() const { return data_; }
            int length() const { return static_cast<int>(cur_ - data_); }

            // write to data_ directly
            char* current() { return cur_; }
            int avail() const { return static_cast<int>(end() - cur_); }
            void add(size_t len) { cur_ += len; }

            void reset() { cur_ = data_; }
            void bzero() { memZero(data_, sizeof data_); }

            // for used by GDB
            const char* debugString();
            void setCookie(void (*cookie)()) { cookie_ = cookie; }
            // for used by unit test
            string toString() const { return string(data_, length()); }
            StringPiece toStringPiece() const { return StringPiece(data_, length()); }

        private:
            const char* end() const { return data_ + sizeof data_; }
            // Must be outline function for cookies.
            static void cookieStart(); // 置空
            static void cookieEnd();   // 置空

            // cookie_成员变量的默认值为一个空的函数指针，
            // 即void (*)()类型的默认值为nullptr。
            // 在使用FixedBuffer对象时，可以通过调用setCookie()方法来设置回调函数指针，
            // 以便在输出缓存区中的数据时调用该函数

            /*
             * cookie_ 用于保存一个回调函数指针。
             * 该回调函数指针可以在FixedBuffer对象输出缓存区中的数据时被调用，以便将缓存区中的数据输出到日志文件中。
             * 该回调函数是一个不带参数和返回值的函数指针，类型为void (*)()。
             * 在FixedBuffer对象输出缓存区中的数据时，
             * 如果该回调函数指针不为空，则会调用该函数指针所指向的函数，将缓存区中的数据输出到日志文件中。
             */
            void (*cookie_)(){}; //空的函数指针

            char data_[SIZE]{};
            // data 首指针
            // end() 返回末尾的指针
            // cur_ 当前指针
            char* cur_;
        };
    } // namespace detail

    // LogStream类，主要作用是重载<<操作符，将基本类型的数据格式成字符串通过append接口存入Buffer中。
    class LogStream : muduo::noncopyable{
        typedef LogStream self;
    public:
        typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

        self& operator<<(bool v)
        {
            buffer_.append(v ? "1" : "0", 1);
            return *this;
        }

        self& operator<<(short);
        self& operator<<(unsigned short);
        self& operator<<(int);
        self& operator<<(unsigned int);
        self& operator<<(long);
        self& operator<<(unsigned long);
        self& operator<<(long long);
        self& operator<<(unsigned long long);

        self& operator<<(const void*);

        self& operator<<(float v)
        {
            *this << static_cast<double>(v);
            return *this;
        }
        self& operator<<(double);
        // self& operator<<(long double);

        self& operator<<(char v)
        {
            buffer_.append(&v, 1);
            return *this;
        }

        // self& operator<<(signed char);
        // self& operator<<(unsigned char);

        self& operator<<(const char* str)
        {
            if (str)
            {
                buffer_.append(str, strlen(str));
            }
            else
            {
                buffer_.append("(null)", 6);
            }
            return *this;
        }

        self& operator<<(const unsigned char* str)
        {
            return operator<<(reinterpret_cast<const char*>(str));
        }

        self& operator<<(const string& v)
        {
            buffer_.append(v.c_str(), v.size());
            return *this;
        }

        self& operator<<(const StringPiece& v)
        {
            buffer_.append(v.data(), v.size());
            return *this;
        }

        self& operator<<(const Buffer& v)
        {
            *this << v.toStringPiece();
            return *this;
        }

        void append(const char* data, int len) { buffer_.append(data, len); }
        const Buffer& buffer() const { return buffer_; }
        void resetBuffer() { buffer_.reset(); }

    private:
        Buffer buffer_;

        void staticCheck();

        template<typename T>
        void formatInteger(T);

        static const int kMaxNumericSize = 32;
    };

    /*
     * muduo::LogStream本身不支持格式化，
     * 定义了一个不影响其状态的class Fmt类。
     * 将一个数值类型数据转化成一个长度不超过32位的字符串对象Fmt，
     * 并重载了支持Fmt输出到LogStream的<<操作符模板函数。
     */
    class Fmt // : noncopyable
    {
    public:
        template<typename T>
        Fmt(const char* fmt, T val);

        const char* data() const { return buf_; }
        int length() const { return length_; }

    private:
        char buf_[32];
        int length_;
    };

    inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
    {
        s.append(fmt.data(), fmt.length());
        return s;
    }

// Format quantity n in SI units (k, M, G, T, P, E).
// The returned string is atmost 5 characters long.
// Requires n >= 0
    string formatSI(int64_t n);

// Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
// The returned string is atmost 6 characters long.
// Requires n >= 0
    string formatIEC(int64_t n);
}



#endif //MUDUO_BASE_LOGSTREAM_H
