// ==WindhawkMod==
// @id           toggle-navigation-pane
// @name         Toggle Navigation Pane
// @description  Toggle the visibility of the navigation pane in Windows Explorer using Ctrl+Alt+P
// @version      1.1.0
// @author       Asteski
// @github       https://github.com/Asteski
// @include      windhawk.exe
// @compilerOptions -lole32 -loleaut32 -luuid
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Toggle Navigation Pane

This mod allows you to toggle the visibility of the navigation pane in Windows
Explorer using the Ctrl+Alt+P keyboard shortcut.

## Features
- Ctrl+Alt+P hotkey that works only when Explorer windows are focused
- Toggles the navigation pane live on the focused Explorer window
- Triggers the exact built-in Explorer command used by the right-click
  "Navigation pane" context-menu entry
- The change applies instantly with no Explorer restart and is persisted, so
  newly opened windows reflect it too

## Usage
1. **Focus an Explorer window** - Click on or open any File Explorer window
2. **Press Ctrl+Alt+P** - Use the keyboard shortcut to toggle the navigation pane

## Technical Details
- Only activates when Windows Explorer windows are in focus
- Registers a hidden, per-user shell verb (HKCU, no elevation needed) that
  maps to Explorer's built-in navigation-pane toggle command via its
  CanonicalName/PaneID/PaneVisibleProperty
- On the hotkey, locates the focused Explorer window through the Shell COM
  automation and invokes that verb against its active view's background,
  exactly as the context-menu entry does
- COM work runs on a dedicated worker thread to keep the keyboard hook fast
- Cleans up the keyboard hook, worker thread, and registered verb on unload
*/
// ==/WindhawkModReadme==

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>
#include <exdisp.h>
#include <servprov.h>

// Global variables
HHOOK g_hKeyboardHook = nullptr;
bool g_modEnabled = false;

// Worker thread that performs the COM-based toggle off the keyboard hook
// thread, so the low-level hook callback stays fast.
HANDLE g_workerThread = nullptr;
DWORD g_workerThreadId = 0;
#define WM_APP_TOGGLE_NAV_PANE (WM_APP + 1)

// Built-in Explorer "toggle navigation pane" command, exposed as a shell verb
// via the CanonicalName/PaneID/PaneVisibleProperty trio. Registering these
// under a class's shell key lets Explorer toggle the pane live on the active
// window (and persist the setting) the same way the right-click entry does.
const wchar_t* NAVPANE_VERB = L"WindhawkToggleNavPane";
const char* NAVPANE_VERB_A = "WindhawkToggleNavPane";
const wchar_t* NAVPANE_CANONICAL_NAME = L"{41dd5f6f-9f9e-4066-8836-722fe4bb950e}";
const wchar_t* NAVPANE_PANE_ID = L"{cb316b22-25f7-42b8-8a09-540d23a43c2f}";
const wchar_t* NAVPANE_PANE_VISIBLE_PROPERTY = L"PageSpaceControlSizer_Visible";

// Classes whose shell key carries the verb. Directory\Background and
// LibraryFolder\Background cover the folder-background context used when the
// pane is toggled against the active view.
const wchar_t* g_navPaneClasses[] = {
    L"Directory\\Background",
    L"LibraryFolder\\Background",
    L"Drive",
    L"AllFilesystemObjects",
};

// Window context enumeration
enum WindowContext {
    CONTEXT_UNKNOWN = 0,
    CONTEXT_EXPLORER = 1,
    CONTEXT_DESKTOP = 2
};

// Function declarations
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
bool IsCtrlAltPPressed(WPARAM wParam, LPARAM lParam);
WindowContext GetCurrentWindowContext();
void RegisterNavPaneVerb();
void UnregisterNavPaneVerb();
bool InvokeNavPaneToggleForWindow(HWND hwndTarget);

