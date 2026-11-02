#!/usr/bin/env python3
"""Run the port's programs on an Android device or emulator (adb), as if they were the host executables the gate scripts expect.

  python tools/android_prog.py setup --apk p4pc-x86-debug.apk [--iso game.iso] [--refs dir]
      install the APK, copy the disc image and the reference frames to the app's media directory (/sdcard/Android/media/<package>)
  python tools/android_prog.py run <p4pc|p4pc_viewer> [<elf>] [args...]
      run one program in the app and behave like it: the app's log goes to stdout, the exit status is the program's, and the files it was
      told to write (frames, saves) are copied back to where the environment variables name them on this machine

The gate scripts (tests/*.sh) take the program as an argument and set P4PC_* variables; tools/android_bin/p4pc and p4pc_viewer are shims that
call `run`, so passing them to a gate script in place of the host executables runs that gate on the device with the same script the
host runs. The environment crosses to the app as launch extras (P4Activity): P4PC_* variables only (SDL_VIDEODRIVER and the like are the host's
business, not the phone's). Variables that name host paths are translated: P4PC_REF_RGB and P4PC_CAPTURE_REF are copied to the device first;
P4PC_CAPTURE_PNG, P4PC_CAPTURE_RGB, P4PC_DUMP_RAM and a viewer's output .png argument are copied back; P4PC_WALK_SCRIPT is copied into the app's media directory;
P4PC_SAVE_DIR is the app's save folder for the run, seeded from the host directory if it has files and copied back after. P4PC_ISO and P4PC_ELF are the device's (the app reads SLUS_217.82 from the disc
image). Only org.p4pc.game.debug is accepted: tests never install, launch, stop or clear the player's release app.
The app must be a debuggable build: the log (boot.log) is read with run-as.

Environment: ANDROID_ADB (adb path, default adb), ANDROID_SERIAL (device), P4PC_ANDROID_TIMEOUT (seconds a run may take, default 1500).
Installing an APK also requires SDK build-tools aapt2 (ANDROID_AAPT2 or setup --aapt2 if it is not on PATH).
"""
import argparse
import glob
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import time
import mod_witness

PKG = "org.p4pc.game.debug"
# Android 11 and later keep adb (the shell user) out of Android/data; Android/media/<package> is writable by both it and the app.
BASE = "/sdcard/Android/media/%s/files" % PKG
ADB = os.environ.get("ANDROID_ADB", "adb")
# Variables that name host paths (translated below) or the host's own business (never forwarded).
NOT_FORWARDED = {"P4PC_ISO", "P4PC_ELF", "P4PC_REF_DIR", "P4PC_HIDDEN_WINDOW", "P4PC_LOCAL_ISO", "P4PC_LOCAL_ELF", "P4PC_ANDROID_TIMEOUT",
                 "P4PC_HOST", "P4PC_PYTHON", "P4PC_SAVE_DIR", "P4PC_REF_RGB", "P4PC_CAPTURE_REF", "P4PC_CAPTURE_PNG", "P4PC_CAPTURE_RGB", "P4PC_DUMP_RAM", "P4PC_WALK_SCRIPT",
                 "P4PC_LOG_FILE", "P4PC_PROGRAM", "P4PC_ARGS", "P4PC_MOD_WITNESS_OUT"}


