#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY 1

static NOTIFYICONDATAW g_nid;
static BOOL g_isExtend = TRUE;
static WCHAR g_stateFile[MAX_PATH];

static void BuildStatePath(void)
{
    DWORD n = GetTempPathW(MAX_PATH, g_stateFile);
    if (n == 0 || n > MAX_PATH - 32) {
        lstrcpyW(g_stateFile, L"switch-display-state.txt");
        return;
    }
    lstrcatW(g_stateFile, L"switch-display-state.txt");
}

static BOOL ReadState(void)
{
    HANDLE h = CreateFileW(g_stateFile, GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return TRUE;

    char buf[16] = {0};
    DWORD rd = 0;
    ReadFile(h, buf, sizeof(buf) - 1, &rd, NULL);
    CloseHandle(h);
    return lstrcmpA(buf, "external") != 0;
}

static void WriteState(BOOL isExtend)
{
    HANDLE h = CreateFileW(g_stateFile, GENERIC_WRITE, 0,
                           NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    const char* s = isExtend ? "extend" : "external";
    DWORD wr = 0;
    WriteFile(h, s, lstrlenA(s), &wr, NULL);
    CloseHandle(h);
}

static void Toggle(void)
{
    ShellExecuteW(NULL, L"open", L"DisplaySwitch.exe",
                  g_isExtend ? L"/external" : L"/extend",
                  NULL, SW_HIDE);
    g_isExtend = !g_isExtend;
    WriteState(g_isExtend);
}

static LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM w, LPARAM l)
{
    if (msg == WM_TRAYICON) {
        if (LOWORD(l) == WM_LBUTTONDBLCLK) Toggle();
        return 0;
    }
    if (msg == WM_DESTROY) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

void WINAPI WinMainCRTStartup(void)
{
    HANDLE mutex = CreateMutexW(NULL, TRUE, L"SwitchDisplayTray_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        ExitProcess(0);
    }

    BuildStatePath();
    g_isExtend = ReadState();

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"SwitchDisplayTrayClass";
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0,
                                HWND_MESSAGE, NULL, wc.hInstance, NULL);

    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hWnd;
    g_nid.uID = ID_TRAY;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    ExitProcess(0);
}
