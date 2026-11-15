package org.p4pc.game;

/** Touch-pad geometry in overlay-local pixels. The safe rectangle is supplied in
 * root-view coordinates, since a parent may already have moved the overlay. */
final class PadLayout {
    final float left, top, width, height, gutter, unit, dpadX, dpadY, faceX, faceY;
    final boolean sideBars;

    PadLayout(int viewWidth, int viewHeight, int rootWidth, int rootHeight, int viewX, int viewY,
              int insetLeft, int insetTop, int insetRight, int insetBottom) {
        left = Math.max(0, Math.min(viewWidth, insetLeft - viewX));
        top = Math.max(0, Math.min(viewHeight, insetTop - viewY));
        final float right = Math.max(left, Math.min(viewWidth, rootWidth - insetRight - viewX));
        final float bottom = Math.max(top, Math.min(viewHeight, rootHeight - insetBottom - viewY));
        width = right - left;
        height = bottom - top;
        gutter = Math.max(0f, (width - height * 4f / 3f) / 2f);
        sideBars = gutter >= height * 0.32f && height > 0f;
        unit = sideBars ? Math.min(height / 14.4f, gutter / 6.4f) : Math.min(width, height * 1.7f) / 17f;
        dpadX = left + (sideBars ? gutter / 2f : 3.2f * unit);
        dpadY = top + height - (sideBars ? 4.4f : 3.4f) * unit;
        faceX = left + width - (dpadX - left);
        faceY = dpadY;
    }
}
