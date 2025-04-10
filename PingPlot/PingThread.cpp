#include "PingThread.h"
#include "GraphDrawing.h"

// Function for pinging a host
void PingThread() {
    // Initialize Winsock (required for DNS resolution)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox(g_hWnd, L"Failed to initialize Winsock", L"Error", MB_ICONERROR);
        g_ThreadRunning = false;
        return;
    }

    // Open ICMP handle
    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        MessageBox(g_hWnd, L"Failed to create ICMP handle", L"Error", MB_ICONERROR);
        WSACleanup();
        g_ThreadRunning = false;
        return;
    }

    // Convert wide string to narrow string
    char hostBuffer[256];
    WideCharToMultiByte(CP_ACP, 0, g_HostToPing.c_str(), -1, hostBuffer, sizeof(hostBuffer), NULL, NULL);
    
    // Try to convert string as IP address first
    IN_ADDR addr;
    BOOL isIpAddress = (InetPton(AF_INET, g_HostToPing.c_str(), &addr) == 1);
    
    // If not a valid IP, try to resolve as hostname
    if (!isIpAddress) {
        struct addrinfo hints = {0};
        struct addrinfo* result = NULL;
        
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        
        // Use getaddrinfo to resolve the hostname
        int res = getaddrinfo(hostBuffer, NULL, &hints, &result);
        
        if (res != 0 || !result) {
            // Get the specific error message
            WCHAR errorMsg[256];
            swprintf_s(errorMsg, L"Could not resolve hostname: %S\nError: %d", hostBuffer, res);
            MessageBox(g_hWnd, errorMsg, L"Error", MB_ICONERROR);
            IcmpCloseHandle(hIcmp);
            WSACleanup();
            g_ThreadRunning = false;
            return;
        }
        
        // Get the IP address from the first result
        struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)result->ai_addr;
        addr = sockaddr_ipv4->sin_addr;
        
        // Free the address info
        freeaddrinfo(result);
    }
    
    // Prepare for ping
    char replyBuffer[sizeof(ICMP_ECHO_REPLY) + 32];
    unsigned long replySize = sizeof(replyBuffer);
    char sendData[32] = "PingPlotData";
    
    // Initialize PPS tracking time
    g_LastPPSUpdateTime = std::chrono::steady_clock::now();
    g_LastPPSCount = g_TotalPings.load();
    
    // Ping loop
    while (g_Running) {
        // Use high-resolution timer for more precise measurements
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Send ping using the address
        DWORD result = IcmpSendEcho(hIcmp, addr.S_un.S_addr, 
            sendData, sizeof(sendData), NULL, replyBuffer, replySize, DEFAULT_PING_TIMEOUT_MS);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // Increment ping counter regardless of success
        g_TotalPings++;
        
        // Calculate pings per second every 1 second
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_LastPPSUpdateTime).count();
        if (timeSinceLastUpdate >= 1000) {  // Update PPS once per second
            unsigned long long currentCount = g_TotalPings.load();
            unsigned long long countDifference = currentCount - g_LastPPSCount;
            double secondsElapsed = timeSinceLastUpdate / 1000.0;
            
            g_PingsPerSecond = countDifference / secondsElapsed;
            
            // Update dynamic data points to use configurable history seconds
            int newDataPoints = static_cast<int>(g_PingsPerSecond * g_HistorySeconds);
            // Ensure we have at least some minimum number of data points
            if (newDataPoints < 100) newDataPoints = 100;
            // Cap it to prevent excessive memory usage
            if (newDataPoints > 10000) newDataPoints = 10000;
            g_DynamicDataPoints = newDataPoints;
            
            g_LastPPSUpdateTime = now;
            g_LastPPSCount = currentCount;
        }
        
        if (result > 0) {
            // Use our own high-precision timing instead of the API's integer milliseconds
            double pingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            
            // Save the ping time
            {
                std::lock_guard<std::mutex> lock(g_PingDataMutex);
                g_PingTimes.push_back(pingTime);
                if (g_PingTimes.size() > g_DynamicDataPoints) {
                    g_PingTimes.pop_front();
                }
            }
            
            // Just mark that we have new data, don't request redraw here
            g_DataUpdated = true;
        } else {
            // Ping failed, store timeout value
            std::lock_guard<std::mutex> lock(g_PingDataMutex);
            g_PingTimes.push_back(DEFAULT_PING_TIMEOUT_MS);
            if (g_PingTimes.size() > g_DynamicDataPoints) {
                g_PingTimes.pop_front();
            }
            
            // Just mark that we have new data, don't request redraw here
            g_DataUpdated = true;
        }
        
        // Calculate sleep time to maintain ping interval
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        int sleepTime = g_PingInterval - (int)elapsed.count();
        
        if (sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }
    
    // Clean up
    IcmpCloseHandle(hIcmp);
    WSACleanup();
    g_ThreadRunning = false; // Mark thread as finished
}

// Function to start pinging
void StartPinging() {
    // Make sure any previous thread is completely stopped
    StopPinging();
    
    // Get host from edit box
    WCHAR hostBuffer[256];
    GetWindowText(g_hEditHost, hostBuffer, 256);
    g_HostToPing = hostBuffer;
    
    // Clear previous data
    {
        std::lock_guard<std::mutex> lock(g_PingDataMutex);
        g_PingTimes.clear();
    }
    
    // Reset ping counter
    g_TotalPings = 0;
    g_PingsPerSecond = 0.0;
    
    // Set up UI update timer if it's not already running
    if (g_UITimer == 0) {
        g_UITimer = SetTimer(g_hWnd, 1, UI_UPDATE_INTERVAL_MS, UIUpdateTimerProc);
    }
    
    // Start pinging thread
    g_Running = true;
    g_ThreadRunning = true;
    g_PingThreadHandle = std::thread(PingThread);
}

// Function to stop pinging
void StopPinging() {
    // Signal the thread to stop
    g_Running = false;
    
    // Wait for thread to finish if it's running
    if (g_ThreadRunning && g_PingThreadHandle.joinable()) {
        g_PingThreadHandle.join();
    }
    
    // Kill the UI update timer
    if (g_UITimer != 0) {
        KillTimer(g_hWnd, g_UITimer);
        g_UITimer = 0;
    }
}

// Function to update the ping frequency
void UpdatePingFrequency() {
    WCHAR buffer[16];
    GetWindowText(g_hEditFrequency, buffer, 16);
    
    int newInterval = _wtoi(buffer);
    
    // Validate input - ensure it's between 0ms and 10000ms
    if (newInterval >= 0 && newInterval <= 10000) {
        g_PingInterval = newInterval;
        
        // Update the edit box in case the value was changed due to validation
        swprintf_s(buffer, L"%d", g_PingInterval);
        SetWindowText(g_hEditFrequency, buffer);
    } else {
        // Reset to current value if invalid
        swprintf_s(buffer, L"%d", g_PingInterval);
        SetWindowText(g_hEditFrequency, buffer);
        MessageBox(g_hWnd, L"Please enter a value between 0ms and 10000 ms", 
            L"Invalid Input", MB_ICONWARNING);
    }
}
