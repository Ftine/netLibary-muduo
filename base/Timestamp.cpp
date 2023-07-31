//
// Created by ftion on 2023/5/18.
//


#include "Timestamp.h"
#include <sys/time.h>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

using namespace muduo;

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp is same size as int64_t");


/*
 * 将当前时间戳转换为 “秒数.微秒数” 的格式字符串。
 * 微秒数类型为int64_t在不同平台下定义不同，
 * 为跨平台方便直接使用 PRId64宏，来自动选择使用控制符%ld或者%lld
 */
string Timestamp::toString() const {
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

/*
 * 将时间戳转换为 “年:月:日 时:分:秒” 或者 “年:月:日 时:分:秒.微秒” 格式字符串。
 * 将当前时间秒数转换为日期，使用可重入的线程安全的函数gmtime_r()
 * （用户提供数据内存，而不是gmtime()中的全局变量数据）获取当前GMT时间，
 * 也可以使用localtime_r()获取当前系统本地时间。
 */
string Timestamp::toFormattedString(bool showMicroseconds) const {
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    struct tm tm_time;
    // gmtime_r()函数将当前系统时间转换为UTC时间下的tm结构体 -- 年、月、日、时、分、秒等时间信息。
    // 程序可以从tm对象中提取年、月、日、时、分、秒等时间信息，并将其输出到控制台上。
    // tm结构体中的月份从0开始计数，因此需要将其加1后输出。
    gmtime_r(&seconds, &tm_time);

    if(showMicroseconds){
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                 microseconds);
    }else{
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }


    return buf;
}


/*
 * gettimeoftoday()获取时间精确到微秒
 * struct timeval
 * {
 * __time_t tv_sec;          //  1970年1月1日 距今的秒数
 * __suseconds_t tv_usec;	//   微秒数部分
 * };
 * */

Timestamp Timestamp::now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

