#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#define INJECTED_INPUT_FLAG 0xDEADC0DE

#include <windows.h>
#include <stdio.h>
#include <malloc.h>

typedef struct
{
    unsigned long long timestamp;
    LONG x;
    LONG y;
    LONG dx;
    LONG dy;
	int isInjected;
} MouseLogEvent;

static FILE* log_file = NULL;
#define BUFFER_SIZE 4096

static MouseLogEvent buffer[BUFFER_SIZE];
static int buffer_index = 0;


LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void flush_buffer();
void cleanup();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "InputDelayWindowClass";

    if (!RegisterClass(&wc)) return 1;

    HWND hwnd = CreateWindow(
        "InputDelayWindowClass",
        "Input Delay Hidden Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        300, 200,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 1;

    fopen_s(&log_file, "mouse_log.csv", "w");

    if (log_file) {
        fprintf(log_file, "timestamp_ms,x,y,dx,dy\n");
        fflush(log_file);
    }

    buffer_index = 0;

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE))) {
        cleanup();
        return 1;
    }

   RegisterHotKey(hwnd, 1, MOD_ALT, 'Q');

   ShowWindow(hwnd, SW_HIDE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void flush_buffer()
{
    OutputDebugStringA("Flushing mouse log buffer\n");
    if (!log_file || buffer_index == 0) return;

    for (int i = 0; i < buffer_index; i++) {
        fprintf(log_file, "%llu,%ld,%ld,%ld,%ld,%ld\n",
            buffer[i].timestamp,
            buffer[i].x,
            buffer[i].y,
            buffer[i].dx,
            buffer[i].dy,
            buffer[i].isInjected);
    }

    fflush(log_file);
    buffer_index = 0;
}

void cleanup()
{
    if (log_file) {
        flush_buffer();
        fclose(log_file);
        log_file = NULL;
    }
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg) {
    case WM_HOTKEY:
        if (wParam == 1) { 
            DestroyWindow(hwnd); 
        }
        break;

    case WM_INPUT:
    {
        UINT dwSize = sizeof(BYTE) * 256;
        BYTE lpb[256];

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != (UINT)-1)
        {
            RAWINPUT* raw = (RAWINPUT*)lpb;

            if (raw->header.dwType == RIM_TYPEMOUSE)
            {
				int is_injected = (raw->data.mouse.ulExtraInformation == INJECTED_INPUT_FLAG) ? 1 : 0;
                
                LONG dx = raw->data.mouse.lLastX;
                LONG dy = raw->data.mouse.lLastY;

                if (dx != 0 || dy != 0)
                {
                    POINT pt;
                    GetCursorPos(&pt);

                    FILETIME ft;
                    GetSystemTimeAsFileTime(&ft);
                    ULONGLONG t = (((ULONGLONG)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
                    unsigned long long ms = t / 10000ULL;
                    //ULONGLONG ms = (t - 116444736000000000ULL) / 10000ULL;

                    buffer[buffer_index].timestamp = ms;
                    buffer[buffer_index].dx = dx;
                    buffer[buffer_index].dy = dy;
                    buffer[buffer_index].x = pt.x; 
                    buffer[buffer_index].y = pt.y;
					buffer[buffer_index].isInjected = is_injected;
                    buffer_index++;

                    if (buffer_index >= BUFFER_SIZE) {
                        flush_buffer();
                    }
                }
            }
        }
        break;
    }

    case WM_DESTROY:
        UnregisterHotKey(hwnd, 1);
        cleanup();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

