// ==WindhawkMod==
// @id              auto-dark-titlebar
// @name            Auto Dark Titlebar
// @description     Automatically enables/disables dark titlebars based on Windows theme mode
// @version         1.0.0
// @author          Asteski
// @github          https://github.com/Asteski
// @include         *
// @compilerOptions -ldwmapi -luxtheme
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Auto Dark Titlebar

This mod automatically switches titlebar dark mode based on your Windows system theme.
When Windows is in dark mode, it enables dark titlebars for all eligible windows.
When Windows switches to light mode, it disables dark titlebars.

The mod listens for theme changes in real-time and updates all windows accordingly.

## How it works
- Monitors Windows theme mode changes (WM_DWMCOLORIZATIONCOLORCHANGED, WM_SETTINGCHANGE)
- Automatically applies DWMWA_USE_IMMERSIVE_DARK_MODE attribute to windows
- Works with all standard Win32 windows that have titlebars in injected processes
*/
// ==/WindhawkModReadme==

#include <windows.h>
#include <dwmapi.h>

// DWMWA_USE_IMMERSIVE_DARK_MODE attribute
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// Function pointer types
typedef HRESULT(WINAPI* pShouldAppsUseDarkMode)();
typedef HRESULT(WINAPI* pShouldSystemUseDarkMode)();

// Global variables
static pShouldSystemUseDarkMode g_ShouldSystemUseDarkMode = nullptr;
static BOOL g_isDarkMode = FALSE;

// Check if system is using dark mode
BOOL IsSystemDarkMode() {
    // Check registry first (most reliable)
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        DWORD value = 0;
        DWORD size = sizeof(DWORD);
        // AppsUseLightTheme: 0 = dark mode, 1 = light mode
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
            (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return value == 0;
        }
        RegCloseKey(hKey);
    }
    
    // Fallback: Try uxtheme function
    if (!g_ShouldSystemUseDarkMode) {
        HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
        if (hUxtheme) {
            // Ordinal 138 is ShouldSystemUseDarkMode
            g_ShouldSystemUseDarkMode = (pShouldSystemUseDarkMode)GetProcAddress(
                hUxtheme, MAKEINTRESOURCEA(138));
        }
    }
    
    if (g_ShouldSystemUseDarkMode) {
        return g_ShouldSystemUseDarkMode() != 0;
    }
    
    return FALSE;
}

// Check if current process should be excluded
BOOL IsProcessExcluded() {
    static int isExcluded = -1; // -1 = not checked, 0 = not excluded, 1 = excluded
    
    if (isExcluded != -1) {
        return isExcluded == 1;
    }
    
    WCHAR exePath[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) {
        isExcluded = 0;
        return FALSE;
    }
    
    // Get just the filename
    WCHAR* fileName = wcsrchr(exePath, L'\\');
    if (fileName) {
        fileName++; // Skip the backslash
    } else {
        fileName = exePath;
    }
    
    // Convert to lowercase for comparison
    for (WCHAR* p = fileName; *p; p++) {
        *p = towlower(*p);
    }
    
    // List of excluded processes
    const WCHAR* excludedProcesses[] = {
        L"systemsettings.exe",
        L"applicationframehost.exe", // UWP app host
        nullptr
    };
    
    for (int i = 0; excludedProcesses[i] != nullptr; i++) {
        if (wcscmp(fileName, excludedProcesses[i]) == 0) {
            Wh_Log(L"Process excluded: %s", fileName);
            isExcluded = 1;
            return TRUE;
        }
    }
    
    isExcluded = 0;
    return FALSE;
}

// Check if window is eligible for dark mode
BOOL IsWindowEligible(HWND hWnd) {
    if (!hWnd || !IsWindow(hWnd))
        return FALSE;
        
    // Get window styles
    LONG style = GetWindowLongW(hWnd, GWL_STYLE);
    LONG styleEx = GetWindowLongW(hWnd, GWL_EXSTYLE);
    
    // Must have a titlebar (caption)
    if (!(style & WS_CAPTION))
        return FALSE;
        
    // Skip tool windows
    if (styleEx & WS_EX_TOOLWINDOW)
        return FALSE;
        
    // Skip child windows
    if (style & WS_CHILD)
        return FALSE;
    
    return TRUE;
}

