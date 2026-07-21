#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <commdlg.h>
#pragma comment(lib, "comctl32.lib")
#define IDC_LISTBOX         101
#define IDC_BTN_ADD         102
#define IDC_BTN_EDIT        103
#define IDC_BTN_DELETE      104
#define IDC_BTN_SAVE        105
#define IDC_BTN_LOAD        106
#define TIMER_ID            1
#define IDD_TASK_DIALOG     200
#define IDC_EDIT_TITLE      201
#define IDC_EDIT_DESC       202
#define IDC_EDIT_HOUR       203
#define IDC_EDIT_MINUTE     204
#define IDC_COMBO_AMPM      205
#define IDOK_CUSTOM         206
#define IDCANCEL_CUSTOM     207
struct Task {
    std::wstring title;
    std::wstring description;
    int hour;          
    int minute;
    bool notified;
    int lastNotifiedDay;
    Task() : hour(0), minute(0), notified(false), lastNotifiedDay(-1) {}
    int get12Hour() const {
        if (hour == 0) return 12;      // منتصف الليل
        if (hour <= 12) return hour;
        return hour - 12;
    }
    bool isAM() const {
        return hour < 12;
    }
    std::wstring getAMPM() const {
        return isAM() ? L"AM" : L"PM";
    }
    void setFrom12Hour(int hour12, bool isAM) {
        if (hour12 == 12) {
            hour = isAM ? 0 : 12;}
        else {
            hour = isAM ? hour12 : (hour12 + 12);
        }
    }
};
HINSTANCE hInst;
HWND hMainWindow;
HWND hListBox;
HWND hTaskDialog = NULL;
HWND hComboAMPM = NULL;
std::vector<Task> tasks;
Task tempTask;
int editingIndex = -1;
int lastCheckedMinute = -1;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TaskDialogProc(HWND, UINT, WPARAM, LPARAM);
void CreateControls(HWND hwnd);
void RefreshTaskList();
void AddTask();
void EditTask();
void DeleteTask();
void SaveTasksToFile();
void LoadTasksFromFile();
void CheckReminders();
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TaskReminderClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"فشل تسجيل النافذة!", L"خطأ", MB_OK | MB_ICONERROR);
        return 0;
    }
    hMainWindow = CreateWindowEx(
        0,
        L"TaskReminderClass",
        L"TaskReminder - To-Do List مع Reminder (12 Hour Format)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 480,
        NULL, NULL, hInstance, NULL
    );
    if (!hMainWindow) {
        MessageBox(NULL, L"فشل إنشاء النافذة!", L"خطأ", MB_OK | MB_ICONERROR);
        return 0;
    }
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateControls(hwnd);
        SetTimer(hwnd, TIMER_ID, 1000, NULL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_ADD:
            AddTask();
            break;
        case IDC_BTN_EDIT:
            EditTask();
            break;
        case IDC_BTN_DELETE:
            DeleteTask();
            break;
        case IDC_BTN_SAVE:
            SaveTasksToFile();
            break;
        case IDC_BTN_LOAD:
            LoadTasksFromFile();
            break;
        }
        break;
    case WM_TIMER:
        if (wParam == TIMER_ID) {
            CheckReminders();
        }
        break;
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
void CreateControls(HWND hwnd)
{
    hListBox = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"LISTBOX",
        NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        10, 10, 470, 400,
        hwnd,
        (HMENU)IDC_LISTBOX,
        hInst,
        NULL
    );
    CreateWindow(L"BUTTON", L"إضافة مهمة",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        490, 10, 110, 35, hwnd, (HMENU)IDC_BTN_ADD, hInst, NULL);
    CreateWindow(L"BUTTON", L"تعديل مهمة",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        490, 55, 110, 35, hwnd, (HMENU)IDC_BTN_EDIT, hInst, NULL);
    CreateWindow(L"BUTTON", L"حذف مهمة",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        490, 100, 110, 35, hwnd, (HMENU)IDC_BTN_DELETE, hInst, NULL);
    CreateWindow(L"BUTTON", L"حفظ الملف",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        490, 170, 110, 35, hwnd, (HMENU)IDC_BTN_SAVE, hInst, NULL);
    CreateWindow(L"BUTTON", L"تحميل الملف",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        490, 215, 110, 35, hwnd, (HMENU)IDC_BTN_LOAD, hInst, NULL);
}
void RefreshTaskList()
{
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < tasks.size(); i++) {
        std::wstringstream ss;
        int hour12 = tasks[i].get12Hour();
        ss << L"[" << (hour12 < 10 ? L"0" : L"") << hour12
            << L":" << (tasks[i].minute < 10 ? L"0" : L"") << tasks[i].minute
            << L" " << tasks[i].getAMPM() << L"] " << tasks[i].title;
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)ss.str().c_str());
    }
}
LRESULT CALLBACK TaskDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK_CUSTOM)
        {
            wchar_t buffer[256];
            GetDlgItemText(hDlg, IDC_EDIT_TITLE, buffer, 256);
            tempTask.title = buffer;
            GetDlgItemText(hDlg, IDC_EDIT_DESC, buffer, 256);
            tempTask.description = buffer;
            int hour12 = GetDlgItemInt(hDlg, IDC_EDIT_HOUR, NULL, FALSE);
            tempTask.minute = GetDlgItemInt(hDlg, IDC_EDIT_MINUTE, NULL, FALSE);
            if (hour12 < 1 || hour12 > 12) {
                MessageBox(hDlg, L"الرجاء إدخال ساعة صحيحة!\nالساعة: 1-12",
                    L"خطأ", MB_OK | MB_ICONWARNING);
                return 0;
            }
            if (tempTask.minute < 0 || tempTask.minute > 59) {
                MessageBox(hDlg, L"الرجاء إدخال دقيقة صحيحة!\nالدقيقة: 0-59",
                    L"خطأ", MB_OK | MB_ICONWARNING);
                return 0;
            }
            int ampmIndex = (int)SendMessage(hComboAMPM, CB_GETCURSEL, 0, 0);
            bool isAM = (ampmIndex == 0);
            tempTask.setFrom12Hour(hour12, isAM);
            if (editingIndex == -1) {
                tempTask.notified = false;
                tempTask.lastNotifiedDay = -1;
                if (!tempTask.title.empty()) {
                    tasks.push_back(tempTask);
                }
            }
            else {
                if (!tempTask.title.empty()) {
                    tempTask.notified = tasks[editingIndex].notified;
                    tempTask.lastNotifiedDay = tasks[editingIndex].lastNotifiedDay;
                    tasks[editingIndex] = tempTask;
                }
                editingIndex = -1;
            }
            RefreshTaskList();
            EnableWindow(hMainWindow, TRUE);
            SetFocus(hMainWindow);
            DestroyWindow(hDlg);
            hTaskDialog = NULL;
            return 0;
        }
        else if (LOWORD(wParam) == IDCANCEL_CUSTOM)
        {
            editingIndex = -1;
            EnableWindow(hMainWindow, TRUE);
            SetFocus(hMainWindow);
            DestroyWindow(hDlg);
            hTaskDialog = NULL;
            return 0;
        }
        break;
    case WM_CLOSE:
        editingIndex = -1;
        EnableWindow(hMainWindow, TRUE);
        SetFocus(hMainWindow);
        DestroyWindow(hDlg);
        hTaskDialog = NULL;
        return 0;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}
