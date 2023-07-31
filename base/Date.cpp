//
// Created by fight on 2023/5/21.
//

#include "Date.h"

namespace muduo
{
    namespace detail
    {

        char require_32_bit_integer_at_least[sizeof(int) >= sizeof(int32_t) ? 1 : -1];

// algorithm and explanation see:
// http://www.faqs.org/faqs/calendars/faq/part2/
// http://blog.csdn.net/Solstice

        int getJulianDayNumber(int year, int month, int day)
        {
            (void) require_32_bit_integer_at_least; // no warning please
            int a = (14 - month) / 12;
            int y = year + 4800 - a;
            int m = month + 12 * a - 3;
            return day + (153*m + 2) / 5 + y*365 + y/4 - y/100 + y/400 - 32045;
        }

        struct Date::YearMonthDay getYearMonthDay(int julianDayNumber)
        {
            int a = julianDayNumber + 32044;
            int b = (4 * a + 3) / 146097;
            int c = a - ((b * 146097) / 4);
            int d = (4 * c + 3) / 1461;
            int e = c - ((1461 * d) / 4);
            int m = (5 * e + 2) / 153;
            Date::YearMonthDay ymd{};
            ymd.day = e - ((153 * m + 2) / 5) + 1;
            ymd.month = m + 3 - 12 * (m / 10);
            ymd.year = b * 100 + d - 4800 + (m / 10);
            return ymd;
        }
    }  // namespace detail
    const int Date::kJulianDayOf1970_01_01 = detail::getJulianDayNumber(1970, 1, 1);
}  // namespace muduo

using namespace muduo::detail;

muduo::Date::Date(int year, int month, int day) :julianDayNumber_(getJulianDayNumber(year, month, day)) {}

muduo::Date::Date(const struct tm &t) : julianDayNumber_(getJulianDayNumber(
        t.tm_year+1900,
        t.tm_mon+1,
        t.tm_mday)){}

muduo::string muduo::Date::toIsoString() const {
    char buf[32];
    YearMonthDay ymd(yearMonthDay());
    snprintf(buf, sizeof buf, "%4d-%02d-%02d", ymd.year, ymd.month, ymd.day);
    return buf;
}

muduo::Date::YearMonthDay muduo::Date::yearMonthDay() const {
    return getYearMonthDay(julianDayNumber_);
}
