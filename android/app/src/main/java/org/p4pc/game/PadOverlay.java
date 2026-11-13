package org.p4pc.game;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import org.libsdl.app.SDLActivity;

/**
 * The virtual pad: d-pad, a floating analog stick, the four face buttons, L1/R1/L2/R2, Start/Select and CONFIG, drawn over the game surface. Every
 * pill carries its label. Each button injects the Android key the port's keyboard binding for that PS2 button uses (src/platform/pad/pad_sdl.c
 * kKeys: arrows, Z X C V, Enter, Backspace, Q E, 1 3; CONFIG is F1, which opens the native settings like on a keyboard), through SDL's key entry
 * points, so those buttons need no code of their own on the native side. The stick is analog: the thumb's offset from where it touched goes to
 * P4Activity.nativeTouchStick, which pad_sdl.c merges into the game's left stick (dead zone, input.deadzone and all). Bluetooth and USB controllers
 * reach the game through SDL's gamepad support (pad_sdl.c) and hide this overlay.
 */
final class PadOverlay extends View {
    private static final int UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, CROSS = 4, CIRCLE = 5, TRIANGLE = 6, SQUARE = 7;
    private static final int START = 8, SELECT = 9, L1 = 10, R1 = 11, L2 = 12, R2 = 13, MENU = 14, COUNT = 15;
    private static final int[] KEYS = {
        KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_RIGHT,
        KeyEvent.KEYCODE_Z, KeyEvent.KEYCODE_X, KeyEvent.KEYCODE_C, KeyEvent.KEYCODE_V,
        KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_DEL, KeyEvent.KEYCODE_Q, KeyEvent.KEYCODE_E, KeyEvent.KEYCODE_1, KeyEvent.KEYCODE_3,
        KeyEvent.KEYCODE_F1,
    };
    /** The pills' labels, indexed from START. */
    private static final String[] LABELS = { "START", "SELECT", "L1", "R1", "L2", "R2", "CONFIG" };
    private static final String PREFS = "pad", PREF_STICK = "stick";

    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF[] rect = new RectF[COUNT];
    private boolean[] down = new boolean[COUNT];
    private float u, dpadX, dpadY, faceX, faceY;

    // The left stick floats inside stickZone: the thumb's first contact is its centre, the knob follows within stickR of it.
    private final RectF stickZone = new RectF();
    private final RectF stickToggle = new RectF();
    private boolean sideBars;
    /** Without side bars there is room for one left control only: the d-pad or the stick, switched by the toggle pill. */
    private boolean stickMode;
    private float stickR, stickIdleX, stickIdleY, stickBaseX, stickBaseY, stickKnobX, stickKnobY, sentX, sentY;
    private int stickId = -1;

    PadOverlay(Context c) {
        super(c);
        line.setStyle(Paint.Style.STROKE);
        line.setColor(0xCCFFFFFF);
        fill.setStyle(Paint.Style.FILL);
        text.setTextAlign(Paint.Align.CENTER);
        text.setTypeface(Typeface.DEFAULT_BOLD);
        for (int i = 0; i < COUNT; i++) rect[i] = new RectF();
        stickMode = c.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getBoolean(PREF_STICK, false);
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

    private boolean showDpad() { return sideBars || !stickMode; }
    private boolean showStick() { return sideBars || stickMode; }

    @Override
    protected void onSizeChanged(int w, int h, int ow, int oh) {
        // The Android picture is fitted to 4:3. On wide phones keep the pad in its
        // side bars rather than sizing it from the whole screen and covering play.
        final float gutter = Math.max(0f, (w - h * 4f / 3f) / 2f);
        sideBars = gutter >= h * 0.32f;
        // Side bars: the left bar stacks L1, L2, the stick zone, the d-pad and Select within 14.4 u of height (a 20:9 phone's bar is 6.4 u wide).
        u = sideBars ? Math.min(h / 14.4f, gutter / 6.4f) : Math.min(w, h * 1.7f) / 17f;
        dpadX = sideBars ? gutter / 2f : 3.2f * u;
        dpadY = h - (sideBars ? 4.4f : 3.4f) * u;
        faceX = w - dpadX;
        faceY = dpadY;
        line.setStrokeWidth(Math.max(2f, u * 0.08f));
        text.setTextSize(0.46f * u);
        // shoulder buttons: two rows at the top corners, clear of the cutout
        rect[L1].set(dpadX - 1.4f * u, 0.4f * u, dpadX + 1.4f * u, 1.5f * u);
        rect[L2].set(dpadX - 1.4f * u, 1.7f * u, dpadX + 1.4f * u, 2.8f * u);
        rect[R1].set(faceX - 1.4f * u, 0.4f * u, faceX + 1.4f * u, 1.5f * u);
        rect[R2].set(faceX - 1.4f * u, 1.7f * u, faceX + 1.4f * u, 2.8f * u);
        // CONFIG under R2, above the face buttons, in the same column as the shoulders
        rect[MENU].set(faceX - 1.4f * u, 3.0f * u, faceX + 1.4f * u, 4.1f * u);
        final float selectX = sideBars ? dpadX : w / 2f - 1.8f * u;
        final float startX = sideBars ? faceX : w / 2f + 1.8f * u;
        rect[SELECT].set(selectX - 1.4f * u, h - 1.5f * u, selectX + 1.4f * u, h - 0.5f * u);
        rect[START].set(startX - 1.4f * u, h - 1.5f * u, startX + 1.4f * u, h - 0.5f * u);
        stickR = 1.8f * u;
        if (sideBars) {
            // between L2 and the d-pad's ring, over the whole bar width
            stickZone.set(0f, 3.0f * u, gutter, dpadY - 2.6f * u);
            stickIdleX = dpadX;
            stickIdleY = (stickZone.top + stickZone.bottom) / 2f;
            stickToggle.setEmpty();
        } else {
            // the stick takes the d-pad's place; the toggle sits under L2 like CONFIG under R2
            stickZone.set(0f, dpadY - 2.6f * u, dpadX + 2.6f * u, h);
            stickIdleX = dpadX;
            stickIdleY = dpadY;
            stickToggle.set(dpadX - 1.4f * u, 3.0f * u, dpadX + 1.4f * u, 4.1f * u);
        }
        releaseStick();
    }

    @Override
    protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (visibility != View.VISIBLE) releaseStick();
    }

