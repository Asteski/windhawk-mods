// ==WindhawkMod==
// @id              explorer-force-details-columns-fork
// @name            Explorer Details View Columns - Fork
// @description     Forces a fixed set of columns in File Explorer Details view. Columns and their order are configurable, globally and per-folder. Has no effect on other view modes.
// @version         1.3
// @author          ernisn
// @github          https://github.com/ernisn
// @include         explorer.exe
// @compilerOptions -lole32 -lshlwapi -lpropsys -lgdi32 -lshell32
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Explorer Details View Columns

Forces a specific set of columns whenever a File Explorer folder is in _Details_ view. **Other view modes (_Icons_, _Tiles_, _List_, etc.) are not affected.**

Configure which columns to show and in what order in Settings. Each entry takes a Shell property name and a width **(if _Force Width_ enabled)** in logical pixels at 100% DPI scaling (for consistent widths in case of multiple monitors with different DPI settings).

Common property names:
- `System.ItemNameDisplay` - Name
- `System.Size` - Size
- `System.DateModified` - Date modified
- `System.DateCreated` - Date created
- `System.ItemTypeText` - Type
- `System.FileAttributes` - Attributes
- `System.Author` - Authors
- `System.Music.Artist` - Contributing artists
- `System.Media.Duration` - Length
- `System.Image.Dimensions` - Dimensions

For a full list of available Shell property names, see: https://learn.microsoft.com/en-us/windows/win32/properties/props

## Folder-specific rules

In addition to the global columns, you can define **per-folder rules** that override the columns in specific folders. Each rule has:
- A **folder path** (e.g. `C:\Users\me\Music`). Environment variables such as `%USERPROFILE%\Downloads` are expanded, so rules stay portable across users. Matching is case-insensitive.
- An **Include subfolders** toggle. When enabled, the rule also applies to every folder beneath the path.
- Its own **columns** list, in the same format as the global one.

Matching behaviour:
- A folder that matches no rule uses the **global Columns** list.
- When several rules match the same folder (e.g. a parent rule with subfolders plus a rule directly on the folder), the rule with the **most-specific (longest) path** wins.
- Virtual/special locations without a real filesystem path (e.g. _This PC_) always use the global columns.

**Note:**
- Any property name not recognised by Windows will be skipped.
- Duplicate property entries will only take the first occurrence.
- To allow a column to be freely resized, turn off "Force Width".
- Changes should take effect immediately to opened folders, but if the opened window is on **another monitor with a differnt DPI settings**, the width will be updated **after re-opening the folder once**.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- columns:
  - - property: System.ItemNameDisplay
      $name: Column Property Name
    - force_width: true
      $name: Force Width
      $description: If enabled, the column width will always use the value below, regardless of manual changes, which will stay temporary and be reverted after re-opening the folder.
    - width: 270
      $name: Width
      $description: Only takes effect if Force Width is turned on.
  - - property: System.Size
    - force_width: false
    - width: 30
  - - property: System.DateModified
    - force_width: true
    - width: 110
  - - property: System.ItemTypeText
    - force_width: false
    - width: 60
  $name: Columns
  $description: "Columns to show in order in Details view. Width is in logical pixels at 100% DPI scaling."
- folderRules:
  - - path: ""
      $name: Folder path
      $description: "Absolute path, e.g. C:\\Users\\me\\Music or %USERPROFILE%\\Downloads (environment variables are expanded). Leave empty to disable this rule."
    - include_subfolders: false
      $name: Include subfolders
      $description: "If enabled, this rule also applies to every folder beneath the path."
    - columns:
      - - property: System.ItemNameDisplay
          $name: Column Property Name
        - force_width: true
          $name: Force Width
        - width: 270
          $name: Width
      $name: Columns for this folder
      $description: "Same format as the global Columns list above."
  $name: Folder-specific rules
  $description: "Override the global columns in specific folders. The most-specific (longest) matching path wins; folders matching no rule use the global Columns list."
*/
// ==/WindhawkModSettings==

#include <initguid.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propsys.h>
#include <shellapi.h>
#include <exdisp.h>
#include <shlguid.h>
#include <servprov.h>
#include <shlobj.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <windhawk_utils.h>

