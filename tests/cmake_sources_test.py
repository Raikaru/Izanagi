"""Exercise the real root include driver with private CMake/Ninja projects."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MODULES = {
    "010-project.cmake": '''file(WRITE "${CMAKE_BINARY_DIR}/events.txt" "project\\n")
set(OWNER_VALUE shared)
add_library(parts OBJECT one.c)
target_compile_definitions(parts PRIVATE OWNER_NUMBER=7)
function(require_owner)
  if(NOT OWNER_VALUE STREQUAL "shared")
    message(FATAL_ERROR "fragment lost caller scope")
  endif()
endfunction()
''',
    "020-owner.cmake": '''require_owner()
target_sources(parts PRIVATE two.c)
set_source_files_properties(two.c PROPERTIES COMPILE_DEFINITIONS LOCAL_NUMBER=9)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/source-order.txt"
  CONTENT "$<JOIN:$<TARGET_PROPERTY:parts,SOURCES>,\\n>\\n")
file(APPEND "${CMAKE_BINARY_DIR}/events.txt" "owner\\n")
''',
    "030-android32.cmake": '''if(P4_ANDROID AND NOT P4_ANDROID64)
  file(APPEND "${CMAKE_BINARY_DIR}/events.txt" "android32\\n")
  return()
endif()
''',
    "040-profile.cmake": '''if(P4_ANDROID64)
  file(APPEND "${CMAKE_BINARY_DIR}/events.txt" "android64\\n")
  return()
endif()
file(APPEND "${CMAKE_BINARY_DIR}/events.txt" "desktop\\n")
''',
    "050-player.cmake": '''add_executable(sample main.c $<TARGET_OBJECTS:parts>)
file(APPEND "${CMAKE_BINARY_DIR}/events.txt" "player\\n")
''',
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class SourceFragmentsTest(unittest.TestCase):
    def command(self, *args):
        result = subprocess.run(args, cwd=self.root, capture_output=True, text=True,
                                env=dict(os.environ, P4PC_INPUT="script"), timeout=60)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result

    def objects(self):
        directory = self.build / "CMakeFiles/parts.dir"
        return {path.relative_to(directory).as_posix(): digest(path)
                for path in directory.rglob("*") if path.suffix in (".o", ".obj")}

    def configure(self, android, android64):
        self.command("cmake", "-S", str(self.root), "-B", str(self.build), "-G", "Ninja",
                     "-DCMAKE_C_COMPILER=" + self.compiler, "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                     "-DP4_ANDROID=" + android, "-DP4_ANDROID64=" + android64)

    def test_plain_python_tool_fixtures_share_host_lock(self):
        with tempfile.TemporaryDirectory(prefix="tooling cmake ") as temporary:
            self.root = Path(temporary)
            self.build = self.root / "build"
            prelude = (ROOT / "tests/cmake/000-registration.cmake").read_text().split("\n# Host tests.", 1)[0]
            (self.root / "registration.cmake").write_text(prelude, encoding="utf-8")
            (self.root / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.20)\nproject(tooling NONE)\nenable_testing()\n'
                'set(P4_PYTHON "' + Path(sys.executable).as_posix() + '")\n'
                'include("${CMAKE_CURRENT_SOURCE_DIR}/registration.cmake")\n'
                'add_test(NAME tooling COMMAND "${P4_PYTHON}" future_fixture.py)\n'
                'add_test(NAME non_tool COMMAND "${CMAKE_COMMAND}" -E true)\n'
                'get_test_property(tooling RESOURCE_LOCK TOOL_LOCK)\n'
                'get_test_property(non_tool RESOURCE_LOCK OTHER_LOCK)\n'
                'if(NOT TOOL_LOCK STREQUAL "host-tooling" OR NOT OTHER_LOCK STREQUAL "NOTFOUND")\n'
                '  message(FATAL_ERROR "unexpected locks ${TOOL_LOCK}, ${OTHER_LOCK}")\n'
                'endif()\n', encoding="utf-8")
            self.command("cmake", "-S", str(self.root), "-B", str(self.build))

    def test_order_scope_outputs_and_both_original_project_exits(self):
        compiler = shutil.which("gcc") or shutil.which("cc")
        self.assertIsNotNone(compiler, "a native host C compiler is required")
        wrapper = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        expected = {
            "desktop": ("OFF", "OFF", "project\nowner\ndesktop\nplayer\n"),
            "android32": ("ON", "OFF", "project\nowner\nandroid32\n"),
            "android64": ("ON", "ON", "project\nowner\nandroid64\n"),
        }
        for mode, (android, android64, events) in expected.items():
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="source cmake ") as temporary:
                self.root = Path(temporary)
                self.build = self.root / "build"
                self.compiler = compiler
                (self.root / "one.c").write_text("int one(void) { return OWNER_NUMBER; }\n")
                (self.root / "two.c").write_text("int two(void) { return LOCAL_NUMBER; }\n")
                (self.root / "main.c").write_text(
                    "int one(void); int two(void); int main(void) { return one() + two() != 16; }\n")
                project = self.root / "CMakeLists.txt"
                monolithic = "\n".join(MODULES.values()).replace(
                    "add_library(parts OBJECT one.c)", "add_library(parts OBJECT one.c two.c)").replace(
                    "target_sources(parts PRIVATE two.c)\n", "")
                project.write_text("cmake_minimum_required(VERSION 3.20)\nproject(p4pc C)\n" +
                                   monolithic, encoding="utf-8")
                self.configure(android, android64)
                target = "sample" if mode == "desktop" else "parts"
                self.command("cmake", "--build", str(self.build), "--target", target, "--parallel", "1")
                before_objects = self.objects()
                before_commands = json.loads((self.build / "compile_commands.json").read_text())
                self.assertEqual(len(before_objects), 2)
                self.assertEqual((self.build / "source-order.txt").read_text(), "one.c\ntwo.c\n")
                self.assertEqual((self.build / "events.txt").read_text(), events)
                executable = self.build / ("sample.exe" if os.name == "nt" else "sample")
                before_executable = digest(executable) if mode == "desktop" else None
                if mode == "desktop":
                    self.command(str(executable))
                fragments = self.root / "cmake/sources"
                fragments.mkdir(parents=True)
                for name, text in MODULES.items():
                    # The two old file exits must also stop the root's include loop.
                    text = text.replace("  return()", "  set(_p4_source_stop TRUE)\n  return()")
                    (fragments / name).write_text(text, encoding="utf-8")
                project.write_text(wrapper, encoding="utf-8")
                self.configure(android, android64)
                self.command("cmake", "--build", str(self.build), "--target", target, "--parallel", "1")
                self.assertEqual(self.objects(), before_objects)
                self.assertEqual(json.loads((self.build / "compile_commands.json").read_text()), before_commands)
                self.assertEqual((self.build / "source-order.txt").read_text(), "one.c\ntwo.c\n")
                self.assertEqual((self.build / "events.txt").read_text(), events)
                if mode == "desktop":
                    self.assertEqual(digest(executable), before_executable)
                    self.command(str(executable))
                    (self.root / "three.c").write_text("int three(void) { return 3; }\n")
                    (fragments / "025-new-owner.cmake").write_text(
                        "target_sources(parts PRIVATE three.c)\n", encoding="utf-8")
                    # No explicit configure: the root's CONFIGURE_DEPENDS must find the new owner.
                    self.command("cmake", "--build", str(self.build), "--target", target, "--parallel", "1")
                    self.assertEqual((self.build / "source-order.txt").read_text(), "one.c\ntwo.c\nthree.c\n")
                    after_addition = self.objects()
                    self.assertEqual(len(after_addition), 3)
                    for name, value in before_objects.items():
                        self.assertEqual(after_addition[name], value)



if __name__ == "__main__":
    unittest.main()