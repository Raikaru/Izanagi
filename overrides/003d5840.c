/* Retail 003d5840 (rtanim.c, RtAnimInterpolatorSetCurrentAnim): points the interpolator at a
 * keyframe animation and restarts it at time 0.
 *
 * Interpolator (words): [0] anim, [1] time, [2] next frame, [3] callback, [4] callback data,
 * [5] callback time, [6] loop callback, [7] loop callback data, [9] interpolated keyframe size,
 * [10] animation keyframe size, [0xb] node count, [0xf..0x12] apply, blend, interpolate, add callbacks
 * (copied from the animation's interpolator info), then from +0x4c one interpolated keyframe per node
 * (stride [9]) whose first two words are the node's previous and next animation keyframe.
 * Animation (words): [0] info, [1] frame count, [3] duration, [4] frames, [5] custom data. Info:
 * [1] interpolated keyframe size, [2] animation keyframe size, [3..6] the four callbacks. */
typedef void (*P4AnimInterpCB)(unsigned char *out, unsigned char *in1, unsigned char *in2, float time, void *custom);

int func_003d5840(unsigned char *interp, unsigned char *anim)
{
    unsigned int *w = (unsigned int *)interp;
    unsigned int *a = (unsigned int *)anim;
    unsigned int *info = (unsigned int *)a[0];
    int count, i;
    unsigned int stride;
    unsigned char *frames;
    unsigned int *pair;
    unsigned int prev, next;

    w[0] = (unsigned int)anim;
    w[1] = 0;
    w[9] = info[1];
    w[10] = info[2];
    w[0xf] = info[3];
    w[0x10] = info[4];
    w[0x11] = info[5];
    w[0x12] = info[6];
    count = (int)w[0xb];
    frames = (unsigned char *)a[4];
    stride = w[10];
    for (i = 0; i < count; i++) {
        ((P4AnimInterpCB)w[0x11])(interp + i * w[9] + 0x4c, frames + i * stride,
                                  frames + stride * (unsigned int)(i + count), 0.0f, (void *)a[5]);
        count = (int)w[0xb];
    }
    stride = w[10];
    prev = (unsigned int)frames;
    next = (unsigned int)frames + (unsigned int)count * stride;
    pair = w + 0x13;
    for (i = 0; i < (int)w[0xb]; i++) {
        pair[0] = prev;
        pair[1] = next;
        stride = w[10];
        pair = (unsigned int *)((unsigned char *)pair + w[9]);
        prev += stride;
        next += stride;
    }
    w[2] = (unsigned int)frames + w[0xb] * w[10] * 2;
    return 1;
}
