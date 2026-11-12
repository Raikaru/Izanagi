package org.p4pc.game;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import org.libsdl.app.SDLActivity;

/**
 * The virtual pad: d-pad, the four face buttons, L1/R1/L2/R2 and Start/Select, drawn over the game surface. Each button injects the Android key
 * the port's keyboard binding for that PS2 button uses (src/platform/pad/pad_sdl.c kKeys: arrows, Z X C V, Enter, Backspace, Q E, 1 3), through SDL's
 * key entry points, so the touch pad needs no code of its own on the native side. Bluetooth and USB controllers reach the game through SDL's
 * gamepad support (pad_sdl.c) and hide this overlay.
 */
final class PadOverlay extends View {
    private static final int UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, CROSS = 4, CIRCLE = 5, TRIANGLE = 6, SQUARE = 7;
    private static final int START = 8, SELECT = 9, L1 = 10, R1 = 11, L2 = 12, R2 = 13, COUNT = 14;
    private static final int[] KEYS = {
        KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_RIGHT,
        KeyEvent.KEYCODE_Z, KeyEvent.KEYCODE_X, KeyEvent.KEYCODE_C, KeyEvent.KEYCODE_V,
        KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_DEL, KeyEvent.KEYCODE_Q, KeyEvent.KEYCODE_E, KeyEvent.KEYCODE_1, KeyEvent.KEYCODE_3,
    };

    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF[] rect = new RectF[COUNT];
    private boolean[] down = new boolean[COUNT];
    private float u, dpadX, dpadY, faceX, faceY;

    PadOverlay(Context c) {
        super(c);
        line.setStyle(Paint.Style.STROKE);
        line.setColor(0xCCFFFFFF);
        fill.setStyle(Paint.Style.FILL);
        for (int i = 0; i < COUNT; i++) rect[i] = new RectF();
    }

