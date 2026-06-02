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

| Action       | Shortcut          |
| ------------ | ----------------- |
| Left half    | `Alt` + `Left`    |
| Right half   | `Alt` + `Right`   |
| Top-left     | `Alt` + `U`       |
| Top-right    | `Alt` + `I`       |
| Bottom-left  | `Alt` + `J`       |
| Bottom-right | `Alt` + `K`       |

## How it works

The mod installs a system-wide low-level keyboard hook. When you press the configured
modifier together with an action key, the currently focused window is resized and moved
to the matching region of the monitor it is on. The triggering key press is swallowed so
it never reaches the focused application.

## Configuration

- **Modifier key**: a single modifier or a two-modifier combination (e.g. `Alt`,
  `Ctrl`, `Shift`, `Win`, or `Alt + Ctrl`). The same modifier is shared by every action.
- **Per-action key**: each action gets its own key (arrows, letters `A`-`Z`, or
  digits `0`-`9`).
- **Gaps**: optional space (in pixels) between snapped windows and each screen edge
  (top, bottom, left, right). All default to `0`, so windows sit flush by default.

The modifier combination must match exactly: if you choose `Alt`, then `Ctrl+Alt+Left`
will *not* trigger the action.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- modifier: alt
  $name: Modifier key
  $description: The modifier (or two-modifier combination) shared by all actions.
  $options:
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
- keyLeftHalf: left
  $name: Left half key
  $options:
    - left: Left arrow
    - right: Right arrow
    - up: Up arrow
    - down: Down arrow
    - a: A
    - b: B
    - c: C
    - d: D
    - e: E
    - f: F
    - g: G
    - h: H
    - i: I
    - j: J
    - k: K
    - l: L
    - m: M
    - n: N
    - o: O
    - p: P
    - q: Q
    - r: R
    - s: S
    - t: T
    - u: U
    - v: V
    - w: W
    - x: X
    - y: Y
    - z: Z
    - "0": "0"
    - "1": "1"
    - "2": "2"
    - "3": "3"
    - "4": "4"
    - "5": "5"
    - "6": "6"
    - "7": "7"
    - "8": "8"
    - "9": "9"
- keyRightHalf: right
  $name: Right half key
  $options:
    - left: Left arrow
    - right: Right arrow
    - up: Up arrow
    - down: Down arrow
    - a: A
    - b: B
    - c: C
    - d: D
    - e: E
    - f: F
    - g: G
    - h: H
    - i: I
    - j: J
    - k: K
    - l: L
    - m: M
    - n: N
    - o: O
    - p: P
    - q: Q
    - r: R
    - s: S
    - t: T
    - u: U
    - v: V
    - w: W
    - x: X
    - y: Y
    - z: Z
    - "0": "0"
    - "1": "1"
    - "2": "2"
    - "3": "3"
    - "4": "4"
    - "5": "5"
    - "6": "6"
    - "7": "7"
    - "8": "8"
    - "9": "9"
- keyTopLeft: u
  $name: Top-left key
  $options:
    - left: Left arrow
    - right: Right arrow
    - up: Up arrow
    - down: Down arrow
    - a: A
    - b: B
    - c: C
    - d: D
    - e: E
    - f: F
    - g: G
    - h: H
    - i: I
    - j: J
    - k: K
    - l: L
    - m: M
    - n: N
    - o: O
    - p: P
    - q: Q
    - r: R
    - s: S
    - t: T
    - u: U
    - v: V
    - w: W
    - x: X
    - y: Y
    - z: Z
    - "0": "0"
    - "1": "1"
    - "2": "2"
    - "3": "3"
    - "4": "4"
    - "5": "5"
    - "6": "6"
    - "7": "7"
    - "8": "8"
    - "9": "9"
- keyTopRight: i
  $name: Top-right key
  $options:
    - left: Left arrow
    - right: Right arrow
    - up: Up arrow
    - down: Down arrow
    - a: A
    - b: B
    - c: C
    - d: D
    - e: E
    - f: F
    - g: G
    - h: H
    - i: I
    - j: J
    - k: K
    - l: L
    - m: M
    - n: N
    - o: O
    - p: P
    - q: Q
    - r: R
    - s: S
    - t: T
    - u: U
    - v: V
    - w: W
    - x: X
    - y: Y
    - z: Z
    - "0": "0"
    - "1": "1"
    - "2": "2"
    - "3": "3"
    - "4": "4"
    - "5": "5"
    - "6": "6"
    - "7": "7"
    - "8": "8"
    - "9": "9"
- keyBottomLeft: j
  $name: Bottom-left key
  $options:
    - left: Left arrow
    - right: Right arrow
    - up: Up arrow
    - down: Down arrow
    - a: A
    - b: B
    - c: C
    - d: D
    - e: E
    - f: F
    - g: G
    - h: H
    - i: I
    - j: J
    - k: K
    - l: L
    - m: M
    - n: N
    - o: O
    - p: P
    - q: Q
    - r: R
    - s: S
    - t: T
    - u: U
    - v: V
    - w: W
    - x: X
    - y: Y
    - z: Z
    - "0": "0"
    - "1": "1"
    - "2": "2"
    - "3": "3"
    - "4": "4"
    - "5": "5"
    - "6": "6"
    - "7": "7"
    - "8": "8"
    - "9": "9"
