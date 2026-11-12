package org.p4pc.game;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.util.Log;
import android.widget.TextView;
import java.util.concurrent.atomic.AtomicBoolean;

/** Runs without SDL in :restart. A Binder death notification, not a delay or
 * Activity recreation, is the boundary between the old and new game images. */
public final class P4RestartActivity extends Activity {
    private static final AtomicBoolean relaunched = new AtomicBoolean();
    private IBinder game;
    private IBinder.DeathRecipient death;

    private boolean linked;
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        TextView status = new TextView(this);
        status.setText("Restarting Persona 4…");
        status.setPadding(32, 32, 32, 32);
        setContentView(status);
        Bundle extras = getIntent().getExtras();
        game = extras == null ? null : extras.getBinder("gameLifetime");
        Intent launch = getIntent().getParcelableExtra("gameIntent");
        if (game == null || launch == null) {
            Log.e("p4pc", "restart helper has no game lifetime/intent");
            finishAndRemoveTask();
            return;
        }
        Handler main = new Handler(Looper.getMainLooper());
        death = () -> main.post(() -> {
            if (!relaunched.compareAndSet(false, true)) return;
            Log.i("p4pc", "restart: old game process exited; launching fresh game");
            launch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
            try {
                startActivity(launch);
            } catch (RuntimeException failure) {
                Log.e("p4pc", "restart: cannot relaunch game", failure);
            } finally {
                finishAndRemoveTask();
                // This process owns no SDL/game state and has no further work.
                android.os.Process.killProcess(android.os.Process.myPid());
            }
        });
        try {
            game.linkToDeath(death, 0);
            linked = true;
        } catch (RemoteException alreadyExited) {
            death.binderDied();
        }
    }

    @Override protected void onDestroy() {
        if (linked) game.unlinkToDeath(death, 0);
        super.onDestroy();
    }
}
