package org.p4pc.game;

import android.content.Context;
import android.app.Activity;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.os.Build;
import android.view.DisplayCutout;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.view.WindowInsets;

import org.libsdl.app.SDLActivity;

/**
 * The virtual pad: d-pad, an analog stick, the four face buttons, L1/R1/L2/R2, Start/Select and CONFIG, drawn over the game surface. Every
 * pill carries its label. Each button injects the Android key the port's keyboard binding for that PS2 button uses (src/platform/pad/pad_sdl.c
 * kKeys: arrows, Z X C V, Enter, Backspace, Q E, 1 3; CONFIG is F1, which opens the native settings like on a keyboard), through SDL's key entry
 * points, so those buttons need no code of their own on the native side. The stick is analog: the thumb's offset from its centre goes to
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
    private PadCustomization custom = new PadCustomization();
    private final float[] baseX = new float[PadCustomization.COUNT], baseY = new float[PadCustomization.COUNT];
    private final float[] halfW = new float[PadCustomization.COUNT], halfH = new float[PadCustomization.COUNT];
    private float safeLeft, safeTop, safeWidth, safeHeight;
    private boolean editing, updatingSliders;
    private int selected = -1;
    private float dragX, dragY;
    private LinearLayout editor;
    private TextView selection;
    private SeekBar sizeSlider, opacitySlider, globalSlider;

    // The stick's visible circle is also its touch target; its travel scales with the selected size.
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
        super.onSizeChanged(w, h, ow, oh);
        layoutPad(getRootWindowInsets());
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        layoutPad(getRootWindowInsets());
    }

    @Override
    public WindowInsets onApplyWindowInsets(WindowInsets insets) {
        final WindowInsets rootInsets = getRootWindowInsets();
        layoutPad(rootInsets != null ? rootInsets : insets);
        return super.onApplyWindowInsets(insets);
    }

    private void layoutPad(WindowInsets insets) {
        final int w = getWidth(), h = getHeight();
        if (w <= 0 || h <= 0) return;
        int left = 0, top = 0, right = 0, bottom = 0;
        if (insets != null) {
            left = insets.getSystemWindowInsetLeft();
            top = insets.getSystemWindowInsetTop();
            right = insets.getSystemWindowInsetRight();
            bottom = insets.getSystemWindowInsetBottom();
            if (Build.VERSION.SDK_INT >= 28) {
                final DisplayCutout cutout = insets.getDisplayCutout();
                if (cutout != null) {
                    left = Math.max(left, cutout.getSafeInsetLeft());
                    top = Math.max(top, cutout.getSafeInsetTop());
                    right = Math.max(right, cutout.getSafeInsetRight());
                    bottom = Math.max(bottom, cutout.getSafeInsetBottom());
                }
            }
        }
        final View root = getRootView();
        final int[] rootPos = new int[2], viewPos = new int[2];
        root.getLocationInWindow(rootPos);
        getLocationInWindow(viewPos);
        final PadLayout layout = new PadLayout(w, h, root.getWidth(), root.getHeight(),
                viewPos[0] - rootPos[0], viewPos[1] - rootPos[1], left, top, right, bottom);
        final float safeLeft = layout.left, safeTop = layout.top;
        final float safeWidth = layout.width, safeHeight = layout.height;
        this.safeLeft = safeLeft;
        this.safeTop = safeTop;
        this.safeWidth = safeWidth;
        this.safeHeight = safeHeight;
        final float gutter = layout.gutter;
        sideBars = layout.sideBars;
        u = layout.unit;
        dpadX = layout.dpadX;
        dpadY = layout.dpadY;
        faceX = layout.faceX;
        faceY = layout.faceY;
        line.setStrokeWidth(Math.max(2f, u * 0.08f));
        text.setTextSize(0.46f * u);
        // shoulder buttons: two rows at the top corners, clear of the cutout
        rect[L1].set(dpadX - 1.4f * u, safeTop + 0.4f * u, dpadX + 1.4f * u, safeTop + 1.5f * u);
        rect[L2].set(dpadX - 1.4f * u, safeTop + 1.7f * u, dpadX + 1.4f * u, safeTop + 2.8f * u);
        rect[R1].set(faceX - 1.4f * u, safeTop + 0.4f * u, faceX + 1.4f * u, safeTop + 1.5f * u);
        rect[R2].set(faceX - 1.4f * u, safeTop + 1.7f * u, faceX + 1.4f * u, safeTop + 2.8f * u);
        // CONFIG under R2, above the face buttons, in the same column as the shoulders
        rect[MENU].set(faceX - 1.4f * u, safeTop + 3.0f * u, faceX + 1.4f * u, safeTop + 4.1f * u);
        final float selectX = sideBars ? dpadX : safeLeft + safeWidth / 2f - 1.8f * u;
        final float startX = sideBars ? faceX : safeLeft + safeWidth / 2f + 1.8f * u;
        rect[SELECT].set(selectX - 1.4f * u, safeTop + safeHeight - 1.5f * u, selectX + 1.4f * u, safeTop + safeHeight - 0.5f * u);
        rect[START].set(startX - 1.4f * u, safeTop + safeHeight - 1.5f * u, startX + 1.4f * u, safeTop + safeHeight - 0.5f * u);
        stickR = 1.8f * u;
        if (sideBars) {
            // between L2 and the d-pad's ring, over the whole bar width
            stickZone.set(safeLeft, safeTop + 3.0f * u, safeLeft + gutter, dpadY - 2.6f * u);
            stickIdleX = dpadX;
            stickIdleY = (stickZone.top + stickZone.bottom) / 2f;
            stickToggle.setEmpty();
        } else {
            // the stick takes the d-pad's place; the toggle sits under L2 like CONFIG under R2
            stickZone.set(safeLeft, dpadY - 2.6f * u, dpadX + 2.6f * u, safeTop + safeHeight);
            stickIdleX = dpadX;
            stickIdleY = dpadY;
            stickToggle.set(dpadX - 1.4f * u, safeTop + 3.0f * u, dpadX + 1.4f * u, safeTop + 4.1f * u);
        }
        bounds(PadCustomization.STICK, stickIdleX, stickIdleY, stickR, stickR);
        bounds(PadCustomization.DPAD, dpadX, dpadY, 2.4f * u, 2.4f * u);
        bounds(PadCustomization.CROSS, faceX, faceY + 1.5f * u, .9f * u, .9f * u);
        bounds(PadCustomization.CIRCLE, faceX + 1.5f * u, faceY, .9f * u, .9f * u);
        bounds(PadCustomization.TRIANGLE, faceX, faceY - 1.5f * u, .9f * u, .9f * u);
        bounds(PadCustomization.SQUARE, faceX - 1.5f * u, faceY, .9f * u, .9f * u);
        for (int i = START; i < COUNT; i++)
            bounds(i - START + PadCustomization.START, rect[i].centerX(), rect[i].centerY(),
                    rect[i].width() / 2f, rect[i].height() / 2f);
        bounds(PadCustomization.TOGGLE, stickToggle.centerX(), stickToggle.centerY(),
                stickToggle.width() / 2f, stickToggle.height() / 2f);
        releaseStick();
        invalidate();
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
        if (showDpad() && contains(PadCustomization.DPAD, x, y)) {
            float dx = localX(PadCustomization.DPAD, x) - dpadX, dy = localY(PadCustomization.DPAD, y) - dpadY;
            float d = (float) Math.hypot(dx, dy);
            if (d < 2.4f * u && d > 0.35f * u) {
                if (dx < -0.4f * d) mask |= 1 << LEFT;
                if (dx > 0.4f * d) mask |= 1 << RIGHT;
                if (dy < -0.4f * d) mask |= 1 << UP;
                if (dy > 0.4f * d) mask |= 1 << DOWN;
            }
        }
        for (int i = CROSS; i <= SQUARE; i++)
            if (contains(PadCustomization.CROSS + i - CROSS, x, y)) mask |= 1 << i;
        for (int i = START; i < COUNT; i++) {
            if (contains(PadCustomization.START + i - START, x, y)) mask |= 1 << i;
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
        float knobTravel = Math.max(0f, stickR - .8f * u) / stickR;
        stickKnobX = stickBaseX + dx * knobTravel;
        stickKnobY = stickBaseY + dy * knobTravel;
        sendStick(dx / stickR, dy / stickR);
    }

    private void setStickMode(boolean on) {
        stickMode = on;
        getContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().putBoolean(PREF_STICK, on).apply();
        releaseStick();
    }

    private void bounds(int id, float x, float y, float w, float h) {
        baseX[id] = x; baseY[id] = y; halfW[id] = w; halfH[id] = h;
    }
    private float actualScale(int id) {
        float maxX = halfW[id] > 0 ? safeWidth / (2f * halfW[id]) : custom.scale[id];
        float maxY = halfH[id] > 0 ? safeHeight / (2f * halfH[id]) : custom.scale[id];
        return Math.max(.001f, Math.min(custom.scale[id], Math.min(maxX, maxY)));
    }
    private float cx(int id) {
        float x = custom.x[id] < 0 ? baseX[id] : safeLeft + custom.x[id] * safeWidth;
        return PadCustomization.clampCenter(x, halfW[id] * actualScale(id), safeLeft, safeWidth);
    }
    private float cy(int id) {
        float y = custom.y[id] < 0 ? baseY[id] : safeTop + custom.y[id] * safeHeight;
        return PadCustomization.clampCenter(y, halfH[id] * actualScale(id), safeTop, safeHeight);
    }
    private float localX(int id, float x) { return baseX[id] + (x - cx(id)) / actualScale(id); }
    private float localY(int id, float y) { return baseY[id] + (y - cy(id)) / actualScale(id); }
    private boolean visibleControl(int id) {
        return (id != PadCustomization.STICK || showStick()) && (id != PadCustomization.DPAD || showDpad()) &&
                (id != PadCustomization.TOGGLE || !sideBars);
    }
    private boolean contains(int id, float x, float y) {
        x = localX(id, x) - baseX[id]; y = localY(id, y) - baseY[id];
        if (id <= PadCustomization.SQUARE) {
            if (halfW[id] <= 0 || halfH[id] <= 0) return false;
            return x * x / (halfW[id] * halfW[id]) + y * y / (halfH[id] * halfH[id]) <= 1f;
        }
        float radius = Math.min(.4f * u, Math.min(halfW[id], halfH[id]));
        float dx = Math.max(0f, Math.abs(x) - (halfW[id] - radius));
        float dy = Math.max(0f, Math.abs(y) - (halfH[id] - radius));
        return Math.abs(x) <= halfW[id] && Math.abs(y) <= halfH[id] && dx * dx + dy * dy <= radius * radius;
    }
    private void beginControl(Canvas c, int id) {
        float scale = actualScale(id), alpha = custom.globalOpacity * custom.opacity[id];
        if (editing) alpha = Math.max(.35f, alpha);
        float extra = u * scale;
        c.saveLayerAlpha(cx(id) - halfW[id] * scale - extra, cy(id) - halfH[id] * scale - extra,
                cx(id) + halfW[id] * scale + extra, cy(id) + halfH[id] * scale + extra, Math.round(255f * alpha));
        c.translate(cx(id), cy(id)); c.scale(scale, scale); c.translate(-baseX[id], -baseY[id]);
    }
    private void endControl(Canvas c) { c.restore(); }
    private boolean moveOrResize(int id, float x, float y, float scale) {
        if (safeWidth <= 0 || safeHeight <= 0) return false;
        float oldX = custom.x[id], oldY = custom.y[id], oldScale = custom.scale[id];
        custom.scale[id] = scale;
        custom.x[id] = (PadCustomization.clampCenter(x, halfW[id] * scale, safeLeft, safeWidth) - safeLeft) / safeWidth;
        custom.y[id] = (PadCustomization.clampCenter(y, halfH[id] * scale, safeTop, safeHeight) - safeTop) / safeHeight;
        for (int i = 0; i < PadCustomization.COUNT; i++) {
            if (i == id || !visibleControl(i)) continue;
            float dx = Math.abs(cx(id) - cx(i)), dy = Math.abs(cy(id) - cy(i));
            float wx = halfW[id] * scale + halfW[i] * custom.scale[i];
            float hy = halfH[id] * scale + halfH[i] * custom.scale[i];
            boolean round = id <= PadCustomization.SQUARE && i <= PadCustomization.SQUARE;
            if (round ? dx * dx / (wx * wx) + dy * dy / (hy * hy) < 1f : dx < wx && dy < hy) {
                custom.x[id] = oldX; custom.y[id] = oldY; custom.scale[id] = oldScale; return false;
            }
        }
        invalidate(); return true;
    }
    void loadCustomization(String value) { custom = PadCustomization.parse(value); invalidate(); }
    private void saveCustomization() { P4Activity.nativeSavePadLayout(custom.serialize()); }
    private SeekBar slider(LinearLayout panel, String label) {
        TextView title = new TextView(getContext()); title.setText(label); panel.addView(title);
        SeekBar bar = new SeekBar(getContext()); bar.setMax(100); panel.addView(bar); return bar;
    }
    private void updateSelection() {
        updatingSliders = true;
        final String[] names = {"Stick", "D-pad", "Cross", "Circle", "Triangle", "Square",
                "Start", "Select", "L1", "R1", "L2", "R2", "Config", "Stick toggle"};
        selection.setText(selected < 0 ? "Tap a control, then drag" : names[selected]);
        sizeSlider.setEnabled(selected >= 0); opacitySlider.setEnabled(selected >= 0);
        if (selected >= 0) {
            sizeSlider.setProgress(Math.round((custom.scale[selected] - .5f) * 100f / 1.5f));
            opacitySlider.setProgress(Math.round(custom.opacity[selected] * 100f));
        }
        globalSlider.setProgress(Math.round(custom.globalOpacity * 100f)); updatingSliders = false;
    }
    void startEditing() {
        if (editing) return;
        editing = true; selected = -1; releaseStick();
        for (int i = 0; i < COUNT; i++) if (down[i]) { SDLActivity.onNativeKeyUp(KEYS[i]); down[i] = false; }
        editor = new LinearLayout(getContext()); editor.setOrientation(LinearLayout.VERTICAL);
        editor.setPadding(12, 8, 12, 8); editor.setBackgroundColor(0xEFFFFFFF);
        selection = new TextView(getContext()); editor.addView(selection);
        sizeSlider = slider(editor, "Size"); opacitySlider = slider(editor, "Control opacity");
        globalSlider = slider(editor, "Global opacity");
        SeekBar.OnSeekBarChangeListener listener = new SeekBar.OnSeekBarChangeListener() {
            public void onStartTrackingTouch(SeekBar bar) { }
            public void onStopTrackingTouch(SeekBar bar) { saveCustomization(); updateSelection(); }
            public void onProgressChanged(SeekBar bar, int progress, boolean fromUser) {
                if (!fromUser || updatingSliders) return;
                if (bar == globalSlider) custom.globalOpacity = progress / 100f;
                else if (selected >= 0 && bar == opacitySlider) custom.opacity[selected] = progress / 100f;
                else if (selected >= 0 && bar == sizeSlider)
                    moveOrResize(selected, cx(selected), cy(selected), .5f + 1.5f * progress / 100f);
                invalidate();
            }
        };
        sizeSlider.setOnSeekBarChangeListener(listener); opacitySlider.setOnSeekBarChangeListener(listener);
        globalSlider.setOnSeekBarChangeListener(listener);
        LinearLayout buttons = new LinearLayout(getContext());
        Button reset = new Button(getContext()); reset.setText("Reset to default");
        reset.setOnClickListener(v -> { custom = new PadCustomization(); saveCustomization(); updateSelection(); invalidate(); });
        Button done = new Button(getContext()); done.setText("Done");
        done.setOnClickListener(v -> { saveCustomization(); editing = false; ((ViewGroup) editor.getParent()).removeView(editor); editor = null; invalidate(); });
        buttons.addView(reset); buttons.addView(done); editor.addView(buttons);
        android.widget.FrameLayout.LayoutParams lp = new android.widget.FrameLayout.LayoutParams(
                (int)(260 * getResources().getDisplayMetrics().density), ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.TOP | Gravity.CENTER_HORIZONTAL);
        ((Activity)getContext()).addContentView(editor, lp); updateSelection(); invalidate();
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        final int action = e.getActionMasked();
        final int index = e.getActionIndex();
        if (editing) {
            if (action == MotionEvent.ACTION_DOWN) {
                selected = -1;
                for (int i = PadCustomization.COUNT - 1; i >= 0; i--)
                    if (visibleControl(i) && contains(i, e.getX(), e.getY())) { selected = i; break; }
                if (selected >= 0) { dragX = e.getX() - cx(selected); dragY = e.getY() - cy(selected); }
                updateSelection(); invalidate();
            } else if (action == MotionEvent.ACTION_MOVE && selected >= 0) {
                moveOrResize(selected, e.getX() - dragX, e.getY() - dragY, custom.scale[selected]);
            } else if (action == MotionEvent.ACTION_UP && selected >= 0) saveCustomization();
            return true;
        }
        final boolean pointerUp = action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_UP;
        final int lifted = pointerUp ? index : -1;
        boolean claimed = false;
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            final float x = e.getX(index), y = e.getY(index);
            if (!sideBars && contains(PadCustomization.TOGGLE, x, y)) {
                setStickMode(!stickMode);
                claimed = true;
            } else if (stickId < 0 && showStick() && contains(PadCustomization.STICK, x, y)) {
                stickId = e.getPointerId(index);
                stickBaseX = stickKnobX = stickIdleX;
                stickBaseY = stickKnobY = stickIdleY;
                claimed = true;
            }
        }
        if (stickId >= 0) {
            final int s = e.findPointerIndex(stickId);
            if (action == MotionEvent.ACTION_CANCEL || s < 0 || s == lifted) releaseStick();
            else moveStick(localX(PadCustomization.STICK, e.getX(s)), localY(PadCustomization.STICK, e.getY(s)));
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
        if (showStick()) { beginControl(c, PadCustomization.STICK); drawStick(c); endControl(c); }
        if (showDpad()) {
            beginControl(c, PadCustomization.DPAD);
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
            endControl(c);
        }
        // face buttons: cross (bottom), circle (right), triangle (top), square (left)
        final float r = 0.9f * u, k = 0.4f * u;
        beginControl(c, PadCustomization.CROSS);
        button(c, faceX, faceY + 1.5f * u, r, down[CROSS]);
        c.drawLine(faceX - k, faceY + 1.5f * u - k, faceX + k, faceY + 1.5f * u + k, line);
        c.drawLine(faceX - k, faceY + 1.5f * u + k, faceX + k, faceY + 1.5f * u - k, line);
        endControl(c); beginControl(c, PadCustomization.CIRCLE);
        button(c, faceX + 1.5f * u, faceY, r, down[CIRCLE]);
        c.drawCircle(faceX + 1.5f * u, faceY, k, line);
        endControl(c); beginControl(c, PadCustomization.TRIANGLE);
        button(c, faceX, faceY - 1.5f * u, r, down[TRIANGLE]);
        final Path tri = new Path();
        tri.moveTo(faceX, faceY - 1.5f * u - k); tri.lineTo(faceX - k, faceY - 1.5f * u + k * 0.8f); tri.lineTo(faceX + k, faceY - 1.5f * u + k * 0.8f); tri.close();
        c.drawPath(tri, line);
        endControl(c); beginControl(c, PadCustomization.SQUARE);
        button(c, faceX - 1.5f * u, faceY, r, down[SQUARE]);
        c.drawRect(faceX - 1.5f * u - k, faceY - k, faceX - 1.5f * u + k, faceY + k, line);
        endControl(c);
        // shoulders, Start/Select and CONFIG
        for (int i = START; i < COUNT; i++) {
            beginControl(c, PadCustomization.START + i - START);
            pill(c, rect[i], LABELS[i - START], down[i]);
            endControl(c);
        }
        if (!sideBars) {
            beginControl(c, PadCustomization.TOGGLE);
            pill(c, stickToggle, stickMode ? "D-PAD" : "STICK", false);
            endControl(c);
        }
    }
}