// Apply dark mode to a window
VOID ApplyDarkMode(HWND hWnd, BOOL useDarkMode) {
    if (!IsWindowEligible(hWnd))
        return;
        
    BOOL value = useDarkMode ? TRUE : FALSE;
    HRESULT hr = DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, 
        &value, sizeof(value));
    
    if (SUCCEEDED(hr)) {
        // Force window to redraw titlebar
        SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, 
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
        Wh_Log(L"Applied dark mode (%d) to window: %p", useDarkMode, hWnd);
    }
}

// Apply dark mode to a specific window (called from hook)
VOID NewWindowShown(HWND hWnd) {
    if (IsProcessExcluded())
        return;
        
    if (!hWnd || !IsWindow(hWnd))
        return;
        
    if (!IsWindowEligible(hWnd))
        return;
    
    Wh_Log(L"New window detected: %p, applying dark mode: %d", hWnd, g_isDarkMode);
    ApplyDarkMode(hWnd, g_isDarkMode);
}

// Enumerate callback for existing windows
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    BOOL useDarkMode = (BOOL)lParam;
    
    // Skip if not a top-level window
    HWND hParentWnd = GetAncestor(hWnd, GA_PARENT);
    if (hParentWnd && hParentWnd != GetDesktopWindow())
        return TRUE;
    
    // Check if window belongs to current process
    DWORD dwProcessId = 0;
    if (!GetWindowThreadProcessId(hWnd, &dwProcessId) || 
        dwProcessId != GetCurrentProcessId())
        return TRUE;
    
    ApplyDarkMode(hWnd, useDarkMode);
    return TRUE;
}

// Apply dark mode to all existing windows in current process
VOID ApplyToAllWindows(BOOL useDarkMode) {
    EnumWindows(EnumWindowsProc, (LPARAM)useDarkMode);
}

// Hook DefWindowProc to catch theme changes
using DefWindowProc_t = decltype(&DefWindowProcW);
DefWindowProc_t DefWindowProc_orig;

LRESULT WINAPI DefWindowProc_hook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    // Detect theme change messages
    if (!IsProcessExcluded() && (Msg == WM_DWMCOLORIZATIONCOLORCHANGED || Msg == WM_SETTINGCHANGE)) {
        BOOL newDarkMode = IsSystemDarkMode();
        
        if (newDarkMode != g_isDarkMode) {
            g_isDarkMode = newDarkMode;
            Wh_Log(L"[Process %d] Theme changed to %s mode", 
                GetCurrentProcessId(), newDarkMode ? L"DARK" : L"LIGHT");
            
            // Apply to all windows in current process
            ApplyToAllWindows(g_isDarkMode);
        }
    }
    
    return DefWindowProc_orig(hWnd, Msg, wParam, lParam);
}

// Hook NtUserCreateWindowEx to catch new windows
using NtUserCreateWindowEx_t = HWND(WINAPI*)(
    DWORD dwExStyle, PVOID pClassName, LPCWSTR pWindowName, PVOID pWindowNameU,
    DWORD dwStyle, LONG x, LONG y, LONG nWidth, LONG nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam,
    DWORD dwShowMode, DWORD dwUnknown1, DWORD dwUnknown2, VOID* qwUnknown3);

NtUserCreateWindowEx_t NtUserCreateWindowEx_orig;