// Register the hidden navigation-pane verb per-user (no elevation needed).
// ProgrammaticAccessOnly keeps it out of the right-click menu while still
// allowing IContextMenu::InvokeCommand to invoke it by name.
void RegisterNavPaneVerb() {
    for (const wchar_t* cls : g_navPaneClasses) {
        wchar_t subkey[512];
        swprintf(subkey, ARRAYSIZE(subkey),
                 L"Software\\Classes\\%s\\shell\\%s", cls, NAVPANE_VERB);

        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, subkey, 0, nullptr, 0, KEY_WRITE,
                            nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
            continue;
        }

        auto setString = [&](const wchar_t* name, const wchar_t* value) {
            RegSetValueExW(hKey, name, 0, REG_SZ, (const BYTE*)value,
                           (DWORD)((wcslen(value) + 1) * sizeof(wchar_t)));
        };

        setString(L"CanonicalName", NAVPANE_CANONICAL_NAME);
        setString(L"PaneID", NAVPANE_PANE_ID);
        setString(L"PaneVisibleProperty", NAVPANE_PANE_VISIBLE_PROPERTY);
        setString(L"ProgrammaticAccessOnly", L"");

        RegCloseKey(hKey);
    }
}

// Remove the verb keys created in RegisterNavPaneVerb.
void UnregisterNavPaneVerb() {
    for (const wchar_t* cls : g_navPaneClasses) {
        wchar_t subkey[512];
        swprintf(subkey, ARRAYSIZE(subkey),
                 L"Software\\Classes\\%s\\shell\\%s", cls, NAVPANE_VERB);
        RegDeleteTreeW(HKEY_CURRENT_USER, subkey);
    }
}

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

// Locate the Explorer window matching hwndTarget and invoke the navigation
// pane toggle verb against its active shell view's background, mirroring the
// context-menu command exactly. Must run on a COM-initialized thread.
bool InvokeNavPaneToggleForWindow(HWND hwndTarget) {
    Wh_Log(L"InvokeNavPaneToggleForWindow target=%p", (void*)hwndTarget);

    IShellWindows* psw = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&psw));
    if (FAILED(hr)) {
        Wh_Log(L"CoCreateInstance(ShellWindows) failed hr=0x%08X", hr);
        return false;
    }

    bool done = false;
    bool matched = false;
    long count = 0;
    psw->get_Count(&count);
    Wh_Log(L"ShellWindows count=%ld", count);

    for (long i = 0; i < count && !done; i++) {
        VARIANT vIndex;
        VariantInit(&vIndex);
        V_VT(&vIndex) = VT_I4;
        V_I4(&vIndex) = i;

        IDispatch* pdisp = nullptr;
        if (psw->Item(vIndex, &pdisp) != S_OK || !pdisp) {
            VariantClear(&vIndex);
            continue;
        }
        VariantClear(&vIndex);

        IWebBrowserApp* pwba = nullptr;
        if (SUCCEEDED(pdisp->QueryInterface(IID_PPV_ARGS(&pwba)))) {
            SHANDLE_PTR hwnd = 0;
            if (SUCCEEDED(pwba->get_HWND(&hwnd)) && (HWND)hwnd == hwndTarget) {
                matched = true;
                Wh_Log(L"Matched ShellWindow hwnd=%p", (void*)hwnd);
                IServiceProvider* psp = nullptr;
                if (SUCCEEDED(pwba->QueryInterface(IID_PPV_ARGS(&psp)))) {
                    IShellBrowser* psb = nullptr;
                    hr = psp->QueryService(SID_STopLevelBrowser,
                                           IID_PPV_ARGS(&psb));
                    if (SUCCEEDED(hr)) {
                        IShellView* psv = nullptr;
                        hr = psb->QueryActiveShellView(&psv);
                        if (SUCCEEDED(hr) && psv) {
                            IContextMenu* pcm = nullptr;
                            hr = psv->GetItemObject(SVGIO_BACKGROUND,
                                                    IID_PPV_ARGS(&pcm));
                            if (SUCCEEDED(hr) && pcm) {
                                HMENU hMenu = CreatePopupMenu();
                                if (hMenu) {
                                    hr = pcm->QueryContextMenu(
                                        hMenu, 0, 1, 0x7FFF, CMF_NORMAL);
                                    Wh_Log(L"QueryContextMenu hr=0x%08X", hr);
                                    if (SUCCEEDED(hr)) {
                                        CMINVOKECOMMANDINFO ici = {0};
                                        ici.cbSize = sizeof(ici);
                                        ici.lpVerb = NAVPANE_VERB_A;
                                        ici.nShow = SW_SHOWNORMAL;
                                        hr = pcm->InvokeCommand(&ici);
                                        Wh_Log(L"InvokeCommand hr=0x%08X", hr);
                                        if (SUCCEEDED(hr)) {
                                            done = true;
                                        }
                                    }
                                    DestroyMenu(hMenu);
                                }
                                pcm->Release();
                            } else {
                                Wh_Log(L"GetItemObject(background) hr=0x%08X", hr);
                            }
                            psv->Release();
                        } else {
                            Wh_Log(L"QueryActiveShellView hr=0x%08X", hr);
                        }
                        psb->Release();
                    } else {
                        Wh_Log(L"QueryService(TopLevelBrowser) hr=0x%08X", hr);
                    }
                    psp->Release();
                }
            }
            pwba->Release();
        }
        pdisp->Release();
    }

    if (!matched) {
        Wh_Log(L"No ShellWindow matched target hwnd");
    }

    psw->Release();
    return done;
}

