#include "UIControls.h"
#include "GraphDrawing.h"
#include "PingThread.h"
#include <Richedit.h> // Required for EM_SETBKGNDCOLOR

// Update appearance of all controls based on dark mode setting
void UpdateControlsAppearance(HWND hwnd) {
    // Set colors based on dark mode
    COLORREF textColor = g_DarkMode ? DARK_CONTROL_TEXT : LIGHT_TEXT_COLOR;
    COLORREF bgColor = g_DarkMode ? DARK_CONTROL_BG : RGB(255, 255, 255);
    COLORREF buttonColor = g_DarkMode ? DARK_BUTTON_BG : GetSysColor(COLOR_BTNFACE);
    
    // Update edit controls
    HWND controls[] = {
        g_hEditHost, g_hEditFrequency, g_hEditHistory
    };
    
    for (HWND ctrl : controls) {
        // Set text color and background color for edit controls
        SendMessage(ctrl, EM_SETBKGNDCOLOR, 0, bgColor);
        InvalidateRect(ctrl, NULL, TRUE);
    }

}

// Create UI controls
void CreateUIControls(HWND hwnd, HINSTANCE hInstance) {
    using namespace UILayout;
    
    int currentX = MARGIN_LEFT;
    int currentY = MARGIN_TOP;
    
    // Host edit box
    g_hEditHost = CreateWindow(
        L"EDIT", g_HostToPing.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        currentX, currentY, EDIT_WIDTH, CONTROL_HEIGHT,
        hwnd, (HMENU)ID_EDIT_HOST, hInstance, NULL
    );
    
    // Start button
    currentX += EDIT_WIDTH + ELEMENT_SPACING;
    g_hBtnStart = CreateWindow(
        L"BUTTON", L"Start",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        currentX, currentY, SMALL_BUTTON_WIDTH, CONTROL_HEIGHT,
        hwnd, (HMENU)ID_BTN_START, hInstance, NULL
    );

    // Stop button
    currentX += SMALL_BUTTON_WIDTH + ELEMENT_SPACING;
    g_hBtnStop = CreateWindow(
        L"BUTTON", L"Stop",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        currentX, currentY, SMALL_BUTTON_WIDTH, CONTROL_HEIGHT,
        hwnd, (HMENU)ID_BTN_STOP, hInstance, NULL
    );

    // Label for frequency control
    currentX += SMALL_BUTTON_WIDTH + ELEMENT_SPACING;
    CreateWindow(
        L"STATIC", L"Re-ping after (ms):",
        WS_CHILD | WS_VISIBLE,
        currentX, currentY+RAW_TEXT_PADDING_TOP, FREQ_LABEL_WIDTH, CONTROL_HEIGHT,
        hwnd, NULL, hInstance, NULL
    );

    // Edit box for frequency
    currentX += FREQ_LABEL_WIDTH;
    WCHAR freqStr[16];
    swprintf_s(freqStr, L"%d", PING_INTERVAL_MS);
    g_hEditFrequency = CreateWindow(
        L"EDIT", freqStr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
        currentX, currentY, EDIT_SMALL_WIDTH, CONTROL_HEIGHT,
        hwnd, (HMENU)ID_EDIT_FREQUENCY, hInstance, NULL
    );

    // Apply button
    currentX += EDIT_SMALL_WIDTH + ELEMENT_SPACING;
    g_hBtnApplyFrequency = CreateWindow(
        L"BUTTON", L"Apply",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        currentX, currentY, SMALL_BUTTON_WIDTH, CONTROL_HEIGHT,
        hwnd, (HMENU)ID_BTN_APPLY, hInstance, NULL
    );
    
    // Label for history control
    currentX += SMALL_BUTTON_WIDTH + ELEMENT_SPACING;
    CreateWindow(
        L"STATIC", L"Approx History Length (s):",
        WS_CHILD | WS_VISIBLE,
        currentX, currentY+RAW_TEXT_PADDING_TOP, HISTORY_LABEL_WIDTH, CONTROL_HEIGHT,
        hwnd, NULL, hInstance, NULL
    );

    // Edit box for history length
    currentX += HISTORY_LABEL_WIDTH;
    WCHAR historyStr[16];
    swprintf_s(historyStr, L"%.1f", g_HistorySeconds);
    g_hEditHistory = CreateWindow(
        L"EDIT", historyStr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        currentX, currentY, EDIT_SMALL_WIDTH, CONTROL_HEIGHT,
        hwnd, (HMENU)ID_EDIT_HISTORY, hInstance, NULL
    );

    // Apply history button
    currentX += EDIT_SMALL_WIDTH + ELEMENT_SPACING;
    g_hBtnApplyHistory = CreateWindow(
        L"BUTTON", L"Apply",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        currentX, currentY, SMALL_BUTTON_WIDTH, CONTROL_HEIGHT,
        hwnd, (HMENU)ID_BTN_APPLY_HISTORY, hInstance, NULL
    );
    
    // Dark mode toggle button
    currentX += SMALL_BUTTON_WIDTH + ELEMENT_SPACING;
    g_hBtnDarkMode = CreateWindow(
        L"BUTTON", g_DarkMode ? L"Light Mode" : L"Dark Mode",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        currentX, currentY, CHECKBOX_WIDTH, CONTROL_HEIGHT,
        hwnd, (HMENU)ID_BTN_DARK_MODE, hInstance, NULL
    );
    
    // Apply the initial appearance based on dark mode setting
    UpdateControlsAppearance(hwnd);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Get client area
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            
            // Create double buffer
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
            
            // Fill background with appropriate color based on mode
            HBRUSH bgBrush = CreateSolidBrush(g_DarkMode ? DARK_BACKGROUND_COLOR : BACKGROUND_COLOR);
            FillRect(memDC, &clientRect, bgBrush);
            DeleteObject(bgBrush);
            
            // Draw graph
            DrawGraph(memDC, clientRect);
            
            // Copy to screen
            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);
            
            // Cleanup
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_CTLCOLOREDIT: {
            // Handle the background and text colors for edit controls
            HDC hdcEdit = (HDC)wParam;
            HWND hwndEdit = (HWND)lParam;
            
            // Set text and background colors based on dark mode
            SetTextColor(hdcEdit, g_DarkMode ? DARK_CONTROL_TEXT : LIGHT_TEXT_COLOR);
            SetBkColor(hdcEdit, g_DarkMode ? DARK_CONTROL_BG : RGB(255, 255, 255));
            
            // Create a brush for the background
            static HBRUSH hEditBrush = NULL;
            if (hEditBrush) DeleteObject(hEditBrush);
            hEditBrush = CreateSolidBrush(g_DarkMode ? DARK_CONTROL_BG : RGB(255, 255, 255));
            
            return (LRESULT)hEditBrush;
        }
        
        case WM_CTLCOLORSTATIC: {
            // Handle the background and text colors for static controls (labels)
            HDC hdcStatic = (HDC)wParam;
            
            // Set text and background colors based on dark mode
            SetTextColor(hdcStatic, g_DarkMode ? DARK_CONTROL_TEXT : LIGHT_TEXT_COLOR);
            SetBkColor(hdcStatic, g_DarkMode ? DARK_BACKGROUND_COLOR : BACKGROUND_COLOR);
            
            // Create a brush for the background that matches the main window
            static HBRUSH hStaticBrush = NULL;
            if (hStaticBrush) DeleteObject(hStaticBrush);
            hStaticBrush = CreateSolidBrush(g_DarkMode ? DARK_BACKGROUND_COLOR : BACKGROUND_COLOR);
            
            return (LRESULT)hStaticBrush;
        }
        
        case WM_CTLCOLORBTN: {
            // Handle the background colors for buttons
            HDC hdcBtn = (HDC)wParam;
            
            // Set text color based on dark mode
            SetTextColor(hdcBtn, g_DarkMode ? DARK_CONTROL_TEXT : LIGHT_TEXT_COLOR);
            
            // Create a brush for the button background
            static HBRUSH hBtnBrush = NULL;
            if (hBtnBrush) DeleteObject(hBtnBrush);
            hBtnBrush = CreateSolidBrush(g_DarkMode ? DARK_BUTTON_BG : GetSysColor(COLOR_BTNFACE));
            
            return (LRESULT)hBtnBrush;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_BTN_START: // Start button
                    StopPinging();
                    StartPinging();
                    return 0;
                
                case ID_BTN_STOP: // Stop button
                    StopPinging();
                    return 0;
                
                case ID_BTN_APPLY: // Apply frequency button
                    StopPinging();
                    UpdatePingFrequency();
                    StartPinging();
                    return 0;
                    
                case ID_EDIT_HOST: // Host edit box
                    return 0;
                    
                case ID_BTN_APPLY_HISTORY: // Apply history button
                    {
                        WCHAR buffer[16];
                        GetWindowText(g_hEditHistory, buffer, 16);
                        float newHistory = (float)_wtof(buffer);
                        
                        // Validate input - ensure it's between 1s and 300s
                        if (newHistory >= 1.0f && newHistory <= 300.0f) {
                            g_HistorySeconds = newHistory;
                            
                            // Update the edit box in case the value was changed due to validation
                            swprintf_s(buffer, L"%.1f", g_HistorySeconds);
                            SetWindowText(g_hEditHistory, buffer);
                            
                            // Trigger a redraw
                            InvalidateRect(hwnd, NULL, FALSE);
                        } else {
                            // Reset to current value if invalid
                            swprintf_s(buffer, L"%.1f", g_HistorySeconds);
                            SetWindowText(g_hEditHistory, buffer);
                            MessageBox(g_hWnd, L"Please enter a value between 1.0 and 300.0 seconds", 
                                L"Invalid Input", MB_ICONWARNING);
                        }
                    }
                    return 0;
                    
                case ID_BTN_DARK_MODE: // Dark mode toggle button
                    {
                        // Toggle dark mode
                        g_DarkMode = !g_DarkMode;
                        
                        // Update button text to reflect the next state
                        SetWindowText(g_hBtnDarkMode, g_DarkMode ? L"Light Mode" : L"Dark Mode");
                        
                        // Update the appearance of all controls
                        UpdateControlsAppearance(hwnd);
                        
                        // Trigger a full redraw
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    return 0;
            }
            break;
        }
        
        case WM_DESTROY:
            g_Running = false;
            if (g_UITimer != 0) {
                KillTimer(hwnd, g_UITimer);
                g_UITimer = 0;
            }
            // Make sure thread exits cleanly
            if (g_ThreadRunning && g_PingThreadHandle.joinable()) {
                g_PingThreadHandle.join();
            }
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
