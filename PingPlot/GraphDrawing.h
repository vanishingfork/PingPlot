#pragma once

#include "Common.h"

// Draw the ping graph
void DrawGraph(HDC hdc, RECT clientRect);

// Timer callback for UI updates
VOID CALLBACK UIUpdateTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
