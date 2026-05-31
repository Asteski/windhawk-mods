// ==WindhawkMod==
// @id           toggle-hidden-files
// @name         Toggle Hidden Files
// @description  Toggle the visibility of hidden files in Windows Explorer using Ctrl+H
// @version      1.0.0
// @author       Asteski
// @github       https://github.com/Asteski
// @include      windhawk.exe
// ==/WindhawkMod==

// ==WindhawkModSettings==
/*
- toggleProtectedFiles: true
  $name: Also toggle protected OS files
  $description: When enabled, Ctrl+H will also toggle the visibility of protected operating system files
*/
// ==/WindhawkModSettings==

// ==WindhawkModReadme==
/*
# Toggle Hidden Files

This mod allows you to toggle the visibility of hidden files in Windows Explorer using the Ctrl+H keyboard shortcut.

## Features
- Ctrl+H hotkey that works only when Explorer windows are focused
- Toggles the "Show hidden files" setting
- Optional: Also toggle protected OS files
- Automatically refreshes Explorer windows
- Works with all Windows Explorer windows

## Usage
1. **Focus an Explorer window** - Click on or open any File Explorer window
2. **Press Ctrl+H** - Use the keyboard shortcut to toggle hidden files visibility
3. **The setting will be applied immediately** to all Explorer windows

## Settings
- **Also toggle protected OS files**: When enabled, Ctrl+H will also show/hide protected operating system files

## Technical Details
- Only activates when Windows Explorer windows are in focus
- Modifies the standard registry settings for showing hidden files
- Sends refresh messages to all Explorer windows
- Handles proper cleanup when the mod is unloaded
- Explorer process must be restarted for changes to take effect
*/
// ==/WindhawkModReadme==

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

// Settings structure
struct {
    bool toggleProtectedFiles;
} g_settings;

// Global variables
HHOOK g_hKeyboardHook = nullptr;
bool g_modEnabled = false;

// Registry keys and values for hidden files settings
const wchar_t* EXPLORER_ADVANCED_KEY = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
const wchar_t* HIDDEN_FILES_VALUE = L"Hidden";
const wchar_t* SUPER_HIDDEN_VALUE = L"ShowSuperHidden";

const DWORD SHOW_HIDDEN = 1;
const DWORD HIDE_HIDDEN = 2;
const DWORD SHOW_SUPER_HIDDEN = 1;
const DWORD HIDE_SUPER_HIDDEN = 0;

// Window context enumeration
enum WindowContext {
    CONTEXT_UNKNOWN = 0,
    CONTEXT_EXPLORER = 1,
    CONTEXT_DESKTOP = 2
};

// Function declarations
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
bool ToggleHiddenFiles();
bool ToggleProtectedFiles();
void RefreshAllExplorerWindows();
bool IsCtrlHPressed(WPARAM wParam, LPARAM lParam);
void LoadSettings();
WindowContext GetCurrentWindowContext();
DWORD GetHiddenFilesSetting();
bool SetHiddenFilesSetting(DWORD dwValue);
DWORD GetProtectedFilesSetting();
bool SetProtectedFilesSetting(DWORD dwValue);

// Get current window context based on focused window
WindowContext GetCurrentWindowContext() {
    HWND hForeground = GetForegroundWindow();
    if (!hForeground) {
        return CONTEXT_UNKNOWN;
    }
    
    wchar_t className[256];
    if (GetClassNameW(hForeground, className, sizeof(className) / sizeof(wchar_t)) == 0) {
        return CONTEXT_UNKNOWN;
    }
    
    // Check for Explorer windows
    if (wcscmp(className, L"CabinetWClass") == 0 || 
        wcscmp(className, L"ExploreWClass") == 0) {
        return CONTEXT_EXPLORER;
    }
    
    // Check for Desktop
    if (wcscmp(className, L"Progman") == 0 || 
        wcscmp(className, L"WorkerW") == 0) {
        return CONTEXT_DESKTOP;
    }
    
    // Also check if it's a desktop child window
    HWND hDesktop = GetShellWindow();
    if (hDesktop && (hForeground == hDesktop || IsChild(hDesktop, hForeground))) {
        return CONTEXT_DESKTOP;
    }
    
    return CONTEXT_UNKNOWN;
}

// Get current hidden files setting from registry
DWORD GetHiddenFilesSetting() {
    HKEY hKey;
    DWORD dwValue = HIDE_HIDDEN; // Default to hidden
    DWORD dwSize = sizeof(DWORD);
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, EXPLORER_ADVANCED_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, HIDDEN_FILES_VALUE, nullptr, nullptr, (LPBYTE)&dwValue, &dwSize);
        RegCloseKey(hKey);
    }
    
    return dwValue;
}

