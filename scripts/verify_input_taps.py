"""Windows runtime regression: a press/release within one frame must survive polling.

Run after a Release build. Posts events only to this process's GLFW window.
No FFmpeg, desktop input injection, or third-party Python dependency is required.
"""

import ctypes
from ctypes import wintypes
from pathlib import Path
import subprocess
import threading
import time


ROOT = Path(__file__).resolve().parents[1]


def main():
    output = ROOT / "captures/assessment/input-taps.txt"
    output.parent.mkdir(parents=True, exist_ok=True)
    ready = threading.Event()
    app = subprocess.Popen([str(ROOT / "x64/Release/Voyager-2.exe")], cwd=ROOT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    def collect():
        with output.open("w", encoding="utf-8") as log:
            for line in app.stdout:
                log.write(line)
                log.flush()
                if "[APP] initialized." in line:
                    ready.set()

    reader = threading.Thread(target=collect, daemon=True)
    reader.start()
    user32 = ctypes.windll.user32
    user32.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    windows = []
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    @callback_type
    def visit(handle, _):
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(handle, ctypes.byref(owner))
        if owner.value == app.pid:
            windows.append(handle)
        return True

    try:
        if not ready.wait(60):
            raise RuntimeError("Runtime initialization did not complete")
        user32.EnumWindows(visit, 0)
        if not windows:
            raise RuntimeError("Application window not found")
        hwnd = windows[0]
        time.sleep(0.5)
        # No held interval: both events are queued before the next frame poll.
        for vk in [ord("P"), 0x72, ord("V"), ord("V"), ord("K"), ord("K")]:
            scan = user32.MapVirtualKeyW(vk, 0)
            user32.PostMessageW(hwnd, 0x100, vk, 1 | (scan << 16))
            user32.PostMessageW(hwnd, 0x101, vk, 1 | (scan << 16) | (1 << 30) | (1 << 31))
            time.sleep(0.4)
        user32.PostMessageW(hwnd, 0x10, 0, 0)  # WM_CLOSE: does not depend on keyboard polling
        app.wait(timeout=15)
    finally:
        if app.poll() is None:
            app.terminate()
            app.wait(timeout=10)
        reader.join(timeout=5)
    log = output.read_text(encoding="utf-8")
    expected = ["simulation paused", "shading technique: TOON", "flight mode: Manual",
                "flight mode: Historical", "lighting off", "lighting on", "shutting down"]
    missing = [marker for marker in expected if marker not in log]
    if missing:
        raise AssertionError("Missed short taps: " + ", ".join(missing))
    print("[INPUT] Same-frame press/release taps preserved for pause, shading, flight modes and lighting")


if __name__ == "__main__":
    main()
