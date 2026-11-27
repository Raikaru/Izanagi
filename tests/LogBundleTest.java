package org.p4pc.game;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/** The zip of "Export logs" (LogBundle): which files are in it, in which order, what is left out. Needs no device. */
final class LogBundleTest {
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    private static void put(File dir, String name, String text) throws Exception {
        Files.write(new File(dir, name).toPath(), text.getBytes(StandardCharsets.UTF_8));
    }

    private static Map<String, String> read(byte[] zip) throws Exception {
        final Map<String, String> entries = new LinkedHashMap<>();
        try (ZipInputStream in = new ZipInputStream(new ByteArrayInputStream(zip))) {
            ZipEntry e;
            while ((e = in.getNextEntry()) != null) {
                entries.put(e.getName(), new String(in.readAllBytes(), StandardCharsets.UTF_8));
            }
        }
        return entries;
    }

    public static void main(String[] args) throws Exception {
        final File dir = Files.createTempDirectory("p4pc-logs").toFile();
        put(dir, "boot.log", "p4diag: build: v0.2\nrun two\np4diag: clean exit\n");
        put(dir, "boot.prev.log", "run one\n");
        put(dir, "p4pc-crash.log", "p4pc: CRASH signal 11\n");
        put(dir, "boot.unclean.log", ""); // empty: not exported
        put(dir, "saves.bin", "never");   // not a log
        put(dir, "p4pc.ini", "# Disc image: iso = /x\n[game]\niso = /storage/emulated/0/Persona 4.iso\n[video]\nresolution = scale:2\n[golden]\ninstall = /home/u/Golden\n");

        final ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        final List<String> names = LogBundle.write(bytes, dir, "app: org.p4pc.game 0.2 (code 5)\ndevice: Test Phone\n", new File(dir, "p4pc.ini"), 1 << 20);
        final Map<String, String> zip = read(bytes.toByteArray());
        check(new ArrayList<>(zip.keySet()).equals(names), "the returned names are the zip's entries, in order: " + names + " vs " + zip.keySet());
        check(names.equals(List.of("device.txt", "boot.log", "boot.prev.log", "p4pc-crash.log", "p4pc.ini")), "entries: " + names);
        check(zip.get("device.txt").startsWith("app: org.p4pc.game 0.2 (code 5)\ndevice: Test Phone\n"), "device facts first");
        check(zip.get("device.txt").contains("boot.log") && zip.get("device.txt").contains("boot.unclean.log") && zip.get("device.txt").contains("p4pc-crash.log"), "the listing names every log");
        check(zip.get("boot.log").equals("p4diag: build: v0.2\nrun two\np4diag: clean exit\n"), "log bytes are kept");
        final String ini = zip.get("p4pc.ini");
        check(ini.contains("resolution = scale:2") && ini.contains("[video]"), "ordinary settings are kept");
        check(!ini.contains("/storage") && !ini.contains("/home/u") && ini.contains("iso = (not exported)") && ini.contains("install = (not exported)"),
                "paths are left out: " + ini);
        check(ini.contains("# Disc image: iso = /x"), "a comment is not a key");

        // a log longer than the limit keeps its end and says so
        final StringBuilder big = new StringBuilder();
        for (int i = 0; i < 1000; i++) big.append("line ").append(i).append('\n');
        put(dir, "boot.log", big.toString());
        final ByteArrayOutputStream cut = new ByteArrayOutputStream();
        LogBundle.write(cut, dir, "facts\n", null, 2000);
        final String tail = read(cut.toByteArray()).get("boot.log");
        check(tail.startsWith("[the first ") && tail.endsWith("line 999\n") && tail.length() < 2200, "a long log is cut to its end: " + tail.length());
        check(!read(cut.toByteArray()).containsKey("p4pc.ini"), "no settings file, no entry");

        // no logs at all (nothing has run yet): the facts alone are still a valid zip
        final File empty = Files.createTempDirectory("p4pc-nologs").toFile();
        final ByteArrayOutputStream none = new ByteArrayOutputStream();
        check(LogBundle.write(none, empty, "facts\n", null, 1000).equals(List.of("device.txt")), "facts only");
        check(read(none.toByteArray()).get("device.txt").contains("(not there)"), "the listing says a log is not there");
        System.out.println("LogBundleTest: ok");
    }
}