// Set hidden files setting in registry
bool SetHiddenFilesSetting(DWORD dwValue) {
    HKEY hKey;
    bool success = false;
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, EXPLORER_ADVANCED_KEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (RegSetValueExW(hKey, HIDDEN_FILES_VALUE, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD)) == ERROR_SUCCESS) {
            success = true;
        }
        RegCloseKey(hKey);
    }
    
    return success;
}

// Get current protected files setting from registry
DWORD GetProtectedFilesSetting() {
    HKEY hKey;
    DWORD dwValue = HIDE_SUPER_HIDDEN; // Default to hidden
    DWORD dwSize = sizeof(DWORD);
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, EXPLORER_ADVANCED_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, SUPER_HIDDEN_VALUE, nullptr, nullptr, (LPBYTE)&dwValue, &dwSize);
        RegCloseKey(hKey);
    }
    
    return dwValue;
}

// Set protected files setting in registry
bool SetProtectedFilesSetting(DWORD dwValue) {
    HKEY hKey;
    bool success = false;
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, EXPLORER_ADVANCED_KEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (RegSetValueExW(hKey, SUPER_HIDDEN_VALUE, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD)) == ERROR_SUCCESS) {
            success = true;
        }
        RegCloseKey(hKey);
    }
    
    return success;
}

// Toggle hidden files setting
bool ToggleHiddenFiles() {
    DWORD currentSetting = GetHiddenFilesSetting();
    DWORD newSetting = (currentSetting == SHOW_HIDDEN) ? HIDE_HIDDEN : SHOW_HIDDEN;
    
    return SetHiddenFilesSetting(newSetting);
}

// Toggle protected files setting
bool ToggleProtectedFiles() {
    DWORD currentSetting = GetProtectedFilesSetting();
    DWORD newSetting = (currentSetting == SHOW_SUPER_HIDDEN) ? HIDE_SUPER_HIDDEN : SHOW_SUPER_HIDDEN;
    
    return SetProtectedFilesSetting(newSetting);
}

// Load settings from Windhawk configuration
void LoadSettings() {
    // Default values
    g_settings.toggleProtectedFiles = true;
}

// Refresh all Explorer windows
void RefreshAllExplorerWindows() {
    // Send a message to all windows to refresh their view
    SendNotifyMessageW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState");
    
    // Also try to refresh specifically Explorer windows
    HWND hWnd = nullptr;
    while ((hWnd = FindWindowExW(nullptr, hWnd, L"CabinetWClass", nullptr)) != nullptr) {
        SendNotifyMessageW(hWnd, WM_COMMAND, 41504, 0); // Refresh command
    }
    
    // Also check ExploreWClass windows
    hWnd = nullptr;
    while ((hWnd = FindWindowExW(nullptr, hWnd, L"ExploreWClass", nullptr)) != nullptr) {
        SendNotifyMessageW(hWnd, WM_COMMAND, 41504, 0);
    }
    
    // Refresh desktop as well
    HWND hDesktop = GetShellWindow();
    if (hDesktop) {
        SendNotifyMessageW(hDesktop, WM_COMMAND, 41504, 0);
    }
}

// Check if Ctrl+H is pressed
bool IsCtrlHPressed(WPARAM wParam, LPARAM lParam) {
    if (wParam != WM_KEYDOWN) {
        return false;
    }
    
    KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
    
    // Check if 'H' key is pressed
    if (pKeyboard->vkCode != 'H') {
        return false;
    }
    
    // Check if Ctrl is pressed
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
}

// Keyboard hook procedure
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_modEnabled) {
        WindowContext context = GetCurrentWindowContext();
        
        // Only process if we're in Explorer windows
        if (context == CONTEXT_EXPLORER && IsCtrlHPressed(wParam, lParam)) {
            // Toggle hidden files
            bool success = ToggleHiddenFiles();
            
            // Also toggle protected files if setting is enabled
            if (g_settings.toggleProtectedFiles) {
                success = ToggleProtectedFiles() && success;
            }
            
            if (success) {
                RefreshAllExplorerWindows();
            }
            
            // Consume the key press
            return 1;
        }
    }
    
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// Mod initialization
BOOL WhTool_ModInit() {
    // Load settings
    LoadSettings();
    
    // Install keyboard hook
    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(nullptr), 0);
    
    if (!g_hKeyboardHook) {
        return FALSE;
    }
    
    g_modEnabled = true;
    return TRUE;
}

// Settings changed callback
void WhTool_ModSettingsChanged() {
    LoadSettings();
}

