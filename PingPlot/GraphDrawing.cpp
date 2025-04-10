#include "GraphDrawing.h"
#include <cmath> // For sqrt function

// Function to calculate jitter (standard deviation of ping times)
double CalculateJitter(const std::vector<double>& pingData) {
    if (pingData.size() <= 1) {
        return 0.0;
    }
    
    // Calculate mean first
    double sum = 0.0;
    for (const auto& ping : pingData) {
        sum += ping;
    }
    double mean = sum / pingData.size();
    
    // Calculate sum of squared differences
    double sqSum = 0.0;
    for (const auto& ping : pingData) {
        double diff = ping - mean;
        sqSum += diff * diff;
    }
    
    // Return standard deviation
    return std::sqrt(sqSum / pingData.size());
}

// Function to draw the graph
void DrawGraph(HDC hdc, RECT clientRect) {
    // Define graph area
    RECT graphRect = {
        GRAPH_PADDING,
        GRAPH_PADDING + 30, // Add space for controls
        clientRect.right - GRAPH_PADDING,
        clientRect.bottom - GRAPH_PADDING
    };
    
    // Use appropriate colors based on dark mode setting
    COLORREF textColor = g_DarkMode ? DARK_TEXT_COLOR : LIGHT_TEXT_COLOR;
    COLORREF gridColor = g_DarkMode ? DARK_GRAPH_GRID_COLOR : GRAPH_GRID_COLOR;
    COLORREF lineColor = g_DarkMode ? DARK_GRAPH_LINE_COLOR : GRAPH_LINE_COLOR;
    COLORREF bgColor = g_DarkMode ? DARK_BACKGROUND_COLOR : BACKGROUND_COLOR;
    
    // Fill background with appropriate color based on mode
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &clientRect, bgBrush);
    
    // Draw graph border
    HPEN borderPen = CreatePen(PS_SOLID, 1, textColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    Rectangle(hdc, graphRect.left, graphRect.top, graphRect.right, graphRect.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
    
    // Fill graph area with the background color
    RECT innerGraphRect = {
        graphRect.left + 1,
        graphRect.top + 1,
        graphRect.right - 1,
        graphRect.bottom - 1
    };
    FillRect(hdc, &innerGraphRect, bgBrush);
    DeleteObject(bgBrush);
    
    // Get data with mutex protection
    std::vector<double> pingData;
    {
        std::lock_guard<std::mutex> lock(g_PingDataMutex);
        pingData.assign(g_PingTimes.begin(), g_PingTimes.end());
    }
    
    if (pingData.empty()) {
        // Draw "No data" text if there's no ping data
        SetTextColor(hdc, g_DarkMode ? DARK_TEXT_COLOR : RGB(100, 100, 100));
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 
            (graphRect.left + graphRect.right) / 2 - 30,
            (graphRect.top + graphRect.bottom) / 2,
            L"No data", 7);
        return;
    }
    
    // Calculate max ping time based only on the visible data points (recent values)
    double recentMaxPing = *std::max_element(pingData.begin(), pingData.end());
    
    // Scale adjustment with hysteresis to prevent too frequent rescaling
    // If new max is higher, scale up immediately
    if (recentMaxPing > g_MaxPingTime) {
        g_MaxPingTime = recentMaxPing * 1.2; // Add 20% headroom
    }
    // If new max is significantly lower (less than 70% of current scale), scale down gradually
    else if (recentMaxPing < g_MaxPingTime * 0.7) {
        // Gradually scale down to prevent jumpy rescaling
        g_MaxPingTime = g_MaxPingTime * 0.95; // Scale down by 10%
        
        // Don't go below the recent max plus some headroom
        if (g_MaxPingTime < recentMaxPing * 1.2) {
            g_MaxPingTime = recentMaxPing * 1.2;
        }
        
        // Have a minimum scale to prevent tiny values from dominating
        if (g_MaxPingTime < 1.0) {
            g_MaxPingTime = 1.0;
        }
    }
    
    // Draw grid lines
    HPEN gridPen = CreatePen(PS_DOT, 1, gridColor);
    oldPen = (HPEN)SelectObject(hdc, gridPen);
    
    // Always use exactly 6 divisions for the y-axis
    const int DIVISIONS = 6;
    double yGridStep = g_MaxPingTime / DIVISIONS;
    
    // Draw horizontal grid lines
    SetTextColor(hdc, textColor);
    SetBkMode(hdc, TRANSPARENT);
    
    for (int i = 0; i <= DIVISIONS; i++) {
        // Calculate y position
        double value = i * yGridStep;
        int y = graphRect.bottom - (int)((value / g_MaxPingTime) * 
            (graphRect.bottom - graphRect.top));
        
        // Draw grid line
        MoveToEx(hdc, graphRect.left, y, NULL);
        LineTo(hdc, graphRect.right, y);
        
        // Draw y-axis labels with proper decimal places
        WCHAR label[16];
        swprintf_s(label, L"%.0f ms", value);
        TextOut(hdc, graphRect.left - 50, y - 8, label, (int)wcslen(label));
    }
    
    // Vertical grid lines
    int xGridStep = 100; // pixels
    for (int x = graphRect.left + xGridStep; x < graphRect.right; x += xGridStep) {
        MoveToEx(hdc, x, graphRect.top, NULL);
        LineTo(hdc, x, graphRect.bottom);
    }
    
    // Draw x-axis labels
    SetTextColor(hdc, textColor);
    
    // Current history length at the start of x-axis
    WCHAR historyLabel[16];
    swprintf_s(historyLabel, L"~%.1f s", g_HistorySeconds);
    TextOut(hdc, graphRect.left, graphRect.bottom + 5, 
            historyLabel, (int)wcslen(historyLabel));
    
    // "0" at the end of x-axis
    TextOut(hdc, graphRect.right - 10, graphRect.bottom + 5, L"0", 1);
    
    // Restore pen
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
    
    // Draw ping time graph
    HPEN linePen = CreatePen(PS_SOLID, 2, lineColor);
    SelectObject(hdc, linePen);
    
    // Calculate available graph width
    int graphWidth = graphRect.right - graphRect.left;
    
    // We want newest data on the right side
    // Determine how much horizontal space each point gets
    double xStep = (double)graphWidth / g_DynamicDataPoints;
    
    if (pingData.size() > 0) {
        // Start drawing from the left side of the graph
        // Position depends on how many points we have compared to max
        int startX = int(graphRect.right - ((int)pingData.size() * xStep));
        if (startX < graphRect.left) startX = graphRect.left;
        
        // Calculate the starting index in our data
        // Skip oldest entries if we have more data than can fit in view
        size_t startIdx = 0;
        if (pingData.size() > g_DynamicDataPoints) {
            startIdx = pingData.size() - g_DynamicDataPoints;
        }
        
        // Start at the first point
        int xPos = startX;
        int yPos = graphRect.bottom - (int)((pingData[startIdx] / g_MaxPingTime) * 
            (graphRect.bottom - graphRect.top));
        MoveToEx(hdc, xPos, yPos, NULL);
        
        // Draw line through remaining points, evenly spaced
        for (size_t i = startIdx + 1; i < pingData.size(); i++) {
            xPos = startX + (int)((i - startIdx) * xStep);
            yPos = graphRect.bottom - (int)((pingData[i] / g_MaxPingTime) * 
                (graphRect.bottom - graphRect.top));
            LineTo(hdc, xPos, yPos);
        }
    }
    
    // Draw average, max ping times, and jitter
    if (!pingData.empty()) {
        double currentPing = pingData.back();
        double averagePing = 0;
        
        // Calculate average and find max (use recentMaxPing since we've already calculated it)
        for (const auto& ping : pingData) {
            averagePing += ping;
        }
        averagePing /= pingData.size();
        
        // Calculate jitter (standard deviation)
        double jitter = CalculateJitter(pingData);
        
        // Format ping times with appropriate precision based on value
        WCHAR currentPingStr[16], avgPingStr[16], maxPingStr[16], jitterStr[16];
        
        // Use 3 decimal places for values under 1ms
        if (currentPing < 1.0) {
            swprintf_s(currentPingStr, L"%.3f", currentPing);
        } else {
            swprintf_s(currentPingStr, L"%.1f", currentPing);
        }
        
        if (averagePing < 1.0) {
            swprintf_s(avgPingStr, L"%.3f", averagePing);
        } else {
            swprintf_s(avgPingStr, L"%.1f", averagePing);
        }
        
        if (recentMaxPing < 1.0) {
            swprintf_s(maxPingStr, L"%.3f", recentMaxPing);
        } else {
            swprintf_s(maxPingStr, L"%.1f", recentMaxPing);
        }
        
        if (jitter < 1.0) {
            swprintf_s(jitterStr, L"%.3f", jitter);
        } else {
            swprintf_s(jitterStr, L"%.1f", jitter);
        }
        
        // Display stats - Update to include ping count, PPS, jitter and dynamic data points value
        WCHAR statsText[256];
        swprintf_s(statsText, 
            L"Current: %s ms | Avg: %s ms | Max: %s ms | Jitter: %s ms | Pings per second: %.1f | History: %d points", 
            currentPingStr, avgPingStr, maxPingStr, jitterStr, g_PingsPerSecond.load(), g_DynamicDataPoints.load());
        
        SetTextColor(hdc, textColor);
        TextOut(hdc, graphRect.left + 10, graphRect.top + 10, 
            statsText, (int)wcslen(statsText));
    }
    
    // Cleanup
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
}

// UI update timer callback
VOID CALLBACK UIUpdateTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    if (g_DataUpdated) {
        InvalidateRect(hwnd, NULL, FALSE);
        g_DataUpdated = false;
    }
}
