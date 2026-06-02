// ==WindhawkMod==
// @id              asteski-rectangle
// @name            Rectangle
// @description     Move the active window to screen halves and corners with keyboard shortcuts (inspired by Rectangle on macOS)
// @version         1.0
// @author          Asteski
// @github          https://github.com/Asteski
// @include         windhawk.exe
// @compilerOptions -luser32 -ldwmapi
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Rectangle

Move the active window to specific screen areas using keyboard shortcuts, just like
the [Rectangle](https://rectangleapp.com/) app on macOS.

## Default shortcuts

| Action          | Shortcut    |
| --------------- | ----------- |
| Left half       | `Alt` + `Q` |
| Right half      | `Alt` + `W` |
| Top half        | `Alt` + `E` |
| Bottom half     | `Alt` + `R` |
| Center half     | `Alt` + `T` |
| Top-left        | `Alt` + `U` |
| Top-right       | `Alt` + `I` |
| Bottom-left     | `Alt` + `J` |
| Bottom-right    | `Alt` + `K` |
| Maximize        | `Alt` + `H` |
| Almost maximize | `Alt` + `Y` |
| Center          | `Alt` + `G` |
| Restore         | `Alt` + `Z` |

*Maximize* fills the work area ignoring gaps; *almost maximize* fills it while keeping
the configured gaps. *Center* re-centers the active window without resizing it (unless a
center width/height is set). *Restore* returns the window to where it was before its
first snap.

## How it works

The mod installs a system-wide low-level keyboard hook. Each action is bound to a
shortcut you type as text. When you press it, the focused window is resized and moved to
the matching region of the monitor it is on, and the key press is swallowed so it never
reaches the focused application.

## Shortcuts

Each action's shortcut is free text in the form `[modifier+]...key`, for example:

- `q` — just the `Q` key
- `ctrl+q` — `Ctrl` + `Q`
- `ctrl+shift+left` — `Ctrl` + `Shift` + `Left arrow`

Recognised modifier words are `alt`, `ctrl`, `shift`, and `win`; the key may be a letter
`a`-`z`, a digit `0`-`9`, or `left`/`right`/`up`/`down`. Tokens are separated by `+` and
are case-insensitive.

The **Modifier key** setting adds a modifier (or two) on top of whatever you type for
every action. So with Modifier = `Alt` and Left half = `q`, the shortcut is `Alt+Q`. Set
Modifier = `None` to use only the modifiers written in each shortcut — letting you give
different actions different modifiers (e.g. `q`, `ctrl+w`, `ctrl+shift+e`).

The modifier combination must match exactly: an action bound to `Alt+Q` will *not* fire
on `Ctrl+Alt+Q`. Note that binding a bare key (e.g. `q` with Modifier = `None`) means
that key is swallowed system-wide and can no longer be typed normally.

## Other configuration

- **Gaps**: optional space (in pixels) applied to each side of the snapped window
  (top, bottom, left, right). All default to `0`, so windows sit flush by default. By
  default the gap is applied to every side; enable **Only gap screen edges** to apply
  gaps solely where a window side touches a screen edge (adjacent windows then meet
  flush in the middle).
- **Center size**: optional width/height (in pixels) for the Center action. Leave either
  empty to keep the window's current width or height.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- modifier: alt
  $name: Modifier key
  $description: >-
    Added to every action's shortcut, on top of any modifiers typed into the action
    itself. Choose None to use only the modifiers written in each shortcut.
  $options:
    - none: None
    - alt: Alt
    - ctrl: Ctrl
    - shift: Shift
    - win: Win
    - alt_ctrl: Alt + Ctrl
    - alt_shift: Alt + Shift
    - ctrl_shift: Ctrl + Shift
    - win_alt: Win + Alt
    - win_ctrl: Win + Ctrl
    - win_shift: Win + Shift
- keyLeftHalf: q
  $name: Left half shortcut
  $description: >-
    Shortcut text, e.g. "q", "ctrl+q", or "ctrl+shift+left". The Modifier key above is
    added on top. Leave empty to unbind. Same format for every action below.
- keyRightHalf: w
  $name: Right half shortcut
- keyTopHalf: e
  $name: Top half shortcut
- keyBottomHalf: r
  $name: Bottom half shortcut
- keyCenterHalf: t
  $name: Center half shortcut
  $description: A half-width strip in the middle of the screen (full height).
- keyTopLeft: u
  $name: Top-left shortcut
- keyTopRight: i
  $name: Top-right shortcut
- keyBottomLeft: j
  $name: Bottom-left shortcut
- keyBottomRight: k
  $name: Bottom-right shortcut
- keyMaximize: h
  $name: Maximize shortcut
  $description: Fills the whole work area, ignoring any configured gaps.
- keyAlmostMaximize: y
  $name: Almost-maximize shortcut
  $description: Fills the work area but keeps the configured gaps.
- keyCenter: g
  $name: Center shortcut
  $description: Centers the window without changing its size (unless a size is set below).
- keyRestore: z
  $name: Restore shortcut
  $description: Returns the window to where it was before the first snap.
- gapTop: 0
  $name: Top gap (pixels)
  $description: Space left above the window, even when it borders another window.
- gapBottom: 0
  $name: Bottom gap (pixels)
  $description: Space left below the window, even when it borders another window.
- gapLeft: 0
  $name: Left gap (pixels)
  $description: Space left to the left of the window, even when it borders another window.
- gapRight: 0
  $name: Right gap (pixels)
  $description: Space left to the right of the window, even when it borders another window.
- screenEdgeGapsOnly: false
  $name: Only gap screen edges
  $description: >-
    When enabled, gaps are only applied to window sides that touch a screen edge,
    so adjacent windows meet flush in the middle. When disabled, every side of every
    window is gapped.
- centerWidth: ""
  $name: Center action width (pixels)
  $description: Width to use for the Center action. Leave empty to keep the window's current width.
- centerHeight: ""
  $name: Center action height (pixels)
  $description: Height to use for the Center action. Leave empty to keep the window's current height.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>
#include <windows.h>
#include <dwmapi.h>

#include <atomic>
#include <cstdlib>
#include <cwctype>
#include <string>
#include <unordered_map>

enum class Action {
    None,
    LeftHalf,
    RightHalf,
    TopHalf,
    BottomHalf,
    CenterHalf,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Maximize,
    AlmostMaximize,
    Center,
    RestorePrevious,
};

constexpr UINT MOD_F_ALT = 0x1;
constexpr UINT MOD_F_CTRL = 0x2;
constexpr UINT MOD_F_SHIFT = 0x4;
constexpr UINT MOD_F_WIN = 0x8;

constexpr int ACTION_COUNT = 13;

// Mask of modifiers that must be held (and only those) for an action to fire.
static UINT g_modifierMask = MOD_F_ALT;

// Gaps (in pixels) left on each side of a snapped window.
static int g_gapTop = 0;
static int g_gapBottom = 0;
static int g_gapLeft = 0;
static int g_gapRight = 0;

// When true, a gap is only applied to a window side that touches a screen edge.
static bool g_screenEdgeGapsOnly = false;

// Target size for the Center action; 0 means keep the window's current size.
static int g_centerWidth = 0;
static int g_centerHeight = 0;

// Per-action trigger key and required modifier mask; both indexes line up with
// g_actionByIndex below. A key of 0 means the action is unbound.
static UINT g_actionKeys[ACTION_COUNT] = {};
static UINT g_actionMods[ACTION_COUNT] = {};
static const Action g_actionByIndex[ACTION_COUNT] = {
    Action::LeftHalf,    Action::RightHalf,     Action::TopHalf,
    Action::BottomHalf,  Action::CenterHalf,    Action::TopLeft,
    Action::TopRight,    Action::BottomLeft,    Action::BottomRight,
    Action::Maximize,    Action::AlmostMaximize, Action::Center,
    Action::RestorePrevious,
};

// Window placement captured before the first snap, so RestorePrevious can revert.
// Accessed only from the worker thread, so no synchronization is needed.
static std::unordered_map<HWND, WINDOWPLACEMENT> g_savedPlacements;

static HHOOK g_keyboardHook = nullptr;
static HANDLE g_hookThread = nullptr;
static DWORD g_hookThreadId = 0;
static HANDLE g_hookReadyEvent = nullptr;

static HANDLE g_workerThread = nullptr;
static HANDLE g_workEvent = nullptr;
static std::atomic<bool> g_quit{false};
static std::atomic<HWND> g_pendingWindow{nullptr};
static std::atomic<int> g_pendingAction{(int)Action::None};

static HMODULE GetThisModule() {
    HMODULE module = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&GetThisModule), &module);
    return module;
}

