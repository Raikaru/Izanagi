#!/usr/bin/env python3
"""Run one mod-witness game process on the phone's adb-shell build, as if it were the host player.

A drop-in --driver for the mod witnesses (mod_randomizer_game_test.py and its siblings): they start
`<driver> <elf>` with the leg's environment and read its output; this runs the same game on the
phone pool's device with the leg's files and copies the results back:

  python tests/mod_witness_phone_driver.py <elf>

Environment: P4PC_PHONE_SERIAL (the adb serial, default config/land_policy.json's phone),
P4PC_PHONE_EXPORT (a directory holding the phone runner build: p4pc_phone and its libraries, from
`sh tools/android_build.sh <lane> --abi arm64-v8a --phone-runner`), P4PC_PHONE_LOCK (default the
policy's shared lock), P4PC_PHONE_TIMEOUT (seconds, default 3500), and the leg's own P4PC_* variables.

The ELF, ISO and runner are content-addressed objects below /data/local/tmp/p4pc (tools/phone_stage.py,
shared with the phone pool; nothing is pushed twice). Each run gets its own directory there: the leg's
memory card (P4PC_SAVE_DIR), mod packages (P4PC_MOD_DIR), settings file and its mod-settings folder
(P4PC_CONFIG) and walk script (P4PC_WALK_SCRIPT) are pushed and the variables point at the device
copies (P4PC_SCENE_ARTIFACTS at a device folder). After the game exits, the card and the mod artifacts
(the mod-artifacts folder beside the settings file) come back to the host paths and the run directory
is removed. The phone lock is held for the run, so the land's
phone pool falls back to the PC meanwhile; a lock another session holds is waited for (up to an hour).
Exit status: the game's.
"""
import json
import os
import shlex
import subprocess
import sys
import tempfile
import time
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import phone_stage  # noqa: E402

RUNNER = ("p4pc_phone", "libSDL3.so", "libavcodec.so", "libavutil.so", "libswscale.so")
# Variables naming host paths, translated below, or host-only.
TRANSLATED = {"P4PC_ISO", "P4PC_ELF", "P4PC_CONFIG", "P4PC_SAVE_DIR", "P4PC_SCENE_ARTIFACTS", "P4PC_WALK_SCRIPT",
              "P4PC_MOD_DIR", "P4PC_PHONE_SERIAL", "P4PC_PHONE_EXPORT", "P4PC_PHONE_LOCK", "P4PC_PHONE_TIMEOUT",
              "P4PC_HIDDEN_WINDOW", "P4PC_MOD_WITNESS_OUT"}


def policy_phone():
    return json.loads((ROOT / "config" / "land_policy.json").read_text(encoding="utf-8"))["phone"]