- keyBottomRight: k
  $name: Bottom-right key
  $options:
    - left: Left arrow
    - right: Right arrow
    - up: Up arrow
    - down: Down arrow
    - a: A
    - b: B
    - c: C
    - d: D
    - e: E
    - f: F
    - g: G
    - h: H
    - i: I
    - j: J
    - k: K
    - l: L
    - m: M
    - n: N
    - o: O
    - p: P
    - q: Q
    - r: R
    - s: S
    - t: T
    - u: U
    - v: V
    - w: W
    - x: X
    - y: Y
    - z: Z
    - "0": "0"
    - "1": "1"
    - "2": "2"
    - "3": "3"
    - "4": "4"
    - "5": "5"
    - "6": "6"
    - "7": "7"
    - "8": "8"
    - "9": "9"
- gapTop: 0
  $name: Top gap (pixels)
  $description: Space left between the window and the top edge of the screen.
- gapBottom: 0
  $name: Bottom gap (pixels)
  $description: Space left between the window and the bottom edge of the screen.
- gapLeft: 0
  $name: Left gap (pixels)
  $description: Space left between the window and the left edge of the screen.
- gapRight: 0
  $name: Right gap (pixels)
  $description: Space left between the window and the right edge of the screen.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>
#include <windows.h>
#include <dwmapi.h>

#include <atomic>
#include <string>

enum class Action {
    None,
    LeftHalf,
    RightHalf,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
};

constexpr UINT MOD_F_ALT = 0x1;
constexpr UINT MOD_F_CTRL = 0x2;
constexpr UINT MOD_F_SHIFT = 0x4;
constexpr UINT MOD_F_WIN = 0x8;

constexpr int ACTION_COUNT = 6;

// Mask of modifiers that must be held (and only those) for an action to fire.
static UINT g_modifierMask = MOD_F_ALT;

// Gaps (in pixels) left between snapped windows and each screen edge.
static int g_gapTop = 0;
static int g_gapBottom = 0;
static int g_gapLeft = 0;
static int g_gapRight = 0;

// Per-action trigger virtual key codes, indexed so it lines up with the keys below.
static UINT g_actionKeys[ACTION_COUNT] = {};
static const Action g_actionByIndex[ACTION_COUNT] = {
    Action::LeftHalf, Action::RightHalf, Action::TopLeft,
    Action::TopRight, Action::BottomLeft, Action::BottomRight,
};

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

static Action MatchAction(UINT vkCode) {
    for (int i = 0; i < ACTION_COUNT; i++) {
        if (g_actionKeys[i] != 0 && g_actionKeys[i] == vkCode) {
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

static void SnapWindow(HWND hWnd, Action action) {
    if (!IsEligibleWindow(hWnd)) {
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

    // Work area inset by the configured per-edge gaps.
    RECT area = mi.rcWork;
    area.left += g_gapLeft;
    area.top += g_gapTop;
    area.right -= g_gapRight;
    area.bottom -= g_gapBottom;
    if (area.right <= area.left || area.bottom <= area.top) {
        return;
    }

    const int fullWidth = area.right - area.left;
    const int fullHeight = area.bottom - area.top;
    const int halfWidth = fullWidth / 2;
    const int halfHeight = fullHeight / 2;
    const int rightX = area.left + (fullWidth - halfWidth);
    const int bottomY = area.top + (fullHeight - halfHeight);

    int x = area.left;
    int y = area.top;
    int w = halfWidth;
    int h = fullHeight;

    switch (action) {
        case Action::LeftHalf:
            x = area.left;
            y = area.top;
            w = halfWidth;
            h = fullHeight;
            break;
        case Action::RightHalf:
            x = rightX;
            y = area.top;
            w = halfWidth;
            h = fullHeight;
            break;
        case Action::TopLeft:
            x = area.left;
            y = area.top;
            w = halfWidth;
            h = halfHeight;
            break;
        case Action::TopRight:
            x = rightX;
            y = area.top;
            w = halfWidth;
            h = halfHeight;
            break;
        case Action::BottomLeft:
            x = area.left;
            y = bottomY;
            w = halfWidth;
            h = halfHeight;
            break;
        case Action::BottomRight:
            x = rightX;
            y = bottomY;
            w = halfWidth;
            h = halfHeight;
            break;
        case Action::None:
            return;
    }

    // GetWindowRect/SetWindowPos coordinates include the invisible drop-shadow
    // border that DWM draws around the window. Measure the real visible bounds
    // and expand the target rect by those margins so the window sits flush.
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

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION &&
        (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;

        if (g_modifierMask != 0 &&
            GetCurrentModifierMask() == g_modifierMask) {
            Action action = MatchAction(kbd->vkCode);
            if (action != Action::None) {
                g_pendingWindow = GetForegroundWindow();
                g_pendingAction = (int)action;
                SetEvent(g_workEvent);
                return 1;  // Swallow the key so the focused app never sees it.
            }
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

    static const PCWSTR keyNames[ACTION_COUNT] = {
        L"keyLeftHalf", L"keyRightHalf", L"keyTopLeft",
        L"keyTopRight", L"keyBottomLeft", L"keyBottomRight",
    };

    for (int i = 0; i < ACTION_COUNT; i++) {
        PCWSTR value = Wh_GetStringSetting(keyNames[i]);
        g_actionKeys[i] = GetVkFromKeyName(value ? value : L"");
        if (value) {
            Wh_FreeStringSetting(value);
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
}
