#include "p4gl_golden.h"
#include "p4gl_gl.h"
#include "p4pc/game_mode.h"

uint32_t p4gl_golden_texture_upload(const P4GoldenTexture *t)
{
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN) return 0;
    if(!t || !t->storage || !t->level_count || t->level_count>P4_GOLDEN_TEXTURE_MIPS ||
       !p4gl_glGetIntegerv || !p4gl_glGenTextures || !p4gl_glDeleteTextures ||
       !p4gl_glBindTexture || !p4gl_glTexImage2D || !p4gl_glTexParameteri ||
       !p4gl_glPixelStorei || !p4gl_glGetError) return 0;
    uint32_t w=t->levels[0].width,h=t->levels[0].height;
    if(!w || !h || w>16384 || h>16384) return 0;
    for(unsigned i=0;i<t->level_count;++i) {
        const P4GoldenTextureLevel *l=&t->levels[i];
        if(l->width!=w || l->height!=h || !l->rgba || (i && t->levels[i-1].width==1 && t->levels[i-1].height==1)) return 0;
        w=w>1?w/2:1; h=h>1?h/2:1;
    }
    GLint limit=0,previous=0,alignment=0,row=0;
    p4gl_glGetIntegerv(0x0d33,&limit); /* GL_MAX_TEXTURE_SIZE */
    if(limit<=0 || t->levels[0].width>(uint32_t)limit || t->levels[0].height>(uint32_t)limit) return 0;
    p4gl_glGetIntegerv(0x8069,&previous); /* GL_TEXTURE_BINDING_2D */
    p4gl_glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
    p4gl_glGetIntegerv(GL_UNPACK_ROW_LENGTH,&row);
    GLuint texture=0;
    p4gl_glGenTextures(1,&texture);
    if(!texture) return 0;
    p4gl_glBindTexture(GL_TEXTURE_2D,texture);
    p4gl_glPixelStorei(GL_UNPACK_ALIGNMENT,1); p4gl_glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    for(unsigned i=0;i<t->level_count;++i) {
        const P4GoldenTextureLevel *l=&t->levels[i];
        p4gl_glTexImage2D(GL_TEXTURE_2D,(GLint)i,t->srgb?0x8c43:GL_RGBA8, /* GL_SRGB8_ALPHA8 */
            (GLsizei)l->width,(GLsizei)l->height,0,GL_RGBA,GL_UNSIGNED_BYTE,l->rgba);
    }
    p4gl_glTexParameteri(GL_TEXTURE_2D,0x813d,(GLint)t->level_count-1); /* GL_TEXTURE_MAX_LEVEL */
    p4gl_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,t->level_count>1?0x2703:GL_LINEAR);
    p4gl_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    p4gl_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    p4gl_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    GLenum error=p4gl_glGetError();
    p4gl_glPixelStorei(GL_UNPACK_ALIGNMENT,alignment); p4gl_glPixelStorei(GL_UNPACK_ROW_LENGTH,row);
    p4gl_glBindTexture(GL_TEXTURE_2D,(GLuint)previous);
    if(error!=GL_NO_ERROR) { p4gl_glDeleteTextures(1,&texture); return 0; }
    return texture;
}
