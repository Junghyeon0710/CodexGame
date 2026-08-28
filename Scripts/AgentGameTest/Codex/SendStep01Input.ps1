[CmdletBinding(DefaultParameterSetName = 'Chord')]
param(
    [Parameter(Mandatory = $true)]
    [int]$EditorProcessId,

    [Parameter(Mandatory = $true, ParameterSetName = 'Chord')]
    [ValidateSet('W', 'A', 'S', 'D', 'Space')]
    [string[]]$Keys,

    [Parameter(ParameterSetName = 'Chord')]
    [ValidateRange(20, 5000)]
    [int]$HoldMilliseconds = 250,

    [Parameter(Mandatory = $true, ParameterSetName = 'CursorClick')]
    [int]$CursorX,

    [Parameter(Mandatory = $true, ParameterSetName = 'CursorClick')]
    [int]$CursorY,

    [Parameter(ParameterSetName = 'CursorClick')]
    [switch]$Click,

    [Parameter(Mandatory = $true, ParameterSetName = 'CursorPosition')]
    [switch]$GetCursorPosition
)

$ErrorActionPreference = 'Stop'

if (-not ('CodexStep01NativeInput' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public static class CodexStep01NativeInput
{
    private delegate bool EnumWindowsProc(IntPtr window, IntPtr parameter);

    [StructLayout(LayoutKind.Sequential)]
    public struct Point
    {
        public int X;
        public int Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct Rect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    private static extern bool EnumWindows(EnumWindowsProc callback, IntPtr parameter);

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);

    [DllImport("user32.dll")]
    private static extern bool IsWindowVisible(IntPtr window);

    [DllImport("user32.dll")]
    private static extern bool GetWindowRect(IntPtr window, out Rect rectangle);

    [DllImport("user32.dll")]
    private static extern bool ShowWindowAsync(IntPtr window, int command);

    [DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(IntPtr window);

    [DllImport("user32.dll")]
    private static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll")]
    private static extern bool SetCursorPos(int x, int y);

    [DllImport("user32.dll")]
    private static extern bool GetCursorPos(out Point point);

    [DllImport("user32.dll")]
    private static extern bool PostMessage(IntPtr window, uint message, IntPtr wordParameter, IntPtr longParameter);

    [DllImport("user32.dll")]
    private static extern uint MapVirtualKey(uint code, uint mapType);

    [DllImport("user32.dll")]
    private static extern void keybd_event(byte virtualKey, byte scanCode, uint flags, UIntPtr extraInfo);

    [DllImport("user32.dll")]
    private static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extraInfo);

    private const uint LeftDown = 0x0002;
    private const uint LeftUp = 0x0004;
    private const uint WindowKeyDown = 0x0100;
    private const uint WindowKeyUp = 0x0101;
    private const uint KeyboardKeyUp = 0x0002;
    private const int Restore = 9;

    public static IntPtr FindLargestVisibleWindow(uint targetProcessId)
    {
        IntPtr bestWindow = IntPtr.Zero;
        long bestArea = 0;

        EnumWindows((window, parameter) =>
        {
            uint processId;
            GetWindowThreadProcessId(window, out processId);
            if (processId != targetProcessId || !IsWindowVisible(window))
            {
                return true;
            }

            Rect rectangle;
            if (!GetWindowRect(window, out rectangle))
            {
                return true;
            }

            long area = Math.Max(0, rectangle.Right - rectangle.Left)
                * (long)Math.Max(0, rectangle.Bottom - rectangle.Top);
            if (area > bestArea)
            {
                bestArea = area;
                bestWindow = window;
            }
            return true;
        }, IntPtr.Zero);

        return bestWindow;
    }

    public static bool FocusWindow(IntPtr window)
    {
        ShowWindowAsync(window, Restore);
        return SetForegroundWindow(window);
    }

    public static IntPtr ForegroundWindow()
    {
        return GetForegroundWindow();
    }

    public static bool KeyDown(IntPtr window, byte virtualKey)
    {
        uint scanCode = MapVirtualKey(virtualKey, 0);
        long eventData = 1L | ((long)scanCode << 16);
        keybd_event(virtualKey, (byte)scanCode, 0, UIntPtr.Zero);
        return PostMessage(window, WindowKeyDown, (IntPtr)virtualKey, (IntPtr)eventData);
    }

    public static bool KeyRelease(IntPtr window, byte virtualKey)
    {
        uint scanCode = MapVirtualKey(virtualKey, 0);
        long eventData = 1L | ((long)scanCode << 16) | (1L << 30) | (1L << 31);
        keybd_event(virtualKey, (byte)scanCode, KeyboardKeyUp, UIntPtr.Zero);
        return PostMessage(window, WindowKeyUp, (IntPtr)virtualKey, (IntPtr)eventData);
    }

    public static bool MoveCursor(int x, int y)
    {
        return SetCursorPos(x, y);
    }

    public static Point CursorPosition()
    {
        Point point;
        GetCursorPos(out point);
        return point;
    }

    public static void LeftClick()
    {
        mouse_event(LeftDown, 0, 0, 0, UIntPtr.Zero);
        mouse_event(LeftUp, 0, 0, 0, UIntPtr.Zero);
    }
}
'@
}

$editorProcess = Get-Process -Id $EditorProcessId -ErrorAction Stop
$windowHandle = [CodexStep01NativeInput]::FindLargestVisibleWindow([uint32]$editorProcess.Id)
if ($windowHandle -eq [IntPtr]::Zero) {
    throw "No visible Unreal Editor window was found for process $EditorProcessId."
}

$focused = [CodexStep01NativeInput]::FocusWindow($windowHandle)
Start-Sleep -Milliseconds 150

if ($PSCmdlet.ParameterSetName -eq 'CursorPosition') {
    $position = [CodexStep01NativeInput]::CursorPosition()
    [pscustomobject]@{ X = $position.X; Y = $position.Y }
    return
}

if ($PSCmdlet.ParameterSetName -eq 'CursorClick') {
    if (-not [CodexStep01NativeInput]::MoveCursor($CursorX, $CursorY)) {
        throw "SetCursorPos failed for ($CursorX, $CursorY)."
    }
    Start-Sleep -Milliseconds 100
    if ($Click) {
        [CodexStep01NativeInput]::LeftClick()
    }
    [pscustomobject]@{ X = $CursorX; Y = $CursorY; Clicked = [bool]$Click }
    return
}

$virtualKeys = @{
    W = [byte]0x57
    A = [byte]0x41
    S = [byte]0x53
    D = [byte]0x44
    Space = [byte]0x20
}

foreach ($keyName in $Keys) {
    [void][CodexStep01NativeInput]::KeyDown($windowHandle, $virtualKeys[$keyName])
}

Start-Sleep -Milliseconds $HoldMilliseconds

for ($keyIndex = $Keys.Count - 1; $keyIndex -ge 0; --$keyIndex) {
    [void][CodexStep01NativeInput]::KeyRelease($windowHandle, $virtualKeys[$Keys[$keyIndex]])
}

[pscustomobject]@{
    Keys = ($Keys -join '+')
    HoldMilliseconds = $HoldMilliseconds
    WindowHandle = $windowHandle
    FocusRequested = $focused
    ForegroundHandle = [CodexStep01NativeInput]::ForegroundWindow()
}