    static boolean hasGamepad() {
        for (int id : InputDevice.getDeviceIds()) {
            final InputDevice d = InputDevice.getDevice(id);
            if (d != null && (d.getSources() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD && !d.isVirtual()) {
                return true;
            }
        }
        return false;
    }
    static boolean hasKeyboard() {
        for (int id : InputDevice.getDeviceIds()) {
            final InputDevice d = InputDevice.getDevice(id);
            if (d == null || d.isVirtual()) continue;
            final int sources = d.getSources();
            final int type = d.getKeyboardType();
            final boolean controller =
                    (sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                    (sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
            if ((sources & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD &&
                    (type == InputDevice.KEYBOARD_TYPE_ALPHABETIC ||
                     (type == InputDevice.KEYBOARD_TYPE_NON_ALPHABETIC && !controller))) {
                return true;
            }
        }
        return false;
    }

    @Override
    protected void onSizeChanged(int w, int h, int ow, int oh) {
        // The Android picture is fitted to 4:3. On wide phones keep the pad in its
        // side bars rather than sizing it from the whole screen and covering play.
        final float gutter = Math.max(0f, (w - h * 4f / 3f) / 2f);
        final boolean sideBars = gutter >= h * 0.32f;
        u = sideBars ? Math.min(h / 10f, gutter / 6.4f) : Math.min(w, h * 1.7f) / 17f;
        dpadX = sideBars ? gutter / 2f : 3.2f * u;
        dpadY = h - (sideBars ? 4.4f : 3.4f) * u;
        faceX = w - dpadX;
        faceY = dpadY;
        line.setStrokeWidth(Math.max(2f, u * 0.08f));
        // shoulder buttons: two rows at the top corners, clear of the cutout
        rect[L1].set(dpadX - 1.4f * u, 0.4f * u, dpadX + 1.4f * u, 1.5f * u);
        rect[L2].set(dpadX - 1.4f * u, 1.7f * u, dpadX + 1.4f * u, 2.8f * u);
        rect[R1].set(faceX - 1.4f * u, 0.4f * u, faceX + 1.4f * u, 1.5f * u);
        rect[R2].set(faceX - 1.4f * u, 1.7f * u, faceX + 1.4f * u, 2.8f * u);
        final float selectX = sideBars ? dpadX : w / 2f - 1.8f * u;
        final float startX = sideBars ? faceX : w / 2f + 1.8f * u;
        rect[SELECT].set(selectX - 1.4f * u, h - 1.5f * u, selectX + 1.4f * u, h - 0.5f * u);
        rect[START].set(startX - 1.4f * u, h - 1.5f * u, startX + 1.4f * u, h - 0.5f * u);
    }

    /** Buttons under one finger. */
    private int hit(float x, float y) {
        int mask = 0;
        float dx = x - dpadX, dy = y - dpadY;
        float d = (float) Math.hypot(dx, dy);
        if (d < 2.6f * u && d > 0.35f * u) {
            if (dx < -0.4f * d) mask |= 1 << LEFT;
            if (dx > 0.4f * d) mask |= 1 << RIGHT;
            if (dy < -0.4f * d) mask |= 1 << UP;
            if (dy > 0.4f * d) mask |= 1 << DOWN;
        }
        final float[][] face = {{0, 1.5f, CROSS}, {1.5f, 0, CIRCLE}, {0, -1.5f, TRIANGLE}, {-1.5f, 0, SQUARE}};
        for (float[] f : face) {
            if (Math.hypot(x - (faceX + f[0] * u), y - (faceY + f[1] * u)) < 1.0f * u) mask |= 1 << (int) f[2];
        }
        for (int i = START; i < COUNT; i++) {
            if (rect[i].contains(x, y)) mask |= 1 << i;
        }
        return mask;
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        final int action = e.getActionMasked();
        final int lifted = (action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_UP) ? e.getActionIndex() : -1;
        int mask = 0;
        if (action != MotionEvent.ACTION_CANCEL) {
            for (int i = 0; i < e.getPointerCount(); i++) {
                if (i != lifted) mask |= hit(e.getX(i), e.getY(i));
            }
        }
        final boolean[] now = new boolean[COUNT];
        for (int i = 0; i < COUNT; i++) {
            now[i] = (mask & (1 << i)) != 0;
            if (now[i] != down[i]) {
                if (now[i]) SDLActivity.onNativeKeyDown(KEYS[i]);
                else SDLActivity.onNativeKeyUp(KEYS[i]);
            }
        }
        down = now;
        invalidate();
        return mask != 0 || action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP;
    }

    private void button(Canvas c, float cx, float cy, float r, boolean on) {
        fill.setColor(on ? 0x99FFFFFF : 0x33000000);
        c.drawCircle(cx, cy, r, fill);
        c.drawCircle(cx, cy, r, line);
    }

    @Override
    protected void onDraw(Canvas c) {
        // d-pad: a ring and four arrow wedges
        fill.setColor(0x22000000);
        c.drawCircle(dpadX, dpadY, 2.4f * u, fill);
        c.drawCircle(dpadX, dpadY, 2.4f * u, line);
        final int[] dir = {UP, DOWN, LEFT, RIGHT};
        final float[][] off = {{0, -1.5f}, {0, 1.5f}, {-1.5f, 0}, {1.5f, 0}};
        for (int i = 0; i < 4; i++) {
            final float cx = dpadX + off[i][0] * u, cy = dpadY + off[i][1] * u;
            fill.setColor(down[dir[i]] ? 0x99FFFFFF : 0x33000000);
            final Path p = new Path();
            final float s = 0.55f * u;
            if (i == 0) { p.moveTo(cx, cy - s); p.lineTo(cx - s, cy + s); p.lineTo(cx + s, cy + s); }
            else if (i == 1) { p.moveTo(cx, cy + s); p.lineTo(cx - s, cy - s); p.lineTo(cx + s, cy - s); }
            else if (i == 2) { p.moveTo(cx - s, cy); p.lineTo(cx + s, cy - s); p.lineTo(cx + s, cy + s); }
            else { p.moveTo(cx + s, cy); p.lineTo(cx - s, cy - s); p.lineTo(cx - s, cy + s); }
            p.close();
            c.drawPath(p, fill);
            c.drawPath(p, line);
        }
        // face buttons: cross (bottom), circle (right), triangle (top), square (left)
        final float r = 0.9f * u, k = 0.4f * u;
        button(c, faceX, faceY + 1.5f * u, r, down[CROSS]);
        c.drawLine(faceX - k, faceY + 1.5f * u - k, faceX + k, faceY + 1.5f * u + k, line);
        c.drawLine(faceX - k, faceY + 1.5f * u + k, faceX + k, faceY + 1.5f * u - k, line);
        button(c, faceX + 1.5f * u, faceY, r, down[CIRCLE]);
        c.drawCircle(faceX + 1.5f * u, faceY, k, line);
        button(c, faceX, faceY - 1.5f * u, r, down[TRIANGLE]);
        final Path tri = new Path();
        tri.moveTo(faceX, faceY - 1.5f * u - k); tri.lineTo(faceX - k, faceY - 1.5f * u + k * 0.8f); tri.lineTo(faceX + k, faceY - 1.5f * u + k * 0.8f); tri.close();
        c.drawPath(tri, line);
        button(c, faceX - 1.5f * u, faceY, r, down[SQUARE]);
        c.drawRect(faceX - 1.5f * u - k, faceY - k, faceX - 1.5f * u + k, faceY + k, line);
        // shoulders and start/select
        for (int i = START; i < COUNT; i++) {
            fill.setColor(down[i] ? 0x99FFFFFF : 0x33000000);
            c.drawRoundRect(rect[i], 0.4f * u, 0.4f * u, fill);
            c.drawRoundRect(rect[i], 0.4f * u, 0.4f * u, line);
        }
    }
}