    @Override
    protected void onDetachedFromWindow() {
        releaseStick();
        super.onDetachedFromWindow();
    }

    /** Buttons under one finger. */
    private int hit(float x, float y) {
        int mask = 0;
        if (showDpad()) {
            float dx = x - dpadX, dy = y - dpadY;
            float d = (float) Math.hypot(dx, dy);
            if (d < 2.6f * u && d > 0.35f * u) {
                if (dx < -0.4f * d) mask |= 1 << LEFT;
                if (dx > 0.4f * d) mask |= 1 << RIGHT;
                if (dy < -0.4f * d) mask |= 1 << UP;
                if (dy > 0.4f * d) mask |= 1 << DOWN;
            }
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

    /** Sends the stick's offset (units of its travel, y down) once per change; the native side applies its dead zone. */
    private void sendStick(float x, float y) {
        if (x == sentX && y == sentY) return;
        sentX = x;
        sentY = y;
        P4Activity.nativeTouchStick(x, y);
    }

    private void releaseStick() {
        stickId = -1;
        sendStick(0f, 0f);
    }

    private void moveStick(float x, float y) {
        float dx = x - stickBaseX, dy = y - stickBaseY;
        final float d = (float) Math.hypot(dx, dy);
        if (d > stickR) {
            dx *= stickR / d;
            dy *= stickR / d;
        }
        stickKnobX = stickBaseX + dx;
        stickKnobY = stickBaseY + dy;
        sendStick(dx / stickR, dy / stickR);
    }

    private void setStickMode(boolean on) {
        stickMode = on;
        getContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().putBoolean(PREF_STICK, on).apply();
        releaseStick();
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        final int action = e.getActionMasked();
        final int index = e.getActionIndex();
        final boolean pointerUp = action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_UP;
        final int lifted = pointerUp ? index : -1;
        boolean claimed = false;
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            final float x = e.getX(index), y = e.getY(index);
            if (!sideBars && stickToggle.contains(x, y)) {
                setStickMode(!stickMode);
                claimed = true;
            } else if (stickId < 0 && showStick() && stickZone.contains(x, y)) {
                stickId = e.getPointerId(index);
                stickBaseX = stickKnobX = x;
                stickBaseY = stickKnobY = y;
                claimed = true;
            }
        }
        if (stickId >= 0) {
            final int s = e.findPointerIndex(stickId);
            if (action == MotionEvent.ACTION_CANCEL || s < 0 || s == lifted) releaseStick();
            else moveStick(e.getX(s), e.getY(s));
        }
        int mask = 0;
        if (action != MotionEvent.ACTION_CANCEL) {
            for (int i = 0; i < e.getPointerCount(); i++) {
                if (i != lifted && e.getPointerId(i) != stickId) mask |= hit(e.getX(i), e.getY(i));
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
        return mask != 0 || claimed || stickId >= 0 || pointerUp;
    }

    private void button(Canvas c, float cx, float cy, float r, boolean on) {
        fill.setColor(on ? 0x99FFFFFF : 0x33000000);
        c.drawCircle(cx, cy, r, fill);
        c.drawCircle(cx, cy, r, line);
    }

    /** A labelled pill; the label goes dark while it is held so it stays readable on the lit fill. */
    private void pill(Canvas c, RectF r, String label, boolean on) {
        fill.setColor(on ? 0x99FFFFFF : 0x33000000);
        c.drawRoundRect(r, 0.4f * u, 0.4f * u, fill);
        c.drawRoundRect(r, 0.4f * u, 0.4f * u, line);
        text.setColor(on ? 0xFF000000 : 0xE6FFFFFF);
        text.setTextSize(0.46f * u);
        final Paint.FontMetrics m = text.getFontMetrics();
        c.drawText(label, r.centerX(), r.centerY() - (m.ascent + m.descent) / 2f, text);
    }

    private void drawStick(Canvas c) {
        final boolean active = stickId >= 0;
        final float bx = active ? stickBaseX : stickIdleX, by = active ? stickBaseY : stickIdleY;
        final float kx = active ? stickKnobX : stickIdleX, ky = active ? stickKnobY : stickIdleY;
        fill.setColor(0x22000000);
        c.drawCircle(bx, by, stickR, fill);
        c.drawCircle(bx, by, stickR, line);
        fill.setColor(active ? 0x99FFFFFF : 0x33000000);
        c.drawCircle(kx, ky, 0.8f * u, fill);
        c.drawCircle(kx, ky, 0.8f * u, line);
        if (!active) {
            text.setColor(0xB3FFFFFF);
            text.setTextSize(0.34f * u);
            final Paint.FontMetrics m = text.getFontMetrics();
            c.drawText("STICK", kx, ky - (m.ascent + m.descent) / 2f, text);
        }
    }

    @Override
    protected void onDraw(Canvas c) {
        if (showStick()) drawStick(c);
        if (showDpad()) {
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
        // shoulders, Start/Select and CONFIG
        for (int i = START; i < COUNT; i++) pill(c, rect[i], LABELS[i - START], down[i]);
        if (!sideBars) pill(c, stickToggle, stickMode ? "D-PAD" : "STICK", false);
    }
}