def known_digest(path):
    """The disc image's SHA-256, cached by size and modification time across this machine's runs."""
    cache = Path(tempfile.gettempdir()) / "p4pc-phone-driver-digests.json"
    try:
        known = json.loads(cache.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        known = {}
    stat = path.stat()
    key = f"{path.resolve()}|{stat.st_size}|{stat.st_mtime_ns}"
    if key not in known:
        known[key] = phone_stage.digest(path)[0]
        cache.write_text(json.dumps(known), encoding="utf-8")
    return known[key]


def acquire(lock):
    """The shared phone lock, exclusive for this run; another owner is waited for."""
    lock.parent.mkdir(parents=True, exist_ok=True)
    deadline = time.monotonic() + 3600
    while True:
        try:
            fd = os.open(lock, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        except FileExistsError:
            if time.monotonic() > deadline:
                raise SystemExit(f"mod_witness_phone_driver: the phone lock {lock} stayed owned for an hour")
            time.sleep(15)
            continue
        os.write(fd, f"mod witness phone driver pid={os.getpid()}\n".encode())
        os.close(fd)
        return


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: mod_witness_phone_driver.py <elf>")
    env = dict(os.environ)
    phone_policy = policy_phone()
    serial = env.get("P4PC_PHONE_SERIAL", phone_policy["serial"])
    export = Path(env.get("P4PC_PHONE_EXPORT", ""))
    if not all((export / name).is_file() for name in RUNNER):
        raise SystemExit(f"mod_witness_phone_driver: no phone runner build in P4PC_PHONE_EXPORT ({export})")
    lock = Path(env.get("P4PC_PHONE_LOCK", phone_policy["lock"]))
    limit = int(env.get("P4PC_PHONE_TIMEOUT", "3500"))
    acquire(lock)
    phone = phone_stage.Phone(serial)
    base = phone.root + "/witness-" + uuid.uuid4().hex
    q = shlex.quote
    try:
        phone.connect()
        iso = Path(env["P4PC_ISO"])
        rows = phone.stage([(export / name, None) for name in RUNNER] + [(Path(sys.argv[1]), None),
                                                                         (iso, known_digest(iso))])
        objects = dict(zip([*RUNNER, "elf", "iso"], (row["path"] for row in rows)))
        phone.shell("mkdir", "-p", base + "/bin", base + "/card", base + "/conf", base + "/walks", base + "/art",
                    base + "/mods")
        copies = " && ".join(f"cp {q(objects[name])} {q(base + '/bin/' + name)}" for name in RUNNER)
        phone.shell("sh", "-c", copies + f" && chmod 755 {q(base + '/bin/p4pc_phone')}")
        device = {"P4PC_ISO": objects["iso"], "P4PC_SCENE_ARTIFACTS": base + "/art"}
        save = env.get("P4PC_SAVE_DIR")
        if save:
            if Path(save).is_dir() and any(Path(save).iterdir()):
                phone.call("push", save + "/.", base + "/card", timeout=600)
            device["P4PC_SAVE_DIR"] = base + "/card"
        if env.get("P4PC_MOD_DIR"):
            phone.call("push", env["P4PC_MOD_DIR"] + "/.", base + "/mods", timeout=600)
            device["P4PC_MOD_DIR"] = base + "/mods"
        config = env.get("P4PC_CONFIG")
        if config and config != "none":
            phone.call("push", config, base + "/conf/" + Path(config).name, timeout=120)
            settings = Path(config).parent / "mod-settings"
            if settings.is_dir():
                phone.shell("mkdir", "-p", base + "/conf/mod-settings")
                phone.call("push", str(settings) + "/.", base + "/conf/mod-settings", timeout=120)
            device["P4PC_CONFIG"] = base + "/conf/" + Path(config).name
        if env.get("P4PC_WALK_SCRIPT"):
            walk = base + "/walks/" + Path(env["P4PC_WALK_SCRIPT"]).name
            phone.call("push", env["P4PC_WALK_SCRIPT"], walk, timeout=120)
            device["P4PC_WALK_SCRIPT"] = walk
        assignments = {"LD_LIBRARY_PATH": base + "/bin", "TMPDIR": base, "SDL_AUDIODRIVER": "dummy",
                       "SDL_VIDEODRIVER": "offscreen"}
        assignments.update({key: value for key, value in env.items()
                            if key.startswith("P4PC_") and key not in TRANSLATED})
        assignments.update(device)
        command = (f"cd {q(base)} && exec env {' '.join(q(k + '=' + v) for k, v in sorted(assignments.items()))} "
                   f"timeout -s TERM {limit} {q(base + '/bin/p4pc_phone')} {q(objects['elf'])} 2>&1")
        shell = (f"setsid sh -c {q(command)} & child=$!; "
                 """trap 'kill -TERM -- -"$child" 2>/dev/null' EXIT HUP INT TERM; """
                 'wait "$child"; rc=$?; trap - EXIT HUP INT TERM; printf "\\nP4PC_PHONE_RC:%s\\n" "$rc"')
        process = subprocess.Popen(["adb", "-s", serial, "shell", shell], stdin=subprocess.DEVNULL,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        status = None
        for line in iter(process.stdout.readline, b""):
            text = line.decode("utf-8", "replace").rstrip("\r\n")
            if text.startswith("P4PC_PHONE_RC:"):
                status = int(text.split(":", 1)[1])
                continue
            sys.stdout.write(text + "\n")
            sys.stdout.flush()
        process.wait()
        if status is None:
            raise SystemExit("mod_witness_phone_driver: the phone run ended without an exit status")
        if save:
            Path(save).mkdir(parents=True, exist_ok=True)
            phone.call("pull", base + "/card/.", save, timeout=600)
        # Mod artifacts live beside the settings file (docs/modding/lua.md, p4.artifacts).
        if config and config != "none" and \
                phone.shell("ls", base + "/conf/mod-artifacts", check=False).returncode == 0:
            phone.call("pull", base + "/conf/mod-artifacts", str(Path(config).parent), timeout=600)
        return status
    finally:
        try:
            phone.shell("rm", "-rf", base, check=False, timeout=120)
        finally:
            lock.unlink()


if __name__ == "__main__":
    sys.exit(main())
