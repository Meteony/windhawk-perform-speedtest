# Windhawk — Perform Speed Test Redirect

A [Windhawk](https://windhawk.net/) mod that redirects the **"Perform speed test"** option in the Windows taskbar network right-click menu away from Microsoft's default page to a URL or command of your choice (defaults to [speedtest.net](https://www.speedtest.net)).

## The problem

Right-clicking the network icon in the Windows 11 taskbar and clicking **"Perform speed test"** opens:

```
https://go.microsoft.com/fwlink/?linkid=2324916
```

This mod intercepts that and sends you somewhere better. It can also catch the Action Center speed-test link:

```
https://go.microsoft.com/fwlink/?linkid=2325015
```

## Installation

1. Install [Windhawk](https://windhawk.net/)
2. In Windhawk, go to **Find mods** and search for `perform-speedtest-redirect`, **or** manually install:
   - Click **Create new mod**
   - Paste the contents of [`perform-speedtest-redirect.wh.cpp`](./perform-speedtest-redirect.wh.cpp) into the editor
   - Click **Compile & Run**
3. Right-click the taskbar network icon -> **Perform speed test**. It now opens speedtest.net.

## Settings

In the Windhawk mod settings panel you can configure how matched speed-test launches are handled.

Upgrading from v1.2.x: the old **Redirect URL** setting was renamed to **Action text**. Re-enter your custom URL in **Action text** after upgrading.

### Command execution mode

- **Disabled**: URL replacement mode. This replaces Microsoft's URL inside the original browser launch command.
- **Enabled**: command execution mode. This runs **Action text** instead of the original launch. Bare `http://` and `https://` URLs are opened with the default browser.

### Action text

In URL replacement mode, set **Action text** to any speed test site:

| Site | URL |
|------|-----|
| Speedtest.net (default) | `https://www.speedtest.net/` |
| Fast.com (Netflix) | `https://fast.com` |
| Cloudflare | `https://speed.cloudflare.com` |

In command execution mode, set **Action text** to a URL or full command line, for example:

```txt
https://www.speedtest.net/
```

```txt
explorer.exe shell:AppsFolder\Ookla.SpeedtestbyOokla_43tkc6nmykmb6!App
```

### Target substrings

The default target substrings are `linkid=2324916` and `linkid=2325015`. Add more entries if Microsoft changes the speed-test URLs.

## How it works

The mod injects into `explorer.exe`, `ShellExperienceHost.exe`, `ShellHost.exe`, and `RuntimeBroker.exe`, then hooks `CreateProcessW` to catch the browser being spawned directly.

Any matching process launch that contains a configured target substring is redirected before the browser is launched.

## Building / contributing

The mod is a single self-contained `.wh.cpp` file compiled by Windhawk's built-in Clang toolchain — no external build system needed.

## License

MIT
