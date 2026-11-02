/* SHA-256 over the versioned P4TEX decoded-image wire format. */
#include "p4pc/texture_key.h"
#include "p4pc/content_digest.h"

static void sha_u32le(P4ContentDigest *s, uint32_t x)
{
    uint8_t bytes[4]={(uint8_t)x,(uint8_t)(x>>8),(uint8_t)(x>>16),(uint8_t)(x>>24)};
    p4_content_digest_update(s,bytes,4);
}
int p4_texture_key(const P4TexturePixels *p, char hex[P4_TEXTURE_KEY_HEX+1])
{
    static const uint8_t prefix[6]={'P','4','T','E','X',1};
    P4ContentDigest s;
    uint32_t entries;
    uint64_t image_size;
    if (!p || !hex || !p->rgba || !p->width || !p->height ||
        p->format<P4_TEX_ARGB1555 || p->format>P4_TEX_PAL8) return -1;
    entries=p->format==P4_TEX_PAL4?16u:p->format==P4_TEX_PAL8?256u:0u;
    if (p->palette_entries!=entries ||
        (entries && (!p->palette || p->palette_format<P4_PAL_RGBA8888 || p->palette_format>P4_PAL_RGB555)) ||
        (!entries && p->palette_format!=P4_PAL_NONE)) return -1;
    /* Check the multiplication before reading pixels, also on the i386 reference. */
    if ((uint64_t)p->width*p->height > SIZE_MAX/4u) return -1;
    image_size=(uint64_t)p->width*p->height*4u;
    if (image_size > UINT64_MAX/8u-26u-entries*4u) return -1;
    p4_content_digest_begin(&s);
    p4_content_digest_update(&s,prefix,sizeof prefix);
    sha_u32le(&s,p->width); sha_u32le(&s,p->height); sha_u32le(&s,p->format);
    sha_u32le(&s,p->palette_format); sha_u32le(&s,p->palette_entries);
    p4_content_digest_update(&s,p->rgba,(size_t)image_size);
    if (entries) p4_content_digest_update(&s,p->palette,(size_t)entries*4u);
    p4_content_digest_end(&s,hex);
    return 0;
}
