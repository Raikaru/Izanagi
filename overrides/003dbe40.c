/* Retail 003dbe40 (code1_003d.c): reads a texture stream of the second kind: a mip image count word,
 * the images (read as in func_003db630), then a texture object chunk (func_003dbbc0, whose +0x50 low byte is the
 * filter mode) built into a raster the same way as in func_003db630, attached with func_003ef080, the
 * texture plugins read through the registry at 0x0070b800 (func_003e3110) and the finished texture
 * registered through func_003ef6d0 / func_003ef5b0. Returns 1 when the texture exists at the end, else
 * 0 (the early-exit paths leave the remaining images allocated, as retail does). */
extern int p4_tex_read(void *stream, void *buffer, int size) __asm__("func_003e2910");
extern int p4_tex_chunk(void *stream, int type, int a, int b) __asm__("func_003df050");
extern unsigned char *p4_tex_image_read(void *stream) __asm__("func_003ebbf0");
extern void p4_tex_image_convert(unsigned char *image) __asm__("func_003ebde0");
extern void p4_tex_image_free(unsigned char *image) __asm__("func_003ea370");
extern void p4_tex_raster_size(unsigned char *image, int format, int *w, int *h, int *depth, int *flags) __asm__("func_003ec1f0");
extern unsigned char *p4_tex_raster_create(int w, int h, int depth, int flags) __asm__("func_003ec590");
extern void p4_tex_raster_destroy(unsigned char *raster) __asm__("func_003ec330");
extern int p4_tex_raster_levels(unsigned char *raster) __asm__("func_003ec420");
extern unsigned char *p4_tex_image_scale(unsigned char *image, int w, int h) __asm__("func_003ed2d0");
extern int p4_tex_raster_lock(unsigned char *raster, int level, int mode) __asm__("func_003ec6a0");
extern int p4_tex_raster_set(unsigned char *raster, unsigned char *image) __asm__("func_003ec180");
extern int p4_tex_raster_unlock(unsigned char *raster) __asm__("func_003ec2a0");
extern unsigned char *p4_tex_create(unsigned char *raster) __asm__("func_003ef2e0");
extern void p4_tex_set_name(unsigned char *tex, unsigned char *name) __asm__("func_003ef470");
extern void p4_tex_set_mask(unsigned char *tex, unsigned char *name) __asm__("func_003ef510");
extern void p4_tex_register(unsigned char *slot, unsigned char *tex) __asm__("func_003ef5b0");
extern int p4_tex_error_code(unsigned int code, int size) __asm__("func_003df590");
extern void p4_tex_error_post(void *record) __asm__("func_003df4d0");
extern void *(*p4_tex_malloc[])(int size, int flags) __asm__("D_008873E8");
extern void (*p4_tex_free[])(void *) __asm__("D_008873EC");

extern unsigned char *p4_tex_native_read(void *stream) __asm__("func_003dbbc0");
extern void p4_tex_set_raster(unsigned char *tex, unsigned char *raster) __asm__("func_003ef080");
extern int p4_tex_plugin_read(void *registry, void *stream, unsigned char *tex) __asm__("func_003e3110");
extern unsigned char *p4_tex_dict_slot(void) __asm__("func_003ef6d0");
extern char p4_tex_registry __asm__("D_0070B800");

unsigned int func_003dbe40(void *stream)
{
    int count = 0;
    unsigned char **images;
    unsigned char *tex;
    unsigned char *raster;
    unsigned int format;
    unsigned int saved_flag = 0;
    int w, h, depth, raster_flags;
    int levels;
    int i;

    if (p4_tex_read(stream, &count, 4) != 4) {
        return 0;
    }
    images = (unsigned char **)p4_tex_malloc[0](count << 2, 0x101b3);
    if (images == 0) {
        unsigned int record[2];

        record[0] = 435;
        record[1] = (unsigned int)p4_tex_error_code(0x80000013u, count << 2);
        p4_tex_error_post(record);
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (!p4_tex_chunk(stream, 0x18, 0, 0)) {
            return 0;
        }
        images[i] = p4_tex_image_read(stream);
        if (images[i] == 0) {
            return 0;
        }
        if ((*(unsigned int *)images[i] & 2) == 0) {
            p4_tex_image_convert(images[i]);
        }
    }
    if (!p4_tex_chunk(stream, 6, 0, 0)) {
        return 0;
    }
    tex = p4_tex_native_read(stream);
    if (tex == 0) {
        return 0;
    }

    w = *(int *)(images[0] + 4);
    h = *(int *)(images[0] + 8);
    depth = *(int *)(images[0] + 12);
    if (count >= 2) {
        format = 0x8004;
    } else if (tex[0x50] != 1 && tex[0x50] != 2) {
        format = 0x9004;
    } else {
        format = 4;
    }
    p4_tex_raster_size(images[0], (int)format, &w, &h, &depth, &raster_flags);
    raster = p4_tex_raster_create(w, h, depth, (int)(format | (unsigned int)raster_flags));
    if (raster == 0) {
        for (i = 0; i < count; i++) {
            p4_tex_image_free(images[i]);
        }
        p4_tex_free[0](images);
        return 0;
    }

    {
        int rw = *(int *)(raster + 0xc);
        int rh = *(int *)(raster + 0x10);

        if (*(int *)(images[0] + 4) != rw || *(int *)(images[0] + 8) != rh) {
            if (depth == 8) {
                p4_tex_raster_destroy(raster);
                raster = p4_tex_raster_create(rw, rh, 0x20, (int)(format | 0x500));
                if (raster == 0) {
                    raster = p4_tex_raster_create(rw, rh, 0x10, (int)(format | 0x100));
                }
            }
            levels = p4_tex_raster_levels(raster);
            for (i = 0; i < levels; i++) {
                unsigned char *scaled = p4_tex_image_scale(images[i], rw, rh);

                if (scaled == 0) {
                    return 0;
                }
                p4_tex_image_free(images[i]);
                images[i] = scaled;
                if (rw > 1) {
                    rw >>= 1;
                }
                if (rh > 1) {
                    rh >>= 1;
                }
            }
        }
    }

    if (count >= 2) {
        saved_flag = raster[0x23] & 0x10u;
        raster[0x23] = (unsigned char)(raster[0x23] & ~saved_flag);
    }
    levels = p4_tex_raster_levels(raster);
    i = 0;
    while (i < levels) {
        if (i == count) {
            break;
        }
        if (!p4_tex_raster_lock(raster, i & 0xff, 1)) {
            return 0;
        }
        if (!p4_tex_raster_set(raster, images[i])) {
            return 0;
        }
        if (!p4_tex_raster_unlock(raster)) {
            return 0;
        }
        p4_tex_image_free(images[i]);
        i++;
    }
    if (count >= 2) {
        raster[0x23] = (unsigned char)(raster[0x23] | saved_flag);
    }
    for (; i < count; i++) {
        p4_tex_image_free(images[i]);
    }
    p4_tex_free[0](images);

    p4_tex_set_raster(tex, raster);
    if (!p4_tex_plugin_read(&p4_tex_registry, stream, tex)) {
        tex = 0;
    } else {
        unsigned char *slot = p4_tex_dict_slot();

        if (slot != 0) {
            p4_tex_register(slot, tex);
        }
    }
    return tex != 0;
}