static std::wstring ToLower(std::wstring s) {
    for (auto& c : s) {
        c = (wchar_t)towlower(c);
    }
    return s;
}

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

static UINT GetVkFromKeyName(const std::wstring& name) {
    if (name == L"left") {
        return VK_LEFT;
    }
    if (name == L"right") {
        return VK_RIGHT;
    }
    if (name == L"up") {
        return VK_UP;
    }
    if (name == L"down") {
        return VK_DOWN;
    }
    if (name.size() == 1) {
        wchar_t c = name[0];
        if (c >= L'a' && c <= L'z') {
            return (UINT)(L'A' + (c - L'a'));
        }
        if (c >= L'0' && c <= L'9') {
            return (UINT)c;
        }
    }
    return 0;
}

static UINT ParseModifierMask(const std::wstring& value) {
    if (value == L"none") {
        return 0;
    }
    if (value == L"alt") {
        return MOD_F_ALT;
    }
    if (value == L"ctrl") {
        return MOD_F_CTRL;
    }
    if (value == L"shift") {
        return MOD_F_SHIFT;
    }
    if (value == L"win") {
        return MOD_F_WIN;
    }
    if (value == L"alt_ctrl") {
        return MOD_F_ALT | MOD_F_CTRL;
    }
    if (value == L"alt_shift") {
        return MOD_F_ALT | MOD_F_SHIFT;
    }
    if (value == L"ctrl_shift") {
        return MOD_F_CTRL | MOD_F_SHIFT;
    }
    if (value == L"win_alt") {
        return MOD_F_WIN | MOD_F_ALT;
    }
    if (value == L"win_ctrl") {
        return MOD_F_WIN | MOD_F_CTRL;
    }
    if (value == L"win_shift") {
        return MOD_F_WIN | MOD_F_SHIFT;
    }
    return MOD_F_ALT;
}

