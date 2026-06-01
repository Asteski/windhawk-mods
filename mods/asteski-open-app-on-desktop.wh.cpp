// ==WindhawkMod==
// @id              asteski-open-app-on-desktop
// @name            Open App on Virtual Desktop
// @description     Open selected apps on a specific virtual desktop, creating it if needed.
// @version         1.0
// @author          Asteski
// @license         MIT
// @github          https://github.com/Asteski
// @include         *
// @compilerOptions -lole32 -luser32 -lshlwapi
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Open App on Virtual Desktop

Sends selected applications to a specific virtual desktop when their windows
appear. If the target desktop does not exist yet, it is created automatically.

This lets you click an app icon, run it from the Run dialog or a terminal, and
have it land on a fixed desktop, with no per-launch command needed.

## How it works
- The mod runs inside every process and checks the process executable name.
- If it matches one of your rules, newly shown top-level windows are moved to
  the configured desktop number.
- If fewer desktops exist than the configured number, empty desktops are created
  until that number is reached.
- Each rule can either follow the app to its desktop, or leave the app running
  in the background on its target desktop.

## Rules
Each rule has:
- **Executable name(s)**: one or multiple names in one field.
- **Desktop number**: 1-based. Desktop 1 is your first/leftmost desktop.
- **Switch to that desktop**: follow the app, or keep your current desktop.

## Executable name format
- Case-insensitive.
- You can write `notepad` or `notepad.exe`.
- You can include multiple values separated by `,` `;` or `|`.
- Full paths are also accepted (file name is used).

Examples:
- `notepad.exe`
- `notepad; calc.exe`
- `C:\Windows\System32\mspaint.exe | calc`

## Compatibility
Virtual desktop control on Windows relies on undocumented COM interfaces whose
layout changes between Windows builds. This mod targets **Windows 11 24H2**. On
other builds the desktop calls may silently do nothing and would need updated
interface definitions.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- rules:
    - - executable_names: "notepad.exe"
        $name: Executable name(s)
        $description: "One or more executable names. Separators: comma, semicolon, pipe."
      - desktop_number: 1
        $name: Desktop number
        $description: "1-based virtual desktop. Created if it does not exist."
      - switch_to_desktop: false
        $name: Switch to that desktop
        $description: "Follow the app to its desktop. If off, the app opens in the background."
  $name: Rules
  $description: "Add one item per executable/desktop combination."
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>
#include <windows.h>
#include <objbase.h>
#include <shlwapi.h>

#include <algorithm>
#include <cwctype>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

// --- Virtual desktop COM interfaces (Windows 11 24H2 layout) ---------------
// GUIDs and vtable order taken from MScholtes/VirtualDesktop (24H2 build).
// Interfaces are given local names to avoid clashing with the public SDK ones.

static const CLSID CLSID_ImmersiveShell =
    {0xC2F03A33, 0x21F5, 0x47FA, {0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39}};
static const CLSID CLSID_VirtualDesktopManagerInternal =
    {0xC5E0CDCA, 0x7B6E, 0x41B2, {0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B}};
static const CLSID CLSID_VirtualDesktopManager =
    {0xAA509086, 0x5CA9, 0x4C25, {0x8F, 0x95, 0x58, 0x9D, 0x3C, 0x07, 0xB4, 0x8A}};

static const IID IID_IVDServiceProvider =
    {0x6D5140C1, 0x7436, 0x11CE, {0x80, 0x34, 0x00, 0xAA, 0x00, 0x60, 0x09, 0xFA}};
static const IID IID_IVDManagerInternal =
    {0x53F5CA0B, 0x158F, 0x4124, {0x90, 0x0C, 0x05, 0x71, 0x58, 0x06, 0x0B, 0x27}};
static const IID IID_IVDesktop =
    {0x3F07F4BE, 0xB107, 0x441A, {0xAF, 0x0F, 0x39, 0xD8, 0x25, 0x29, 0x07, 0x2C}};
static const IID IID_IVDObjectArray =
    {0x92CA9DCD, 0x5622, 0x4BBA, {0xA8, 0x05, 0x5E, 0x9F, 0x54, 0x1B, 0xD8, 0xC9}};
static const IID IID_IVDManagerPublic =
    {0xA5CD92FF, 0x29BE, 0x454C, {0x8D, 0x04, 0xD8, 0x28, 0x79, 0xFB, 0x3F, 0x1B}};

