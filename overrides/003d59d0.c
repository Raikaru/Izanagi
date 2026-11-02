/* Retail 003d59d0 (rtanim.c, RtAnimInterpolatorSubAnimTime): moves the interpolator back by `dt`.
 * Layout: see overrides/003d5840.c. Each node's previous/next keyframe pair steps back until the
 * previous keyframe is not later than the new time, then the node is interpolated. A time below zero
 * wraps by the animation duration: the animation is restarted (func_003d5840) and advanced to the
 * wrapped time (func_003d5bc0), and the loop callback is armed. The callback fires when the time
 * crossed the callback time on the way back. A callback that returns 0 is cleared. */
typedef void (*P4AnimInterpCB)(unsigned char *out, unsigned char *in1, unsigned char *in2, float time, void *custom);
typedef void *(*P4AnimCB)(unsigned char *interp, void *data);

extern int func_003d5840(unsigned char *interp, unsigned char *anim);
extern int func_003d5bc0(unsigned char *interp, float dt);

int func_003d59d0(unsigned char *interp, float dt)
{
    unsigned int *w = (unsigned int *)interp;
    float *f = (float *)interp;
    unsigned char *anim = (unsigned char *)w[0];
    int hit = 0, wrapped = 0;
    unsigned int *node;
    int n;

    if (f[5] < f[1] && f[1] - dt <= f[5]) {
        hit = 1;
    }
    f[1] = f[1] - dt;
    if (0.0f <= f[1]) {
        n = (int)w[0xb];
        node = w + 0x13;
        for (; n > 0; n--) {
            unsigned char *prev = (unsigned char *)node[0];

            if (f[1] < *(float *)(prev + 4)) {
                do {
                    node[1] = (unsigned int)prev;
                    node[0] = *(unsigned int *)node[0];
                    prev = (unsigned char *)node[0];
                } while (f[1] < *(float *)(prev + 4));
            }
            ((P4AnimInterpCB)w[0x11])((unsigned char *)node, prev, (unsigned char *)node[1], f[1],
                                      (void *)*(unsigned int *)(anim + 0x14));
            node = (unsigned int *)((unsigned char *)node + w[9]);
        }
        w[2] = 0;
    } else {
        float duration = *(float *)(anim + 0xc);
        float t = f[1];

        while (t < 0.0f) {
            t = f[1] + duration;
            f[1] = t;
        }
        func_003d5840(interp, anim);
        func_003d5bc0(interp, t);
        if (f[1] < f[5]) {
            hit = 1;
        }
        wrapped = 1;
    }
    if (hit && w[3] != 0 && ((P4AnimCB)w[3])(interp, (void *)w[4]) == 0) {
        w[3] = 0;
    }
    if (wrapped && w[6] != 0 && ((P4AnimCB)w[6])(interp, (void *)w[7]) == 0) {
        w[6] = 0;
    }
    return 1;
}
