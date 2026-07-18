// ==WindhawkMod==
// @id              custom-taskbar-toolbars
// @name            Custom Taskbar Toolbars
// @description     Restores the classic Windows taskbar toolbars with clickable buttons, text/icon display modes, and shell-style actions.
// @version         1.0.0
// @author          Asteski
// @include         explorer.exe
// @compilerOptions -lruntimeobject -lversion -luuid -luser32 -lwindowsapp -lshell32 -lgdi32 -lshlwapi -lwindowscodecs -ldwmapi -lshcore -lksuser
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Windows 11 Taskbar Toolbars

Restores the classic Windows taskbar toolbars (also known as DeskBands), including support for custom folder toolbars. 
It allows you to define a set of buttons that can launch applications, open folders, run commands, or send keyboard shortcuts. 
The buttons can display text, icons, or both, and can be placed in various locations on the taskbar.
 
## Features

* Define each button separately with a label, icon, and action.
* Show text only, icon only, or both.
* Place the bar near the Start button or in the tray area.
* Choose horizontal or vertical layout.
* Set spacing between buttons.
* Support the same action formats as Ultimate Custom Tray:
  * `C:\Path\To\App.exe`
  * `C:\Path\To\App.exe --some-argument`
  * `cmd:explorer`
  * `shell:Start-Process notepad`
  * `key:Ctrl+Shift+Esc`
  * `web:https://example.com`

## Icon format

* `E774` for a Segoe Fluent Icons glyph.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- MenuBarSettings:
  - placement: "taskbar_right_start"
    $name: Placement
    $options:
    - "taskbar_left_edge": "Taskbar - Left edge (Overlay)"
    - "taskbar_center_edge": "Taskbar - Center (Overlay)"
    - "taskbar_right_edge": "Taskbar - Right edge (Overlay)"
    - "taskbar_left_start": "Taskbar - Left of Start button"
    - "taskbar_right_start": "Taskbar - Right of Start button"
    - "taskbar_after_search_left": "Taskbar - Left of Search button"
    - "taskbar_after_search_right": "Taskbar - Right of Search button"
    - "taskbar_after_taskview_left": "Taskbar - Left of Task View button"
    - "taskbar_after_taskview_right": "Taskbar - Right of Task View button"
    - "taskbar_after_widgets_left": "Taskbar - Left of Widgets button"
    - "taskbar_after_widgets_right": "Taskbar - Right of Widgets button"
    - "tray_left": "Tray - Far left"
    - "tray_right": "Tray - Far right"
    - "tray_before_clock": "Tray - Left of Clock"
    - "tray_after_clock": "Tray - Right of Clock"
    - "tray_before_omni_left": "Tray - Left of Network/Volume button"
    - "tray_before_omni_right": "Tray - Right of Network/Volume button"
    - "tray_language_left": "Tray - Left of Language button"
    - "tray_language_right": "Tray - Right of Language button"
    - "tray_icons_left": "Tray - Left of Tray Icons"
    - "tray_icons_right": "Tray - Right of Tray Icons"
    - "tray_hidden_icons_left": "Tray - Left of Hidden icons button"
    - "tray_hidden_icons_right": "Tray - Right of Hidden icons button"
    - "tray_after_showdesktop_left": "Tray - Left of Show Desktop"
    - "tray_after_showdesktop_right": "Tray - Right of Show Desktop"
  - orientation: "horizontal"
    $name: Layout
    $options:
    - horizontal: Horizontal
    - vertical: Vertical
  - displayMode: "both"
    $name: Display mode
    $options:
    - text: Label only
    - both: Icon and label
    - icon: Icon only
  - buttonContentAlignment: "center"
    $name: Button content alignment
    $options:
    - left: Left
    - center: Center
    - right: Right
  - highlightSize: "default"
    $name: Highlight style
    $options:
    - compact: Compact
    - default: Default
  - buttonWidthMode: "dynamic"
    $name: Fixed button width
    $options:
    - dynamic: Dynamic
    - static: Static
    - custom: Custom
  - customButtonWidth: 180
    $name: Custom button width (px)
  - buttonSpacing: 4
    $name: Spacing between buttons
  - buttonOffset: "0,0"
    $name: Button offset (horizontal,vertical)
  - buttonPadding: "8,0"
    $name: Button padding (horizontal,vertical)
  - iconSize: 16
    $name: Icon size
  - textSize: 12
    $name: Font size
  $name: Menu Bar

- ButtonsSettings:
    - buttons:
        - - label: "Explorer"
            $name: Name
          - icon: "EC50"
            $name: Icon
          - action: "cmd:explorer"
            $name: Action
          - displayMode: "default"
            $name: Display mode
            $options:
            - default: Use menu default
            - text: Label only
            - both: Icon and label
            - icon: Icon only
        - - label: "Settings"
            $name: Name
          - icon: "E713"
            $name: Icon
          - action: "ms-settings:"
            $name: Action
          - displayMode: "default"
            $name: Display mode
            $options:
            - default: Use menu default
            - text: Label only
            - both: Icon and label
            - icon: Icon only
        - - label: "About Windows"
            $name: Name
          - icon: "E946"
            $name: Icon
          - action: "winver"
            $name: Action
          - displayMode: "icon"
            $name: Display mode
            $options:
            - default: Use menu default
            - text: Label only
            - both: Icon and label
            - icon: Icon only
  $name: Buttons
  $description: Each item becomes one clickable taskbar menu button.
*/
// ==/WindhawkModSettings==

#define INITGUID
#include <windhawk_utils.h>

#include <windows.h>
#include <shellapi.h>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Markup.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>

#include <algorithm>
#include <cmath>
#include <atomic>
#include <chrono>
#include <cctype>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Markup;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;

struct MenuBarButton {
    std::wstring label;
    std::wstring iconRaw;
    std::wstring action;
    std::wstring displayMode;
};

struct Settings {
    std::wstring placement      = L"taskbar_right_start";
    std::wstring orientation    = L"horizontal";
    std::wstring displayMode    = L"text";
    std::wstring buttonContentAlignment = L"center";
    std::wstring highlightSize  = L"compact";
    std::wstring buttonWidthMode = L"dynamic";
    int          customButtonWidth = 80;
    int          buttonSpacing  = 8;
    int          buttonOffsetX  = 0;
    int          buttonOffsetY  = 0;
    int          buttonPaddingX  = 8;
    int          buttonPaddingY  = 4;
    int          iconSize       = 16;
    int          textSize       = 13;
};

static Settings g_settings;
static std::vector<MenuBarButton> g_buttons;
static std::mutex g_buttonsMutex;
static HWND g_taskbarWnd = nullptr;
static bool g_unloading = false;
static bool g_applyingSettings = false;
static const int kOverlayColumn = -1;

using WindowThreadProc = void(*)(void*);
using TrayUI_StartTaskbar_t = void(WINAPI*)(void*);
using CTaskBand_GetTaskbarHost_t = void*(WINAPI*)(void*, void*);
using TaskbarHost_FrameHeight_t = int(WINAPI*)(void*);
using Std_Ref_Decref_t = void(WINAPI*)(void*);

static TrayUI_StartTaskbar_t TrayUI_StartTaskbar_Original = nullptr;
static CTaskBand_GetTaskbarHost_t CTaskBand_GetTaskbarHost_Original = nullptr;
static TaskbarHost_FrameHeight_t TaskbarHost_FrameHeight_Original = nullptr;
static Std_Ref_Decref_t Std_Ref_Decref_Original = nullptr;
static void* CTaskBand_ITaskListWndSite_vftable = nullptr;

static Grid g_menuBarRoot = nullptr;
static FrameworkElement g_menuBarHost = nullptr;
static Grid g_injectionParent = nullptr;
static FrameworkElement g_menuBarAnchor = nullptr;
static Grid g_layoutUpdatedSource = nullptr;
static event_token g_layoutUpdatedToken{};
static bool g_layoutUpdatedAttached = false;
static FrameworkElement g_trackedElement = nullptr;
static Thickness g_trackedElementOriginalMargin{};
static bool g_hasTrackedElementOriginalMargin = false;
static std::wstring g_trackPosition;
static int g_insertedColumn = -1;
static int g_placementKind = 0;
static double g_lastMenuBarX = -1.0;
static double g_lastMenuBarY = -1.0;
static bool g_hasLastMenuBarPlacement = false;
static double g_lastHighlightMinHeight = -1.0;
static bool g_hasLastHighlightMinHeight = false;
static std::atomic<bool> g_retryScheduled{false};

static std::wstring Trim(std::wstring s) {
    auto isWs = [](wchar_t c) { return iswspace(c) != 0; };
    while (!s.empty() && isWs(s.front())) s.erase(s.begin());
    while (!s.empty() && isWs(s.back())) s.pop_back();
    return s;
}

static std::wstring ToUpper(std::wstring s) {
    for (auto& ch : s) ch = (wchar_t)towupper(ch);
    return s;
}

static bool StartsWithCI(const std::wstring& s, const wchar_t* prefix) {
    size_t n = wcslen(prefix);
    return s.size() >= n && _wcsnicmp(s.c_str(), prefix, n) == 0;
}

static bool SplitCommandAndArguments(const std::wstring& raw, std::wstring& file, std::wstring& args) {
    std::wstring s = Trim(raw);
    if (s.empty()) return false;

    if (s.front() == L'"') {
        size_t end = s.find(L'"', 1);
        if (end == std::wstring::npos || end == 1) return false;
        file = s.substr(1, end - 1);
        size_t pos = end + 1;
        while (pos < s.size() && iswspace(s[pos])) ++pos;
        args = s.substr(pos);
        return !file.empty();
    }

    size_t pos = 0;
    while (pos < s.size() && !iswspace(s[pos])) ++pos;
    file = s.substr(0, pos);
    while (pos < s.size() && iswspace(s[pos])) ++pos;
    args = s.substr(pos);
    return !file.empty();
}

static bool LaunchDefaultAction(const std::wstring& raw) {
    std::wstring file;
    std::wstring args;
    if (!SplitCommandAndArguments(raw, file, args)) return false;

    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOASYNC;
    sei.lpVerb = L"open";
    sei.lpFile = file.c_str();
    sei.lpParameters = args.empty() ? nullptr : args.c_str();
    sei.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&sei) != FALSE;
}