struct IVDObjectArray : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetCount(UINT* pCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAt(UINT index, REFIID riid, void** ppv) = 0;
};

struct IVDesktop : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE IsViewVisible(IUnknown* view, int* pVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetId(GUID* pId) = 0;
    // Remaining methods are unused.
};

struct IVDManagerInternal : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetCount(int* pCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(IUnknown* view, IVDesktop* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(IUnknown* view, int* pCan) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(IVDesktop** ppDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktops(IVDObjectArray** ppDesktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdjacentDesktop(IVDesktop* from, int dir, IVDesktop** ppDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(IVDesktop* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktopAndMoveForegroundView(IVDesktop* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDesktop(IVDesktop** ppNewDesktop) = 0;
    // Remaining methods are unused.
};

struct IVDServiceProvider : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppv) = 0;
};

struct IVDManagerPublic : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE IsWindowOnCurrentVirtualDesktop(HWND hWnd, int* pOnCurrent) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWindowDesktopId(HWND hWnd, GUID* pId) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveWindowToDesktop(HWND hWnd, REFGUID desktopId) = 0;
};

// Don't try to create more desktops than this, in case a rule is misconfigured.
static const int kMaxDesktops = 64;

// --- Mod state -------------------------------------------------------------

struct Rule {
    std::vector<std::wstring> executables;
    int desktopNumber = 1;
    bool switchToDesktop = false;
};

static std::vector<Rule> g_rules;
static std::mutex g_rulesMutex;

static std::unordered_set<HWND> g_movedWindows;
static std::mutex g_movedMutex;

static std::wstring g_currentExeName;
static bool g_isTargetProcess = false;
static int g_targetDesktopNumber = 1;
static bool g_targetSwitch = false;

static thread_local bool g_internalMove = false;

using ShowWindow_t = decltype(&ShowWindow);
using ShowWindowAsync_t = decltype(&ShowWindowAsync);
using SetWindowPos_t = decltype(&SetWindowPos);

static ShowWindow_t ShowWindow_Original;
static ShowWindowAsync_t ShowWindowAsync_Original;
static SetWindowPos_t SetWindowPos_Original;

// --- String helpers --------------------------------------------------------

static std::wstring Trim(const std::wstring& s) {
    size_t start = 0;
    while (start < s.size() && iswspace(s[start])) {
        start++;
    }

    size_t end = s.size();
    while (end > start && iswspace(s[end - 1])) {
        end--;
    }

    return s.substr(start, end - start);
}

static std::wstring ToLower(std::wstring s) {
    for (auto& c : s) {
        c = (wchar_t)towlower(c);
    }
    return s;
}

static std::wstring NormalizeExecutableName(std::wstring name) {
    name = Trim(name);
    if (name.size() >= 2 && name.front() == L'"' && name.back() == L'"') {
        name = name.substr(1, name.size() - 2);
    }

    if (name.empty()) {
        return name;
    }

    const wchar_t* fileName = PathFindFileNameW(name.c_str());
    if (fileName && *fileName) {
        name = fileName;
    }

    name = ToLower(Trim(name));

    if (!name.empty() && name.find(L'.') == std::wstring::npos) {
        name += L".exe";
    }

    return name;
}

static std::vector<std::wstring> SplitExecutableList(const std::wstring& value) {
    std::vector<std::wstring> result;

    size_t start = 0;
    while (start <= value.size()) {
        size_t pos = value.find_first_of(L",;|", start);
        std::wstring part = pos == std::wstring::npos
            ? value.substr(start)
            : value.substr(start, pos - start);

        std::wstring normalized = NormalizeExecutableName(part);
        if (!normalized.empty() &&
            std::find(result.begin(), result.end(), normalized) == result.end()) {
            result.push_back(normalized);
        }

        if (pos == std::wstring::npos) {
            break;
        }
        start = pos + 1;
    }

    return result;
}

static std::wstring GetCurrentProcessExecutableName() {
    wchar_t path[MAX_PATH] = {};
    DWORD n = GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    if (n == 0 || n == ARRAYSIZE(path)) {
        return L"";
    }

    const wchar_t* fileName = PathFindFileNameW(path);
    if (!fileName || !*fileName) {
        return L"";
    }

    return ToLower(fileName);
}

// --- Window tracking -------------------------------------------------------

static void ResetMovedWindows() {
    std::lock_guard<std::mutex> lock(g_movedMutex);
    g_movedWindows.clear();
}

static bool HasMovedWindow(HWND hWnd) {
    std::lock_guard<std::mutex> lock(g_movedMutex);
    return g_movedWindows.find(hWnd) != g_movedWindows.end();
}

static void MarkWindowMoved(HWND hWnd) {
    std::lock_guard<std::mutex> lock(g_movedMutex);
    g_movedWindows.insert(hWnd);
}

static bool IsEligibleTopLevelWindow(HWND hWnd) {
    if (!hWnd || !IsWindow(hWnd)) {
        return false;
    }

    if (GetAncestor(hWnd, GA_ROOT) != hWnd) {
        return false;
    }

    if (GetWindow(hWnd, GW_OWNER) != nullptr) {
        return false;
    }

    LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }

    RECT rc = {};
    if (!GetWindowRect(hWnd, &rc)) {
        return false;
    }

    if (rc.right <= rc.left || rc.bottom <= rc.top) {
        return false;
    }

    return true;
}

// --- Virtual desktop move --------------------------------------------------

static bool MoveWindowToVirtualDesktop(HWND hWnd, int desktopNumber, bool switchTo) {
    if (desktopNumber < 1 || desktopNumber > kMaxDesktops) {
        return false;
    }

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool needUninit = SUCCEEDED(hrInit);

    bool result = false;
    IVDServiceProvider* shell = nullptr;
    IVDManagerInternal* vdmi = nullptr;
    IVDManagerPublic* vdm = nullptr;
    IVDObjectArray* desktops = nullptr;
    IVDesktop* target = nullptr;

    do {
        HRESULT hr = CoCreateInstance(CLSID_ImmersiveShell, nullptr, CLSCTX_ALL,
                                      IID_IVDServiceProvider, (void**)&shell);
        if (FAILED(hr) || !shell) {
            break;
        }

        hr = shell->QueryService(CLSID_VirtualDesktopManagerInternal,
                                 IID_IVDManagerInternal, (void**)&vdmi);
        if (FAILED(hr) || !vdmi) {
            break;
        }

        hr = CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL,
                              IID_IVDManagerPublic, (void**)&vdm);
        if (FAILED(hr) || !vdm) {
            break;
        }

        int count = 0;
        if (FAILED(vdmi->GetCount(&count))) {
            break;
        }

        while (count < desktopNumber) {
            IVDesktop* created = nullptr;
            if (FAILED(vdmi->CreateDesktop(&created)) || !created) {
                break;
            }
            created->Release();
            count++;
        }
        if (count < desktopNumber) {
            break;
        }

        if (FAILED(vdmi->GetDesktops(&desktops)) || !desktops) {
            break;
        }

        hr = desktops->GetAt((UINT)(desktopNumber - 1), IID_IVDesktop, (void**)&target);
        if (FAILED(hr) || !target) {
            break;
        }

        GUID desktopId = {};
        if (FAILED(target->GetId(&desktopId))) {
            break;
        }

        g_internalMove = true;
        HRESULT hrMove = vdm->MoveWindowToDesktop(hWnd, desktopId);
        if (SUCCEEDED(hrMove) && switchTo) {
            vdmi->SwitchDesktop(target);
        }
        g_internalMove = false;

        result = SUCCEEDED(hrMove);
    } while (false);

    if (target) target->Release();
    if (desktops) desktops->Release();
    if (vdm) vdm->Release();
    if (vdmi) vdmi->Release();
    if (shell) shell->Release();

    if (needUninit) {
        CoUninitialize();
    }

    return result;
}

// --- Rule application ------------------------------------------------------

static void TryApplyToWindow(HWND hWnd) {
    if (!g_isTargetProcess) {
        return;
    }

    if (!IsEligibleTopLevelWindow(hWnd)) {
        return;
    }

    if (HasMovedWindow(hWnd)) {
        return;
    }

    if (MoveWindowToVirtualDesktop(hWnd, g_targetDesktopNumber, g_targetSwitch)) {
        MarkWindowMoved(hWnd);
    }
}

static void RefreshTargetStateLocked() {
    g_isTargetProcess = false;

    for (const auto& rule : g_rules) {
        for (const auto& exe : rule.executables) {
            if (exe == g_currentExeName) {
                g_targetDesktopNumber = rule.desktopNumber;
                g_targetSwitch = rule.switchToDesktop;
                g_isTargetProcess = true;
                return;
            }
        }
    }
}

static void LoadSettings() {
    std::vector<Rule> newRules;

    for (int i = 0; i <= 255; i++) {
        wchar_t key[256] = {};

        swprintf_s(key, L"rules[%d].executable_names", i);
        PCWSTR pExec = Wh_GetStringSetting(key);
        std::wstring executableNames = pExec ? pExec : L"";
        if (pExec) {
            Wh_FreeStringSetting(pExec);
        }

        swprintf_s(key, L"rules[%d].desktop_number", i);
        int desktopNumber = Wh_GetIntSetting(key);

        swprintf_s(key, L"rules[%d].switch_to_desktop", i);
        bool switchToDesktop = Wh_GetIntSetting(key) != 0;

        if (executableNames.empty()) {
            if (i >= (int)newRules.size() + 2) {
                break;
            }
            continue;
        }

        if (desktopNumber < 1) {
            desktopNumber = 1;
        }

        Rule rule;
        rule.executables = SplitExecutableList(executableNames);
        rule.desktopNumber = desktopNumber;
        rule.switchToDesktop = switchToDesktop;

        if (!rule.executables.empty()) {
            newRules.push_back(std::move(rule));
        }
    }

    {
        std::lock_guard<std::mutex> lock(g_rulesMutex);
        g_rules = std::move(newRules);
        RefreshTargetStateLocked();
    }

    ResetMovedWindows();

    if (g_isTargetProcess) {
        Wh_Log(L"Process %s matched a virtual desktop rule (desktop %d)",
               g_currentExeName.c_str(), g_targetDesktopNumber);
    }
}

static BOOL CALLBACK EnumWindowsCallback(HWND hWnd, LPARAM) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == GetCurrentProcessId()) {
        TryApplyToWindow(hWnd);
    }
    return TRUE;
}

