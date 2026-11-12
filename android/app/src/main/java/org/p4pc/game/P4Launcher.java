package org.p4pc.game;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import android.app.AlertDialog;
import java.io.File;
/**
 * Setup screen: the user picks the disc image (and, optionally, the game executable) through the Storage Access Framework document
 * picker. The grants are persisted, so the next launch starts without asking. Nothing is copied; the game reads the picked documents
 * through descriptors (P4Activity). A launch with an `iso` extra (tests) skips this screen and forwards its extras to the game.
 */
public class P4Launcher extends Activity {
    private static final int REQ_ISO = 1;
    private static final int REQ_ELF = 2;
    private static final String PREFS = "p4pc";

    private SharedPreferences prefs;
    private TextView status;
    private Button start;

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        prefs = getSharedPreferences(PREFS, MODE_PRIVATE);
        final File crash = new File(getFilesDir(), "p4pc-crash.log");
        if (crash.length() > 0 && prefs.getLong("crashShown", 0) != crash.lastModified()) {
            new AlertDialog.Builder(this).setTitle("Previous native crash")
                    .setMessage("The previous run stopped unexpectedly. Crash details are saved at:\n"
                            + crash.getAbsolutePath() + "\n\nInclude this file and boot.log in a bug report.")
                    .setPositiveButton("OK", (dialog, which) ->
                            prefs.edit().putLong("crashShown", crash.lastModified()).apply()).show();
        }

        final Intent in = getIntent();
        if (in.hasExtra("iso")) {
            launch(in.getStringExtra("iso"), in.getStringExtra("elf"), in);
            return;
        }

        final LinearLayout col = new LinearLayout(this);
        col.setOrientation(LinearLayout.VERTICAL);
        col.setGravity(Gravity.CENTER);
        col.setPadding(48, 48, 48, 48);
        final TextView title = new TextView(this);
        title.setText("Persona 4");
        title.setTextSize(28);
        col.addView(title);
        status = new TextView(this);
        status.setPadding(0, 32, 0, 32);
        col.addView(status);
        col.addView(button("Choose disc image (.iso)", REQ_ISO));
        col.addView(button("Choose SLUS_217.82 (optional)", REQ_ELF));
        start = new Button(this);
        start.setText("Start");
        start.setOnClickListener(v -> launch(prefs.getString("iso", null), prefs.getString("elf", null), getIntent()));
        col.addView(start);
        setContentView(col);
        refresh();
    }

    private Button button(String label, int req) {
        final Button btn = new Button(this);
        btn.setText(label);
        btn.setOnClickListener(v -> {
            final Intent pick = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            pick.addCategory(Intent.CATEGORY_OPENABLE);
            pick.setType("*/*");
            pick.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            startActivityForResult(pick, req);
        });
        return btn;
    }

    @Override
    protected void onActivityResult(int req, int result, Intent data) {
        super.onActivityResult(req, result, data);
        if (result != RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        final Uri uri = data.getData();
        getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        prefs.edit().putString(req == REQ_ISO ? "iso" : "elf", uri.toString()).apply();
        refresh();
    }

    private boolean readable(String uri) {
        if (uri == null) return false;
        try (ParcelFileDescriptor fd = getContentResolver().openFileDescriptor(Uri.parse(uri), "r")) {
            return fd != null;
        } catch (Exception e) {
            return false;
        }
    }

    private void refresh() {
        final boolean iso = readable(prefs.getString("iso", null));
        final boolean elf = readable(prefs.getString("elf", null));
        status.setText("Disc image: " + (iso ? "ready" : "not chosen") + "\nExecutable: " + (elf ? "ready" : "from the disc image")
                + "\nSaves: " + getFilesDir() + "/saves");
        start.setEnabled(iso);
    }

    private void launch(String iso, String elf, Intent from) {
        final Intent g = new Intent(this, P4Activity.class);
        if (from != null && from.getExtras() != null) {
            g.putExtras(from.getExtras());
        }
        g.putExtra("iso", iso);
        if (elf != null) g.putExtra("elf", elf);
        startActivity(g);
        finish();
    }
}