static bool ParseIntPair(const wchar_t* key, const wchar_t* def, int& a, int& b) {
    PCWSTR p = Wh_GetStringSetting(key);
    std::wstring s = p ? p : def;
    if (p) Wh_FreeStringSetting(p);

    for (auto& ch : s) {
        if (ch == L',') ch = L' ';
    }

    int x = 0, y = 0;
    if (swscanf_s(s.c_str(), L"%d %d", &x, &y) == 2) {
        a = x;
        b = y;
        return true;
    }
    if (swscanf_s(def, L"%d %d", &x, &y) == 2) {
        a = x;
        b = y;
    }
    return false;
}

static std::wstring GetStringSetting(const wchar_t* key, const wchar_t* def = L"") {
    PCWSTR p = Wh_GetStringSetting(key);
    std::wstring s = p ? p : def;
    if (p) Wh_FreeStringSetting(p);
    return s;
}

static int GetIntSetting(const wchar_t* key, int minValue, int maxValue, int defValue) {
    int v = Wh_GetIntSetting(key);
    if (v < minValue || v > maxValue) return defValue;
    return v;
}

static bool TryParseShortcut(std::wstring_view shortcut, UINT* modifiersOut, UINT* vkOut) {
    auto trim = [](std::wstring_view s) {
        size_t start = 0;
        while (start < s.size() && iswspace(s[start])) start++;
        size_t end = s.size();
        while (end > start && iswspace(s[end - 1])) end--;
        return s.substr(start, end - start);
    };

    std::unordered_map<std::wstring, UINT> modifiersMap = {
        {L"ALT", MOD_ALT},
        {L"CTRL", MOD_CONTROL},
        {L"CONTROL", MOD_CONTROL},
        {L"SHIFT", MOD_SHIFT},
        {L"WIN", MOD_WIN},
    };

    std::unordered_map<std::wstring, UINT> vkMap = {
        {L"TAB", VK_TAB},
        {L"ENTER", VK_RETURN},
        {L"RETURN", VK_RETURN},
        {L"SPACE", VK_SPACE},
        {L"ESC", VK_ESCAPE},
        {L"ESCAPE", VK_ESCAPE},
        {L"BACKSPACE", VK_BACK},
        {L"HOME", VK_HOME},
        {L"END", VK_END},
        {L"PAGEUP", VK_PRIOR},
        {L"PAGEDOWN", VK_NEXT},
        {L"INSERT", VK_INSERT},
        {L"DELETE", VK_DELETE},
        {L"LEFT", VK_LEFT},
        {L"RIGHT", VK_RIGHT},
        {L"UP", VK_UP},
        {L"DOWN", VK_DOWN},
        {L"NUMLOCK", VK_NUMLOCK},
        {L"VOLUMEMUTE", VK_VOLUME_MUTE},
        {L"VOLUMEUP", VK_VOLUME_UP},
        {L"VOLUMEDOWN", VK_VOLUME_DOWN},
        {L"MEDIAPLAYPAUSE", VK_MEDIA_PLAY_PAUSE},
        {L"MEDIANEXT", VK_MEDIA_NEXT_TRACK},
        {L"MEDIAPREV", VK_MEDIA_PREV_TRACK},
        {L"MEDIASTOP", VK_MEDIA_STOP},
    };

    UINT modifiers = 0;
    UINT vk = 0;
    size_t start = 0;

    while (start <= shortcut.size()) {
        size_t plus = shortcut.find(L'+', start);
        std::wstring_view part = plus == std::wstring_view::npos
            ? shortcut.substr(start)
            : shortcut.substr(start, plus - start);
        part = trim(part);
        if (part.empty()) return false;

        std::wstring token(part);
        token = ToUpper(std::move(token));

        auto modIt = modifiersMap.find(token);
        if (modIt != modifiersMap.end()) {
            modifiers |= modIt->second;
        } else {
            if (vk != 0) return false;

            if (token.size() == 1 && token[0] >= L'A' && token[0] <= L'Z') {
                vk = (UINT)token[0];
            } else if (token.size() == 1 && token[0] >= L'0' && token[0] <= L'9') {
                vk = (UINT)token[0];
            } else if (token.size() >= 2 && token[0] == L'F') {
                int fn = _wtoi(token.c_str() + 1);
                if (fn >= 1 && fn <= 24) {
                    vk = VK_F1 + (UINT)(fn - 1);
                } else {
                    return false;
                }
            } else if (token.rfind(L"NUMPAD", 0) == 0 && token.size() == 7 &&
                       token[6] >= L'0' && token[6] <= L'9') {
                vk = VK_NUMPAD0 + (UINT)(token[6] - L'0');
            } else {
                auto vkIt = vkMap.find(token);
                if (vkIt != vkMap.end()) {
                    vk = vkIt->second;
                } else {
                    try {
                        size_t pos = 0;
                        unsigned long parsed = std::stoul(token, &pos, 0);
                        if (pos != token.size() || parsed == 0 || parsed > 0xFF) return false;
                        vk = (UINT)parsed;
                    } catch (...) {
                        return false;
                    }
                }
            }
        }

        if (plus == std::wstring_view::npos) break;
        start = plus + 1;
    }

    if (vk == 0) return false;
    *modifiersOut = modifiers;
    *vkOut = vk;
    return true;
}

static bool SendShortcut(UINT modifiers, UINT vk) {
    std::vector<INPUT> inputs;
    inputs.reserve(10);

    auto addKey = [&inputs](WORD key, DWORD flags) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = key;
        in.ki.dwFlags = flags;
        inputs.push_back(in);
    };

    if (modifiers & MOD_CONTROL) addKey(VK_CONTROL, 0);
    if (modifiers & MOD_SHIFT) addKey(VK_SHIFT, 0);
    if (modifiers & MOD_ALT) addKey(VK_MENU, 0);
    if (modifiers & MOD_WIN) addKey(VK_LWIN, 0);

    addKey((WORD)vk, 0);
    addKey((WORD)vk, KEYEVENTF_KEYUP);

    if (modifiers & MOD_WIN) addKey(VK_LWIN, KEYEVENTF_KEYUP);
    if (modifiers & MOD_ALT) addKey(VK_MENU, KEYEVENTF_KEYUP);
    if (modifiers & MOD_SHIFT) addKey(VK_SHIFT, KEYEVENTF_KEYUP);
    if (modifiers & MOD_CONTROL) addKey(VK_CONTROL, KEYEVENTF_KEYUP);

    UINT sent = SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
    return sent == inputs.size();
}