// IID_IServiceProvider from Windows SDK {6D5140C1-7436-11CE-8034-00AA006009FA}
DEFINE_GUID(IID_IServiceProvider_,
    0x6D5140C1, 0x7436, 0x11CE,
    0x80, 0x34, 0x00, 0xAA, 0x00, 0x60, 0x09, 0xFA);

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

struct ColumnEntry {
    PROPERTYKEY key;
    int         width; // logical pixels at 100% DPI (96 dpi); 0 = free resize
};

struct FolderRule {
    std::wstring             path;               // expanded, lowercased, no trailing '\'
    bool                     include_subfolders; // also match folders beneath path
    std::vector<ColumnEntry> columns;
};

static std::vector<ColumnEntry> g_columns;     // global default
static std::vector<FolderRule>  g_folderRules; // per-folder overrides

// Width comparison tolerance (in physical pixels) to avoid width update loops
static constexpr UINT kWidthTolerancePx = 1;

static bool PropertyKeysEqual(const PROPERTYKEY& a, const PROPERTYKEY& b) {
    return a.fmtid == b.fmtid && a.pid == b.pid;
}

// Normalize a folder path for matching: expand environment variables, lowercase,
// and strip any trailing backslash.
static std::wstring NormalizePath(PCWSTR raw) {
    if (!raw || raw[0] == L'\0') return std::wstring();

    std::wstring expanded;
    DWORD needed = ExpandEnvironmentStringsW(raw, nullptr, 0);
    if (needed > 0) {
        expanded.resize(needed);
        DWORD written = ExpandEnvironmentStringsW(raw, expanded.data(), needed);
        // 'written' includes the terminating null; drop it.
        if (written > 0) {
            expanded.resize(written - 1);
        } else {
            expanded = raw;
        }
    } else {
        expanded = raw;
    }

    CharLowerW(expanded.data());

    while (!expanded.empty() && expanded.back() == L'\\') {
        expanded.pop_back();
    }

    return expanded;
}

// Load a column list. ruleIndex < 0 reads the global "columns[%d]..." settings;
// otherwise it reads the nested "folderRules[ruleIndex].columns[%d]..." settings.
static void LoadColumnList(int ruleIndex, std::vector<ColumnEntry>& out) {
    out.clear();

    int duplicatesSkipped = 0;
    int unrecognisedSkipped = 0;

    for (int i = 0; ; i++) {
        PCWSTR rawProp = (ruleIndex < 0)
            ? Wh_GetStringSetting(L"columns[%d].property", i)
            : Wh_GetStringSetting(L"folderRules[%d].columns[%d].property", ruleIndex, i);
        if (!rawProp || rawProp[0] == L'\0') {
            Wh_FreeStringSetting(rawProp);
            break;
        }

        PROPERTYKEY key;
        if (SUCCEEDED(PSGetPropertyKeyFromName(rawProp, &key))) {
            // Skip if this key is already present
            bool isDuplicate = false;
            for (const auto& existing : out) {
                if (PropertyKeysEqual(existing.key, key)) {
                    isDuplicate = true;
                    break;
                }
            }

            if (isDuplicate) {
                Wh_Log(L"Column[%d]: %s -> duplicate, skipped", i, rawProp);
                duplicatesSkipped++;
            } else {
                BOOL forceWidth = (ruleIndex < 0)
                    ? Wh_GetIntSetting(L"columns[%d].force_width", i)
                    : Wh_GetIntSetting(L"folderRules[%d].columns[%d].force_width", ruleIndex, i);
                int width = (ruleIndex < 0)
                    ? Wh_GetIntSetting(L"columns[%d].width", i)
                    : Wh_GetIntSetting(L"folderRules[%d].columns[%d].width", ruleIndex, i);

                if (!forceWidth) {
                    width = 0;
                }

                out.push_back({ key, width });
                Wh_Log(L"Column[%d]: %s force=%d width=%d -> OK",
                       i, rawProp, forceWidth, width);
            }
        } else {
            Wh_Log(L"Column[%d]: %s -> not recognised, skipped", i, rawProp);
            unrecognisedSkipped++;
        }

        Wh_FreeStringSetting(rawProp);
    }

    Wh_Log(L"Loaded %zu columns (skipped %d duplicates, %d unrecognised)",
           out.size(), duplicatesSkipped, unrecognisedSkipped);
}

