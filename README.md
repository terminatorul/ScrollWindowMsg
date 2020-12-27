# ScrollWindowMsg
Sends WinAPI message to horizontal scroll to the window under the mouse cursor.

Simple Windows command to use with mice that can map executable programs to mouse buttons, in my case for:
 - Logitech G600 Gaming Mouse
 - Razer Naga Trinity

Most mice can map other functions to the buttons, but this command is intended for mice that have buttons for horizontal scrolling, either with a second scroll wheel, either with wheel tilt on the main scroll wheel, either with extra buttons for this purpose.

## Purpose
The default behavior for the software suite provided with these mice, for horizontal scroll, if any, works with most, but not all applications. Some Windows applications do not translate WM_MOUSEHWHEEL message correctly to the associated WM_HSCROLL messages. In my case I needed horizontal scroll to work with gVim, and I also noticed WordPad would not scroll horizontally with my mice.

## Configuration
The command is currently built with Visual Studio 2019 (Community Edition).

Use the software suite of the mouse to launch ScrollWindowMsg.exe when horizontal scroll buttons are pressed, with the following arguments:
    - "left" for scrolling 3 chars to the left
    - "right" for scrolling 3 chars to the right

You should configure the mouse software to repeat this action for as long as the button is held down.

However in case of Vim the scroll speed is to low for the default repeat rate of the Logitech G HUB software (20 events per second), and the scroll messages accumulate in the window message queue. So scrolling continues after the button is released.

## How it works
The tool will use Windows API functions to find the (most nested) window under the mouse cursor, then look for an active horizontal scrollbar on that window or its parent windows.

If a horizontal scrollbar is found, this command will send a number of WM_HSCROLL messages to that window, according to lines per scroll in Windows Settings.

If no horizontal scrollbar is found, which happens for applications using with custom UI frameworks, the generic WM_MOUSEHWHEEL message is sent instead.

## Issues
The delayed effect resulting from scroll messages accumulating in the window message queue can be fixed by:
 - designing start and stop commands for the application
 - using the mouse software suite to map different command arguments for mouse key down and mouse key up events
 - this way start command can be mapped on mouse key down, and stop command can be mapped on mouse key up.