// Worker thread: owns an STA apartment and a message loop so the toggle runs
// off the low-level keyboard hook callback.
DWORD WINAPI WorkerThreadProc(LPVOID lpParam) {
    // Force the message queue to exist, then signal that posting is safe.
    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    if (lpParam) {
        SetEvent((HANDLE)lpParam);
    }

    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_APP_TOGGLE_NAV_PANE) {
            InvokeNavPaneToggleForWindow((HWND)msg.lParam);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (SUCCEEDED(hrCo)) {
        CoUninitialize();
    }
    return 0;
}

// Check if Ctrl+Alt+P is pressed
bool IsCtrlAltPPressed(WPARAM wParam, LPARAM lParam) {
    if (wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN) {
        return false;
    }

    KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

    // Check if 'P' key is pressed
    if (pKeyboard->vkCode != 'P') {
        return false;
    }

    // Check if both Ctrl and Alt are pressed
    bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

    return ctrlDown && altDown;
}

// Keyboard hook procedure
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_modEnabled) {
        WindowContext context = GetCurrentWindowContext();

        // Only process if we're in Explorer windows
        if (context == CONTEXT_EXPLORER && IsCtrlAltPPressed(wParam, lParam)) {
            HWND hForeground = GetForegroundWindow();
            Wh_Log(L"Hotkey detected, foreground=%p workerTid=%lu",
                   (void*)hForeground, g_workerThreadId);
            if (hForeground && g_workerThreadId) {
                PostThreadMessageW(g_workerThreadId, WM_APP_TOGGLE_NAV_PANE, 0,
                                   (LPARAM)hForeground);
            }

            // Consume the key press
            return 1;
        }
    }

    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// Mod initialization
BOOL WhTool_ModInit() {
    RegisterNavPaneVerb();

    // Start the COM worker thread and wait until its message queue is ready.
    HANDLE hReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_workerThread =
        CreateThread(nullptr, 0, WorkerThreadProc, hReady, 0, &g_workerThreadId);
    if (!g_workerThread) {
        if (hReady) {
            CloseHandle(hReady);
        }
        UnregisterNavPaneVerb();
        return FALSE;
    }
    if (hReady) {
        // The thread id from CreateThread is valid; give the queue a moment to
        // be created before any messages are posted.
        WaitForSingleObject(hReady, 2000);
        CloseHandle(hReady);
    }

    // Install keyboard hook
    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc,
                                        GetModuleHandle(nullptr), 0);

    if (!g_hKeyboardHook) {
        if (g_workerThreadId) {
            PostThreadMessageW(g_workerThreadId, WM_QUIT, 0, 0);
        }
        WaitForSingleObject(g_workerThread, 2000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
        g_workerThreadId = 0;
        UnregisterNavPaneVerb();
        return FALSE;
    }

    g_modEnabled = true;
    return TRUE;
}

// Settings changed callback
void WhTool_ModSettingsChanged() {
}

// Mod cleanup
void WhTool_ModUninit() {
    g_modEnabled = false;

    // Remove keyboard hook
    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = nullptr;
    }

    // Stop the worker thread
    if (g_workerThreadId) {
        PostThreadMessageW(g_workerThreadId, WM_QUIT, 0, 0);
    }
    if (g_workerThread) {
        WaitForSingleObject(g_workerThread, 2000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }
    g_workerThreadId = 0;

    UnregisterNavPaneVerb();
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

BOOL Wh_ModInit() {
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
    swprintf(commandLine, ARRAYSIZE(commandLine), L"\"%s\" -tool-mod \"%s\"",
             currentProcessPath, WH_MOD_ID);

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

void Wh_ModSettingsChanged() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModSettingsChanged();
}

void Wh_ModUninit() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModUninit();
    ExitProcess(0);
}