static void ExecuteAction(const std::wstring& raw) {
    if (raw.empty()) return;

    std::thread([raw]() {
        const std::wstring& a = raw;

        if (StartsWithCI(a, L"web:")) {
            ShellExecuteW(nullptr, L"open", a.substr(4).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
        if (StartsWithCI(a, L"ms-settings:")) {
            ShellExecuteW(nullptr, L"open", a.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
        if (StartsWithCI(a, L"cmd:")) {
            std::wstring arg = L"/C " + a.substr(4);
            ShellExecuteW(nullptr, L"open", L"cmd.exe", arg.c_str(), nullptr, SW_HIDE);
            return;
        }
        if (StartsWithCI(a, L"shell:")) {
            std::wstring arg = L"-NoProfile -ExecutionPolicy Bypass -Command " + a.substr(6);
            ShellExecuteW(nullptr, L"open", L"powershell.exe", arg.c_str(), nullptr, SW_HIDE);
            return;
        }
        if (StartsWithCI(a, L"key:") || StartsWithCI(a, L"hotkey:")) {
            std::wstring shortcut = StartsWithCI(a, L"key:") ? a.substr(4) : a.substr(7);
            UINT modifiers = 0;
            UINT vk = 0;
            if (!TryParseShortcut(shortcut, &modifiers, &vk)) {
                Wh_Log(L"Invalid shortcut action: %s", shortcut.c_str());
                return;
            }
            if (!SendShortcut(modifiers, vk)) {
                Wh_Log(L"Failed to send shortcut: %s", shortcut.c_str());
            }
            return;
        }
        if (!LaunchDefaultAction(a)) {
            Wh_Log(L"Failed to launch default action: %s", a.c_str());
        }
    }).detach();
}

static SolidColorBrush GetTextBrush() {
    try {
        auto resources = Application::Current().Resources();
        for (const auto& key : {L"SystemControlForegroundBaseHighBrush", L"TextFillColorPrimaryBrush"}) {
            auto value = resources.Lookup(box_value(key));
            if (auto brush = value.try_as<SolidColorBrush>()) {
                return brush;
            }
        }
    } catch (...) {}
    return SolidColorBrush(Colors::White());
}

static Brush GetThemeBrush(const wchar_t* key) {
    try {
        auto resources = Application::Current().Resources();
        auto value = resources.Lookup(box_value(std::wstring(key)));
        if (auto brush = value.try_as<Brush>()) {
            return brush;
        }
    } catch (...) {}
    return SolidColorBrush(Colors::Transparent());
}

static FrameworkElement MakeGlyphIcon(const std::wstring& iconRaw, double size) {
    try {
        WCHAR glyph = 0xE700;
        if (!iconRaw.empty()) {
            glyph = (WCHAR)wcstoul(iconRaw.c_str(), nullptr, 16);
            if (glyph == 0) glyph = 0xE700;
        }

        TextBlock tb;
        tb.Text(hstring(std::wstring(1, glyph)));
        tb.FontFamily(FontFamily(L"Segoe Fluent Icons"));
        tb.FontSize(size);
        tb.Foreground(GetTextBrush());
        tb.HorizontalAlignment(HorizontalAlignment::Center);
        tb.VerticalAlignment(VerticalAlignment::Center);
        return tb;
    } catch (...) {
        return nullptr;
    }
}

static FrameworkElement MakeIconElement(const std::wstring& iconRaw, double size) {
    if (iconRaw.empty()) return nullptr;
    return MakeGlyphIcon(iconRaw, size);
}

static std::wstring ResolveDisplayMode(const MenuBarButton& item) {
    if (item.displayMode.empty() || item.displayMode == L"default") {
        return g_settings.displayMode;
    }
    if (item.displayMode == L"text" || item.displayMode == L"both" || item.displayMode == L"icon") {
        return item.displayMode;
    }
    return g_settings.displayMode;
}

static HorizontalAlignment ResolveButtonContentAlignment() {
    if (g_settings.buttonContentAlignment == L"left") return HorizontalAlignment::Left;
    if (g_settings.buttonContentAlignment == L"right") return HorizontalAlignment::Right;
    return HorizontalAlignment::Center;
}

static void ApplyButtonContentAlignment(FrameworkElement const& element, HorizontalAlignment alignment) {
    if (!element) return;

    try {
        element.HorizontalAlignment(alignment);

        if (auto tb = element.try_as<TextBlock>()) {
            if (alignment == HorizontalAlignment::Left) {
                tb.TextAlignment(TextAlignment::Left);
            } else if (alignment == HorizontalAlignment::Right) {
                tb.TextAlignment(TextAlignment::Right);
            } else {
                tb.TextAlignment(TextAlignment::Center);
            }
        }
    } catch (...) {}
}

static double GetButtonContentMinWidth() {
    if (g_settings.buttonContentAlignment == L"center") {
        return 0.0;
    }

    return std::max(64.0, (double)g_settings.iconSize + (double)g_settings.buttonPaddingX * 2.0 + 24.0);
}

static FrameworkElement MakeButtonContent(const MenuBarButton& item, const std::wstring& displayMode) {
    bool showText = (displayMode == L"text" || displayMode == L"both");
    bool showIcon = (displayMode == L"icon" || displayMode == L"both");
    bool hasText = !item.label.empty();
    bool hasIcon = !item.iconRaw.empty();

    if (displayMode == L"both" && (showIcon || showText)) {
        if (hasIcon && hasText) {
            StackPanel panel;
            panel.Orientation(Orientation::Horizontal);

            auto icon = MakeIconElement(item.iconRaw, (double)g_settings.iconSize);
            if (icon) {
                if (auto fe = icon.try_as<FrameworkElement>()) {
                    fe.Margin({0, 0, 6, 0});
                }
                panel.Children().Append(icon);
            } else {
                TextBlock spacer;
                spacer.Text(L"");
                spacer.Margin({0, 0, 6, 0});
                panel.Children().Append(spacer);
            }

            TextBlock text;
            text.Text(hstring(item.label));
            text.FontSize((double)g_settings.textSize);
            text.Foreground(GetTextBrush());
            text.TextWrapping(TextWrapping::NoWrap);
            text.TextTrimming(TextTrimming::CharacterEllipsis);
            text.VerticalAlignment(VerticalAlignment::Center);
            panel.Children().Append(text);
            ApplyButtonContentAlignment(panel, ResolveButtonContentAlignment());
            return panel;
        }

        if (hasIcon) {
            auto icon = MakeIconElement(item.iconRaw, (double)g_settings.iconSize);
            if (auto fe = icon.try_as<FrameworkElement>()) {
                ApplyButtonContentAlignment(fe, ResolveButtonContentAlignment());
            }
            return icon;
        }

        if (hasText) {
            TextBlock text;
            text.Text(hstring(item.label));
            text.FontSize((double)g_settings.textSize);
            text.Foreground(GetTextBrush());
            text.TextWrapping(TextWrapping::NoWrap);
            text.TextTrimming(TextTrimming::CharacterEllipsis);
            text.VerticalAlignment(VerticalAlignment::Center);
            ApplyButtonContentAlignment(text, ResolveButtonContentAlignment());
            return text;
        }
    }

    if (showIcon && !showText) {
        if (hasIcon) {
            auto icon = MakeIconElement(item.iconRaw, (double)g_settings.iconSize);
            if (auto fe = icon.try_as<FrameworkElement>()) {
                ApplyButtonContentAlignment(fe, ResolveButtonContentAlignment());
            }
            return icon;
        }
        auto glyph = MakeGlyphIcon(L"E700", (double)g_settings.iconSize);
        if (auto fe = glyph.try_as<FrameworkElement>()) {
            ApplyButtonContentAlignment(fe, ResolveButtonContentAlignment());
        }
        return glyph;
    }

    TextBlock text;
    text.Text(hstring(item.label.empty() ? L"Button" : item.label));
    text.FontSize((double)g_settings.textSize);
    text.Foreground(GetTextBrush());
    text.TextWrapping(TextWrapping::NoWrap);
    text.TextTrimming(TextTrimming::CharacterEllipsis);
    text.VerticalAlignment(VerticalAlignment::Center);
    ApplyButtonContentAlignment(text, ResolveButtonContentAlignment());
    return text;
}

static Style CreateTaskbarLikeButtonStyle() {
    static const wchar_t kStyleXaml[] = LR"XAML(
<Style TargetType="Button"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
  <Style.Resources>
    <SolidColorBrush x:Key="TaskbarMenuBarButtonHoverBorderBrush" Color="#26FFFFFF"/>
    <SolidColorBrush x:Key="TaskbarMenuBarButtonPressedBorderBrush" Color="#1EFFFFFF"/>
  </Style.Resources>
  <Setter Property="Background" Value="Transparent"/>
  <Setter Property="BorderBrush" Value="Transparent"/>
  <Setter Property="BorderThickness" Value="1"/>
  <Setter Property="CornerRadius" Value="4"/>
  <Setter Property="UseSystemFocusVisuals" Value="False"/>
  <Setter Property="Padding" Value="0"/>
  <Setter Property="Template">
    <Setter.Value>
      <ControlTemplate TargetType="Button">
        <Grid x:Name="Root">
          <VisualStateManager.VisualStateGroups>
            <VisualStateGroup x:Name="CommonStates">
              <VisualState x:Name="Normal">
                <Storyboard>
                  <DoubleAnimation Storyboard.TargetName="ContentPresenter"
                                   Storyboard.TargetProperty="(UIElement.RenderTransform).(ScaleTransform.ScaleX)"
                                   To="1"
                                   Duration="0:0:0.12"
                                   EnableDependentAnimation="True"/>
                  <DoubleAnimation Storyboard.TargetName="ContentPresenter"
                                   Storyboard.TargetProperty="(UIElement.RenderTransform).(ScaleTransform.ScaleY)"
                                   To="1"
                                   Duration="0:0:0.12"
                                   EnableDependentAnimation="True"/>
                </Storyboard>
              </VisualState>
              <VisualState x:Name="PointerOver">
                <Storyboard>
                  <ObjectAnimationUsingKeyFrames Storyboard.TargetName="BackgroundBorder" Storyboard.TargetProperty="Background">
                    <DiscreteObjectKeyFrame KeyTime="0" Value="{ThemeResource ControlFillColorSecondaryBrush}"/>
                  </ObjectAnimationUsingKeyFrames>
                  <ObjectAnimationUsingKeyFrames Storyboard.TargetName="BackgroundBorder" Storyboard.TargetProperty="BorderBrush">
                    <DiscreteObjectKeyFrame KeyTime="0" Value="{StaticResource TaskbarMenuBarButtonHoverBorderBrush}"/>
                  </ObjectAnimationUsingKeyFrames>
                  <DoubleAnimation Storyboard.TargetName="ContentPresenter"
                                   Storyboard.TargetProperty="(UIElement.RenderTransform).(ScaleTransform.ScaleX)"
                                   To="1"
                                   Duration="0:0:0.12"
                                   EnableDependentAnimation="True"/>
                  <DoubleAnimation Storyboard.TargetName="ContentPresenter"
                                   Storyboard.TargetProperty="(UIElement.RenderTransform).(ScaleTransform.ScaleY)"
                                   To="1"
                                   Duration="0:0:0.12"
                                   EnableDependentAnimation="True"/>
                </Storyboard>
              </VisualState>
              <VisualState x:Name="Pressed">
                <Storyboard>
                  <ObjectAnimationUsingKeyFrames Storyboard.TargetName="BackgroundBorder" Storyboard.TargetProperty="Background">
                    <DiscreteObjectKeyFrame KeyTime="0" Value="{ThemeResource ControlFillColorTertiaryBrush}"/>
                  </ObjectAnimationUsingKeyFrames>
                  <ObjectAnimationUsingKeyFrames Storyboard.TargetName="BackgroundBorder" Storyboard.TargetProperty="BorderBrush">
                    <DiscreteObjectKeyFrame KeyTime="0" Value="{StaticResource TaskbarMenuBarButtonPressedBorderBrush}"/>
                  </ObjectAnimationUsingKeyFrames>
                  <DoubleAnimation Storyboard.TargetName="ContentPresenter"
                                   Storyboard.TargetProperty="(UIElement.RenderTransform).(ScaleTransform.ScaleX)"
                                   To="0.92"
                                   Duration="0:0:0.05"
                                   EnableDependentAnimation="True"/>
                  <DoubleAnimation Storyboard.TargetName="ContentPresenter"
                                   Storyboard.TargetProperty="(UIElement.RenderTransform).(ScaleTransform.ScaleY)"
                                   To="0.92"
                                   Duration="0:0:0.05"
                                   EnableDependentAnimation="True"/>
                </Storyboard>
              </VisualState>
            </VisualStateGroup>
          </VisualStateManager.VisualStateGroups>
          <Grid>
            <Border x:Name="BackgroundBorder"
                    Background="{TemplateBinding Background}"
                    BorderBrush="{TemplateBinding BorderBrush}"
                    BorderThickness="{TemplateBinding BorderThickness}"
                    CornerRadius="{TemplateBinding CornerRadius}"/>
            <ContentPresenter x:Name="ContentPresenter"
              Content="{TemplateBinding Content}"
              ContentTemplate="{TemplateBinding ContentTemplate}"
              ContentTransitions="{TemplateBinding ContentTransitions}"
              HorizontalAlignment="{TemplateBinding HorizontalContentAlignment}"
              VerticalAlignment="{TemplateBinding VerticalContentAlignment}"
              Padding="{TemplateBinding Padding}"
              RenderTransformOrigin="0.5,0.5">
              <ContentPresenter.RenderTransform>
                <ScaleTransform ScaleX="1" ScaleY="1"/>
              </ContentPresenter.RenderTransform>
            </ContentPresenter>
          </Grid>
        </Grid>
      </ControlTemplate>
    </Setter.Value>
  </Setter>
</Style>)XAML";

    try {
        return XamlReader::Load(hstring(kStyleXaml)).as<Style>();
    } catch (...) {
        return nullptr;
    }
}

static void ApplyTaskbarLikeButtonChrome(Button const& btn) {
    try {
        if (auto style = CreateTaskbarLikeButtonStyle()) {
            btn.Style(style);
        } else {
            btn.Background(SolidColorBrush(Colors::Transparent()));
            btn.BorderBrush(SolidColorBrush(Colors::Transparent()));
            btn.BorderThickness({1, 1, 1, 1});
            btn.UseSystemFocusVisuals(false);
            btn.CornerRadius({4, 4, 4, 4});
        }
    } catch (...) {}
}

static bool IsTaskbarHorizontal(HWND hWnd);

static double ComputeHighlightMinHeight(double taskbarHeight) {
    if (g_settings.highlightSize != L"default") {
        return 0.0;
    }

    return std::max(24.0, taskbarHeight - 8.0);
}

static bool IsTaskbarHorizontal(HWND hWnd) {
    if (!hWnd) return true;

    APPBARDATA abd{};
    abd.cbSize = sizeof(abd);
    abd.hWnd = hWnd;
    if (SHAppBarMessage(ABM_GETTASKBARPOS, &abd)) {
        return abd.uEdge == ABE_TOP || abd.uEdge == ABE_BOTTOM;
    }

    RECT rc{};
    if (GetWindowRect(hWnd, &rc)) {
        return (rc.right - rc.left) >= (rc.bottom - rc.top);
    }

    return true;
}

enum class PlacementKind {
    TrayLeft = 1,
    TrayRight = 2,
    TrayBeforeClock = 3,
    TrayAfterClock = 4,
    TrayBeforeOmniLeft = 5,
    TrayBeforeOmniRight = 6,
    TrayLanguageLeft = 7,
    TrayLanguageRight = 8,
    TrayIconsLeft = 9,
    TrayIconsRight = 10,
    TrayHiddenIconsLeft = 11,
    TrayHiddenIconsRight = 12,
    TrayAfterShowDesktopLeft = 13,
    TrayAfterShowDesktopRight = 14,
    TaskbarLeftEdge = 15,
    TaskbarCenterEdge = 16,
    TaskbarRightEdge = 17,
    TaskbarLeftStart = 18,
    TaskbarRightStart = 19,
    TaskbarAfterSearchLeft = 20,
    TaskbarAfterSearchRight = 21,
    TaskbarAfterTaskViewLeft = 22,
    TaskbarAfterTaskViewRight = 23,
    TaskbarAfterWidgetsLeft = 24,
    TaskbarAfterWidgetsRight = 25,
};

struct PlacementResult {
    Grid parent = nullptr;
    FrameworkElement anchor = nullptr;
    PlacementKind kind = PlacementKind::TaskbarRightStart;
    int insertColumn = kOverlayColumn;
};

static bool RunFromWindowThread(HWND hWnd, WindowThreadProc proc, void* param) {
    static const UINT kMsg = RegisterWindowMessage(L"Windhawk_RunFromWindowThread_" WH_MOD_ID);

    struct Payload {
        WindowThreadProc proc;
        void* param;
    };

    DWORD tid = GetWindowThreadProcessId(hWnd, nullptr);
    if (!tid) return false;

    if (tid == GetCurrentThreadId()) {
        proc(param);
        return true;
    }

    HHOOK hook = SetWindowsHookExW(
        WH_CALLWNDPROC,
        [](int code, WPARAM wParam, LPARAM lParam) CALLBACK -> LRESULT {
            if (code == HC_ACTION) {
                auto* cwp = reinterpret_cast<const CWPSTRUCT*>(lParam);
                static const UINT kM = RegisterWindowMessage(L"Windhawk_RunFromWindowThread_" WH_MOD_ID);
                if (cwp->message == kM) {
                    auto* p = reinterpret_cast<Payload*>(cwp->lParam);
                    p->proc(p->param);
                }
            }
            return CallNextHookEx(nullptr, code, wParam, lParam);
        },
        nullptr,
        tid);

    if (!hook) return false;

    Payload pay{proc, param};
    SendMessageW(hWnd, kMsg, 0, reinterpret_cast<LPARAM>(&pay));
    UnhookWindowsHookEx(hook);
    return true;
}

static FrameworkElement FindChildByName(FrameworkElement const& root, std::wstring_view name, int depth = 32) {
    if (!root || depth == 0) return nullptr;
    int count = VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; ++i) {
        auto child = VisualTreeHelper::GetChild(root, i).try_as<FrameworkElement>();
        if (!child) continue;
        if (child.Name() == name) return child;
        if (auto found = FindChildByName(child, name, depth - 1)) return found;
    }
    return nullptr;
}

static const wchar_t* const kStartButtonNames[] = {
    L"StartButton",
    L"StartMenuButton",
    L"StartMenuLaunchButton",
    L"LaunchListButton",
};

static Grid FindTaskbarRootGrid(FrameworkElement const& root) {
    FrameworkElement taskbarFrame = nullptr;
    int count = VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; ++i) {
        auto c = VisualTreeHelper::GetChild(root, i).try_as<FrameworkElement>();
        if (c) {
            auto className = winrt::get_class_name(c);
            if (className == L"Taskbar.TaskbarFrame") {
                taskbarFrame = c;
                break;
            }
        }
    }

    if (!taskbarFrame) return nullptr;
    auto rootGrid = FindChildByName(taskbarFrame, L"RootGrid");
    return rootGrid ? rootGrid.try_as<Grid>() : nullptr;
}

static FrameworkElement FindElementInRepeater(FrameworkElement const& repeater, const wchar_t* const* names, int nameCount) {
    if (!repeater) return nullptr;

    int childCount = VisualTreeHelper::GetChildrenCount(repeater);
    for (int i = 0; i < childCount; i++) {
        auto child = VisualTreeHelper::GetChild(repeater, i).try_as<FrameworkElement>();
        if (!child) continue;

        for (int j = 0; j < nameCount; j++) {
            if (child.Name() == names[j]) return child;
        }
    }

    for (int i = 0; i < childCount; i++) {
        auto child = VisualTreeHelper::GetChild(repeater, i).try_as<FrameworkElement>();
        if (!child) continue;

        int subChildCount = VisualTreeHelper::GetChildrenCount(child);
        for (int k = 0; k < subChildCount; k++) {
            auto subChild = VisualTreeHelper::GetChild(child, k).try_as<FrameworkElement>();
            if (!subChild) continue;

            for (int j = 0; j < nameCount; j++) {
                if (subChild.Name() == names[j]) return subChild;
            }
        }
    }

    return nullptr;
}

static FrameworkElement FindElementByClassName(FrameworkElement const& parent, const wchar_t* className) {
    if (!parent) return nullptr;

    int childCount = VisualTreeHelper::GetChildrenCount(parent);
    for (int i = 0; i < childCount; i++) {
        auto child = VisualTreeHelper::GetChild(parent, i).try_as<FrameworkElement>();
        if (!child) continue;

        if (winrt::get_class_name(child) == className) return child;
    }

    return nullptr;
}

static FrameworkElement FindNthElementByClassName(FrameworkElement const& parent, const wchar_t* className, int index) {
    if (!parent) return nullptr;

    int foundCount = 0;
    int childCount = VisualTreeHelper::GetChildrenCount(parent);
    for (int i = 0; i < childCount; i++) {
        auto child = VisualTreeHelper::GetChild(parent, i).try_as<FrameworkElement>();
        if (!child) continue;

        if (winrt::get_class_name(child) == className) {
            if (foundCount == index) return child;
            foundCount++;
        }
    }

    return nullptr;
}

static FrameworkElement FindChildByClassName(FrameworkElement const& parent, const wchar_t* className, int depth = 32) {
    if (!parent || depth <= 0) return nullptr;

    int childCount = VisualTreeHelper::GetChildrenCount(parent);
    for (int i = 0; i < childCount; i++) {
        auto child = VisualTreeHelper::GetChild(parent, i).try_as<FrameworkElement>();
        if (!child) continue;

        if (winrt::get_class_name(child) == className) return child;
        if (auto found = FindChildByClassName(child, className, depth - 1)) return found;
    }

    return nullptr;
}

static FrameworkElement FindTrayElement(FrameworkElement const& trayGrid, FrameworkElement const& root, const wchar_t* name) {
    auto elem = FindChildByName(trayGrid, name);
    if (!elem) elem = FindChildByName(root, name);
    return elem;
}

static HWND FindCurrentProcessTaskbarWnd() {
    HWND result = nullptr;
    EnumWindows([](HWND hWnd, LPARAM lp) CALLBACK -> BOOL {
        DWORD pid = 0;
        wchar_t cls[32] = {};
        if (GetWindowThreadProcessId(hWnd, &pid) && pid == GetCurrentProcessId() &&
            GetClassNameW(hWnd, cls, ARRAYSIZE(cls)) &&
            _wcsicmp(cls, L"Shell_TrayWnd") == 0) {
            *reinterpret_cast<HWND*>(lp) = hWnd;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&result));
    return result;
}

static XamlRoot GetTaskbarXamlRoot(HWND hTaskbarWnd) {
    HWND hTaskSwWnd = (HWND)GetProp(hTaskbarWnd, L"TaskbandHWND");
    if (!hTaskSwWnd) return nullptr;

    void* taskBand = (void*)GetWindowLongPtrW(hTaskSwWnd, 0);
    void* taskBandForTaskListWndSite = taskBand;
    for (int i = 0; *(void**)taskBandForTaskListWndSite != CTaskBand_ITaskListWndSite_vftable; i++) {
        if (i == 20) return nullptr;
        taskBandForTaskListWndSite = (void**)taskBandForTaskListWndSite + 1;
    }

    void* taskbarHostSharedPtr[2]{};
    CTaskBand_GetTaskbarHost_Original(taskBandForTaskListWndSite, taskbarHostSharedPtr);
    if (!taskbarHostSharedPtr[0] && !taskbarHostSharedPtr[1]) return nullptr;

    size_t taskbarElementIUnknownOffset = 0x10;
#if defined(_M_X64) || defined(__x86_64__)
    {
        const BYTE* b = (const BYTE*)TaskbarHost_FrameHeight_Original;
        if (b[0] == 0x48 && b[1] == 0x83 && b[2] == 0xEC && b[4] == 0x48 &&
            b[5] == 0x83 && b[6] == 0xC1 && b[7] <= 0x7F) {
            taskbarElementIUnknownOffset = b[7];
        }
    }
#elif defined(_M_ARM64) || defined(__aarch64__)
    {
        const DWORD* p = (const DWORD*)TaskbarHost_FrameHeight_Original;
        if (p[0] == 0xD503237F && (p[1] & 0xFFC07FFF) == 0xA9807BFD &&
            p[2] == 0x910003FD && (p[3] & 0xFFF00FE0) == 0xF8400C00) {
            taskbarElementIUnknownOffset = (p[3] >> 12) & 0xFF;
        }
    }
#endif

    auto* taskbarElementIUnknown = *(::IUnknown**)((BYTE*)taskbarHostSharedPtr[0] + taskbarElementIUnknownOffset);
    FrameworkElement taskbarElement{nullptr};
    taskbarElementIUnknown->QueryInterface(winrt::guid_of<FrameworkElement>(), winrt::put_abi(taskbarElement));

    auto result = taskbarElement ? taskbarElement.XamlRoot() : nullptr;
    if (taskbarHostSharedPtr[1] && Std_Ref_Decref_Original) Std_Ref_Decref_Original(taskbarHostSharedPtr[1]);
    return result;
}

static std::vector<Button> GetMenuBarButtons() {
    std::vector<Button> buttons;
    if (!g_menuBarRoot) return buttons;

    try {
        auto panel = FindChildByName(g_menuBarRoot, L"TaskbarMenuBarPanel").try_as<Panel>();
        if (!panel) return buttons;

        auto children = panel.Children();
        for (uint32_t i = 0; i < children.Size(); ++i) {
            if (auto child = children.GetAt(i).try_as<Button>()) {
                buttons.push_back(child);
            }
        }
    } catch (...) {}

    return buttons;
}

static void UpdateButtonWidths() {
    if (!g_menuBarRoot) return;
    if (g_settings.buttonWidthMode != L"static" &&
        g_settings.buttonWidthMode != L"custom") {
        return;
    }

    try {
        auto buttons = GetMenuBarButtons();
        if (buttons.empty()) return;

        double targetWidth = 0.0;
        if (g_settings.buttonWidthMode == L"custom") {
            targetWidth = (double)std::max(1, g_settings.customButtonWidth);
        } else {
            for (auto const& button : buttons) {
                targetWidth = std::max(targetWidth, button.ActualWidth());
            }
        }

        if (targetWidth <= 0.5) return;

        for (auto const& button : buttons) {
            double targetMinWidth = std::min(button.MinWidth(), targetWidth);
            if (std::abs(button.MinWidth() - targetMinWidth) > 0.5) {
                button.MinWidth(targetMinWidth);
            }
            double currentWidth = button.Width();
            if (std::isnan(currentWidth) || std::abs(currentWidth - targetWidth) > 0.5) {
                button.Width(targetWidth);
            }
        }
    } catch (...) {}
}

static void UpdateButtonHighlightHeight() {
    if (!g_menuBarRoot || !g_injectionParent) return;

    double taskbarHeight = std::max(1.0, g_injectionParent.ActualHeight());
    double highlightMinHeight = ComputeHighlightMinHeight(taskbarHeight);

    if (g_hasLastHighlightMinHeight &&
        std::abs(g_lastHighlightMinHeight - highlightMinHeight) <= 0.5) {
        return;
    }

    try {
        auto buttons = GetMenuBarButtons();
        for (auto const& button : buttons) {
            button.MinHeight(highlightMinHeight);
        }

        g_lastHighlightMinHeight = highlightMinHeight;
        g_hasLastHighlightMinHeight = true;
    } catch (...) {}
}

static Grid BuildMenuBar() {
    Grid root;
    root.Name(L"TaskbarMenuBarRoot");
    root.VerticalAlignment(VerticalAlignment::Center);
    root.HorizontalAlignment(HorizontalAlignment::Left);
    root.IsHitTestVisible(true);
    root.Background(SolidColorBrush(Colors::Transparent()));
    root.MinWidth(1.0);
    root.MinHeight(1.0);

    std::vector<MenuBarButton> items;
    {
        std::lock_guard<std::mutex> lk(g_buttonsMutex);
        items = g_buttons;
    }

    std::vector<MenuBarButton> visibleItems;
    visibleItems.reserve(items.size());
    for (const auto& item : items) {
        if (item.label.empty() && item.iconRaw.empty() && item.action.empty()) continue;
        visibleItems.push_back(item);
    }

    if (visibleItems.empty()) return nullptr;

    bool taskbarHorizontal = IsTaskbarHorizontal(g_taskbarWnd);
    bool wrapVerticalInTwoRows = g_settings.orientation == L"vertical" && taskbarHorizontal && visibleItems.size() > 2;
    bool wrapHorizontalInTwoColumns = g_settings.orientation == L"horizontal" && !taskbarHorizontal && visibleItems.size() > 2;

    auto buildButton = [&](const MenuBarButton& item, size_t index) {
        Button btn;
        btn.Name(hstring(std::wstring(L"TaskbarMenuBarButton_") + std::to_wstring(index)));
        btn.VerticalAlignment(VerticalAlignment::Center);
        btn.HorizontalAlignment(HorizontalAlignment::Center);
        btn.HorizontalContentAlignment(ResolveButtonContentAlignment());
        btn.MinWidth(GetButtonContentMinWidth());
        btn.Padding({(double)g_settings.buttonPaddingX, (double)g_settings.buttonPaddingY,
                     (double)g_settings.buttonPaddingX, (double)g_settings.buttonPaddingY});
        ApplyTaskbarLikeButtonChrome(btn);
        btn.Click([action = item.action](auto const&, auto const&) {
            ExecuteAction(action);
        });
        ToolTipService::SetToolTip(btn, box_value(item.label));

        auto content = MakeButtonContent(item, ResolveDisplayMode(item));
        if (content) {
            btn.Content(content);
        }

        return btn;
    };

    if (wrapVerticalInTwoRows) {
        Grid panel;
        panel.Name(L"TaskbarMenuBarPanel");
        panel.VerticalAlignment(VerticalAlignment::Center);
        panel.HorizontalAlignment(HorizontalAlignment::Left);
        panel.Margin({(double)g_settings.buttonOffsetX, (double)g_settings.buttonOffsetY, 0, 0});

        const int rowCount = 2;
        const int columnCount = (int)((visibleItems.size() + rowCount - 1) / rowCount);
        for (int r = 0; r < rowCount; ++r) {
            panel.RowDefinitions().Append(RowDefinition());
        }
        for (int c = 0; c < columnCount; ++c) {
            panel.ColumnDefinitions().Append(ColumnDefinition());
        }

        for (size_t i = 0; i < visibleItems.size(); ++i) {
            auto btn = buildButton(visibleItems[i], i);

            int row = (int)(i % rowCount);
            int column = (int)(i / rowCount);
            Grid::SetRow(btn, row);
            Grid::SetColumn(btn, column);

            Thickness margin{0, 0, 0, 0};
            if (row == 0) {
                margin.Bottom = (double)g_settings.buttonSpacing;
            }
            if (column + 1 < columnCount) {
                margin.Right = (double)g_settings.buttonSpacing;
            }
            btn.Margin(margin);

            panel.Children().Append(btn);
        }

        root.Children().Append(panel);
        return root;
    }

    if (wrapHorizontalInTwoColumns) {
        Grid panel;
        panel.Name(L"TaskbarMenuBarPanel");
        panel.VerticalAlignment(VerticalAlignment::Center);
        panel.HorizontalAlignment(HorizontalAlignment::Left);
        panel.Margin({(double)g_settings.buttonOffsetX, (double)g_settings.buttonOffsetY, 0, 0});

        const int columnCount = 2;
        const int rowCount = (int)((visibleItems.size() + columnCount - 1) / columnCount);
        for (int r = 0; r < rowCount; ++r) {
            panel.RowDefinitions().Append(RowDefinition());
        }
        for (int c = 0; c < columnCount; ++c) {
            panel.ColumnDefinitions().Append(ColumnDefinition());
        }

        for (size_t i = 0; i < visibleItems.size(); ++i) {
            auto btn = buildButton(visibleItems[i], i);

            int row = (int)(i / columnCount);
            int column = (int)(i % columnCount);
            Grid::SetRow(btn, row);
            Grid::SetColumn(btn, column);

            Thickness margin{0, 0, 0, 0};
            if (column == 0) {
                margin.Right = (double)g_settings.buttonSpacing;
            }
            if (row + 1 < rowCount) {
                margin.Bottom = (double)g_settings.buttonSpacing;
            }
            btn.Margin(margin);

            panel.Children().Append(btn);
        }

        root.Children().Append(panel);
        return root;
    }

    StackPanel panel;
    panel.Name(L"TaskbarMenuBarPanel");
    panel.Orientation(g_settings.orientation == L"vertical" ? Orientation::Vertical : Orientation::Horizontal);
    panel.VerticalAlignment(VerticalAlignment::Center);
    panel.HorizontalAlignment(HorizontalAlignment::Left);
    panel.Margin({(double)g_settings.buttonOffsetX, (double)g_settings.buttonOffsetY, 0, 0});

    for (size_t i = 0; i < visibleItems.size(); ++i) {
        auto btn = buildButton(visibleItems[i], i);

        if (i + 1 < visibleItems.size()) {
            if (g_settings.orientation == L"vertical") {
                btn.Margin({0, 0, 0, (double)g_settings.buttonSpacing});
            } else {
                btn.Margin({0, 0, (double)g_settings.buttonSpacing, 0});
            }
        }

        panel.Children().Append(btn);
    }

    root.Children().Append(panel);
    return root;
}

static bool IsTrayPlacement(PlacementKind kind) {
    return kind >= PlacementKind::TrayLeft && kind <= PlacementKind::TrayAfterShowDesktopRight;
}

static bool IsTaskbarTrackingPlacement(PlacementKind kind) {
    return kind >= PlacementKind::TaskbarLeftStart && kind <= PlacementKind::TaskbarAfterWidgetsRight;
}

static bool InsertMenuBarColumn(Grid const& parent, int index) {
    if (!parent || index < 0) return false;

    try {
        auto cols = parent.ColumnDefinitions();
        ColumnDefinition newCol;
        newCol.Width({1.0, GridUnitType::Auto});

        if (index >= (int)cols.Size()) {
            cols.Append(newCol);
        } else {
            cols.InsertAt(index, newCol);
            auto children = parent.Children();
            for (uint32_t i = 0; i < children.Size(); ++i) {
                auto child = children.GetAt(i).try_as<FrameworkElement>();
                if (!child) continue;
                int childCol = Grid::GetColumn(child);
                if (childCol >= index) {
                    Grid::SetColumn(child, childCol + 1);
                }
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

static void RemoveMenuBarColumn(Grid const& parent, int index) {
    if (!parent || index < 0) return;

    try {
        auto cols = parent.ColumnDefinitions();
        if (index >= (int)cols.Size()) return;

        auto children = parent.Children();
        for (uint32_t i = 0; i < children.Size(); ++i) {
            auto child = children.GetAt(i).try_as<FrameworkElement>();
            if (!child) continue;
            int childCol = Grid::GetColumn(child);
            if (childCol > index) {
                Grid::SetColumn(child, childCol - 1);
            }
        }

        cols.RemoveAt(index);
    } catch (...) {}
}

static PlacementResult ResolvePlacement(FrameworkElement const& root) {
    auto trayFrame = FindChildByName(root, L"SystemTrayFrameGrid");
    auto trayGrid = trayFrame ? trayFrame.try_as<Grid>() : nullptr;
    auto rootGrid = FindTaskbarRootGrid(root);

    auto placement = g_settings.placement;

    if (trayGrid) {
        int col = -1;
        PlacementKind kind = PlacementKind::TrayLeft;

        if (placement == L"tray_right") {
            col = (int)trayGrid.ColumnDefinitions().Size();
            kind = PlacementKind::TrayRight;
        } else if (placement == L"tray_left") {
            col = 0;
            kind = PlacementKind::TrayLeft;
        } else if (placement == L"tray_before_clock") {
            auto clockBtn = FindChildByName(trayGrid, L"NotificationCenterButton");
            if (!clockBtn) clockBtn = FindChildByName(root, L"NotificationCenterButton");
            col = clockBtn ? Grid::GetColumn(clockBtn) : -1;
            kind = PlacementKind::TrayBeforeClock;
        } else if (placement == L"tray_after_clock") {
            auto showDesktop = FindChildByName(trayGrid, L"ShowDesktopStack");
            if (!showDesktop) showDesktop = FindChildByName(root, L"ShowDesktopStack");
            col = showDesktop ? Grid::GetColumn(showDesktop) : -1;
            kind = PlacementKind::TrayAfterClock;
        } else if (placement == L"tray_before_omni_left") {
            auto omniBtn = FindChildByName(trayGrid, L"ControlCenterButton");
            if (!omniBtn) omniBtn = FindChildByName(root, L"ControlCenterButton");
            col = omniBtn ? Grid::GetColumn(omniBtn) : -1;
            kind = PlacementKind::TrayBeforeOmniLeft;
        } else if (placement == L"tray_before_omni_right") {
            auto omniBtn = FindChildByName(trayGrid, L"ControlCenterButton");
            if (!omniBtn) omniBtn = FindChildByName(root, L"ControlCenterButton");
            col = omniBtn ? Grid::GetColumn(omniBtn) + 1 : -1;
            kind = PlacementKind::TrayBeforeOmniRight;
        } else if (placement == L"tray_language_left") {
            auto languageBtn = FindTrayElement(trayGrid, root, L"NonActivatableStack");
            col = languageBtn ? Grid::GetColumn(languageBtn) : -1;
            kind = PlacementKind::TrayLanguageLeft;
        } else if (placement == L"tray_language_right") {
            auto languageBtn = FindTrayElement(trayGrid, root, L"NonActivatableStack");
            col = languageBtn ? Grid::GetColumn(languageBtn) + 1 : -1;
            kind = PlacementKind::TrayLanguageRight;
        } else if (placement == L"tray_icons_left") {
            auto trayIcons = FindTrayElement(trayGrid, root, L"NotificationAreaIcons");
            col = trayIcons ? Grid::GetColumn(trayIcons) : -1;
            kind = PlacementKind::TrayIconsLeft;
        } else if (placement == L"tray_icons_right") {
            auto trayIcons = FindTrayElement(trayGrid, root, L"NotificationAreaIcons");
            col = trayIcons ? Grid::GetColumn(trayIcons) + 1 : -1;
            kind = PlacementKind::TrayIconsRight;
        } else if (placement == L"tray_hidden_icons_left") {
            auto hiddenIconsBtn = FindTrayElement(trayGrid, root, L"NotifyIconStack");
            col = hiddenIconsBtn ? Grid::GetColumn(hiddenIconsBtn) : -1;
            kind = PlacementKind::TrayHiddenIconsLeft;
        } else if (placement == L"tray_hidden_icons_right") {
            auto hiddenIconsBtn = FindTrayElement(trayGrid, root, L"NotifyIconStack");
            col = hiddenIconsBtn ? Grid::GetColumn(hiddenIconsBtn) + 1 : -1;
            kind = PlacementKind::TrayHiddenIconsRight;
        } else if (placement == L"tray_after_showdesktop_left") {
            auto showDesktop = FindChildByName(trayGrid, L"ShowDesktopStack");
            if (!showDesktop) showDesktop = FindChildByName(root, L"ShowDesktopStack");
            col = showDesktop ? Grid::GetColumn(showDesktop) : -1;
            kind = PlacementKind::TrayAfterShowDesktopLeft;
        } else if (placement == L"tray_after_showdesktop_right") {
            auto showDesktop = FindChildByName(trayGrid, L"ShowDesktopStack");
            if (!showDesktop) showDesktop = FindChildByName(root, L"ShowDesktopStack");
            col = showDesktop ? Grid::GetColumn(showDesktop) + 1 : (int)trayGrid.ColumnDefinitions().Size();
            kind = PlacementKind::TrayAfterShowDesktopRight;
        }

        if (col >= 0) {
            return {trayGrid, nullptr, kind, col};
        }
    }

    if (!rootGrid) return {};

    if (placement == L"taskbar_left_edge") {
        return {rootGrid, rootGrid, PlacementKind::TaskbarLeftEdge, kOverlayColumn};
    }
    if (placement == L"taskbar_center_edge") {
        return {rootGrid, rootGrid, PlacementKind::TaskbarCenterEdge, kOverlayColumn};
    }
    if (placement == L"taskbar_right_edge") {
        return {rootGrid, rootGrid, PlacementKind::TaskbarRightEdge, kOverlayColumn};
    }

    auto repeater = FindChildByName(rootGrid, L"TaskbarFrameRepeater");
    if (placement == L"taskbar_left_start") {
        auto target = FindElementInRepeater(repeater, kStartButtonNames, ARRAYSIZE(kStartButtonNames));
        return {rootGrid, target, target ? PlacementKind::TaskbarLeftStart : PlacementKind::TaskbarLeftEdge, kOverlayColumn};
    }
    if (placement == L"taskbar_right_start") {
        auto target = FindElementInRepeater(repeater, kStartButtonNames, ARRAYSIZE(kStartButtonNames));
        return {rootGrid, target, target ? PlacementKind::TaskbarRightStart : PlacementKind::TaskbarLeftEdge, kOverlayColumn};
    }
    if (placement == L"taskbar_after_search_left" || placement == L"taskbar_after_search_right") {
        auto target = FindElementByClassName(repeater, L"Taskbar.TaskbarExtensionElement");
        if (!target) return {rootGrid, rootGrid, PlacementKind::TaskbarLeftEdge, kOverlayColumn};
        return {rootGrid, target,
                placement == L"taskbar_after_search_left" ? PlacementKind::TaskbarAfterSearchLeft : PlacementKind::TaskbarAfterSearchRight,
                kOverlayColumn};
    }
    if (placement == L"taskbar_after_taskview_left" || placement == L"taskbar_after_taskview_right") {
        auto target = FindNthElementByClassName(repeater, L"Taskbar.ExperienceToggleButton", 1);
        if (!target) return {rootGrid, rootGrid, PlacementKind::TaskbarLeftEdge, kOverlayColumn};
        return {rootGrid, target,
                placement == L"taskbar_after_taskview_left" ? PlacementKind::TaskbarAfterTaskViewLeft : PlacementKind::TaskbarAfterTaskViewRight,
                kOverlayColumn};
    }
    if (placement == L"taskbar_after_widgets_left" || placement == L"taskbar_after_widgets_right") {
        auto target = FindChildByName(repeater, L"AugmentedEntryPointButton");
        if (!target) target = FindChildByClassName(repeater, L"Taskbar.AugmentedEntryPointButton");
        if (!target) return {rootGrid, rootGrid, PlacementKind::TaskbarLeftEdge, kOverlayColumn};
        return {rootGrid, target,
                placement == L"taskbar_after_widgets_left" ? PlacementKind::TaskbarAfterWidgetsLeft : PlacementKind::TaskbarAfterWidgetsRight,
                kOverlayColumn};
    }

    auto startButton = FindElementInRepeater(repeater, kStartButtonNames, ARRAYSIZE(kStartButtonNames));
    return {rootGrid, startButton, startButton ? PlacementKind::TaskbarRightStart : PlacementKind::TaskbarLeftEdge, kOverlayColumn};
}

static void RemoveMenuBar() {
    try {
        if (g_layoutUpdatedAttached && g_layoutUpdatedSource) {
            try { g_layoutUpdatedSource.LayoutUpdated(g_layoutUpdatedToken); } catch (...) {}
        }
        g_layoutUpdatedAttached = false;
        g_layoutUpdatedSource = nullptr;

        if (g_trackedElement) {
            try {
                if (g_hasTrackedElementOriginalMargin) {
                    g_trackedElement.Margin(g_trackedElementOriginalMargin);
                }
            } catch (...) {}
            g_trackedElement = nullptr;
        }
        g_hasTrackedElementOriginalMargin = false;
        g_trackPosition.clear();

        if (g_injectionParent && g_menuBarHost) {
            auto children = g_injectionParent.Children();
            for (uint32_t i = 0; i < children.Size(); ++i) {
                auto child = children.GetAt(i).try_as<FrameworkElement>();
                if (child && (child == g_menuBarHost || child.Name() == L"TaskbarMenuBarHost")) {
                    children.RemoveAt(i);
                    break;
                }
            }
        }

        if (g_injectionParent && g_insertedColumn >= 0) {
            RemoveMenuBarColumn(g_injectionParent, g_insertedColumn);
        }
    } catch (...) {}

    g_menuBarRoot = nullptr;
    g_menuBarHost = nullptr;
    g_injectionParent = nullptr;
    g_menuBarAnchor = nullptr;
    g_trackedElement = nullptr;
    g_hasTrackedElementOriginalMargin = false;
    g_trackPosition.clear();
    g_insertedColumn = -1;
    g_placementKind = 0;
    g_hasLastMenuBarPlacement = false;
    g_lastMenuBarX = -1.0;
    g_lastMenuBarY = -1.0;
    g_hasLastHighlightMinHeight = false;
    g_lastHighlightMinHeight = -1.0;
}

static bool IsLeftTrackingPlacement(PlacementKind kind) {
    return kind == PlacementKind::TaskbarLeftStart ||
           kind == PlacementKind::TaskbarAfterSearchLeft ||
           kind == PlacementKind::TaskbarAfterTaskViewLeft ||
           kind == PlacementKind::TaskbarAfterWidgetsLeft;
}

static bool IsWidgetsTrackingPlacement(PlacementKind kind) {
    return kind == PlacementKind::TaskbarAfterWidgetsLeft ||
           kind == PlacementKind::TaskbarAfterWidgetsRight;
}

static double GetTrackedPlacementGap(PlacementKind kind) {
    return IsWidgetsTrackingPlacement(kind) ? 8.0 : 0.0;
}

static bool AccumulateVisibleDescendantBounds(FrameworkElement const& root,
                                              FrameworkElement const& relativeTo,
                                              int depth,
                                              double rootWidth,
                                              double& left,
                                              double& right,
                                              double& top,
                                              double& bottom,
                                              bool& found) {
    if (!root || !relativeTo || depth <= 0) return found;

    int count = VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; ++i) {
        auto child = VisualTreeHelper::GetChild(root, i).try_as<FrameworkElement>();
        if (!child) continue;

        try {
            if (child.Visibility() == Visibility::Visible &&
                child.ActualWidth() > 1.0 &&
                child.ActualHeight() > 1.0) {
                std::wstring className(winrt::get_class_name(child).c_str());
                bool wideLayoutContainer =
                    child.ActualWidth() >= rootWidth - 2.0 &&
                    (className.find(L"Grid") != std::wstring::npos ||
                     className.find(L"Panel") != std::wstring::npos ||
                     className.find(L"Presenter") != std::wstring::npos ||
                     className.find(L"Border") != std::wstring::npos);

                if (!wideLayoutContainer) {
                    auto transform = child.TransformToVisual(relativeTo);
                    auto point = transform.TransformPoint({0, 0});
                    double childLeft = point.X;
                    double childRight = point.X + child.ActualWidth();
                    double childTop = point.Y;
                    double childBottom = point.Y + child.ActualHeight();

                    if (!found) {
                        left = childLeft;
                        right = childRight;
                        top = childTop;
                        bottom = childBottom;
                        found = true;
                    } else {
                        left = std::min(left, childLeft);
                        right = std::max(right, childRight);
                        top = std::min(top, childTop);
                        bottom = std::max(bottom, childBottom);
                    }
                }
            }
        } catch (...) {}

        AccumulateVisibleDescendantBounds(child, relativeTo, depth - 1, rootWidth,
                                          left, right, top, bottom, found);
    }

    return found;
}

static bool TryGetWidgetVisualBounds(FrameworkElement const& anchor,
                                     FrameworkElement const& relativeTo,
                                     double& left,
                                     double& right,
                                     double& top,
                                     double& height) {
    if (!anchor || !relativeTo) return false;

    try {
        double boundsLeft = 0.0;
        double boundsRight = 0.0;
        double boundsTop = 0.0;
        double boundsBottom = 0.0;
        bool found = false;
        AccumulateVisibleDescendantBounds(anchor, relativeTo, 8,
                                          std::max(1.0, anchor.ActualWidth()),
                                          boundsLeft, boundsRight,
                                          boundsTop, boundsBottom, found);
        if (!found || boundsRight <= boundsLeft || boundsBottom <= boundsTop) {
            return false;
        }

        left = boundsLeft;
        right = boundsRight;
        top = boundsTop;
        height = boundsBottom - boundsTop;
        return true;
    } catch (...) {
        return false;
    }
}

static void UpdateTrackedReservation(double barWidth) {
    if (!g_trackedElement || g_insertedColumn >= 0) return;

    auto kind = static_cast<PlacementKind>(g_placementKind);
    if (!IsTaskbarTrackingPlacement(kind)) return;

    try {
        double desiredGap = std::max(0.0, barWidth + GetTrackedPlacementGap(kind));
        auto margin = g_hasTrackedElementOriginalMargin ? g_trackedElementOriginalMargin : g_trackedElement.Margin();

        if (IsLeftTrackingPlacement(kind)) {
            margin.Left += desiredGap;
        } else {
            margin.Right += desiredGap;
        }

        auto current = g_trackedElement.Margin();
        if (std::abs(current.Left - margin.Left) > 0.5 ||
            std::abs(current.Right - margin.Right) > 0.5 ||
            std::abs(current.Top - margin.Top) > 0.5 ||
            std::abs(current.Bottom - margin.Bottom) > 0.5) {
            g_trackedElement.Margin(margin);
        }
    } catch (...) {
        g_trackedElement = nullptr;
        g_hasTrackedElementOriginalMargin = false;
        g_trackPosition.clear();
    }
}

static void UpdateMenuBarPlacement() {
    if (!g_menuBarRoot || !g_injectionParent || !g_menuBarHost) return;

    try {
        auto parent = g_injectionParent;
        auto bar = g_menuBarRoot;
        auto anchor = g_menuBarAnchor;
        auto host = g_menuBarHost;
        if (!parent || !bar || !host) return;

        UpdateButtonWidths();
        try { bar.UpdateLayout(); } catch (...) {}

        double barW = std::max(1.0, bar.ActualWidth());
        double barH = std::max(1.0, bar.ActualHeight());
        if (g_insertedColumn >= 0) {
            UpdateButtonHighlightHeight();
            return;
        }

        UpdateTrackedReservation(barW);

        double parentW = std::max(1.0, parent.ActualWidth());
        double parentH = std::max(1.0, parent.ActualHeight());
        double x = 0.0;
        double y = std::max(0.0, (parentH - barH) / 2.0);
        auto kind = static_cast<PlacementKind>(g_placementKind);
        const double gap = IsTaskbarTrackingPlacement(kind) ? GetTrackedPlacementGap(kind) : 6.0;

        switch (kind) {
            case PlacementKind::TrayLeft:
                if (anchor) {
                    auto transform = anchor.TransformToVisual(parent);
                    auto point = transform.TransformPoint({0, 0});
                    double anchorW = std::max(1.0, anchor.ActualWidth());
                    double anchorH = std::max(1.0, anchor.ActualHeight());
                    y = std::max(0.0, point.Y + (anchorH - barH) / 2.0);
                    x = std::max(0.0, point.X - barW - gap);
                }
                break;
            case PlacementKind::TrayRight:
                if (anchor) {
                    auto transform = anchor.TransformToVisual(parent);
                    auto point = transform.TransformPoint({0, 0});
                    double anchorW = std::max(1.0, anchor.ActualWidth());
                    double anchorH = std::max(1.0, anchor.ActualHeight());
                    y = std::max(0.0, point.Y + (anchorH - barH) / 2.0);
                    x = std::max(0.0, point.X + anchorW + gap);
                }
                break;
            case PlacementKind::TaskbarLeftEdge:
                x = 0.0;
                break;
            case PlacementKind::TaskbarCenterEdge:
                x = std::max(0.0, (parentW - barW) / 2.0);
                break;
            case PlacementKind::TaskbarRightEdge:
                x = std::max(0.0, parentW - barW);
                break;
            case PlacementKind::TaskbarLeftStart:
            case PlacementKind::TaskbarRightStart:
            case PlacementKind::TaskbarAfterSearchLeft:
            case PlacementKind::TaskbarAfterSearchRight:
            case PlacementKind::TaskbarAfterTaskViewLeft:
            case PlacementKind::TaskbarAfterTaskViewRight:
            case PlacementKind::TaskbarAfterWidgetsLeft:
            case PlacementKind::TaskbarAfterWidgetsRight:
                if (anchor) {
                    auto transform = anchor.TransformToVisual(parent);
                    auto point = transform.TransformPoint({0, 0});
                    double anchorW = std::max(1.0, anchor.ActualWidth());
                    double anchorH = std::max(1.0, anchor.ActualHeight());
                    double anchorLeft = point.X;
                    double anchorRight = point.X + anchorW;
                    double anchorTop = point.Y;
                    double anchorHeight = anchorH;
                    if (IsWidgetsTrackingPlacement(kind)) {
                        double visualLeft = 0.0;
                        double visualRight = 0.0;
                        double visualTop = 0.0;
                        double visualHeight = 0.0;
                        if (TryGetWidgetVisualBounds(anchor, parent, visualLeft, visualRight,
                                                     visualTop, visualHeight)) {
                            anchorLeft = visualLeft;
                            anchorRight = visualRight;
                            anchorTop = visualTop;
                            anchorHeight = visualHeight;
                        }
                    }
                    y = std::max(0.0, anchorTop + (anchorHeight - barH) / 2.0);
                    if (IsLeftTrackingPlacement(kind)) {
                        x = std::max(0.0, anchorLeft - barW - gap);
                    } else {
                        x = std::max(0.0, anchorRight + gap);
                    }
                }
                break;
            default:
                break;
        }

        if (!g_hasLastMenuBarPlacement ||
            std::abs(g_lastMenuBarX - x) > 0.5 ||
            std::abs(g_lastMenuBarY - y) > 0.5) {
            Canvas::SetLeft(bar, x);
            Canvas::SetTop(bar, y);
            g_lastMenuBarX = x;
            g_lastMenuBarY = y;
            g_hasLastMenuBarPlacement = true;
        }

        UpdateButtonHighlightHeight();
    } catch (...) {}
}

static bool InjectMenuBar() {
    if (g_unloading) return false;
    if (g_buttons.empty()) return false;

    HWND hWnd = g_taskbarWnd ? g_taskbarWnd : FindCurrentProcessTaskbarWnd();
    if (!hWnd) return false;
    g_taskbarWnd = hWnd;

    auto xamlRoot = GetTaskbarXamlRoot(hWnd);
    if (!xamlRoot) return false;

    auto root = xamlRoot.Content().try_as<FrameworkElement>();
    if (!root) return false;

    auto placement = ResolvePlacement(root);
    if (!placement.parent) return false;
    auto parent = placement.parent;
    g_injectionParent = parent;
    g_menuBarAnchor = placement.anchor;
    g_insertedColumn = -1;
    g_trackedElement = nullptr;
    g_hasTrackedElementOriginalMargin = false;
    g_trackPosition.clear();

    Grid bar = BuildMenuBar();
    if (!bar) return false;

    FrameworkElement host = nullptr;
    auto kind = placement.kind;

    if (IsTrayPlacement(kind) && placement.insertColumn >= 0) {
        if (!InsertMenuBarColumn(parent, placement.insertColumn)) return false;

        bar.Name(L"TaskbarMenuBarHost");
        bar.Margin({(double)g_settings.buttonOffsetX, 0, 0, 0});
        Grid::SetColumn(bar, placement.insertColumn);
        Canvas::SetZIndex(bar, 1000);
        parent.Children().Append(bar);

        host = bar;
        g_insertedColumn = placement.insertColumn;
    } else {
        Canvas canvasHost;
        canvasHost.Name(L"TaskbarMenuBarHost");
        canvasHost.HorizontalAlignment(HorizontalAlignment::Stretch);
        canvasHost.VerticalAlignment(VerticalAlignment::Stretch);
        canvasHost.IsHitTestVisible(true);
        Canvas::SetZIndex(canvasHost, 1000);
        canvasHost.Children().Append(bar);

        parent.Children().Append(canvasHost);
        host = canvasHost;

        if (IsTaskbarTrackingPlacement(kind) && placement.anchor) {
            g_trackedElement = placement.anchor;
            g_trackedElementOriginalMargin = placement.anchor.Margin();
            g_hasTrackedElementOriginalMargin = true;
            g_trackPosition = IsLeftTrackingPlacement(kind) ? L"left" : L"right";
        }
    }

    g_menuBarRoot = bar;
    g_menuBarHost = host;
    g_placementKind = (int)kind;

    if (g_layoutUpdatedAttached && g_layoutUpdatedSource) {
        try { g_layoutUpdatedSource.LayoutUpdated(g_layoutUpdatedToken); } catch (...) {}
    }
    g_layoutUpdatedSource = parent;
    g_layoutUpdatedToken = parent.LayoutUpdated([](auto const&, auto const&) {
        UpdateMenuBarPlacement();
    });
    g_layoutUpdatedAttached = true;

    try {
        parent.UpdateLayout();
    } catch (...) {}
    UpdateMenuBarPlacement();
    return true;
}

static bool TryApplySettings() {
    if (g_unloading || g_applyingSettings) return false;
    g_applyingSettings = true;
    bool ok = false;
    try {
        RemoveMenuBar();
        ok = InjectMenuBar();
    } catch (...) {
        ok = false;
    }
    g_applyingSettings = false;
    return ok;
}

static void ApplySettingsWithRetry(HWND hWnd) {
    if (g_retryScheduled.exchange(true)) return;
    std::thread([hWnd]() {
        struct ResetGuard {
            ~ResetGuard() { g_retryScheduled.store(false); }
        } guard;

        for (int i = 0; i < 20 && !g_unloading; ++i) {
            bool ok = false;
            RunFromWindowThread(hWnd, [](void* param) {
                *reinterpret_cast<bool*>(param) = TryApplySettings();
            }, &ok);
            if (ok) return;
            Sleep(100);
        }
    }).detach();
}

static void LoadSettings() {
    g_settings.placement = GetStringSetting(L"MenuBarSettings.placement", L"taskbar_right_start");
    g_settings.orientation = GetStringSetting(L"MenuBarSettings.orientation", L"horizontal");
    g_settings.displayMode = GetStringSetting(L"MenuBarSettings.displayMode", L"text");
    g_settings.buttonContentAlignment = GetStringSetting(L"MenuBarSettings.buttonContentAlignment", L"center");
    g_settings.highlightSize = GetStringSetting(L"MenuBarSettings.highlightSize", L"compact");
    g_settings.buttonWidthMode = GetStringSetting(L"MenuBarSettings.buttonWidthMode", L"dynamic");
    if (g_settings.buttonWidthMode != L"dynamic" &&
        g_settings.buttonWidthMode != L"static" &&
        g_settings.buttonWidthMode != L"custom") {
        g_settings.buttonWidthMode = L"dynamic";
    }
    g_settings.customButtonWidth = GetIntSetting(L"MenuBarSettings.customButtonWidth", 1, 1024, 80);
    g_settings.buttonSpacing = GetIntSetting(L"MenuBarSettings.buttonSpacing", 0, 64, 8);
    g_settings.iconSize = GetIntSetting(L"MenuBarSettings.iconSize", 8, 48, 16);
    g_settings.textSize = GetIntSetting(L"MenuBarSettings.textSize", 8, 32, 13);
    g_settings.buttonPaddingX = 8;
    g_settings.buttonPaddingY = 4;
    g_settings.buttonOffsetX = 0;
    g_settings.buttonOffsetY = 0;
    ParseIntPair(L"MenuBarSettings.buttonOffset", L"0,0", g_settings.buttonOffsetX, g_settings.buttonOffsetY);
    ParseIntPair(L"MenuBarSettings.buttonPadding", L"8,4", g_settings.buttonPaddingX, g_settings.buttonPaddingY);

    std::vector<MenuBarButton> newButtons;
    for (int i = 0; i <= 127; ++i) {
        WCHAR key[256];

        swprintf_s(key, L"ButtonsSettings.buttons[%d].label", i);
        std::wstring label = GetStringSetting(key);

        swprintf_s(key, L"ButtonsSettings.buttons[%d].icon", i);
        std::wstring icon = GetStringSetting(key);

        swprintf_s(key, L"ButtonsSettings.buttons[%d].action", i);
        std::wstring action = GetStringSetting(key);

        swprintf_s(key, L"ButtonsSettings.buttons[%d].displayMode", i);
        std::wstring displayMode = GetStringSetting(key, L"default");

        bool allEmpty = label.empty() && icon.empty() && action.empty();
        if (allEmpty) {
            if (i >= (int)newButtons.size() + 2) break;
            continue;
        }

        MenuBarButton item;
        item.label = label.empty() ? L"Button" : label;
        item.iconRaw = icon;
        item.action = action;
        item.displayMode = displayMode.empty() ? L"default" : displayMode;
        newButtons.push_back(std::move(item));
    }

    {
        std::lock_guard<std::mutex> lk(g_buttonsMutex);
        g_buttons = std::move(newButtons);
    }
}

static void WINAPI TrayUI_StartTaskbar_Hook(void* pThis) {
    TrayUI_StartTaskbar_Original(pThis);
    if (g_unloading) return;

    g_taskbarWnd = FindCurrentProcessTaskbarWnd();
    if (!g_taskbarWnd) return;

    g_menuBarRoot = nullptr;
    g_injectionParent = nullptr;
    g_menuBarAnchor = nullptr;
    g_trackedElement = nullptr;
    g_hasTrackedElementOriginalMargin = false;
    g_trackPosition.clear();
    g_insertedColumn = -1;

    ApplySettingsWithRetry(g_taskbarWnd);
}

void WINAPI EntryPoint_Hook() {
    Wh_Log(L">");
    ExitThread(0);
}

BOOL HookTaskbarDllSymbols() {
    static const wchar_t* const kCandidates[] = { L"taskbar.dll" };
    HMODULE h = nullptr;
    for (auto* name : kCandidates) {
        h = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (h) break;
    }
    if (!h) return FALSE;

    WindhawkUtils::SYMBOL_HOOK hooks[] = {
        {{LR"(const CTaskBand::`vftable'{for `ITaskListWndSite'})"}, &CTaskBand_ITaskListWndSite_vftable},
        {{LR"(public: virtual class std::shared_ptr<class TaskbarHost> __cdecl CTaskBand::GetTaskbarHost(void)const )"}, &CTaskBand_GetTaskbarHost_Original},
        {{LR"(public: int __cdecl TaskbarHost::FrameHeight(void)const )"}, &TaskbarHost_FrameHeight_Original},
        {{LR"(public: void __cdecl std::_Ref_count_base::_Decref(void))"}, &Std_Ref_Decref_Original},
        {{LR"(public: virtual void __cdecl TrayUI::StartTaskbar(void))"}, &TrayUI_StartTaskbar_Original, TrayUI_StartTaskbar_Hook},
    };

    if (!WindhawkUtils::HookSymbols(h, hooks, ARRAYSIZE(hooks))) {
        return FALSE;
    }

    return TRUE;
}

BOOL Wh_ModInit() {
    g_unloading = false;
    g_applyingSettings = false;
    g_retryScheduled = false;
    g_taskbarWnd = nullptr;
    g_menuBarRoot = nullptr;
    g_injectionParent = nullptr;
    g_menuBarAnchor = nullptr;
    g_menuBarHost = nullptr;
    g_trackedElement = nullptr;
    g_hasTrackedElementOriginalMargin = false;
    g_trackPosition.clear();
    g_insertedColumn = -1;
    g_placementKind = 0;
    g_layoutUpdatedSource = nullptr;
    g_layoutUpdatedAttached = false;

    LoadSettings();
    return HookTaskbarDllSymbols();
}

void Wh_ModAfterInit() {
    g_taskbarWnd = FindCurrentProcessTaskbarWnd();
    if (!g_taskbarWnd) return;

    ApplySettingsWithRetry(g_taskbarWnd);
}

void Wh_ModSettingsChanged() {
    if (g_unloading) return;
    LoadSettings();

    HWND hWnd = g_taskbarWnd ? g_taskbarWnd : FindCurrentProcessTaskbarWnd();
    if (!hWnd) return;
    g_taskbarWnd = hWnd;

    ApplySettingsWithRetry(hWnd);
}

void Wh_ModUninit() {
    g_unloading = true;
    g_retryScheduled = false;
    if (g_taskbarWnd) {
        RunFromWindowThread(g_taskbarWnd, [](void*) {
            RemoveMenuBar();
        }, nullptr);
    }
}
