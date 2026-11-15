package org.p4pc.game;

final class PadLayoutTest {
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    private static void inside(PadLayout p) {
        final float r = 2.6f * p.unit;
        check(p.dpadX - r >= p.left && p.dpadX + r <= p.left + p.width, "d-pad outside safe width");
        check(p.faceX - r >= p.left && p.faceX + r <= p.left + p.width, "face buttons outside safe width");
        check(p.dpadY - r >= p.top && p.dpadY + r <= p.top + p.height, "buttons outside safe height");
        check(p.unit > 0, "empty pad");
    }

    public static void main(String[] args) {
        PadLayout plain = new PadLayout(2340, 1080, 2340, 1080, 0, 0, 0, 0, 0, 0);
        check(plain.sideBars && plain.dpadX == 225f, "unobstructed 19.5:9 layout changed");
        inside(plain);
        PadLayout left = new PadLayout(2340, 1080, 2340, 1080, 0, 0, 132, 0, 0, 0);
        check(left.left == 132f && left.dpadX - 2.6f * left.unit >= 132f, "132px left inset ignored");
        inside(left);
        PadLayout right = new PadLayout(2340, 1080, 2340, 1080, 0, 0, 0, 0, 132, 0);
        check(right.left + right.width == 2208f, "right inset ignored");
        inside(right);
        PadLayout vertical = new PadLayout(2340, 1080, 2340, 1080, 0, 0, 0, 60, 0, 48);
        check(vertical.top == 60f && vertical.height == 972f, "vertical insets ignored");
        inside(vertical);
        PadLayout moved = new PadLayout(2208, 1080, 2340, 1080, 132, 0, 132, 0, 0, 0);
        check(moved.left == 0f && moved.width == 2208f, "already-inset view was inset twice");
        inside(moved);
        PadLayout narrow = new PadLayout(1280, 720, 1280, 720, 0, 0, 132, 20, 50, 30);
        check(!narrow.sideBars, "narrow layout should overlay the frame");
        inside(narrow);
        PadCustomization custom = new PadCustomization();
        custom.x[PadCustomization.CROSS] = 0.75f;
        custom.y[PadCustomization.CROSS] = 0.4f;
        custom.scale[PadCustomization.CROSS] = 1.5f;
        custom.opacity[PadCustomization.CROSS] = 0.5f;
        custom.globalOpacity = 0.5f;
        PadCustomization roundTrip = PadCustomization.parse(custom.serialize());
        check(roundTrip.serialize().equals(custom.serialize()), "layout round trip");
        check(roundTrip.x[PadCustomization.CROSS] == 0.75f, "layout position");
        check(roundTrip.opacity[PadCustomization.CROSS] == 0.5f, "layout opacity");
        check(PadCustomization.parse(null).serialize().equals(new PadCustomization().serialize()), "missing layout fallback");
        check(PadCustomization.parse("1,NaN").serialize().equals(new PadCustomization().serialize()), "corrupt layout fallback");
        check(PadCustomization.parse(custom.serialize().replace("0.750", "NaN")).serialize()
                .equals(new PadCustomization().serialize()), "nonfinite layout fallback");
        check(PadCustomization.clampCenter(0, 20, 100, 200) == 120, "left safe clamp");
        check(PadCustomization.clampCenter(400, 20, 100, 200) == 280, "right safe clamp");
        check(PadCustomization.clampCenter(0, 120, 100, 200) == 200, "oversize safe clamp");
    }
}
