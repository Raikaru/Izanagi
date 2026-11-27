package org.p4pc.game;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/**
 * The zip of "Export logs" (LogExport gathers the device facts, this class is the part that needs no Android classes, so a host JDK tests it):
 * device.txt first (the facts, then a listing of the files with sizes and dates), then every log that has content, then the settings
 * without the lines that name a path. A log longer than the limit keeps its last part, the recent one, with a note where it starts.
 */
final class LogBundle {
    /** The files of the files directory that are logs, in the order they are written. */
    static final String[] LOGS = {"boot.log", "boot.prev.log", "boot.unclean.log", "p4pc-crash.log"};

    private LogBundle() {}

    /** The settings file's text without the values of keys that hold a path (the disc image, a folder), which are a user's own. */
    static String filterSettings(String text) {
        final StringBuilder out = new StringBuilder();
        for (String line : text.split("\r?\n", -1)) {
            final int eq = line.indexOf('=');
            final String key = (eq > 0 ? line.substring(0, eq) : line).trim().toLowerCase(Locale.US);
            if (eq > 0 && !key.startsWith("#") && !key.startsWith(";")
                    && (key.contains("iso") || key.contains("install") || key.contains("dir") || key.contains("path") || key.contains("root"))) {
                out.append(key).append(" = (not exported)\n");
            } else {
                out.append(line).append('\n');
            }
        }
        // split keeps a final empty piece for a text that ends in a newline: that made one newline too many
        if (out.length() > 0 && text.endsWith("\n")) out.setLength(out.length() - 1);
        return out.toString();
    }

    /** Writes the zip to `out` (not closed) and returns the names of its entries in order. */
    static List<String> write(OutputStream out, File filesDir, String deviceFacts, File settings, long tailLimit) throws IOException {
        final List<String> names = new ArrayList<>();
        final ZipOutputStream zip = new ZipOutputStream(out);
        final StringBuilder listing = new StringBuilder("\nFiles:\n");
        for (String name : LOGS) {
            final File f = new File(filesDir, name);
            listing.append(String.format(Locale.US, "%-18s %10d bytes  modified %s%n", name, f.length(),
                    f.exists() ? new SimpleDateFormat("yyyy-MM-dd HH:mm:ss Z", Locale.US).format(new Date(f.lastModified())) : "(not there)"));
        }
        putText(zip, names, "device.txt", deviceFacts + listing);
        for (String name : LOGS) {
            final File f = new File(filesDir, name);
            if (f.length() > 0) putLog(zip, names, name, f, tailLimit);
        }
        if (settings != null && settings.isFile()) {
            final StringBuilder text = new StringBuilder();
            try (BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(settings), StandardCharsets.UTF_8))) {
                final char[] buf = new char[8192];
                int n;
                while ((n = in.read(buf)) > 0 && text.length() < (1 << 20)) text.append(buf, 0, n);
            }
            putText(zip, names, "p4pc.ini", filterSettings(text.toString()));
        }
        zip.finish();
        zip.flush();
        return names;
    }

    private static void putText(ZipOutputStream zip, List<String> names, String name, String text) throws IOException {
        zip.putNextEntry(new ZipEntry(name));
        zip.write(text.getBytes(StandardCharsets.UTF_8));
        zip.closeEntry();
        names.add(name);
    }

    private static void putLog(ZipOutputStream zip, List<String> names, String name, File file, long tailLimit) throws IOException {
        zip.putNextEntry(new ZipEntry(name));
        try (InputStream in = new FileInputStream(file)) {
            final long skip = file.length() - tailLimit;
            if (skip > 0) {
                for (long left = skip; left > 0; ) {
                    final long done = in.skip(left);
                    if (done <= 0) break;
                    left -= done;
                }
                zip.write(("[the first " + skip + " bytes of this log are left out]\n").getBytes(StandardCharsets.UTF_8));
            }
            final byte[] buf = new byte[16384];
            int n;
            while ((n = in.read(buf)) > 0) zip.write(buf, 0, n);
        }
        zip.closeEntry();
        names.add(name);
    }
}
