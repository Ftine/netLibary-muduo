//
// Created by fight on 2023/5/21.
//

#ifndef MUDUO_BASE_DATE_H
#define MUDUO_BASE_DATE_H

#include "copyable.h"
#include "Types.h"

struct tm; // 时间变量结构体

/*#include <time.h>
struct tm {
int tm_sec;     *//* 秒 – 取值区间为[0,59] *//*
int tm_min;     *//* 分 - 取值区间为[0,59] *//*
int tm_hour;    *//* 时 - 取值区间为[0,23] *//*
int tm_mday;    *//* 一个月中的日期 - 取值区间为[1,31] *//*
int tm_mon;     *//* 月份（从一月开始，0代表一月） 取值区间为[0,11] *//*
int tm_year;    *//* 年份，其值等于实际年份减去1900 *//*
int tm_wday;    *//* 星期 – 取值区间[0,6]，其中0代表星期天，1代表星期一，以此类推 *//*
int tm_yday;    *//* 从每年的1月1日开始的天数 – 取值区间[0,365]，其中0代表1月1日，1代表1月2日，以此类推 *//*

int tm_isdst;   *//* 夏令时标识符- 实行夏令时为正，不实行夏令时为0；不了解情况时为负。*//*

long int tm_gmtoff;  *//*指定了日期变更线东面时区中UTC东部时区正秒数或UTC西部时区的负秒数*//*
const char *tm_zone; *//*当前时区的名字(与环境变量TZ有关)*//*
};*/



namespace muduo{
    /*
     * 私有成员变量julianDayNumber_存储着这个天数，
     * 定义结构体YearMonthDay方便两者之间的转换，
     * toIosString把日期转换为string
     */
    class Date : public muduo::copyable
    {
    public:

        struct YearMonthDay{
            int year; // [1900..2500]
            int month;  // [1..12]
            int day;  // [1..31]
        };

        static const int kDaysPerWeek = 7;
        static const int kJulianDayOf1970_01_01;

        Date() : julianDayNumber_(0) {}
        Date(int year, int month, int day);
        explicit Date(int julianDayNum) : julianDayNumber_(julianDayNum) {}
        explicit Date(const struct tm&);

        void swap(Date& that) { std::swap(julianDayNumber_, that.julianDayNumber_); }
        bool valid() const { return julianDayNumber_ > 0; }

        /// 格式化字符串yyyy-mm-dd
        string toIsoString() const;

        struct YearMonthDay yearMonthDay() const;

        int year() const  { return yearMonthDay().year; }
        int month() const { return yearMonthDay().month;}
        int day() const   { return yearMonthDay().day;  }

        // [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
        int weekDay() const{ return (julianDayNumber_+1) % kDaysPerWeek; }

        int julianDayNumber() const { return julianDayNumber_; }

    private:
        int julianDayNumber_;
    };

    inline bool operator<(Date x, Date y){
        return x.julianDayNumber() < y.julianDayNumber();
    }

    inline bool operator==(Date x, Date y){
        return x.julianDayNumber() == y.julianDayNumber();
    }

} // namespace muduo

#endif //MUDUO_BASE_DATE_H