// Maps a single modifier word (alt/ctrl/shift/win and common aliases) to its bit.
// Returns true and sets *bit if the token is a recognised modifier.
static bool ParseModifierToken(const std::wstring& token, UINT* bit) {
    if (token == L"alt" || token == L"menu") {
        *bit = MOD_F_ALT;
        return true;
    }
    if (token == L"ctrl" || token == L"control") {
        *bit = MOD_F_CTRL;
        return true;
    }
    if (token == L"shift") {
        *bit = MOD_F_SHIFT;
        return true;
    }
    if (token == L"win" || token == L"super" || token == L"meta" ||
        token == L"cmd") {
        *bit = MOD_F_WIN;
        return true;
    }
    return false;
}

// Parses a "[mod+]...key" shortcut string into a key code and modifier mask.
// Returns true only if a key token was found; modifier-only strings are invalid.
static bool ParseShortcut(const std::wstring& text, UINT* outVk, UINT* outMods) {
    *outVk = 0;
    *outMods = 0;
    bool haveKey = false;

    size_t start = 0;
    while (start <= text.size()) {
        size_t pos = text.find(L'+', start);
        std::wstring token = ToLower(Trim(
            pos == std::wstring::npos ? text.substr(start)
                                      : text.substr(start, pos - start)));

        if (!token.empty()) {
            UINT bit = 0;
            if (ParseModifierToken(token, &bit)) {
                *outMods |= bit;
            } else {
                UINT vk = GetVkFromKeyName(token);
                if (vk != 0) {
                    *outVk = vk;
                    haveKey = true;
                }
            }
        }

        if (pos == std::wstring::npos) {
            break;
        }
        start = pos + 1;
    }

    return haveKey;
}

static UINT GetCurrentModifierMask() {
    UINT mask = 0;
    if (GetAsyncKeyState(VK_MENU) & 0x8000) {
        mask |= MOD_F_ALT;
    }
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        mask |= MOD_F_CTRL;
    }
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        mask |= MOD_F_SHIFT;
    }
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) ||
        (GetAsyncKeyState(VK_RWIN) & 0x8000)) {
        mask |= MOD_F_WIN;
    }
    return mask;
}

