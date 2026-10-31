import ctypes
import ctypes.wintypes
import os
import time

# Windows Constants
WM_CHAR = 0x0102
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
VK_RETURN = 0x0D

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

# Define Argument Types to prevent 64-bit truncation
user32.IsWindowVisible.argtypes = [ctypes.wintypes.HWND]
user32.IsWindowVisible.restype = ctypes.wintypes.BOOL

user32.GetWindowThreadProcessId.argtypes = [ctypes.wintypes.HWND, ctypes.POINTER(ctypes.wintypes.DWORD)]
user32.GetWindowThreadProcessId.restype = ctypes.wintypes.DWORD

user32.EnumWindows.argtypes = [ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.wintypes.HWND, ctypes.wintypes.LPARAM), ctypes.wintypes.LPARAM]
user32.EnumWindows.restype = ctypes.wintypes.BOOL

user32.SetForegroundWindow.argtypes = [ctypes.wintypes.HWND]
user32.SetForegroundWindow.restype = ctypes.wintypes.BOOL

user32.SendMessageW.argtypes = [ctypes.wintypes.HWND, ctypes.wintypes.UINT, ctypes.wintypes.WPARAM, ctypes.wintypes.LPARAM]
user32.SendMessageW.restype = ctypes.wintypes.LPARAM

user32.GetWindowTextLengthW.argtypes = [ctypes.wintypes.HWND]
user32.GetWindowTextLengthW.restype = ctypes.c_int
user32.GetWindowTextW.argtypes = [ctypes.wintypes.HWND, ctypes.wintypes.LPWSTR, ctypes.c_int]
user32.GetWindowTextW.restype = ctypes.c_int

def send_command_to_pid(target_pid, command):
    """Finds visible top-level windows for PID, sets foreground, and sends command."""
    if target_pid != 8080:
        raise RuntimeError("Only authorized UnrealEditor PID8080")

    windows = []

    def enum_windows_proc(hwnd, lparam):
        if user32.IsWindowVisible(hwnd):
            pid = ctypes.wintypes.DWORD()
            user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            if pid.value == target_pid:
                length = user32.GetWindowTextLengthW(hwnd)
                if length > 0:
                    buf = ctypes.create_unicode_buffer(length + 1)
                    user32.GetWindowTextW(hwnd, buf, length + 1)
                    title = buf.value
                    if "dancer" in title.lower():
                        windows.append(hwnd)
        return True

    WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.wintypes.HWND, ctypes.wintypes.LPARAM)
    callback = WNDENUMPROC(enum_windows_proc)
    user32.EnumWindows(callback, 0)

    if not windows:
        print(f"No visible windows found for PID {target_pid}")
        return False

    # Target the first valid window found
    hwnd = windows[0]
    
    if not user32.SetForegroundWindow(hwnd):
        print(f"Failed to set foreground for HWND {hwnd}")
        return False
    
    rect = ctypes.wintypes.RECT()
    user32.GetWindowRect.argtypes = [ctypes.wintypes.HWND, ctypes.POINTER(ctypes.wintypes.RECT)]
    user32.GetWindowRect(hwnd, ctypes.byref(rect))
    user32.SetCursorPos(rect.left + 480, rect.bottom - 20)
    user32.mouse_event(0x0002, 0, 0, 0, 0)
    user32.mouse_event(0x0004, 0, 0, 0, 0)
    time.sleep(0.6)
    for _ in range(100):
        user32.SendMessageW(hwnd, WM_CHAR, 8, 0)
        time.sleep(0.01)
    time.sleep(0.6)
    # Send command string
    for char in command:
        user32.SendMessageW(hwnd, WM_CHAR, ord(char), 0)
        time.sleep(0.06)
    
    time.sleep(1.0)
    # Send Enter
    user32.SendMessageW(hwnd, WM_KEYDOWN, VK_RETURN, 0)
    user32.SendMessageW(hwnd, WM_KEYUP, VK_RETURN, 0)
    
    print(f"Command sent to HWND {hwnd}")
    return True

if __name__ == "__main__":
    # Example usage
    TARGET_PID = 8080
    CMD = 'py "G:/UE/dancer/docs/Rhythm/tools/verify_editor_demo.py"' 
    send_command_to_pid(TARGET_PID, CMD)