static void ApplyToExistingWindows() {
    if (!g_isTargetProcess) {
        return;
    }

    EnumWindows(EnumWindowsCallback, 0);
}

static bool ShouldProcessShowCommand(int nCmdShow) {
    return nCmdShow != SW_HIDE &&
           nCmdShow != SW_MINIMIZE &&
           nCmdShow != SW_SHOWMINIMIZED &&
           nCmdShow != SW_SHOWMINNOACTIVE;
}

BOOL WINAPI ShowWindow_Hook(HWND hWnd, int nCmdShow) {
    BOOL result = ShowWindow_Original(hWnd, nCmdShow);

    if (result && !g_internalMove && ShouldProcessShowCommand(nCmdShow)) {
        TryApplyToWindow(hWnd);
    }

    return result;
}

BOOL WINAPI ShowWindowAsync_Hook(HWND hWnd, int nCmdShow) {
    BOOL result = ShowWindowAsync_Original(hWnd, nCmdShow);

    if (result && !g_internalMove && ShouldProcessShowCommand(nCmdShow)) {
        TryApplyToWindow(hWnd);
    }

    return result;
}

BOOL WINAPI SetWindowPos_Hook(
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags) {
    BOOL result = SetWindowPos_Original(
        hWnd,
        hWndInsertAfter,
        X,
        Y,
        cx,
        cy,
        uFlags);

    if (result && !g_internalMove && (uFlags & SWP_SHOWWINDOW)) {
        TryApplyToWindow(hWnd);
    }

    return result;
}

BOOL Wh_ModInit() {
    g_currentExeName = GetCurrentProcessExecutableName();

    Wh_SetFunctionHook((void*)ShowWindow, (void*)ShowWindow_Hook,
                       (void**)&ShowWindow_Original);
    Wh_SetFunctionHook((void*)ShowWindowAsync, (void*)ShowWindowAsync_Hook,
                       (void**)&ShowWindowAsync_Original);
    Wh_SetFunctionHook((void*)SetWindowPos, (void*)SetWindowPos_Hook,
                       (void**)&SetWindowPos_Original);

    LoadSettings();
    ApplyToExistingWindows();

    return TRUE;
}

void Wh_ModSettingsChanged() {
    LoadSettings();
    ApplyToExistingWindows();
}

void Wh_ModUninit() {
    ResetMovedWindows();
}
