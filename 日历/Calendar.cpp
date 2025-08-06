#include "Calendar.h"

// 判断是否闰年
bool IsLeapYear(int year) {
    return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

// 返回指定年月的天数
int DaysInMonth(int year, int month) {
    int days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2 && IsLeapYear(year))
        return 29;
    return days[month - 1];
}

// 计算指定年月第一天是星期几，0=星期天，1=星期一，...6=星期六
int Weekday(int year, int month) {
    if (month == 1 || month == 2) {
        month += 12;
        year -= 1;
    }
    int q = 1; // 日期为1号
    int m = month;
    int K = year % 100;
    int J = year / 100;
    int h = (q + 13 * (m + 1) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
    int dayOfWeek = (h + 6) % 7; // 转换为0=星期天
    return dayOfWeek;
}

int Weekday(int year, int month, int day) {
    if (month == 1 || month == 2) {
        month += 12;
        year -= 1;
    }
    int q = day;    // 日期
    int m = month;
    int K = year % 100;
    int J = year / 100;
    int h = (q + 13 * (m + 1) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
    int dayOfWeek = (h + 6) % 7; // 转换为0=星期天
    return dayOfWeek;
}