def adb(*args, check=True, timeout=None, binary=False):
    p = subprocess.run([ADB] + list(args), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    out = p.stdout if binary else p.stdout.decode("utf-8", "replace")
    if check and p.returncode != 0:
        raise SystemExit("adb %s failed (%d): %s" % (" ".join(args[:3]), p.returncode, out[-400:] if not binary else out[-400:].decode("utf-8", "replace")))
    return out


def shell(cmd, **kw):
    return adb("shell", cmd, **kw)


def device_size(path):
    out = shell("stat -c %%s %s 2>/dev/null" % shlex.quote(path), check=False).strip()
    return int(out) if out.isdigit() else -1


def push_if_changed(host, dev):
    if device_size(dev) == os.path.getsize(host):
        return
    shell("mkdir -p %s" % shlex.quote(os.path.dirname(dev)))
    adb("push", host, dev, timeout=3600)


def validate_debug_apk(path, aapt2):
    try:
        badging = subprocess.check_output([aapt2, "dump", "badging", path], text=True, stderr=subprocess.STDOUT)
    except FileNotFoundError as error:
        raise SystemExit("android_prog: SDK aapt2 is required; set ANDROID_AAPT2 or setup --aapt2") from error
    package = re.search(r"^package: name='([^']+)'", badging, re.MULTILINE)
    if not package or package.group(1) != PKG or not re.search(r"^application-debuggable\s*$", badging, re.MULTILINE):
        raise SystemExit("android_prog: refusing APK; expected debuggable " + PKG + ", never the player's release app")


def setup(args):
    refs = args.refs or (os.path.join(os.path.dirname(args.iso), "refs") if args.iso else None)
    reference_files = sorted(glob.glob(os.path.join(refs, "*.rgb"))) if refs and os.path.isdir(refs) else []
    for path in reference_files:
        mod_witness.refuse_reference(path)
    if args.apk:
        validate_debug_apk(args.apk, args.aapt2)
    adb("wait-for-device")
    if args.apk:
        out = adb("install", "-r", "-t", args.apk, check=False, timeout=600)
        if "INSTALL_FAILED_UPDATE_INCOMPATIBLE" in out:
            # Only the disposable .debug app can reach this path; never remove the player's release app.
            print("android_prog: the isolated debug app has another signing key; reinstalling it")
            adb("uninstall", PKG, check=False)
            out = adb("install", "-t", args.apk, check=False, timeout=600)
        if "Success" not in out:
            raise SystemExit("android_prog: install failed: " + out.strip()[-300:])
        print("android_prog: installed " + os.path.basename(args.apk))
    shell("mkdir -p %s/refs %s/out" % (BASE, BASE))
    if args.iso:
        push_if_changed(args.iso, BASE + "/p4.iso")
    for f in reference_files:
        push_if_changed(f, BASE + "/refs/" + os.path.basename(f))
    print("android_prog: device ready (%s)" % shell("getprop ro.product.cpu.abi").strip())


def log_bytes(offset):
    """The part of the app's log (files/boot.log) after `offset` bytes: only the new part crosses adb, a long run's log is megabytes."""
    return adb("exec-out", "run-as %s tail -c +%d files/boot.log 2>/dev/null" % (PKG, offset + 1), check=False, binary=True)


def run(args):
    prog = args.prog
    env = dict(os.environ)
    env["P4PC_INPUT"] = "script"  # Standalone shim runs are automated too, not just gate-runner callers.
    witness_root = None
    if mod_witness.VARIABLE in env:
        if not env.get("P4PC_MOD_WITNESS_OUT"):
            raise ValueError("Android mod-test lane requires an owned P4PC_MOD_WITNESS_OUT directory")
        witness_root = Path(env["P4PC_MOD_WITNESS_OUT"]).absolute()
        def owned_output(value):
            path = Path(value)
            path = (path if path.is_absolute() else witness_root / path).resolve()
            if not path.is_relative_to(witness_root.resolve()):
                raise ValueError("Android witness outputs must stay in P4PC_MOD_WITNESS_OUT")
            return str(path)

        for name in ("P4PC_CAPTURE_PNG", "P4PC_CAPTURE_RGB", "P4PC_SAVE_DIR", "P4PC_DUMP_RAM"):
            if env.get(name):
                env[name] = owned_output(env[name])
        if prog == "p4pc_viewer":
            args.rest = [owned_output(word) if word.endswith(".png") else word for word in args.rest]
        mod_witness.begin(witness_root, env)
    for name in ("P4PC_REF_RGB", "P4PC_CAPTURE_REF"):
        if env.get(name):
            mod_witness.refuse_reference(env[name])
    extras = []
    cleanup_pulls = []  # (device path, host path, is_dir)
    shell("am force-stop %s" % PKG)
    shell("rm -rf %s/out %s/save; mkdir -p %s/out %s/save" % (BASE, BASE, BASE, BASE))
    shell("run-as %s rm -f files/boot.log" % PKG, check=False)

    def e(name, value):
        extras.append("%s=%s" % (name, value))

    if env.get("SDL_AUDIODRIVER"):
        e("SDL_AUDIODRIVER", env["SDL_AUDIODRIVER"])  # the gate scripts ask for the dummy driver: a real one paces an unattended run at real time
    if "P4PC_CONFIG" not in env:
        e("P4PC_CONFIG", "none")  # no settings file on the device: a gate does not depend on what a player saved
    e("P4PC_LOGCAT", "0")  # boot.log is read here; logcat is slow when a game prints thousands of lines a second
    for name, value in sorted(env.items()):
        if name.startswith("P4PC_") and name not in NOT_FORWARDED:
            e(name, value)
    for name in ("P4PC_REF_RGB", "P4PC_CAPTURE_REF"):
        if env.get(name):
            dev = BASE + "/refs/" + os.path.basename(env[name])
            push_if_changed(env[name], dev)
            e(name, dev)
    if env.get("P4PC_WALK_SCRIPT"):
        dev = BASE + "/walks/" + os.path.basename(env["P4PC_WALK_SCRIPT"])
        push_if_changed(env["P4PC_WALK_SCRIPT"], dev)
        e("P4PC_WALK_SCRIPT", dev)
    if env.get("P4PC_CAPTURE_PNG"):
        host_dir, pattern = os.path.split(env["P4PC_CAPTURE_PNG"])
        e("P4PC_CAPTURE_PNG", BASE + "/out/" + pattern)
        cleanup_pulls.append((BASE + "/out/.", host_dir or ".", True))
    if env.get("P4PC_CAPTURE_RGB"):
        e("P4PC_CAPTURE_RGB", BASE + "/out/" + os.path.basename(env["P4PC_CAPTURE_RGB"]))
        cleanup_pulls.append((BASE + "/out/" + os.path.basename(env["P4PC_CAPTURE_RGB"]), env["P4PC_CAPTURE_RGB"], False))
    if env.get("P4PC_DUMP_RAM"):
        e("P4PC_DUMP_RAM", BASE + "/out/" + os.path.basename(env["P4PC_DUMP_RAM"]))
        cleanup_pulls.append((BASE + "/out/" + os.path.basename(env["P4PC_DUMP_RAM"]), env["P4PC_DUMP_RAM"], False))
    if env.get("P4PC_SAVE_DIR"):
        host = env["P4PC_SAVE_DIR"]
        if os.path.isdir(host) and os.listdir(host):
            adb("push", host + "/.", BASE + "/save", timeout=600)
        e("P4PC_SAVE_DIR", BASE + "/save")
        cleanup_pulls.append((BASE + "/save/.", host, True))
    words = []
    if prog == "p4pc_viewer":
        e("P4PC_PROGRAM", "viewer")
        for a in args.rest:
            if a.endswith(".png"):
                dev = BASE + "/out/" + os.path.basename(a)
                cleanup_pulls.append((dev, a, False))
                a = dev
            words.append(a)
        e("P4PC_ARGS", " ".join(words))

    cmd = ["am", "start", "-W", "-n", PKG + "/org.p4pc.game.P4Launcher", "--es", "iso", "file://" + BASE + "/p4.iso", "--es", "pad", "0"]
    for i, kv in enumerate(extras):
        cmd += ["--es", "env%d" % i, kv]
    t0 = time.time()
    shell(" ".join(shlex.quote(c) for c in cmd))
    limit = float(env.get("P4PC_ANDROID_TIMEOUT", "1500"))
    status = None
    raw = b""
    text = ""
    printed = 0  # the log is copied to stdout as it grows, so a run killed from outside (a gate's timeout) still leaves what it printed

    def flush(upto):
        nonlocal printed
        sys.stdout.write(upto[printed:])
        sys.stdout.flush()
        printed = max(printed, len(upto))

    while time.time() - t0 < limit:
        time.sleep(3)
        raw += log_bytes(len(raw))
        text = raw.decode("utf-8", "replace")
        flush(text)
        m = re.findall(r"p4pc: exited with status (-?\d+)", text)
        if m:
            status = int(m[-1])
            break
        if not text.strip() and time.time() - t0 > 60:
            # no log after a minute: the activity is up but libmain never started (a library that will not load, a missing disc image)
            err = shell("logcat -d -s System.err AndroidRuntime p4pc SDL/APP -t 40", check=False)
            sys.stdout.write("android_prog: the game printed nothing in 60 s; logcat:\n" + err)
            status = 125
            shell("am force-stop %s" % PKG)
            break
        if not shell("pidof %s" % PKG, check=False).strip():
            time.sleep(2)
            raw += log_bytes(len(raw))
            text = raw.decode("utf-8", "replace")
            flush(text)
            m = re.findall(r"p4pc: exited with status (-?\d+)", text)
            status = int(m[-1]) if m else 139
            if not m:
                crash = shell("logcat -d -b crash -t 60", check=False)
                sys.stdout.write("\nandroid_prog: the app died without an exit status; crash log:\n" + crash)
            break
    else:
        status = 124
        sys.stdout.write("\nandroid_prog: timed out after %d s\n" % limit)
        shell("am force-stop %s" % PKG)
    sys.stdout.flush()
    for dev, host, is_dir in cleanup_pulls:
        if is_dir:
            os.makedirs(host, exist_ok=True)
            adb("pull", dev, host, check=False, timeout=600)
        else:
            os.makedirs(os.path.dirname(os.path.abspath(host)), exist_ok=True)
            adb("pull", dev, host, check=False, timeout=600)
    if witness_root is not None:
        (witness_root / "boot.log").write_bytes(raw)
        mod_witness.finish(witness_root, status, environment=env, outcome="android-status")
    return status


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("setup")
    s.add_argument("--apk")
    s.add_argument("--aapt2", default=os.environ.get("ANDROID_AAPT2", "aapt2"), help="SDK build-tools aapt2; validates the debug-only APK before adb")
    s.add_argument("--iso")
    s.add_argument("--refs")
    r = sub.add_parser("run")
    r.add_argument("prog", choices=["p4pc", "p4pc_viewer"])
    r.add_argument("rest", nargs=argparse.REMAINDER, help="<elf> then the program's arguments (the elf is the device's, the argument is ignored)")
    a = ap.parse_args()
    if a.cmd == "setup":
        setup(a)
        return 0
    if a.rest and not a.rest[0].startswith("-"):
        a.rest = a.rest[1:]  # the host's elf path
    return run(a)


if __name__ == "__main__":
    sys.exit(main())
