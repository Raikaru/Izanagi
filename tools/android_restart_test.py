#!/usr/bin/env python3
"""One real debug-app restart; never install, stop or modify the release package.

Stage a debuggable APK and its own ISO first with android_prog.py setup, then:
  python tools/android_restart_test.py --serial HOST:PORT --out PRIVATE_DIRECTORY
"""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import tempfile
import time
import uuid

import android_prog as device

POLICY = Path(__file__).resolve().parents[1] / "config" / "land_policy.json"


def default_lock():
    """The shared phone lock: the land policy's, where a repository has one; otherwise one in the temporary folder."""
    if POLICY.is_file():
        return json.loads(POLICY.read_text(encoding="utf-8"))["phone"]["lock"]
    return os.path.join(tempfile.gettempdir(), "p4pc-phone.lock")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--serial", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--iso-uri", help="debug app's own persisted URI; default its staged media ISO")
    parser.add_argument("--lock", default=default_lock())
    args = parser.parse_args()
    # Refuse an old runner before any adb command or package lifecycle operation.
    if device.PKG != "org.p4pc.game.debug":
        raise SystemExit("restart witness requires the fixed P4PC Debug runner")
    os.environ["ANDROID_SERIAL"] = args.serial
    lock = Path(args.lock)
    lock.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(lock, os.O_WRONLY | os.O_CREAT | os.O_EXCL)
    try:
        os.write(fd, f"PortARM restart witness pid={os.getpid()}\n".encode())
    except BaseException:
        lock.unlink()
        raise
    finally:
        os.close(fd)
    run_id = uuid.uuid4().hex
    out = Path(args.out) / ("restart-" + run_id)
    stem = device.BASE + "/out/restart-" + run_id
    marker, sentinel = stem + ".marker", stem + ".inbox"
    quote = shlex.quote
    try:
        out.mkdir(parents=True, exist_ok=True)
        device.shell("run-as " + device.PKG + " id")
        device.shell("am force-stop " + device.PKG)
        device.shell("mkdir -p " + quote(device.BASE + "/out"))
        device.shell("printf inbox-preserved > " + quote(sentinel))
        device.shell("run-as " + device.PKG + " rm -f files/boot.log")
        values = {"P4PC_INPUT": "script", "P4PC_CONFIG": "none", "SDL_AUDIODRIVER": "dummy",
                  "P4PC_LOGCAT": "0", "P4PC_TEST_RESTART": marker}
        command = ["am", "start", "-W", "-n", device.PKG + "/org.p4pc.game.P4Launcher",
                   "--es", "iso", args.iso_uri or "file://" + device.BASE + "/p4.iso", "--es", "pad", "0"]
        for index, (key, value) in enumerate(values.items()):
            command += ["--es", f"env{index}", key + "=" + value]
        (out / "launch.log").write_text(device.shell(" ".join(map(quote, command))), encoding="utf-8")
        deadline = time.monotonic() + 45
        text = log = ""
        while time.monotonic() < deadline:
            # `adb shell` allocates a PTY and converts LF to CRLF on some hosts.
            text = device.shell("cat " + quote(marker), check=False).replace("\r\n", "\n")
            if text.endswith("drained\nsecond\n"):
                log = device.shell("run-as " + device.PKG + " cat files/boot.log", check=False)
                if "p4pc restart test: PASS:" in log:
                    break
            time.sleep(0.2)
        else:
            raise AssertionError("fresh debug process did not confirm the drained restart")
        (out / "restart.marker").write_text(text, encoding="utf-8")
        (out / "boot.log").write_text(log, encoding="utf-8")
        match = re.search(r"PASS: old pid=(\d+) new pid=(\d+)", log)
        assert match and match[1] != match[2], "game process was not replaced"
        assert device.shell("cat " + quote(sentinel)).strip() == "inbox-preserved", "media inbox lost"
        result = {"package": device.PKG, "old_pid": int(match[1]), "new_pid": int(match[2]),
                  "drained_exit": True, "same_arguments": True, "media_inbox_preserved": True,
                  "evidence_directory": str(out)}
        (out / "result.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print("Android restart: PASS " + json.dumps(result))
    except BaseException:
        try:
            if out.is_dir():
                (out / "restart.marker").write_text(device.shell("cat " + quote(marker), check=False), encoding="utf-8")
                (out / "boot.log").write_text(device.shell("run-as " + device.PKG + " cat files/boot.log", check=False), encoding="utf-8")
        except Exception as failure:
            print(f"restart: could not capture failure diagnostics: {failure}", flush=True)
        print(f"restart: failure evidence retained in {out}", flush=True)
        raise
    finally:
        try:
            device.shell("am force-stop " + device.PKG, check=False)
            device.shell("rm -f " + quote(marker) + " " + quote(sentinel), check=False)
        finally:
            lock.unlink()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
