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
    environment["P4PC_CI_HARDWARE_GL"] = "1"
    environment["P4PC_CI_GL_RENDERER"] = "__p4pc_intentionally_unmatched_ci_renderer__"
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