static void LoadSettings() {
    Wh_Log(L"Loading global columns");
    LoadColumnList(-1, g_columns);

    // Iterate a generous fixed bound rather than breaking on the first empty
    // entry, so a disabled rule (empty path) left between configured rules - or
    // the empty-path template Windhawk seeds new entries from - does not
    // truncate the list. Out-of-range indices return an empty string.
    g_folderRules.clear();
    for (int i = 0; i < 128; i++) {
        PCWSTR rawPath = Wh_GetStringSetting(L"folderRules[%d].path", i);
        std::wstring path = NormalizePath(rawPath);
        Wh_FreeStringSetting(rawPath);

        // Empty/disabled rule: skip but keep scanning later indices.
        if (path.empty()) {
            continue;
        }

        FolderRule rule;
        rule.path = path;
        rule.include_subfolders = Wh_GetIntSetting(L"folderRules[%d].include_subfolders", i);
        Wh_Log(L"FolderRule[%d]: %s (subfolders=%d)", i, rule.path.c_str(),
               rule.include_subfolders);
        LoadColumnList(i, rule.columns);

        if (rule.columns.empty()) {
            Wh_Log(L"FolderRule[%d]: no valid columns -> skipped", i);
            continue;
        }

        g_folderRules.push_back(std::move(rule));
    }

    Wh_Log(L"Loaded %zu global columns, %zu folder rules",
           g_columns.size(), g_folderRules.size());
}

// Pick the column list to apply for a folder: the most-specific (longest path)
// matching rule, or the global list if none match.
static const std::vector<ColumnEntry>& SelectColumns(const std::wstring& folderPath) {
    const std::vector<ColumnEntry>* best = &g_columns;
    size_t bestLen = 0;
    bool matched = false;

    if (!folderPath.empty()) {
        for (const auto& r : g_folderRules) {
            bool hit = (folderPath == r.path) ||
                       (r.include_subfolders &&
                        folderPath.size() > r.path.size() &&
                        folderPath.compare(0, r.path.size(), r.path) == 0 &&
                        folderPath[r.path.size()] == L'\\');
            if (hit && (!matched || r.path.size() > bestLen)) {
                best = &r.columns;
                bestLen = r.path.size();
                matched = true;
            }
        }
    }

    return *best;
}

// ---------------------------------------------------------------------------
// DPI helpers
// ---------------------------------------------------------------------------

static UINT GetSystemDpi() {
    using GetDpiForSystem_t = UINT(WINAPI*)();
    static auto pGetDpiForSystem = reinterpret_cast<GetDpiForSystem_t>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForSystem"));
    if (pGetDpiForSystem)
        return pGetDpiForSystem();

    HDC hdc = GetDC(nullptr);
    UINT dpi = hdc ? static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX)) : 96;
    if (hdc) ReleaseDC(nullptr, hdc);
    return dpi;
}

static UINT GetWindowDpi(HWND hwnd) {
    using GetDpiForWindow_t = UINT(WINAPI*)(HWND);
    static auto pGetDpiForWindow = reinterpret_cast<GetDpiForWindow_t>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (pGetDpiForWindow && hwnd)
        return pGetDpiForWindow(hwnd);

    return GetSystemDpi();
}

static UINT ScaleToDpi(int logicalPx, UINT dpi) {
    if (logicalPx <= 0) return 0;
    return static_cast<UINT>(std::round(logicalPx * (static_cast<double>(dpi) / 96.0)));
}

// ---------------------------------------------------------------------------
// Folder path retrieval
// ---------------------------------------------------------------------------

// Retrieve the normalized filesystem path of the folder shown by a view.
// Returns false (and leaves 'out' empty) for virtual/special locations that
// have no filesystem path, e.g. "This PC".
static bool GetFolderPath(IFolderView2* pFV2, std::wstring& out) {
    out.clear();

    IPersistFolder2* pPF2 = nullptr;
    if (FAILED(pFV2->GetFolder(IID_IPersistFolder2,
                               reinterpret_cast<void**>(&pPF2))) || !pPF2)
        return false;

    LPITEMIDLIST pidl = nullptr;
    HRESULT hr = pPF2->GetCurFolder(&pidl);
    pPF2->Release();
    if (FAILED(hr) || !pidl)
        return false;

    wchar_t buf[MAX_PATH];
    bool ok = SHGetPathFromIDListW(pidl, buf);
    CoTaskMemFree(pidl);
    if (!ok)
        return false;

    out = NormalizePath(buf);
    return !out.empty();
}