static Action MatchAction(UINT vkCode, UINT currentMods) {
    for (int i = 0; i < ACTION_COUNT; i++) {
        if (g_actionKeys[i] != 0 && g_actionKeys[i] == vkCode &&
            g_actionMods[i] == currentMods) {
            return g_actionByIndex[i];
        }
    }
    return Action::None;
}

static bool IsEligibleWindow(HWND hWnd) {
    if (!hWnd || !IsWindow(hWnd)) {
        return false;
    }

    if (GetAncestor(hWnd, GA_ROOT) != hWnd) {
        return false;
    }

    LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }

    LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
    if (!(style & WS_CAPTION)) {
        return false;
    }

    return true;
}

// Position the window so its *visible* bounds match the given screen rectangle.
// GetWindowRect/SetWindowPos coordinates include the invisible drop-shadow border
// that DWM draws around a window, so measure it and expand the target accordingly.
static void ApplyVisibleRect(HWND hWnd, int x, int y, int w, int h) {
    RECT windowRect = {};
    RECT frameRect = {};
    int marginLeft = 0, marginTop = 0, marginRight = 0, marginBottom = 0;
    if (GetWindowRect(hWnd, &windowRect) &&
        SUCCEEDED(DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS,
                                        &frameRect, sizeof(frameRect)))) {
        marginLeft = frameRect.left - windowRect.left;
        marginTop = frameRect.top - windowRect.top;
        marginRight = windowRect.right - frameRect.right;
        marginBottom = windowRect.bottom - frameRect.bottom;
    }

    SetWindowPos(hWnd, nullptr,
                 x - marginLeft,
                 y - marginTop,
                 w + marginLeft + marginRight,
                 h + marginTop + marginBottom,
                 SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
}

// Current visible size of the window (excludes the invisible drop-shadow border).
static void GetVisibleSize(HWND hWnd, int* width, int* height) {
    RECT rect = {};
    if (FAILED(DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect,
                                     sizeof(rect)))) {
        GetWindowRect(hWnd, &rect);
    }
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
}

static void SavePlacementIfNew(HWND hWnd) {
    if (g_savedPlacements.find(hWnd) != g_savedPlacements.end()) {
        return;
    }
    WINDOWPLACEMENT wp = {};
    wp.length = sizeof(wp);
    if (GetWindowPlacement(hWnd, &wp)) {
        g_savedPlacements[hWnd] = wp;
    }
}

static void RestorePreviousPlacement(HWND hWnd) {
    auto it = g_savedPlacements.find(hWnd);
    if (it == g_savedPlacements.end()) {
        return;
    }
    SetWindowPlacement(hWnd, &it->second);
    g_savedPlacements.erase(it);
}

