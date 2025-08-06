#pragma once

// 判断是否闰年
bool IsLeapYear(int year);

// 返回指定年月的天数
int DaysInMonth(int year, int month);

// 计算指定年月第一天是星期几，0=星期天，1=星期一，...6=星期六
int Weekday(int year, int month);
int Weekday(int year, int month, int day);