// Mod cleanup
void WhTool_ModUninit() {
    g_modEnabled = false;
    
    // Remove keyboard hook
    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Windhawk tool mod implementation for mods which don't need to inject to other
// processes or hook other functions. Context:
// https://github.com/ramensoftware/windhawk-mods/pull/1916
//
// The mod will load and run in a dedicated windhawk.exe process.
//
// Paste the code below as part of the mod code, and use these callbacks:
// * WhTool_ModInit
// * WhTool_ModSettingsChanged
// * WhTool_ModUninit
//
// Currently, other callbacks are not supported.

bool g_isToolModProcessLauncher;
HANDLE g_toolModProcessMutex;

void WINAPI EntryPoint_Hook() {
    Wh_Log(L">");
    ExitThread(0);
}

BOOL WhTool_ModInit() {
    bool isService = false;
    bool isToolModProcess = false;
    bool isCurrentToolModProcess = false;
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (!argv) {
        Wh_Log(L"CommandLineToArgvW failed");
        return FALSE;
    }

    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-service") == 0) {
            isService = true;
            break;
        }
    }

    for (int i = 1; i < argc - 1; i++) {
        if (wcscmp(argv[i], L"-tool-mod") == 0) {
            isToolModProcess = true;
            if (wcscmp(argv[i + 1], WH_MOD_ID) == 0) {
                isCurrentToolModProcess = true;
            }
            break;
        }
    }

    LocalFree(argv);

    if (isService) {
        return FALSE;
    }

    if (isCurrentToolModProcess) {
        g_toolModProcessMutex =
            CreateMutex(nullptr, TRUE, L"windhawk-tool-mod_" WH_MOD_ID);
        if (!g_toolModProcessMutex) {
            Wh_Log(L"CreateMutex failed");
            ExitProcess(1);
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            Wh_Log(L"Tool mod already running (%s)", WH_MOD_ID);
            ExitProcess(1);
        }

        if (!WhTool_ModInit()) {
            ExitProcess(1);
        }

        IMAGE_DOS_HEADER* dosHeader =
            (IMAGE_DOS_HEADER*)GetModuleHandle(nullptr);
        IMAGE_NT_HEADERS* ntHeaders =
            (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);

        DWORD entryPointRVA = ntHeaders->OptionalHeader.AddressOfEntryPoint;
        void* entryPoint = (BYTE*)dosHeader + entryPointRVA;

        Wh_SetFunctionHook(entryPoint, (void*)EntryPoint_Hook, nullptr);
        return TRUE;
    }

    if (isToolModProcess) {
        return FALSE;
    }

    g_isToolModProcessLauncher = true;
    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_isToolModProcessLauncher) {
        return;
    }

    WCHAR currentProcessPath[MAX_PATH];
    switch (GetModuleFileName(nullptr, currentProcessPath,
                              ARRAYSIZE(currentProcessPath))) {
        case 0:
        case ARRAYSIZE(currentProcessPath):
            Wh_Log(L"GetModuleFileName failed");
            return;
    }

    WCHAR
    commandLine[MAX_PATH + 2 +
                (sizeof(L" -tool-mod \"" WH_MOD_ID "\"") / sizeof(WCHAR)) - 1];
    swprintf_s(commandLine, L"\"%s\" -tool-mod \"%s\"", currentProcessPath,
               WH_MOD_ID);

    HMODULE kernelModule = GetModuleHandle(L"kernelbase.dll");
    if (!kernelModule) {
        kernelModule = GetModuleHandle(L"kernel32.dll");
        if (!kernelModule) {
            Wh_Log(L"No kernelbase.dll/kernel32.dll");
            return;
        }
    }

    using CreateProcessInternalW_t = BOOL(WINAPI*)(
        HANDLE hUserToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes, WINBOOL bInheritHandles,
        DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation,
        PHANDLE hRestrictedUserToken);
    CreateProcessInternalW_t pCreateProcessInternalW =
        (CreateProcessInternalW_t)GetProcAddress(kernelModule,
                                                 "CreateProcessInternalW");
    if (!pCreateProcessInternalW) {
        Wh_Log(L"No CreateProcessInternalW");
        return;
    }

    STARTUPINFO si{
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_FORCEOFFFEEDBACK,
    };
    PROCESS_INFORMATION pi;
    if (!pCreateProcessInternalW(nullptr, currentProcessPath, commandLine,
                                 nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS,
                                 nullptr, nullptr, &si, &pi, nullptr)) {
        Wh_Log(L"CreateProcess failed");
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void WhTool_ModSettingsChanged() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModSettingsChanged();
}

void WhTool_ModUninit() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModUninit();
    ExitProcess(0);
}