// ==WindhawkMod==
// @id              perform-speedtest-redirect
// @name            Perform Speed Test Redirect
// @description     Redirects the "Perform speed test" link in the taskbar network right-click menu from the default Microsoft page to a custom URL (defaults to speedtest.net).
// @version         1.3.0
// @author          mynameistito
// @github          https://github.com/mynameistito
// @license         MIT
// @include         explorer.exe
// @include         ShellExperienceHost.exe
// @include         ShellHost.exe
// @include         RuntimeBroker.exe
// @architecture    x86-64
// @compilerOptions -lshell32 -lkernel32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Perform Speed Test Redirect

![PSR Image](https://raw.githubusercontent.com/Meteony/meteoni-assets/main/perform-speedtest-redirect/psr.png)

When you right-click the network icon in the Windows taskbar and click
**"Perform speed test"**, Windows normally launches the browser to:

```
https://go.microsoft.com/fwlink/?linkid=2324916
```

This mod can either: 
1. Replace that URL, or 
2. Let you execute a command of your choice instead.

By default, it redirects the URL to **https://www.speedtest.net**.

## Settings

### Command execution mode

Controls what the mod does after detecting the speed-test launch.

- **Disabled**: URL replacement mode.
- **Enabled**: command execution mode.

### Action text

- **URL Replacement Mode** – the URL to open instead of the default Microsoft speed
  test link. Change this to any speed test site you prefer, e.g.
  `https://fast.com` or `https://cloudflare.com/speed`.

- **Command execution mode** - the full command line to run. Example:
    
    ```explorer.exe shell:AppsFolder\Ookla.SpeedtestbyOokla_43tkc6nmykmb6!App```

### Target substrings

A list of substrings to detect inside the launched command line. Default entries: 

`linkid=2324916`

`linkid=2325015`

Feel free to add more entries here if Microsoft ever changes them. 

## How it works

The mod hooks `CreateProcessW` in shell-related processes such as:

- `explorer.exe`
- `ShellExperienceHost.exe`
- `ShellHost.exe`
- `RuntimeBroker.exe`

When a process launch attempt contains one of the configured target substrings, the mod partially / fully redirects that launch.

The original launch is left untouched when none of the configured substrings match.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- commandMode: false
  $name: Command execution mode
  $description: If disabled, replace the URL. If enabled, execute Action text as a command line.

- actionText: "https://www.speedtest.net/"
  $name: Action text
  $description: In URL mode, this is the redirected URL. In command mode, this is the full command to execute.

- targetSubstrings:
  - linkid=2324916
  - linkid=2325015
  $name: Target substrings
  $description: Extra substrings to detect in the Microsoft URL. E.g. "...microsoft.com/fwlink/?linkid=2325015".  Add URL fragments here if they are changed.
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

#define LOG(fmt, ...) Wh_Log(L"[speedtest] " fmt, ##__VA_ARGS__)

static bool g_commandMode = false;
static std::wstring g_actionText;
static std::vector<std::wstring> g_targetSubstrings;

// ── helpers ──────────────────────────────────────────────────────────────────

static bool HasLinkId(LPCWSTR s) {
    if (!s) return false;

    for (const auto & sstr : g_targetSubstrings){
        if (wcsstr(s, sstr.c_str())) return true;
    }
    return false;
}

// Replace the full URL that contains LINK_ID with redirectUrl.
// Returns the modified string, or empty if LINK_ID not found.
static std::wstring ReplaceUrl(LPCWSTR src) {
    Wh_Log(L"ReplaceUrl: %ls", src);
    if (!src) return {};

    std::wstring s(src);

    const wchar_t* hit = nullptr;
    for (const auto & link : g_targetSubstrings){
        hit = wcsstr(src, link.c_str());
        if (hit) break;
    }
    if (!hit) return {};

    size_t pos      = hit - src;
    size_t urlStart = s.rfind(L"http", pos);
    if (urlStart == std::wstring::npos) urlStart = pos;
    size_t urlEnd   = s.find_first_of(L" \t\r\n\"'", pos);
    if (urlEnd   == std::wstring::npos) urlEnd   = s.size();

    PCWSTR url = Wh_GetStringSetting(L"redirectUrl");
    s.replace(urlStart, urlEnd - urlStart, url);
    Wh_FreeStringSetting(url);
    return s;
}


static PCWSTR SafeStr(PCWSTR s) {
    return s ? s : L"(null)";
}


// ── CreateProcessW hook ──────────────────────────────────────────────────────
// Last-resort: catches the browser being spawned with the URL in its
// command line, regardless of which higher-level API triggered it.

using CreateProcessW_t = decltype(&CreateProcessW);
CreateProcessW_t CreateProcessW_Original;

BOOL WINAPI CreateProcessW_Hook(
    LPCWSTR app,
    LPWSTR cmd,
    LPSECURITY_ATTRIBUTES psa,
    LPSECURITY_ATTRIBUTES tsa,
    BOOL inherit,
    DWORD flags,
    LPVOID env,
    LPCWSTR dir,
    LPSTARTUPINFOW si,
    LPPROCESS_INFORMATION pi
) {
    LOG(L"CreateProcessW(app=\"%ls\", cmd=\"%ls\", flags=0x%08lX, dir=\"%ls\")",
        SafeStr(app),
        SafeStr(cmd),
        flags,
        SafeStr(dir));

    if (!HasLinkId(cmd)) {
        return CreateProcessW_Original(
            app, cmd, psa, tsa, inherit, flags, env, dir, si, pi
        );
    }


    BOOL ret = FALSE;

    if (g_commandMode) {
        if (g_actionText.empty()) {
            LOG(L"Command mode enabled, but actionText is empty");
            SetLastError(ERROR_INVALID_PARAMETER);
            ret = FALSE;
        } else {
            std::wstring newCmd = g_actionText;

            LOG(L"Command mode: executing \"%ls\"", newCmd.c_str());

            ret = CreateProcessW_Original(
                nullptr,
                newCmd.data(),
                psa,
                tsa,
                inherit,
                flags,
                env,
                dir,
                si,
                pi
            );

            LOG(L"Command mode result: ret=%d err=%lu",
                ret,
                ret ? 0 : GetLastError());
        }
    } else {
        std::wstring newCmd = ReplaceUrl(cmd);

        if (newCmd.empty()) {
            LOG(L"URL mode: ReplaceUrl returned empty, falling back to original command");

            ret = CreateProcessW_Original(
                app, cmd, psa, tsa, inherit, flags, env, dir, si, pi
            );
        } else {
            LOG(L"URL mode: replacing command with \"%ls\"", newCmd.c_str());

            ret = CreateProcessW_Original(
                app,
                newCmd.data(),
                psa,
                tsa,
                inherit,
                flags,
                env,
                dir,
                si,
                pi
            );

            LOG(L"URL mode result: ret=%d err=%lu",
                ret,
                ret ? 0 : GetLastError());
        }
    }

    return ret;
}

// ── Init ─────────────────────────────────────────────────────────────────────

static bool HookFn(void* target, void* hook, void** orig, const wchar_t* name){
    if (!target){
        LOG(L"Could not resolve %ls", name);
        return false;
    }

    if (!Wh_SetFunctionHook(target, hook, orig)){
        LOG(L"Hook failed for %ls", name);
        return false;
    }

    LOG(L"Hooked %ls", name);
    return true;
}

static void LoadSettings() {
    g_commandMode = Wh_GetIntSetting(L"commandMode") != 0;

    PCWSTR actionText = Wh_GetStringSetting(L"actionText");
    g_actionText = actionText ? actionText : L"";
    Wh_FreeStringSetting(actionText);

    g_targetSubstrings.clear();

    for (int i = 0;; i++) {
        PCWSTR substring = Wh_GetStringSetting(L"targetSubstrings[%d]", i);

        if (!substring || !*substring) {
            Wh_FreeStringSetting(substring);
            break;
        }

        g_targetSubstrings.emplace_back(substring);
        LOG(L"Loaded targetSubstrings[%d]=\"%ls\"", i, substring);

        Wh_FreeStringSetting(substring);
    }

    if (g_targetSubstrings.empty()) {
        g_targetSubstrings.emplace_back(L"linkid=2324916");
        g_targetSubstrings.emplace_back(L"linkid=2325015");
    }

    LOG(L"Settings loaded: commandMode=%d, actionText=\"%ls\", substringCount=%zu",
        g_commandMode,
        g_actionText.c_str(),
        g_targetSubstrings.size());
}

BOOL Wh_ModInit() {
    LOG(L"Init in %ls", GetCommandLineW());

    LoadSettings();

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");

    bool ok = true;

    ok &= HookFn(
        hKernel32 ? (void*)GetProcAddress(hKernel32, "CreateProcessW") : nullptr,
        (void*)CreateProcessW_Hook,
        (void**)&CreateProcessW_Original,
        L"CreateProcessW"
    );

    if (!ok) {
        LOG(L"One or more hooks failed, aborting");
        return FALSE;
    }

    LOG(L"All hooks installed");
    return TRUE;
}

void Wh_ModSettingsChanged() {
    LOG(L"Settings changed");
    LoadSettings();
}
