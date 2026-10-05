"""Record the application's 120-second tour, never the entire desktop.

Run from any directory after a Release build. Requires imageio-ffmpeg.
The output contains actual live motion, captions and the application's HUD.
"""

import argparse
import ctypes
from ctypes import wintypes
from pathlib import Path
import subprocess
import threading

import imageio_ffmpeg


ROOT = Path(__file__).resolve().parents[1]


def find_window(pid):
    user32 = ctypes.windll.user32
    user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user32.IsWindowVisible.argtypes = [wintypes.HWND]
    windows = []
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    @callback_type
    def visit(handle, _):
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(handle, ctypes.byref(owner))
        if owner.value == pid and user32.IsWindowVisible(handle):
            windows.append(handle)
        return True

    user32.EnumWindows(visit, 0)
    if not windows:
        raise RuntimeError("The application's window was not found")
    return windows[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=["Debug", "Release"], default="Release")
    parser.add_argument("--output", type=Path, default=ROOT / "docs/report/Voyager_2_Demo.mp4")
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    evidence = ROOT / "captures/assessment/demo"
    evidence.mkdir(parents=True, exist_ok=True)
    ffmpeg = imageio_ffmpeg.get_ffmpeg_exe()
    exe = ROOT / "x64" / args.configuration / "Voyager-2.exe"
    ready = threading.Event()
    app = subprocess.Popen([str(exe), "--capture-demo", str(evidence)], cwd=ROOT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    def collect_log():
        with (evidence / "runtime.txt").open("w", encoding="utf-8") as log:
            for line in app.stdout:
                log.write(line)
                log.flush()
                if "[APP] capture tour:" in line:
                    ready.set()

    reader = threading.Thread(target=collect_log, daemon=True)
    reader.start()
    recorder = None
    try:
        if not ready.wait(90):
            raise RuntimeError("Tour did not initialize; see captures/assessment/demo/runtime.txt")
        handle = find_window(app.pid)
        raw = evidence / "recording.mp4"
        with (evidence / "ffmpeg.txt").open("w", encoding="utf-8") as log:
            recorder = subprocess.Popen([
                ffmpeg, "-y", "-f", "gdigrab", "-framerate", "30", "-draw_mouse", "0",
                "-i", f"hwnd={handle}", "-vf", "scale=1440:900,setsar=1",
                "-c:v", "libx264", "-preset", "veryfast", "-crf", "20", "-pix_fmt", "yuv420p",
                str(raw)], stdin=subprocess.PIPE, stdout=log, stderr=log)
            print("[REPORT] Recording the Voyager window; keep it visible during the tour.", flush=True)
            app.wait(timeout=180)
            if recorder.poll() is None:
                recorder.communicate(b"q\n", timeout=30)
            if app.returncode != 0 or recorder.returncode != 0:
                raise RuntimeError("Recording failed; inspect the runtime and ffmpeg logs")
        # Pad only a possible fraction of a second lost to recorder startup,
        # then enforce exactly 3,600 frames / 120 seconds at 30 fps.
        subprocess.run([
            ffmpeg, "-y", "-i", str(raw), "-vf", "fps=30,tpad=stop_mode=clone:stop_duration=2",
            "-frames:v", "3600", "-an", "-c:v", "libx264", "-preset", "medium", "-crf", "20",
            "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(output)], check=True,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"[REPORT] Saved {output} (120 seconds, 1440 x 900, 30 fps)")
    finally:
        if recorder is not None and recorder.poll() is None:
            recorder.terminate()
            recorder.wait(timeout=15)
        if app.poll() is None:
            app.terminate()
            app.wait(timeout=15)
        reader.join(timeout=5)


if __name__ == "__main__":
    main()
