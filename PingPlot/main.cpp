#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#include "Common.h"
#include "GraphDrawing.h"
#include "PingThread.h"
#include "UIControls.h"

// Global variable definitions
std::wstring g_HostToPing = L"1.1.1.1";  //default host
std::deque<double> g_PingTimes;
std::mutex g_PingDataMutex;
std::atomic<bool> g_Running = true;
HWND g_hWnd = NULL;
HWND g_hEditHost = NULL;
HWND g_hBtnStart = NULL;
HWND g_hBtnStop = NULL;
HWND g_hEditFrequency = NULL;
HWND g_hBtnApplyFrequency = NULL;
int g_PingInterval = PING_INTERVAL_MS;
double g_MaxPingTime = 100.0;
std::atomic<bool> g_DataUpdated = false;
UINT_PTR g_UITimer = 0;
std::atomic<unsigned long long> g_TotalPings = 0;
std::atomic<double> g_PingsPerSecond = 0.0;
std::chrono::steady_clock::time_point g_LastPPSUpdateTime;
unsigned long long g_LastPPSCount = 0;
std::atomic<int> g_DynamicDataPoints = INITIAL_MAX_DATAPOINTS; // Initialize to default value
std::thread g_PingThreadHandle;                               // Thread handle
std::atomic<bool> g_ThreadRunning = false;                    // Thread running flag
float g_HistorySeconds = HISTORY_SECONDS;                     // Initialize with constant
HWND g_hEditHistory = NULL;                                   // History length edit control
HWND g_hBtnApplyHistory = NULL;                               // Apply history button
bool g_DarkMode = true;                                       // Start in dark mode
HWND g_hBtnDarkMode = NULL;                                   // Dark mode toggle button

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PingPlotWindowClass";
    RegisterClassEx(&wc);

    // Calculate window size
    RECT windowRect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    
    // Create window
    g_hWnd = CreateWindowEx(
        0,
        L"PingPlotWindowClass",
        L"PingPlot",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) {
        MessageBox(NULL, L"Window Creation Failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create UI controls
    CreateUIControls(g_hWnd, hInstance);

    // Show window
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // Message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    g_Running = false;
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}