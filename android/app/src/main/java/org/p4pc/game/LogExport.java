package org.p4pc.game;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.os.Build;
import android.os.PowerManager;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * "Export logs": the game's logs and the facts of the device in one zip the player can save to a visible folder (the system file dialog) and attach to a bug report.
 * The logs live in the app's private files directory, which a normal install cannot open:
 *   boot.log            the last run (the game's stdout/stderr, including the diagnostic header and the frame/audio statistics)
 *   boot.prev.log       the run before it; boot.unclean.log: the last run that did not end by itself
 *   p4pc-crash.log      the last native crash report
 *   p4pc.ini            the settings (the lines that name a path are left out)
 *   device.txt          this app's version, the device, the screen, memory, power state
 * Saves, the disc image and the game's files are never included.
 */
final class LogExport {
    private static final long TAIL_LIMIT = 4L << 20; // a log longer than this is cut to its last 4 MiB

    private LogExport() {}

    /** The suggested name of the zip, e.g. p4pc-logs-20261123-0914.zip. */
    static String suggestedName() {
        return "p4pc-logs-" + new SimpleDateFormat("yyyyMMdd-HHmm", Locale.US).format(new Date()) + ".zip";
    }

    /** True when there is anything to export (a run has happened). */
    static boolean hasLogs(Context context) {
        for (String name : LogBundle.LOGS) {
            if (new File(context.getFilesDir(), name).length() > 0) return true;
        }
        return false;
    }

    /** Writes the zip to `out` and returns the names it holds, in order. The stream is not closed. */
    static List<String> write(Context context, OutputStream out) throws IOException {
        final File files = context.getFilesDir();
        return LogBundle.write(out, files, device(context), findSettings(files, 0), TAIL_LIMIT);
    }

    /** p4pc.ini: SDL puts the settings in the files directory; look there and one or two folders down, in case it adds the organization and app names. */
    private static File findSettings(File dir, int depth) {
        final File direct = new File(dir, "p4pc.ini");
        if (direct.isFile()) return direct;
        final File[] children = depth < 3 ? dir.listFiles() : null;
        if (children == null) return null;
        Arrays.sort(children);
        for (File child : children) {
            if (child.isDirectory() && !child.getName().equals("saves")) {
                final File found = findSettings(child, depth + 1);
                if (found != null) return found;
            }
        }
        return null;
    }

    private static String device(Context context) {
        final StringBuilder sb = new StringBuilder();
        sb.append("Persona 4 PC (Izanagi) log export, ")
                .append(new SimpleDateFormat("yyyy-MM-dd HH:mm:ss Z", Locale.US).format(new Date())).append('\n');
        try {
            final PackageInfo info = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            final long code = Build.VERSION.SDK_INT >= 28 ? info.getLongVersionCode() : info.versionCode;
            sb.append("app: ").append(context.getPackageName()).append(' ').append(info.versionName).append(" (code ").append(code).append(")\n");
        } catch (Exception e) {
            sb.append("app: version unknown (").append(e).append(")\n");
        }
        sb.append("device: ").append(Build.MANUFACTURER).append(' ').append(Build.MODEL).append(" (").append(Build.DEVICE).append("), hardware ")
                .append(Build.HARDWARE).append(", board ").append(Build.BOARD).append('\n');
        if (Build.VERSION.SDK_INT >= 31) sb.append("soc: ").append(Build.SOC_MANUFACTURER).append(' ').append(Build.SOC_MODEL).append('\n');
        sb.append("android: ").append(Build.VERSION.RELEASE).append(" (API ").append(Build.VERSION.SDK_INT).append("), security patch ")
                .append(Build.VERSION.SECURITY_PATCH).append('\n');
        sb.append("abis: ").append(Arrays.toString(Build.SUPPORTED_ABIS)).append(", cores ").append(Runtime.getRuntime().availableProcessors()).append('\n');
        try {
            final ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
            final ActivityManager.MemoryInfo mem = new ActivityManager.MemoryInfo();
            am.getMemoryInfo(mem);
            sb.append("memory: ").append(mem.totalMem >> 20).append(" MiB total, ").append(mem.availMem >> 20).append(" MiB free now, low-memory state ")
                    .append(mem.lowMemory).append('\n');
        } catch (Exception e) {
            sb.append("memory: unknown\n");
        }
        try {
            final WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
            final Display display = wm.getDefaultDisplay();
            final DisplayMetrics metrics = new DisplayMetrics();
            display.getRealMetrics(metrics);
            sb.append("screen: ").append(metrics.widthPixels).append('x').append(metrics.heightPixels).append(" px, density ").append(metrics.densityDpi)
                    .append(" dpi, ").append(display.getRefreshRate()).append(" Hz\n");
        } catch (Exception e) {
            sb.append("screen: unknown\n");
        }
        try {
            final PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            sb.append("power now: battery saver ").append(pm.isPowerSaveMode());
            if (Build.VERSION.SDK_INT >= 29) sb.append(", thermal status ").append(pm.getCurrentThermalStatus()).append(" (0 none ... 3 severe ... 6 shutdown)");
            sb.append('\n');
        } catch (Exception e) {
            sb.append("power: unknown\n");
        }
        return sb.toString();
    }
}
