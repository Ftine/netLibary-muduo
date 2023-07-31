//
// Created by fight on 2023/5/21.
//
#include "../TimeZone.h"

using namespace muduo;

int main()
{
    time_t now = time(NULL);
    struct tm t1 = *gmtime(&now);		// GMT+0
    struct tm t2 = *localtime(&now);  // +8 hours

    char time_buf[32]= {0};
    strftime(time_buf, 31, "%F %X", &t1);  printf("%s\n", time_buf);
    strftime(time_buf, 31, "%F %X", &t2);  printf("%s\n", time_buf);
    printf("%ld \n", now);

    TimeZone tzLondon("/usr/share/zoneinfo/Europe/London"); // GMT+0
    printf( "%ld \n", tzLondon.fromUtcTime(t1));   // shouid be equal now

    TimeZone tzChina(8*3600, "CST");	// GMT+8
    struct tm tm = tzChina.toLocalTime(now);
    strftime(time_buf, 31, "%F %X", &tm);  printf("%s\n", time_buf);  // shouid be equal t2's string
}
