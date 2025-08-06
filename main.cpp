#define _CRT_SECURE_NO_WARNINGS
#define txt_name "D:\\visual studio 2022\\vs文件\\日历\\schedules.txt"
#include <graphics.h>
#include <conio.h>
#include <string>
#include <ctime>
#include <sstream>
#include <vector>
#include <windows.h>
#include <map>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include "Calendar.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

void DrawCalendar(int year, int month);
void DrawYearView(int year);
void DrawWeekView(int year, int month, int day);
std::string FormatDate(int y, int m, int d);

enum class ViewMode { MONTH, YEAR, WEEK };

struct Button {
    int x, y, w, h;
    wchar_t label[32];
};

Button btnPrev;
Button btnNext;
Button btnBack;
Button btnExit;
int inputX, inputY, inputW, inputH;

int year, month;
bool inputMode = false;
std::wstring inputStr;
ViewMode currentMode = ViewMode::MONTH;
int weekDay = 1; // 当前周视图显示的日期

std::map<std::string, std::vector<std::string>> scheduleMap; // 提醒功能

// 全局同步和线程控制变量
std::atomic<bool> running(true);
std::mutex scheduleMutex;

// 保存提醒，访问加锁
void SaveSchedules(const std::string& filename) {
    std::lock_guard<std::mutex> lock(scheduleMutex);
    std::ofstream ofs(filename);
    for (const auto& entry : scheduleMap) {
        if (entry.second.empty()) {
            continue;  // 跳过没有提醒内容的日期
        }
        ofs << entry.first << "\n";
        for (const auto& item : entry.second) {
            ofs << "  " << item << "\n";
        }
    }
}


// 读取提醒，访问加锁
bool FileExists(const std::string& filename) {
    std::ifstream ifs(filename);
    return ifs.good();
}

bool IsValidDateLine(const std::string& line) {
    if (line.size() != 10) return false;
    if (line[4] != '-' || line[7] != '-') return false;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!isdigit(line[i])) return false;
    }
    return true;
}

void LoadSchedules(const std::string& filename) {
    std::lock_guard<std::mutex> lock(scheduleMutex);

    if (!FileExists(filename)) {
        std::ofstream ofs(filename);  // 创建空文件
    }

    std::ifstream ifs(filename);
    if (!ifs.is_open()) return;

    std::string line, date;
    while (std::getline(ifs, line)) {
        if (!line.empty() && line[0] != ' ' && IsValidDateLine(line)) {
            date = line;
        }
        else if (!date.empty() && line.size() > 2 && line[0] == ' ') {
            scheduleMap[date].push_back(line.substr(2));
        }
    }
}

std::string FormatDate(int y, int m, int d) {
    char buf[16];
    sprintf(buf, "%04d-%02d-%02d", y, m, d);
    return std::string(buf);
}