static void SnapWindow(HWND hWnd, Action action) {
    if (action == Action::None || !IsEligibleWindow(hWnd)) {
        return;
    }

    if (action == Action::RestorePrevious) {
        RestorePreviousPlacement(hWnd);
        return;
    }

    // Remember where the window was before its first snap so it can be restored.
    SavePlacementIfNew(hWnd);

    if (action == Action::Maximize) {
        ShowWindow(hWnd, SW_MAXIMIZE);
        return;
    }

    if (IsIconic(hWnd) || IsZoomed(hWnd)) {
        ShowWindow(hWnd, SW_RESTORE);
    }

    HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) {
        return;
    }

    const RECT& work = mi.rcWork;
    const int fullWidth = work.right - work.left;
    const int fullHeight = work.bottom - work.top;
    const int halfWidth = fullWidth / 2;
    const int midX = work.left + halfWidth;
    const int midY = work.top + fullHeight / 2;

    if (action == Action::Center) {
        int curW = 0, curH = 0;
        GetVisibleSize(hWnd, &curW, &curH);
        int w = g_centerWidth > 0 ? g_centerWidth : curW;
        int h = g_centerHeight > 0 ? g_centerHeight : curH;
        int x = work.left + (fullWidth - w) / 2;
        int y = work.top + (fullHeight - h) / 2;
        ApplyVisibleRect(hWnd, x, y, w, h);
        return;
    }

    // The region of the work area this action targets, before gaps.
    RECT cell = work;
    switch (action) {
        case Action::LeftHalf:
            cell = {work.left, work.top, midX, work.bottom};
            break;
        case Action::RightHalf:
            cell = {midX, work.top, work.right, work.bottom};
            break;
        case Action::TopHalf:
            cell = {work.left, work.top, work.right, midY};
            break;
        case Action::BottomHalf:
            cell = {work.left, midY, work.right, work.bottom};
            break;
        case Action::CenterHalf: {
            int left = work.left + (fullWidth - halfWidth) / 2;
            cell = {left, work.top, left + halfWidth, work.bottom};
            break;
        }
        case Action::TopLeft:
            cell = {work.left, work.top, midX, midY};
            break;
        case Action::TopRight:
            cell = {midX, work.top, work.right, midY};
            break;
        case Action::BottomLeft:
            cell = {work.left, midY, midX, work.bottom};
            break;
        case Action::BottomRight:
            cell = {midX, midY, work.right, work.bottom};
            break;
        case Action::AlmostMaximize:
            cell = work;
            break;
        default:
            return;
    }

    // Inset each side by its gap. In screen-edge-only mode a gap is applied only
    // when that side coincides with the work-area edge; otherwise every side
    // (including inner dividers) is inset.
    if (!g_screenEdgeGapsOnly || cell.left == work.left) {
        cell.left += g_gapLeft;
    }
    if (!g_screenEdgeGapsOnly || cell.top == work.top) {
        cell.top += g_gapTop;
    }
    if (!g_screenEdgeGapsOnly || cell.right == work.right) {
        cell.right -= g_gapRight;
    }
    if (!g_screenEdgeGapsOnly || cell.bottom == work.bottom) {
        cell.bottom -= g_gapBottom;
    }
    if (cell.right <= cell.left || cell.bottom <= cell.top) {
        return;
    }

    ApplyVisibleRect(hWnd, cell.left, cell.top, cell.right - cell.left,
                     cell.bottom - cell.top);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION &&
        (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;

        Action action = MatchAction(kbd->vkCode, GetCurrentModifierMask());
        if (action != Action::None) {
            g_pendingWindow = GetForegroundWindow();
            g_pendingAction = (int)action;
            SetEvent(g_workEvent);
            return 1;  // Swallow the key so the focused app never sees it.
        }
    }

    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

static void LoadSettings() {
    PCWSTR modifier = Wh_GetStringSetting(L"modifier");
    g_modifierMask = ParseModifierMask(modifier ? modifier : L"alt");
    if (modifier) {
        Wh_FreeStringSetting(modifier);
    }

    g_gapTop = Wh_GetIntSetting(L"gapTop");
    g_gapBottom = Wh_GetIntSetting(L"gapBottom");
    g_gapLeft = Wh_GetIntSetting(L"gapLeft");
    g_gapRight = Wh_GetIntSetting(L"gapRight");
    g_screenEdgeGapsOnly = Wh_GetIntSetting(L"screenEdgeGapsOnly") != 0;

    PCWSTR centerWidth = Wh_GetStringSetting(L"centerWidth");
    g_centerWidth = centerWidth ? _wtoi(centerWidth) : 0;
    if (centerWidth) {
        Wh_FreeStringSetting(centerWidth);
    }

    PCWSTR centerHeight = Wh_GetStringSetting(L"centerHeight");
    g_centerHeight = centerHeight ? _wtoi(centerHeight) : 0;
    if (centerHeight) {
        Wh_FreeStringSetting(centerHeight);
    }

    static const PCWSTR keyNames[ACTION_COUNT] = {
        L"keyLeftHalf",    L"keyRightHalf",     L"keyTopHalf",
        L"keyBottomHalf",  L"keyCenterHalf",    L"keyTopLeft",
        L"keyTopRight",    L"keyBottomLeft",    L"keyBottomRight",
        L"keyMaximize",    L"keyAlmostMaximize", L"keyCenter",
        L"keyRestore",
    };

    for (int i = 0; i < ACTION_COUNT; i++) {
        PCWSTR value = Wh_GetStringSetting(keyNames[i]);

        UINT vk = 0, textMods = 0;
        if (value && ParseShortcut(value, &vk, &textMods)) {
            g_actionKeys[i] = vk;
            // The global modifier is required on top of whatever the shortcut typed.
            g_actionMods[i] = g_modifierMask | textMods;
        } else {
            g_actionKeys[i] = 0;  // Unbound or invalid.
            g_actionMods[i] = 0;
        }

        if (value) {
            Wh_FreeStringSetting(value);
        }
    }

    // De-duplicate bindings: if two actions resolve to the same key+modifier
    // combination, only the first one (in keyNames order) stays bound. Later
    // duplicates are disabled so a single keypress doesn't fire two actions.
    for (int i = 0; i < ACTION_COUNT; i++) {
        if (g_actionKeys[i] == 0) {
            continue;
        }
        for (int j = 0; j < i; j++) {
            if (g_actionKeys[j] == g_actionKeys[i] &&
                g_actionMods[j] == g_actionMods[i]) {
                Wh_Log(L"Rectangle: '%s' duplicates '%s' (same key+modifier); "
                       L"disabling '%s'",
                       keyNames[i], keyNames[j], keyNames[i]);
                g_actionKeys[i] = 0;
                g_actionMods[i] = 0;
                break;
            }
        }
    }
}

