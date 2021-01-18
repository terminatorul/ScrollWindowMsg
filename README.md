# ScrollWindowMsg
Tool to implement a few UI operations on the window under the mouse cursor (instead of the currently active window with the keyboard focus):
 - Sends window messages for horizontal scrolling to the window under the mouse cursor.
 - Close / minimize / maximize / restore the window under cursor.
 - Send a number of key presses (Enter/Esc/Tab/Space/Delete/Backspace) to the window under cursor, but will bring the window to the foreground first.

This are simple Windows commands for use with mice that can map external commands (executables) to mouse buttons, in my case for:
 - Logitech G600 Gaming Mouse
 - Razer Naga Trinity

Most gaming mice (and non-gaming) can remap buttons to various functions. This command is intended for mice with suitable buttons for horizontal scrolling, usually either:
 - with a second scroll wheel
 - with wheel tilt on the main scroll wheel
 - with extra buttons for this purpose
 
 This tool is somewhat inspired by the Linux `xdo` and `xdotool` commands.

## Purpose
The problem is default scroll behavior provided by the mouse software suite works with most, but not all, applications in case of horizontal scrolling. Some Windows applications do not respond WM_MOUSEHWHEEL message correctly, but they can handle WM_HSCROLL messages. In my case I needed horizontal scroll to work with gVim, and I also noticed WordPad would not scroll horizontally with my mice.

## Building
The command is currently built with Visual Studio 2019 (Community Edition).

In order to send input and to drive other windows and applications, the commands in the project have to be signed using Authenticode signatures according to Microsoft instructions and examples.

You can generate the needed certificates locally, and after you have done that you can sign all executables with the included `DeploySigned.ps1` script, with a command like:
```batch
  MkDir "%SystemRoot%\System32\WinAPI-UI-Commands"
  MkDir "%SystemRoot%\SysWOW64\WinAPI-UI-Commands"
  CD     x64\Release\
  PowerShell -ExecutionPolicy Bypass -File "DeploySigned.ps1" "%CD%" "%SystemRoot%\System32" "%SystemRoot%\SysWOW64"
```

After building the project, use the mouse software to launch `C:\Windows\System32\WinAPI-UI-Commands\ScrollLeft.exe` / `ScrollRight.exe` when horizontal scroll buttons are pressed. You should configure the mouse software to repeat this action for as long as the button is held down.

The executables need UIAccess privilege level and always run with elevated privilege. No elevation prompt (UAC prompt) is triggered, but programs using Windows `CreateProcess()` API function, instead of `ShellExecute()`, will not be able to invoke the commands. In my case the Logitech G HUB software would not be able to run the commands directly, so instead of the direct invocation you should configure G HUB to runn commands like:
```batch
        C:\Windows\System32\WinAPI-UI-Commands\UI-Cmd.exe SendDeleteKey.exe
```
The `UI-Cmd.exe` process is wrapper that will call `ShellExecute()` Windows API to start `SendDeleteKey.exe` in this case. You can give it any of the other executables available as the first argument, and you can pass further arguments if needed.

After Windows Defender detects the tools as sending input to other applications it reports it as a trojan (like Wacatac.B!ml or similar). There is no such trojan or other malware in this tool, and when this happens you should open the notification from Windows Defender and "Restore" the files from quarantine, so Windows defender knows to leave it alone. 

## How it works
Sending keystrokes with `SendReturnKey.exe` / `SendEscapeKey.exe` / etc... generally requires bringing the window under cursor to foreground first. Unless the window ignores WM_ACTIVATE messages, meaning it is always ready to receive new input.

The tool will use Windows API functions to find the window under the mouse cursor, then look for an active horizontal scrollbar on that window or its parent windows.

If a horizontal scrollbar is found, this command will send a number of WM_HSCROLL messages to that window, according to the value of "lines per scroll" in Windows Settings.

If no horizontal scrollbar is found, which happens for applications using with custom UI frameworks, the generic WM_MOUSEHWHEEL message is sent instead.
