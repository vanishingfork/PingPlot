#pragma once

// Standard includes
#include <WS2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <vector>
#include <string>
#include <deque>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

// Libraries
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Constants
const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;
const int GRAPH_PADDING = 60;
const int INITIAL_MAX_DATAPOINTS = 500; // Initial value, will be adjusted dynamically
const float HISTORY_SECONDS = 15.0; // How long to keep data in seconds
const int PING_INTERVAL_MS = 0; // additional delay between pings.
const int DEFAULT_PING_TIMEOUT_MS = 1000;
const int UI_UPDATE_INTERVAL_MS = 33; // ~30 FPS for UI updates
const COLORREF BACKGROUND_COLOR = RGB(240, 240, 240);
const COLORREF GRAPH_GRID_COLOR = RGB(200, 200, 200);
const COLORREF GRAPH_LINE_COLOR = RGB(0, 100, 200);

// Dark mode colors
const COLORREF DARK_BACKGROUND_COLOR = RGB(30, 30, 30);
const COLORREF DARK_GRAPH_GRID_COLOR = RGB(70, 70, 70);
const COLORREF DARK_GRAPH_LINE_COLOR = RGB(0, 150, 255);
const COLORREF DARK_TEXT_COLOR = RGB(220, 220, 220);
const COLORREF LIGHT_TEXT_COLOR = RGB(0, 0, 0);

// Control colors for dark mode
const COLORREF DARK_CONTROL_BG = RGB(50, 50, 50);
const COLORREF DARK_CONTROL_TEXT = RGB(220, 220, 220);
const COLORREF DARK_BUTTON_BG = RGB(60, 60, 60);

// Global variables
extern std::wstring g_HostToPing;
extern std::deque<double> g_PingTimes;
extern std::mutex g_PingDataMutex;
extern std::atomic<bool> g_Running;
extern HWND g_hWnd;
extern HWND g_hEditHost;
extern HWND g_hBtnStart;
extern HWND g_hBtnStop;
extern HWND g_hEditFrequency;
extern HWND g_hBtnApplyFrequency;
extern int g_PingInterval;
extern double g_MaxPingTime;
extern std::atomic<bool> g_DataUpdated;
extern UINT_PTR g_UITimer;
extern std::atomic<unsigned long long> g_TotalPings;
extern std::atomic<double> g_PingsPerSecond;
extern std::chrono::steady_clock::time_point g_LastPPSUpdateTime;
extern unsigned long long g_LastPPSCount;
extern std::atomic<int> g_DynamicDataPoints; // New variable for dynamic data points
extern std::thread g_PingThreadHandle;        // Handle to the ping thread
extern std::atomic<bool> g_ThreadRunning;     // Flag to track if thread is running
extern float g_HistorySeconds;                // User configurable history length
extern HWND g_hEditHistory;                   // Handle to history length edit control
extern HWND g_hBtnApplyHistory;               // Handle to apply history button
extern bool g_DarkMode;                       // Dark mode toggle
extern HWND g_hBtnDarkMode;                   // Dark mode toggle button

// Control IDs
enum ControlIDs {
    ID_EDIT_HOST = 101,
    ID_BTN_START = 102,
    ID_BTN_STOP = 103,
    ID_EDIT_FREQUENCY = 104,
    ID_BTN_APPLY = 105,
    ID_EDIT_HISTORY = 106,
    ID_BTN_APPLY_HISTORY = 107,
    ID_BTN_DARK_MODE = 108
};
