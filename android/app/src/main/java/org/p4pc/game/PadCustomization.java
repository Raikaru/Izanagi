package org.p4pc.game;

import java.util.Locale;

/** Device-independent custom pad geometry. Coordinates are fractions of the safe area. */
final class PadCustomization {
    static final int STICK = 0, DPAD = 1, CROSS = 2, CIRCLE = 3, TRIANGLE = 4, SQUARE = 5;
    static final int START = 6, SELECT = 7, L1 = 8, R1 = 9, L2 = 10, R2 = 11, CONFIG = 12, TOGGLE = 13, COUNT = 14;
    static final float MIN_SCALE = 0.5f, MAX_SCALE = 2f;
    final float[] x = new float[COUNT], y = new float[COUNT], scale = new float[COUNT], opacity = new float[COUNT];
    float globalOpacity = 1f;

    PadCustomization() {
        for (int i = 0; i < COUNT; i++) {
            x[i] = y[i] = -1f; // use the current safe area's default position
            scale[i] = opacity[i] = 1f;
        }
    }

    static PadCustomization parse(String value) {
        PadCustomization result = new PadCustomization();
        if (value == null || value.isEmpty()) return result;
        try {
            String[] fields = value.split(",", -1);
            if (fields.length != 2 + COUNT * 4 || !fields[0].equals("1")) return result;
            float global = Float.parseFloat(fields[1]);
            if (!valid(global, 0f, 1f)) return result;
            PadCustomization parsed = new PadCustomization();
            parsed.globalOpacity = global;
            for (int i = 0; i < COUNT; i++) {
                int p = 2 + i * 4;
                float x = Float.parseFloat(fields[p]), y = Float.parseFloat(fields[p + 1]);
                float scale = Float.parseFloat(fields[p + 2]), opacity = Float.parseFloat(fields[p + 3]);
                if (!((x == -1f && y == -1f) || (valid(x, 0f, 1f) && valid(y, 0f, 1f))) ||
                    !valid(scale, MIN_SCALE, MAX_SCALE) || !valid(opacity, 0f, 1f)) return result;
                parsed.x[i] = x; parsed.y[i] = y; parsed.scale[i] = scale; parsed.opacity[i] = opacity;
            }
            return parsed;
        } catch (NumberFormatException ex) {
            return result;
        }
    }

    private static boolean valid(float value, float min, float max) {
        return !Float.isNaN(value) && !Float.isInfinite(value) && value >= min && value <= max;
    }

    String serialize() {
        StringBuilder out = new StringBuilder("1");
        out.append(String.format(Locale.US, ",%.3f", globalOpacity));
        for (int i = 0; i < COUNT; i++)
            out.append(String.format(Locale.US, ",%.3f,%.3f,%.3f,%.3f", x[i], y[i], scale[i], opacity[i]));
        return out.toString();
    }

    static float clampCenter(float center, float halfSize, float start, float length) {
        if (length <= 2f * halfSize) return start + length / 2f;
        return Math.max(start + halfSize, Math.min(start + length - halfSize, center));
    }
}
