/* Retail 003d5bc0 (rtanim.c, RtAnimInterpolatorAddAnimTime): advances the interpolator by `dt`
 * (only a positive step does anything). Layout: see overrides/003d5840.c.
 * Past the animation's duration the time wraps, subtracting the duration until it fits, the
 * animation is restarted at its beginning (func_003d5840) and the wrapped time kept. If the next-frame
 * pointer is unset the animation is restarted at the current time. The next-frame pointer then moves
 * forward over every keyframe that is no later than the time; the node that held the preceding
 * keyframe takes it as its previous frame and the new frame as its next. Every node is interpolated.
 * The callback fires when the time crossed the callback time, the loop callback when it wrapped; a
 * callback that returns 0 is cleared. */
typedef void (*P4AnimInterpCB)(unsigned char *out, unsigned char *in1, unsigned char *in2, float time, void *custom);
typedef void *(*P4AnimCB)(unsigned char *interp, void *data);

extern int func_003d5840(unsigned char *interp, unsigned char *anim);

int func_003d5bc0(unsigned char *interp, float dt)
{
    unsigned int *w = (unsigned int *)interp;
    float *f = (float *)interp;
    unsigned char *anim;
    int nodes, hit = 0, wrapped = 0, idx, i;
    float duration, t;
    unsigned int *next;
    unsigned char *end;
    unsigned int *node;

    if (!(0.0f < dt)) {
        return 1;
    }
    anim = (unsigned char *)w[0];
    nodes = (int)w[0xb];
    duration = *(float *)(anim + 0xc);
    if (f[1] < f[5] && f[5] <= f[1] + dt) {
        hit = 1;
    }
    t = f[1] + dt;
    f[1] = t;
    if (duration < t) {
        wrapped = 1;
        while (duration < t) {
            t = f[1] - duration;
            f[1] = t;
            if (f[5] <= t) {
                hit = 1;
            }
        }
        func_003d5840(interp, anim);
        f[1] = t;
    }
    if (w[2] == 0) {
        float saved = f[1];

        func_003d5840(interp, anim);
        f[1] = saved;
    }
    end = (unsigned char *)(*(unsigned int *)(anim + 0x10) + *(unsigned int *)(anim + 4) * w[10]);
    idx = 0;
    next = (unsigned int *)w[2];
    while ((unsigned char *)next < end && *(float *)(*next + 4) <= f[1]) {
        unsigned int off, found;

        do {
            off = (unsigned int)idx * w[9];
            idx++;
            if (nodes <= idx) {
                idx = 0;
            }
            found = *(unsigned int *)(interp + off + 0x50);
        } while (found != *next);
        *(unsigned int *)(interp + off + 0x4c) = found;
        *(unsigned int *)(interp + off + 0x50) = w[2];
        w[2] += w[10];
        next = (unsigned int *)w[2];
    }
    node = w + 0x13;
    for (i = 0; i < nodes; i++) {
        ((P4AnimInterpCB)w[0x11])((unsigned char *)node, (unsigned char *)node[0], (unsigned char *)node[1], f[1],
                                  (void *)*(unsigned int *)(anim + 0x14));
        node = (unsigned int *)((unsigned char *)node + w[9]);
    }
    if (hit && w[3] != 0 && ((P4AnimCB)w[3])(interp, (void *)w[4]) == 0) {
        w[3] = 0;
    }
    if (wrapped && w[6] != 0 && ((P4AnimCB)w[6])(interp, (void *)w[7]) == 0) {
        w[6] = 0;
    }
    return 1;
}
