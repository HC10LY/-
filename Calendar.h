#pragma once

// �ж��Ƿ�����
bool IsLeapYear(int year);

// ����ָ�����µ�����
int DaysInMonth(int year, int month);

// ����ָ�����µ�һ�������ڼ���0=�����죬1=����һ��...6=������
int Weekday(int year, int month);
int Weekday(int year, int month, int day);