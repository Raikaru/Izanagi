"""A reference gate must skip an explicitly different loaded GL renderer."""
import os
import re
import subprocess
import sys


def main():
    if not os.environ.get("P4PC_ISO") or not os.environ.get("P4PC_ELF"):
        print("renderer_scope: P4PC_ISO or P4PC_ELF unset, skipped")
        return 77
    environment = os.environ.copy()
    environment.update({
        "P4PC_CONFIG": "none",
        "P4PC_INPUT": "script",
        "P4PC_HIDDEN_WINDOW": "1",
        "P4PC_OFFSCREEN": "1",
        "SDL_AUDIODRIVER": "dummy",
        "P4PC_GATE_RENDERER": "__p4pc_intentionally_unmatched_reference_renderer__",
    })
    for name in ("P4PC_CI_GL_MODE", "P4PC_CI_HARDWARE_GL", "P4PC_CI_GL_RENDERER"):
        environment.pop(name, None)
    result = subprocess.run(sys.argv[1:], env=environment, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, timeout=60)
    print(result.stdout, end="")
    if result.returncode != 77 or "reference renderer mismatch:" not in result.stdout:
        print(f"renderer_scope: expected the explicit mismatch skip, got {result.returncode}")
        return 1
    identity = re.search(r"got '(.*)'; skipped", result.stdout)
    if identity is None or len(identity.group(1)) < 2:
        print("renderer_scope: missing loaded renderer identity")
        return 1
    # A family substring must not accidentally accept ANGLE or another backend
    # whose description merely contains the qualified hardware's name.
    environment["P4PC_GATE_RENDERER"] = identity.group(1)[:-1]
    result = subprocess.run(sys.argv[1:], env=environment, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, timeout=60)
    print(result.stdout, end="")
    if result.returncode != 77 or "reference renderer mismatch:" not in result.stdout:
        print(f"renderer_scope: partial identities must also skip, got {result.returncode}")
        return 1
    environment["P4PC_GATE_RENDERER"] = "__p4pc_intentionally_unmatched_reference_renderer__"
    software = identity.group(1) == "llvmpipe" or identity.group(1).startswith("llvmpipe (")
    software_hardware = any(name in identity.group(1).lower() for name in
                            ("llvmpipe", "softpipe", "swiftshader", "warp", "software", "basic render", "gdi generic"))
    for case in (
        ("software-reference", None, 77 if software else 1),
        ("hardware", "1", 1 if software_hardware else 77),
        ("unknown", "1", 1),
        ("software", "1", 1),
        ("software-reference", "1", 1),
        ("hardware", None, 1),
        ("hardware", "0", 1),
        # The legacy hardware context (flag alone, mode unset or empty) is the hardware mode: a hardware
        # renderer passes, a software renderer never does, and unset never means the software reference.
        (None, "1", 1 if software_hardware else 77),
        ("", "1", 1 if software_hardware else 77),
        (None, "0", 1),
        (None, None, 1, "__p4pc_ci_renderer_only__"),
    ):
        mode, hardware, expected, *extra = case
        environment.pop("P4PC_CI_GL_MODE", None)
        environment.pop("P4PC_CI_HARDWARE_GL", None)
        environment.pop("P4PC_CI_GL_RENDERER", None)
        if extra:
            environment["P4PC_CI_GL_RENDERER"] = extra[0]
        if mode is not None:
            environment["P4PC_CI_GL_MODE"] = mode
        if hardware is not None:
            environment["P4PC_CI_HARDWARE_GL"] = hardware
        result = subprocess.run(sys.argv[1:], env=environment, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, timeout=60)
        print(result.stdout, end="")
        if result.returncode != expected:
            print(f"renderer_scope: mode {mode!r} expected {expected}, got {result.returncode}")
            return 1
    if sys.platform.startswith("linux"):
        environment.pop("P4PC_CI_HARDWARE_GL", None)
        environment.pop("P4PC_CI_GL_RENDERER", None)
        environment.update(P4PC_CI_GL_MODE="software-reference", LIBGL_ALWAYS_SOFTWARE="1",
                           SDL_VIDEODRIVER="offscreen", EGL_PLATFORM="surfaceless")
        result = subprocess.run(sys.argv[1:], env=environment, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, timeout=60)
        print(result.stdout, end="")
        if result.returncode != 77 or "llvmpipe" not in result.stdout.lower():
            print(f"renderer_scope: llvmpipe reference must reach renderer skip, got {result.returncode}")
            return 1
        for mode in ("hardware", None):
            environment.pop("P4PC_CI_GL_MODE", None)
            environment["P4PC_CI_HARDWARE_GL"] = "1"
            if mode is not None:
                environment["P4PC_CI_GL_MODE"] = mode
            result = subprocess.run(sys.argv[1:], env=environment, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, text=True, timeout=60)
            print(result.stdout, end="")
            if result.returncode != 1 or "CI requires hardware renderer" not in result.stdout:
                print(f"renderer_scope: hardware mode {mode!r} must reject llvmpipe, got {result.returncode}")
                return 1
        for name in ("LIBGL_ALWAYS_SOFTWARE", "SDL_VIDEODRIVER", "EGL_PLATFORM"):
            if name in os.environ:
                environment[name] = os.environ[name]
            else:
                environment.pop(name, None)
    environment.update(P4PC_CI_GL_MODE="hardware", P4PC_CI_HARDWARE_GL="1",
                       P4PC_CI_GL_RENDERER="__p4pc_intentionally_unmatched_ci_renderer__")
    result = subprocess.run(sys.argv[1:], env=environment, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, timeout=60)
    print(result.stdout, end="")
    if result.returncode != 1 or "CI requires hardware renderer" not in result.stdout:
        print(f"renderer_scope: CI renderer mismatch must fail, got {result.returncode}")
        return 1
    print("renderer_scope: another renderer cannot silently judge this port-frame reference")
    return 0


if __name__ == "__main__":
    sys.exit(main())