HWND WINAPI NtUserCreateWindowEx_hook(
    DWORD dwExStyle, PVOID pClassName, LPCWSTR pWindowName, PVOID pWindowNameU,
    DWORD dwStyle, LONG x, LONG y, LONG nWidth, LONG nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam,
    DWORD dwShowMode, DWORD dwUnknown1, DWORD dwUnknown2, VOID* qwUnknown3) {
    
    HWND hWnd = NtUserCreateWindowEx_orig(
        dwExStyle, pClassName, pWindowName, pWindowNameU, dwStyle,
        x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam,
        dwShowMode, dwUnknown1, dwUnknown2, qwUnknown3);
    
    if (hWnd) {
        // Apply dark mode to newly created window
        NewWindowShown(hWnd);
    }
    
    return hWnd;
}

// Windhawk mod initialization
BOOL Wh_ModInit() {
    Wh_Log(L"=======================================");
    Wh_Log(L"[Process %d] Initializing Auto Dark Titlebar mod", GetCurrentProcessId());
    
    // Check if this process should be excluded
    if (IsProcessExcluded()) {
        Wh_Log(L"[Process %d] Process is excluded, skipping initialization", GetCurrentProcessId());
        Wh_Log(L"=======================================");
        return TRUE; // Return TRUE so mod doesn't fail, just does nothing
    }
    
    // Get initial dark mode state
    g_isDarkMode = IsSystemDarkMode();
    Wh_Log(L"[Process %d] Initial theme mode: %s", 
        GetCurrentProcessId(), g_isDarkMode ? L"DARK" : L"LIGHT");
    
    // Hook DefWindowProc to detect theme changes (works globally)
    if (!Wh_SetFunctionHook((void*)DefWindowProcW, (void*)DefWindowProc_hook,
        (void**)&DefWindowProc_orig)) {
        Wh_Log(L"[Process %d] ERROR: Failed to hook DefWindowProcW", GetCurrentProcessId());
    } else {
        Wh_Log(L"[Process %d] Successfully hooked DefWindowProcW", GetCurrentProcessId());
    }
    
    // Hook NtUserCreateWindowEx to catch new windows
    HMODULE hWin32u = GetModuleHandleW(L"win32u.dll");
    if (!hWin32u) {
        Wh_Log(L"[Process %d] WARNING: Failed to get win32u.dll", GetCurrentProcessId());
        return TRUE; // Don't fail completely
    }
    
    void* pNtUserCreateWindowEx = (void*)GetProcAddress(hWin32u, "NtUserCreateWindowEx");
    if (!pNtUserCreateWindowEx) {
        Wh_Log(L"[Process %d] WARNING: Failed to get NtUserCreateWindowEx", GetCurrentProcessId());
        return TRUE; // Don't fail completely
    }
    
    if (!Wh_SetFunctionHook(pNtUserCreateWindowEx, (void*)NtUserCreateWindowEx_hook,
        (void**)&NtUserCreateWindowEx_orig)) {
        Wh_Log(L"[Process %d] ERROR: Failed to hook NtUserCreateWindowEx", GetCurrentProcessId());
    } else {
        Wh_Log(L"[Process %d] Successfully hooked NtUserCreateWindowEx", GetCurrentProcessId());
    }
    
    Wh_Log(L"[Process %d] Initialization complete", GetCurrentProcessId());
    Wh_Log(L"=======================================");
    
    return TRUE;
}

// Apply to existing windows after initialization
VOID Wh_ModAfterInit() {
    if (IsProcessExcluded()) {
        return;
    }
    
    Wh_Log(L"[Process %d] Applying dark mode to existing windows...", GetCurrentProcessId());
    ApplyToAllWindows(g_isDarkMode);
    Wh_Log(L"[Process %d] Finished applying to existing windows", GetCurrentProcessId());
}

// Cleanup when mod is unloaded
VOID Wh_ModUninit() {
    if (IsProcessExcluded()) {
        return;
    }
    
    Wh_Log(L"[Process %d] Uninitializing Auto Dark Titlebar mod", GetCurrentProcessId());
    
    // Restore to default (remove dark mode attribute)
    ApplyToAllWindows(FALSE);
    
    Wh_Log(L"[Process %d] Cleanup complete", GetCurrentProcessId());
}