// ---------------------------------------------------------------------------
// Column enforcement
// ---------------------------------------------------------------------------

static void ApplyForcedColumns(void* pThis) {
    // Check empty configuration
    if (g_columns.empty() && g_folderRules.empty()) {
        Wh_Log(L"ApplyForcedColumns: empty configuration");
        LoadSettings();
        if (g_columns.empty() && g_folderRules.empty()) {
            // No enforcement including defaults
            return;
        }
    }

    auto* pShellView = reinterpret_cast<IShellView*>(pThis);

    HWND hwnd = nullptr;
    HRESULT hrWnd = pShellView->GetWindow(&hwnd);
    if (FAILED(hrWnd)) {
        Wh_Log(L"ApplyForcedColumns: GetWindow failed hr=0x%08X", hrWnd);
        // GetWindowDpi falls back to system DPI
    }
    UINT windowDpi = GetWindowDpi(hwnd);

    IFolderView2* pFV2 = nullptr;
    if (FAILED(pShellView->QueryInterface(IID_IFolderView2,
                                          reinterpret_cast<void**>(&pFV2))) || !pFV2)
        return;

    FOLDERVIEWMODE viewMode = FVM_AUTO;
    int iconSize = 0;
    HRESULT hr = pFV2->GetViewModeAndIconSize(&viewMode, &iconSize);

    std::wstring folderPath;
    GetFolderPath(pFV2, folderPath);

    pFV2->Release();

    if (FAILED(hr) || viewMode != FVM_DETAILS)
        return;

    // Pick the column set for this folder (folder-specific rule or global default).
    const std::vector<ColumnEntry>& cols = SelectColumns(folderPath);
    if (cols.empty()) {
        Wh_Log(L"ApplyForcedColumns: no columns for '%s'", folderPath.c_str());
        return;
    }

    IColumnManager* pCM = nullptr;
    if (FAILED(pShellView->QueryInterface(IID_IColumnManager,
                                          reinterpret_cast<void**>(&pCM))) || !pCM)
        return;

    // Check if the column settings need update
    UINT colCount = 0;
    bool orderNeedsUpdate = false;

    if (FAILED(pCM->GetColumnCount(CM_ENUM_VISIBLE, &colCount)) || colCount != cols.size()) {
        orderNeedsUpdate = true;
    } else {
        std::vector<PROPERTYKEY> currentKeys(colCount);
        if (SUCCEEDED(pCM->GetColumns(CM_ENUM_VISIBLE, currentKeys.data(), colCount))) {
            for (size_t i = 0; i < colCount; i++) {
                if (!PropertyKeysEqual(currentKeys[i], cols[i].key)) {
                    orderNeedsUpdate = true;
                    break;
                }
            }
        } else {
            orderNeedsUpdate = true;
        }
    }

    if (orderNeedsUpdate) {
        std::vector<PROPERTYKEY> keys;
        keys.reserve(cols.size());
        for (const auto& col : cols) {
            keys.push_back(col.key);
        }
        pCM->SetColumns(keys.data(), static_cast<UINT>(keys.size()));
    }

    // Column width enforcement for those using a width value
    for (const auto& col : cols) {
        if (col.width <= 0)
            continue;

        CM_COLUMNINFO ci = {};
        ci.cbSize = sizeof(ci);
        ci.dwMask = CM_MASK_WIDTH;
        if (SUCCEEDED(pCM->GetColumnInfo(col.key, &ci))) {
            UINT expectedWidth = ScaleToDpi(col.width, windowDpi);
            // Avoids re-applying on px differences caused by rounding
            int diff = static_cast<int>(ci.uWidth) - static_cast<int>(expectedWidth);
            if (std::abs(diff) > static_cast<int>(kWidthTolerancePx)) {
                ci.uWidth = expectedWidth;
                pCM->SetColumnInfo(col.key, &ci);
            }
        }
    }

    pCM->Release();
}

// ---------------------------------------------------------------------------
// Apply to all currently open Explorer windows
// ---------------------------------------------------------------------------