HWND CreateTaskDialog(HWND hwndParent)
{
    RECT rcParent;
    GetWindowRect(hwndParent, &rcParent);
    int x = rcParent.left + ((rcParent.right - rcParent.left) - 420) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - 320) / 2;
    HWND hDlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770",
        L"إضافة/تعديل مهمة",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, 420, 320,
        hwndParent, NULL, hInst, NULL
    );
    CreateWindow(L"STATIC", L"عنوان المهمة:",
        WS_CHILD | WS_VISIBLE,
        20, 20, 120, 20, hDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
        150, 18, 240, 22, hDlg, (HMENU)IDC_EDIT_TITLE, hInst, NULL);
    CreateWindow(L"STATIC", L"الوصف:",
        WS_CHILD | WS_VISIBLE,
        20, 55, 120, 20, hDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP,
        150, 53, 240, 60, hDlg, (HMENU)IDC_EDIT_DESC, hInst, NULL);
    CreateWindow(L"STATIC", L"وقت التذكير:",
        WS_CHILD | WS_VISIBLE,
        20, 135, 120, 20, hDlg, NULL, hInst, NULL);
    CreateWindow(L"STATIC", L"الساعة (1-12):",
        WS_CHILD | WS_VISIBLE,
        150, 135, 100, 20, hDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"12",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_TABSTOP,
        260, 133, 50, 22, hDlg, (HMENU)IDC_EDIT_HOUR, hInst, NULL);
    CreateWindow(L"STATIC", L"الدقيقة (0-59):",
        WS_CHILD | WS_VISIBLE,
        150, 165, 100, 20, hDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_TABSTOP,
        260, 163, 50, 22, hDlg, (HMENU)IDC_EDIT_MINUTE, hInst, NULL);
    CreateWindow(L"STATIC", L"AM/PM:",
        WS_CHILD | WS_VISIBLE,
        150, 195, 100, 20, hDlg, NULL, hInst, NULL);
    hComboAMPM = CreateWindowEx(
        0, L"COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        260, 193, 60, 100,
        hDlg, (HMENU)IDC_COMBO_AMPM, hInst, NULL
    );
    SendMessage(hComboAMPM, CB_ADDSTRING, 0, (LPARAM)L"AM");
    SendMessage(hComboAMPM, CB_ADDSTRING, 0, (LPARAM)L"PM");
    SendMessage(hComboAMPM, CB_SETCURSEL, 0, 0); 
    CreateWindow(L"BUTTON", L"موافق",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        120, 240, 80, 30, hDlg, (HMENU)IDOK_CUSTOM, hInst, NULL);
    CreateWindow(L"BUTTON", L"إلغاء",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        220, 240, 80, 30, hDlg, (HMENU)IDCANCEL_CUSTOM, hInst, NULL);
    EnableWindow(hwndParent, FALSE);
    SetWindowLongPtr(hDlg, GWLP_WNDPROC, (LONG_PTR)TaskDialogProc);
    SetFocus(GetDlgItem(hDlg, IDC_EDIT_TITLE));
    return hDlg;
}
void AddTask()
{
    if (hTaskDialog != NULL) return;
    tempTask = Task();
    editingIndex = -1;
    hTaskDialog = CreateTaskDialog(hMainWindow);
    SetDlgItemText(hTaskDialog, IDC_EDIT_TITLE, L"");
    SetDlgItemText(hTaskDialog, IDC_EDIT_DESC, L"");
    SetDlgItemInt(hTaskDialog, IDC_EDIT_HOUR, 12, FALSE);
    SetDlgItemInt(hTaskDialog, IDC_EDIT_MINUTE, 0, FALSE);
    SendMessage(hComboAMPM, CB_SETCURSEL, 0, 0); 
}
void EditTask()
{
    if (hTaskDialog != NULL) return;
    int selectedIndex = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
    if (selectedIndex == LB_ERR) {
        MessageBox(hMainWindow, L"الرجاء اختيار مهمة للتعديل", L"تنبيه", MB_OK | MB_ICONINFORMATION);
        return;
    }
    tempTask = tasks[selectedIndex];
    editingIndex = selectedIndex;
    hTaskDialog = CreateTaskDialog(hMainWindow);
    SetDlgItemText(hTaskDialog, IDC_EDIT_TITLE, tempTask.title.c_str());
    SetDlgItemText(hTaskDialog, IDC_EDIT_DESC, tempTask.description.c_str());
    SetDlgItemInt(hTaskDialog, IDC_EDIT_HOUR, tempTask.get12Hour(), FALSE);
    SetDlgItemInt(hTaskDialog, IDC_EDIT_MINUTE, tempTask.minute, FALSE);
    SendMessage(hComboAMPM, CB_SETCURSEL, tempTask.isAM() ? 0 : 1, 0);
}
void DeleteTask()
{
    int selectedIndex = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
    if (selectedIndex == LB_ERR) {
        MessageBox(hMainWindow, L"الرجاء اختيار مهمة للحذف", L"تنبيه", MB_OK | MB_ICONINFORMATION);
        return;
    }
    int result = MessageBox(hMainWindow, L"هل أنت متأكد من حذف هذه المهمة؟",
        L"تأكيد الحذف", MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES) {
        tasks.erase(tasks.begin() + selectedIndex);
        RefreshTaskList();
    }
}
void SaveTasksToFile()
{
    wchar_t filePath[MAX_PATH] = L"";
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrTitle = L"اختار مكان حفظ ملف المهام";
    if (!GetSaveFileName(&ofn)) {
        return; 
    }
    std::wofstream file(filePath);
    if (!file.is_open()) {
        MessageBox(hMainWindow, L"فشل حفظ الملف!", L"خطأ", MB_OK | MB_ICONERROR);
        return;
    }
    for (const auto& task : tasks) {
        file << task.title << L"|"
            << task.description << L"|"
            << task.hour << L"|"
            << task.minute << L"|"
            << task.lastNotifiedDay << L"\n";
    }
    file.close();
    MessageBox(hMainWindow, L"تم حفظ المهام بنجاح", L"نجاح", MB_OK | MB_ICONINFORMATION);
}
void LoadTasksFromFile()
{
    wchar_t filePath[MAX_PATH] = L"";
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"اختار ملف المهام";
    if (!GetOpenFileName(&ofn)) {
        return;
    }
    std::wifstream file(filePath);
    if (!file.is_open()) {
        MessageBox(hMainWindow, L"فشل فتح الملف!", L"خطأ", MB_OK | MB_ICONERROR);
        return;}
    tasks.clear();
    std::wstring line;
    while (std::getline(file, line)) {
        Task task;
        std::wstringstream ss(line);
        std::wstring token;
        std::getline(ss, task.title, L'|');
        std::getline(ss, task.description, L'|');
        std::getline(ss, token, L'|');
        task.hour = std::stoi(token);
        std::getline(ss, token, L'|');
        task.minute = std::stoi(token);
        if (std::getline(ss, token, L'|')) {
            task.lastNotifiedDay = std::stoi(token);}
        else {
            task.lastNotifiedDay = -1;}
        task.notified = false;
        tasks.push_back(task);
    }
    file.close();
    RefreshTaskList();
    MessageBox(hMainWindow, L"تم تحميل المهام بنجاح", L"نجاح", MB_OK | MB_ICONINFORMATION);
}
void CheckReminders()
{
    time_t now = time(0);
    tm localTime;
    localtime_s(&localTime, &now);
    int currentHour = localTime.tm_hour;
    int currentMinute = localTime.tm_min;
    int currentDay = localTime.tm_mday;
    int currentTimeKey = currentHour * 60 + currentMinute;
    if (lastCheckedMinute == currentTimeKey) {
        return;}
    lastCheckedMinute = currentTimeKey;
    for (auto& task : tasks) {
        if (task.hour == currentHour &&
            task.minute == currentMinute &&
            task.lastNotifiedDay != currentDay) {
            std::wstring message = L"حان وقت المهمة:\n\n" + task.title;
            if (!task.description.empty()) {
                message += L"\n\nالوصف: " + task.description;
            }
            MessageBox(hMainWindow, message.c_str(), L"تذكير!",
                MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
            task.lastNotifiedDay = currentDay;
            task.notified = true;
        }
        if (task.lastNotifiedDay != currentDay &&
            (task.hour > currentHour ||
                (task.hour == currentHour && task.minute > currentMinute))) {
            task.notified = false;
        }
    }
}
