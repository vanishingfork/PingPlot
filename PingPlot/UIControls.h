#pragma once

#include "Common.h"

// UI Layout constants
namespace UILayout {
    // Margins and spacing
    const int MARGIN_LEFT = 15;
    const int MARGIN_TOP = 15;
    const int ELEMENT_SPACING = 10;
    const int RAW_TEXT_PADDING_TOP = 1;
    
    // Element sizes
    const int EDIT_WIDTH = 200;
    const int BUTTON_WIDTH = 80;
    const int SMALL_BUTTON_WIDTH = 60;
    const int EDIT_SMALL_WIDTH = 60;
    const int CONTROL_HEIGHT = 20;
    
    // Label sizes
    const int FREQ_LABEL_WIDTH = 120;
    const int HISTORY_LABEL_WIDTH = 170;
    const int CHECKBOX_WIDTH = 100;
}

// Create UI controls
void CreateUIControls(HWND hwnd, HINSTANCE hInstance);

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Update control appearance
void UpdateControlsAppearance(HWND hwnd);