static void ApplyToAllOpenWindows() {
    IShellWindows* psw = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_LOCAL_SERVER,
                                IID_IShellWindows, reinterpret_cast<void**>(&psw))) || !psw)
        return;

    long count = 0;
    if (FAILED(psw->get_Count(&count))) {
        psw->Release();
        return;
    }

    DWORD currentProcessId = GetCurrentProcessId();

    for (long i = 0; i < count; i++) {
        VARIANT vi;
        vi.vt = VT_I4;
        vi.lVal = i;

        IDispatch* pDisp = nullptr;
        if (FAILED(psw->Item(vi, &pDisp)) || !pDisp)
            continue;

        IWebBrowserApp* pWBA = nullptr;
        if (SUCCEEDED(pDisp->QueryInterface(IID_IWebBrowserApp,
                                            reinterpret_cast<void**>(&pWBA))) && pWBA) {

            HWND wnd = nullptr;
            pWBA->get_HWND(reinterpret_cast<SHANDLE_PTR*>(&wnd));

            DWORD windowProcessId = 0;
            if (wnd) {
                GetWindowThreadProcessId(wnd, &windowProcessId);
            }

            if (windowProcessId == currentProcessId) {
                IServiceProvider* pSP = nullptr;
                if (SUCCEEDED(pWBA->QueryInterface(IID_IServiceProvider_,
                                                   reinterpret_cast<void**>(&pSP))) && pSP) {
                    IShellBrowser* pSB = nullptr;
                    if (SUCCEEDED(pSP->QueryService(SID_STopLevelBrowser, IID_IShellBrowser,
                                                    reinterpret_cast<void**>(&pSB))) && pSB) {
                        IShellView* pSV = nullptr;
                        if (SUCCEEDED(pSB->QueryActiveShellView(&pSV)) && pSV) {
                            ApplyForcedColumns(pSV);
                            // Refresh directly here
                            pSV->Refresh();
                            pSV->Release();
                        }
                        pSB->Release();
                    }
                    pSP->Release();
                }
            }
            pWBA->Release();
        }
        pDisp->Release();
    }
    psw->Release();
}

// ---------------------------------------------------------------------------
// Hook: CDefView::UIActivate  (shell32.dll)
// ---------------------------------------------------------------------------

using CDefView_UIActivate_t = HRESULT(__thiscall*)(void* pThis, UINT uState);

CDefView_UIActivate_t CDefView_UIActivate_orig = nullptr;

HRESULT __thiscall CDefView_UIActivate_hook(void* pThis, UINT uState) {
    HRESULT hr = CDefView_UIActivate_orig(pThis, uState);

    if (SUCCEEDED(hr) &&
        (uState == SVUIA_ACTIVATE_FOCUS || uState == SVUIA_ACTIVATE_NOFOCUS)) {
        ApplyForcedColumns(pThis);
    }
    return hr;
}

// ---------------------------------------------------------------------------
// Windhawk entry points
// ---------------------------------------------------------------------------

BOOL Wh_ModInit() {
    Wh_Log(L"Init");

    LoadSettings();

    HMODULE hShell32 = LoadLibraryW(L"shell32.dll");
    if (!hShell32) {
        Wh_Log(L"Failed to load shell32.dll");
        return FALSE;
    }

    const WindhawkUtils::SYMBOL_HOOK shell32DllHooks[] = {
        {
            {
                L"public: virtual long __cdecl CDefView::UIActivate(unsigned int)",
                L"long __cdecl CDefView::UIActivate(unsigned int)",
                L"public: virtual long __thiscall CDefView::UIActivate(unsigned int)",
                L"long __thiscall CDefView::UIActivate(unsigned int)",
            },
            &CDefView_UIActivate_orig,
            CDefView_UIActivate_hook,
            false
        },
    };

    if (!WindhawkUtils::HookSymbols(hShell32, shell32DllHooks, ARRAYSIZE(shell32DllHooks))) {
        Wh_Log(L"ERROR: Could not resolve CDefView::UIActivate");
        return FALSE;
    }

    Wh_Log(L"CDefView::UIActivate hooked successfully");
    return TRUE;
}

void Wh_ModAfterInit() {
    Wh_Log(L"Applying to existing windows");
    ApplyToAllOpenWindows();
}

void Wh_ModUninit() {
    Wh_Log(L"Uninit");
}

void Wh_ModSettingsChanged() {
    Wh_Log(L"SettingsChanged");
    LoadSettings();
    ApplyToAllOpenWindows();
}