static DWORD WINAPI WorkerThreadProc(LPVOID) {
    for (;;) {
        WaitForSingleObject(g_workEvent, INFINITE);
        if (g_quit.load()) {
            break;
        }

        HWND hWnd = g_pendingWindow.exchange(nullptr);
        Action action = (Action)g_pendingAction.exchange((int)Action::None);
        if (hWnd && action != Action::None) {
            SnapWindow(hWnd, action);
        }
    }
    return 0;
}

static DWORD WINAPI HookThreadProc(LPVOID) {
    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                       GetThisModule(), 0);
    if (!g_keyboardHook) {
        Wh_Log(L"Rectangle: SetWindowsHookEx failed, error=%lu", GetLastError());
        SetEvent(g_hookReadyEvent);
        return 0;
    }

    Wh_Log(L"Rectangle: keyboard hook installed");
    SetEvent(g_hookReadyEvent);

    MSG msg;
    while (!g_quit.load()) {
        BOOL gm = GetMessageW(&msg, nullptr, 0, 0);
        if (gm == 0 || gm == -1) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
    return 0;
}

BOOL Wh_ModInit() {
    LoadSettings();

    g_quit = false;

    g_workEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_workEvent) {
        Wh_Log(L"Rectangle: CreateEvent (work) failed");
        return FALSE;
    }

    g_workerThread = CreateThread(nullptr, 0, WorkerThreadProc, nullptr, 0, nullptr);
    if (!g_workerThread) {
        Wh_Log(L"Rectangle: CreateThread (worker) failed");
        CloseHandle(g_workEvent);
        g_workEvent = nullptr;
        return FALSE;
    }

    g_hookReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_hookReadyEvent) {
        Wh_Log(L"Rectangle: CreateEvent (hook ready) failed");
        g_quit = true;
        SetEvent(g_workEvent);
        WaitForSingleObject(g_workerThread, 2000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
        CloseHandle(g_workEvent);
        g_workEvent = nullptr;
        return FALSE;
    }

    g_hookThread = CreateThread(nullptr, 0, HookThreadProc, nullptr, 0,
                               &g_hookThreadId);
    if (!g_hookThread) {
        Wh_Log(L"Rectangle: CreateThread (hook) failed");
        CloseHandle(g_hookReadyEvent);
        g_hookReadyEvent = nullptr;
        g_quit = true;
        SetEvent(g_workEvent);
        WaitForSingleObject(g_workerThread, 2000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
        CloseHandle(g_workEvent);
        g_workEvent = nullptr;
        return FALSE;
    }

    WaitForSingleObject(g_hookReadyEvent, 3000);
    CloseHandle(g_hookReadyEvent);
    g_hookReadyEvent = nullptr;

    return TRUE;
}

void Wh_ModSettingsChanged() {
    LoadSettings();
}

void Wh_ModUninit() {
    g_quit = true;

    if (g_hookThreadId) {
        PostThreadMessageW(g_hookThreadId, WM_QUIT, 0, 0);
    }
    if (g_hookThread) {
        WaitForSingleObject(g_hookThread, 2000);
        CloseHandle(g_hookThread);
        g_hookThread = nullptr;
        g_hookThreadId = 0;
    }

    if (g_workEvent) {
        SetEvent(g_workEvent);
    }
    if (g_workerThread) {
        WaitForSingleObject(g_workerThread, 2000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }
    if (g_workEvent) {
        CloseHandle(g_workEvent);
        g_workEvent = nullptr;
    }

    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }

    g_savedPlacements.clear();
}