// 提醒线程函数
void ReminderThreadFunc() {
    while (running) {
        time_t now = time(nullptr);
        tm* ltm = localtime(&now);
        char nowStr[20];
        strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M", ltm);
        std::string nowKey(nowStr);

        {
            std::lock_guard<std::mutex> lock(scheduleMutex);
            for (const auto& entry : scheduleMap) {
                const std::string& date = entry.first;
                const std::vector<std::string>& reminders = entry.second;

                for (const auto& reminder : reminders) {
                    if (reminder.size() >= 5) {
                        std::string timePart = reminder.substr(0, 5);  // "HH:MM"
                        std::string fullTime = date + " " + timePart;

                        if (fullTime == nowKey) {
                            std::wstring wmsg(reminder.begin(), reminder.end());
                            std::wstring wtitle = L"日程提醒";
                            MessageBox(nullptr, wmsg.c_str(), wtitle.c_str(), MB_OK | MB_ICONINFORMATION);
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void UpdateButtonLabels() {
    if (currentMode == ViewMode::MONTH) {
        wcscpy(btnPrev.label, L"上一月");
        wcscpy(btnNext.label, L"下一月");
    }
    else if (currentMode == ViewMode::YEAR) {
        wcscpy(btnPrev.label, L"上一年");
        wcscpy(btnNext.label, L"下一年");
    }
    else if (currentMode == ViewMode::WEEK) {
        wcscpy(btnPrev.label, L"上一周");
        wcscpy(btnNext.label, L"下一周");
    }
}

void UpdateUILayout() {
    int margin = 20;
    int btnWidth = WINDOW_WIDTH / 6;
    int btnHeight = 30;
    int inputWidth = WINDOW_WIDTH / 3;
    int inputHeight = 30;

    btnPrev = { margin, margin, btnWidth, btnHeight, L"" };
    btnNext = { WINDOW_WIDTH - margin - btnWidth, margin, btnWidth, btnHeight, L"" };

    inputX = (WINDOW_WIDTH - inputWidth) / 2;
    inputY = margin;
    inputW = inputWidth;
    inputH = inputHeight;

    UpdateButtonLabels();

    if (currentMode == ViewMode::WEEK) {
        // 放在右下角
        btnBack.w = 120;
        btnBack.h = 30;
        btnBack.x = WINDOW_WIDTH - margin - btnBack.w;
        btnBack.y = WINDOW_HEIGHT - margin - btnBack.h;
        wcscpy(btnBack.label, L"返回月视图");
    }
    else {
        // 不显示按钮
        btnBack.x = -200;
        btnBack.y = -200;
        btnBack.w = 0;
        btnBack.h = 0;
        btnBack.label[0] = L'\0';
    }

    if (currentMode == ViewMode::WEEK) {
        btnBack.w = 120;
        btnBack.h = 30;
        btnBack.x = WINDOW_WIDTH - margin - btnBack.w;  // 右下角
        btnBack.y = WINDOW_HEIGHT - margin - btnBack.h;
        wcscpy(btnBack.label, L"返回月视图");
    }
    else {
        btnBack.x = -200;
        btnBack.y = -200;
        btnBack.w = 0;
        btnBack.h = 0;
        btnBack.label[0] = L'\0';
    }

    // 新增退出按钮，固定放左下角
    btnExit.w = 120;
    btnExit.h = 30;
    btnExit.x = margin;  // 左边距离20
    btnExit.y = WINDOW_HEIGHT - margin - btnExit.h;  // 底部距离20
    wcscpy(btnExit.label, L"退出程序");
}


void DrawButton(const Button& btn) {
    setlinecolor(BLACK);
    setfillcolor(RGB(60, 60, 60));
    solidrectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    settextcolor(WHITE);
    settextstyle(20, 0, L"微软雅黑");

    int tw = textwidth(btn.label);
    int th = textheight(btn.label);
    int tx = btn.x + (btn.w - tw) / 2;
    int ty = btn.y + (btn.h - th) / 2;
    outtextxy(tx, ty, btn.label);
}

void DrawInputBox(const std::wstring& text, bool editing) {
    setlinecolor(BLACK);
    setfillcolor(editing ? RGB(60, 60, 60) : RGB(230, 230, 230));
    solidrectangle(inputX, inputY, inputX + inputW, inputY + inputH);

    settextcolor(editing ? WHITE : RGB(100, 100, 100));
    settextstyle(20, 0, L"微软雅黑");
    outtextxy(inputX + 5, inputY + 5, text.c_str());
}

void DrawCalendarWithUI() {
    cleardevice();

    DrawButton(btnPrev);
    DrawButton(btnNext);
    DrawButton(btnExit);

    if (currentMode == ViewMode::WEEK) {
        DrawButton(btnBack);
    }

    std::wstring inputDisplay;
    if (inputMode) {
        inputDisplay = inputStr + L"_";
    }
    else {
        if (currentMode == ViewMode::YEAR) {
            wchar_t buf[32];
            swprintf(buf, 32, L"%d 年", year);
            inputDisplay = buf;
        }
        else {
            wchar_t buf[32];
            swprintf(buf, 32, L"%d 年 %d 月", year, month);
            inputDisplay = buf;
        }
    }
    DrawInputBox(inputDisplay, inputMode);

    if (currentMode == ViewMode::MONTH) {
        DrawCalendar(year, month);
    }
    else if (currentMode == ViewMode::YEAR) {
        DrawYearView(year);
    }
    else if (currentMode == ViewMode::WEEK) {
        DrawWeekView(year, month, weekDay);
    }
}

void DrawCalendar(int year, int month) {
    int offsetX = 50;
    int offsetY = 110;
    int cellW = (WINDOW_WIDTH - 2 * offsetX) / 7;
    int cellH = (WINDOW_HEIGHT - offsetY - 50) / 6;

    const wchar_t* weekNames[7] = { L"日", L"一", L"二", L"三", L"四", L"五", L"六" };
    settextstyle(24, 0, L"微软雅黑");
    settextcolor(WHITE);
    for (int i = 0; i < 7; i++) {
        outtextxy(offsetX + i * cellW + cellW / 4, offsetY - 35, weekNames[i]);
    }

    setlinecolor(BLACK);
    int rows = 6;
    for (int r = 0; r <= rows; r++) {
        line(offsetX, offsetY + r * cellH, offsetX + 7 * cellW, offsetY + r * cellH);
    }
    for (int c = 0; c <= 7; c++) {
        line(offsetX + c * cellW, offsetY, offsetX + c * cellW, offsetY + rows * cellH);
    }

    int firstDay = Weekday(year, month);
    int days = DaysInMonth(year, month);

    time_t now = time(nullptr);
    tm* ltm = localtime(&now);
    int todayYear = 1900 + ltm->tm_year;
    int todayMonth = 1 + ltm->tm_mon;
    int todayDay = ltm->tm_mday;

    setbkmode(TRANSPARENT);
    int day = 1;
    for (int i = firstDay; day <= days; i++) {
        int r = i / 7;
        int c = i % 7;

        int x = offsetX + c * cellW;
        int y = offsetY + r * cellH;

        bool isToday = (year == todayYear && month == todayMonth && day == todayDay);

        if (isToday) {
            setfillcolor(RGB(100, 200, 100));
            solidroundrect(x + 5, y + 5, x + cellW - 5, y + cellH - 5, 10, 10);
            settextcolor(BLACK);
        }
        else {
            settextcolor(WHITE);
        }

        wchar_t buf[4];
        swprintf(buf, 4, L"%d", day++);
        outtextxy(x + cellW / 3, y + cellH / 4, buf);
    }
}

void DrawYearView(int year) {
    settextcolor(WHITE);

    const int cols = 4;
    const int rows = 3;
    const int gapX = 20;
    const int gapY = 20;
    const int marginTop = 60;

    int usableWidth = WINDOW_WIDTH - 2 * gapX;
    int usableHeight = WINDOW_HEIGHT - marginTop - 40;

    int monthWidth = (usableWidth - (cols - 1) * gapX) / cols;
    int monthHeight = (usableHeight - (rows - 1) * gapY) / rows;

    int cellW = monthWidth / 7;
    int cellH = monthHeight / 7;

    const wchar_t* weekDays[7] = { L"日", L"一", L"二", L"三", L"四", L"五", L"六" };

    for (int m = 1; m <= 12; m++) {
        int col = (m - 1) % cols;
        int row = (m - 1) / cols;
        int startX = gapX + col * (monthWidth + gapX);
        int startY = marginTop + row * (monthHeight + gapY);

        settextstyle(16, 0, L"微软雅黑");
        wchar_t monthTitle[16];
        swprintf(monthTitle, 16, L"%d月", m);
        int monthTitleW = textwidth(monthTitle);
        outtextxy(startX + (monthWidth - monthTitleW) / 2, startY, monthTitle);

        settextstyle(12, 0, L"微软雅黑");
        for (int i = 0; i < 7; ++i) {
            outtextxy(startX + i * cellW + 3, startY + 20, weekDays[i]);
        }

        int firstWeekday = Weekday(year, m);
        int days = DaysInMonth(year, m);

        settextcolor(WHITE);
        int day = 1;
        for (int i = firstWeekday; day <= days; ++i) {
            int r = i / 7;
            int c = i % 7;
            wchar_t buf[4];
            swprintf(buf, 4, L"%d", day++);
            outtextxy(startX + c * cellW + 3, startY + 25 + r * cellH + cellH / 4, buf);
        }
    }

    settextstyle(20, 0, L"微软雅黑");
}

void DrawWeekView(int year, int month, int day) {
    // cleardevice();

    // 设置和月视图相同的背景色
    setfillcolor(RGB(30, 30, 30));
    solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    // 之后绘制标题、日期框、日期文本等内容，颜色根据需要设置成白色或其他
    settextstyle(24, 0, L"微软雅黑");
    settextcolor(WHITE);

    wchar_t title[64];
    swprintf(title, 64, L"%d 年 %d 月 %d 日 - 星期视图", year, month, day);
    int tw = textwidth(title);
    outtextxy((WINDOW_WIDTH - tw) / 2, 10, title);

    int wday = Weekday(year, month, day);
    int startDay = day - wday;

    int displayYear = year;
    int displayMonth = month;
    int daysInMonth = DaysInMonth(year, month);

    if (startDay < 1) {
        displayMonth = month - 1;
        if (displayMonth < 1) {
            displayMonth = 12;
            displayYear--;
        }
        daysInMonth = DaysInMonth(displayYear, displayMonth);
        startDay = daysInMonth + startDay;
    }

    const wchar_t* weekNames[7] = { L"日", L"一", L"二", L"三", L"四", L"五", L"六" };

    int marginTop = 60;
    int marginLeft = 80;
    int cellW = (WINDOW_WIDTH - 2 * marginLeft) / 7;
    int cellH = WINDOW_HEIGHT - marginTop - 50;

    setlinecolor(WHITE);
    for (int i = 0; i < 7; i++) {
        int x0 = marginLeft + i * cellW;
        int y0 = marginTop;
        int x1 = x0 + cellW;
        int y1 = y0 + cellH;

        // rectangle(x0, y0, x1, y1);

        settextstyle(20, 0, L"微软雅黑");
        outtextxy(x0 + cellW / 3, y0 + 10, weekNames[i]);

        int displayDay = startDay + i;
        int displayY = displayYear;
        int displayM = displayMonth;

        int maxDays = DaysInMonth(displayY, displayM);
        if (displayDay > maxDays) {
            displayDay -= maxDays;
            displayM++;
            if (displayM > 12) {
                displayM = 1;
                displayY++;
            }
        }

        wchar_t buf[16];
        swprintf(buf, 16, L"%d-%02d-%02d", displayY, displayM, displayDay);
        outtextxy(x0 + cellW / 4, y0 + 40, buf);

        // 如果该天有提醒，提示显示
        std::string dateKey = FormatDate(displayY, displayM, displayDay);
        if (!scheduleMap[dateKey].empty()) {
            outtextxy(x0 + 10, y0 + 70, L"有提醒");
        }
    }
    // 按钮绘制
    DrawButton(btnPrev);
    DrawButton(btnNext);
    DrawButton(btnBack);
}

int main() {
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
    LoadSchedules(txt_name);
    UpdateUILayout();

    time_t now = time(nullptr);
    tm* ltm = localtime(&now);
    year = 1900 + ltm->tm_year;
    month = 1 + ltm->tm_mon;

    DrawCalendarWithUI();

    // 启动提醒线程
    std::thread reminderThread(ReminderThreadFunc);

    while (true) {
        while (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                int x = msg.x, y = msg.y;

                // 输入框点击
                if (x >= inputX && x <= inputX + inputW &&
                    y >= inputY && y <= inputY + inputH) {
                    inputMode = true;
                    inputStr.clear();
                    DrawCalendarWithUI();
                    continue;
                }

                // 退出按钮点击
                if (x >= btnExit.x && x <= btnExit.x + btnExit.w &&
                    y >= btnExit.y && y <= btnExit.y + btnExit.h) {
                    running = false;
                    goto exit_main_loop;
                }

                // 在周视图添加提醒时加锁写scheduleMap
                if (currentMode == ViewMode::WEEK) {
                    int marginLeft = 80;
                    int marginTop = 60;   // 和周视图绘制顶部对齐
                    int cellW = (WINDOW_WIDTH - 2 * marginLeft) / 7;
                    int cellH = WINDOW_HEIGHT - marginTop - 50;  // 跟绘制时同高度

                    int wday = Weekday(year, month, weekDay);
                    int startDay = weekDay - wday;
                    for (int i = 0; i < 7; i++) {
                        int x0 = marginLeft + i * cellW;
                        int x1 = x0 + cellW;
                        int y0 = marginTop;
                        int y1 = y0 + cellH;

                        if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
                            int dY = year, dM = month, dD = startDay + i;
                            if (dD < 1) {
                                dM--;
                                if (dM < 1) { dM = 12; dY--; }
                                dD += DaysInMonth(dY, dM);
                            }
                            else if (dD > DaysInMonth(dY, dM)) {
                                dD -= DaysInMonth(dY, dM);
                                dM++;
                                if (dM > 12) { dM = 1; dY++; }
                            }
                            std::string dateKey = FormatDate(dY, dM, dD);
                            wchar_t buf[128];
                            InputBox(buf, 128, L"请输入提醒（如 14:00 开会）", L"添加提醒");
                            std::wstring ws(buf);
                            if (!ws.empty()) {
                                std::lock_guard<std::mutex> lock(scheduleMutex);
                                scheduleMap[dateKey].push_back(std::string(ws.begin(), ws.end()));
                                DrawCalendarWithUI();
                            }
                            break;  // 添加成功后跳出循环，避免重复处理
                        }
                    }
                }

                // btnPrev 点击
                if (x >= btnPrev.x && x <= btnPrev.x + btnPrev.w &&
                    y >= btnPrev.y && y <= btnPrev.y + btnPrev.h) {
                    if (currentMode == ViewMode::MONTH) {
                        month--;
                        if (month < 1) {
                            month = 12;
                            year--;
                        }
                    }
                    else if (currentMode == ViewMode::YEAR) {
                        year--;
                    }
                    else if (currentMode == ViewMode::WEEK) {
                        // 上一周
                        weekDay -= 7;
                        if (weekDay < 1) {
                            month--;
                            if (month < 1) {
                                month = 12;
                                year--;
                            }
                            weekDay += DaysInMonth(year, month);
                        }
                    }
                    UpdateButtonLabels();
                    DrawCalendarWithUI();
                    continue;
                }

                // btnNext 点击
                if (x >= btnNext.x && x <= btnNext.x + btnNext.w &&
                    y >= btnNext.y && y <= btnNext.y + btnNext.h) {
                    if (currentMode == ViewMode::MONTH) {
                        month++;
                        if (month > 12) {
                            month = 1;
                            year++;
                        }
                    }
                    else if (currentMode == ViewMode::YEAR) {
                        year++;
                    }
                    else if (currentMode == ViewMode::WEEK) {
                        weekDay += 7;
                        int maxDay = DaysInMonth(year, month);
                        if (weekDay > maxDay) {
                            weekDay -= maxDay;
                            month++;
                            if (month > 12) {
                                month = 1;
                                year++;
                            }
                        }
                    }
                    UpdateButtonLabels();
                    DrawCalendarWithUI();
                    continue;
                }

                // 当前视图点击
                if (currentMode == ViewMode::MONTH) {
                    // 计算日期格区域范围
                    int offsetX = 50;
                    int offsetY = 110;
                    int cellW = (WINDOW_WIDTH - 2 * offsetX) / 7;
                    int cellH = (WINDOW_HEIGHT - offsetY - 50) / 6;

                    int firstDay = Weekday(year, month);
                    int days = DaysInMonth(year, month);

                    bool clickedDay = false;
                    for (int dayIndex = 0; dayIndex < days; ++dayIndex) {
                        int i = firstDay + dayIndex;
                        int r = i / 7;
                        int c = i % 7;
                        int x0 = offsetX + c * cellW;
                        int y0 = offsetY + r * cellH;
                        if (x >= x0 && x <= x0 + cellW &&
                            y >= y0 && y <= y0 + cellH) {
                            weekDay = dayIndex + 1;
                            currentMode = ViewMode::WEEK;
                            UpdateButtonLabels();
                            UpdateUILayout();  // ⭐别忘了更新按钮坐标
                            DrawCalendarWithUI();
                            clickedDay = true;
                            break;
                        }
                    }

                    if (clickedDay) {
                        continue;  // 点击了日期格，进入周视图，跳过后续逻辑
                    }

                    // 否则是点击空白区域，进入年视图
                    if (y >= inputY + inputH + 50 && y <= WINDOW_HEIGHT) {
                        currentMode = ViewMode::YEAR;
                        UpdateButtonLabels();
                        UpdateUILayout();
                        DrawCalendarWithUI();
                        continue;
                    }

                    continue;
                }
                else if (currentMode == ViewMode::YEAR) {
                    // 点击月份切换回月视图
                    const int cols = 4;
                    const int rows = 3;
                    const int gapX = 20;
                    const int gapY = 20;
                    const int marginTop = 60;

                    int usableWidth = WINDOW_WIDTH - 2 * gapX;
                    int usableHeight = WINDOW_HEIGHT - marginTop - 40;

                    int monthWidth = (usableWidth - (cols - 1) * gapX) / cols;
                    int monthHeight = (usableHeight - (rows - 1) * gapY) / rows;

                    int cellW = monthWidth / 7;
                    int cellH = monthHeight / 7;

                    for (int m = 1; m <= 12; m++) {
                        int col = (m - 1) % cols;
                        int row = (m - 1) / cols;
                        int startX = gapX + col * (monthWidth + gapX);
                        int startY = marginTop + row * (monthHeight + gapY);

                        // 月份标题区域大致范围，用于判断点击
                        int titleHeight = 20;
                        int titleWidth = textwidth((std::to_wstring(m) + L"月").c_str()) + 10;

                        if (x >= startX && x <= startX + monthWidth &&
                            y >= startY && y <= startY + monthHeight) {
                            // 这里简单判断点击整个月份格子，直接切换到对应月视图
                            month = m;
                            currentMode = ViewMode::MONTH;
                            UpdateButtonLabels();
                            DrawCalendarWithUI();
                            break;
                        }
                    }
                    continue;
                }
                else if (currentMode == ViewMode::WEEK) {
                    // 点击返回月视图按钮
                    if (x >= btnBack.x && x <= btnBack.x + btnBack.w &&
                        y >= btnBack.y && y <= btnBack.y + btnBack.h) {
                        currentMode = ViewMode::MONTH;
                        UpdateButtonLabels();
                        UpdateUILayout();
                        DrawCalendarWithUI();
                        continue;
                    }
                }

                // 点击空白区域切换到年视图（仅月视图才切换）
                if (currentMode == ViewMode::MONTH) {
                    if (x >= 0 && x <= WINDOW_WIDTH &&
                        y >= inputY + inputH + 50 && y <= WINDOW_HEIGHT) {
                        currentMode = ViewMode::YEAR;
                        UpdateButtonLabels();
                        DrawCalendarWithUI();
                        continue;
                    }
                }

                // 关闭输入框
                if (inputMode) {
                    inputMode = false;
                    DrawCalendarWithUI();
                }
            }
        }

        // 处理键盘输入，输入年月
        if (_kbhit() && inputMode) {
            int ch = _getch();
            if (ch == 13) { // 回车
                std::wistringstream wiss(inputStr);
                int yInput = 0, mInput = 0;
                wiss >> yInput >> mInput;
                if (yInput >= 1900 && yInput <= 2100 && mInput >= 1 && mInput <= 12) {
                    year = yInput;
                    month = mInput;
                }
                inputMode = false;
                inputStr.clear();
                DrawCalendarWithUI();
            }
            else if (ch == 8) { // 退格
                if (!inputStr.empty()) {
                    inputStr.pop_back();
                    DrawCalendarWithUI();
                }
            }
            else if ((ch >= '0' && ch <= '9') || ch == ' ') {
                inputStr.push_back((wchar_t)ch);
                DrawCalendarWithUI();
            }
            else if (ch == 27) { // ESC取消输入
                inputMode = false;
                inputStr.clear();
                DrawCalendarWithUI();
            }
        }

        Sleep(20);
    }

exit_main_loop:
    // 关闭程序时停止线程
    running = false;
    reminderThread.join();

    SaveSchedules(txt_name);
    closegraph();
    return 0;
}
