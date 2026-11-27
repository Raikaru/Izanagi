/* GL call recorder: exercises production upload with exact synthetic pixels. */
#include "../src/render/p4gl_golden.h"
#include "../src/render/p4gl_gl.h"
#include "p4pc/game_mode.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static P4GameMode mode=P4_GAME_MODE_PS2;
P4GameMode p4_game_mode(void) { return mode; }
static unsigned calls,uploads,deletions; static GLint bound=77,alignment=4,row=9; static GLenum error;
static uint8_t pixels[64];
static void P4GL_APIENTRY get(GLenum n,GLint *v) { ++calls; *v=n==0x0d33?4096:n==0x8069?bound:n==GL_UNPACK_ALIGNMENT?alignment:row; }
static void P4GL_APIENTRY gen(GLsizei n,GLuint *t) { ++calls; assert(n==1); *t=42; }
static void P4GL_APIENTRY del(GLsizei n,const GLuint *t) { ++calls; assert(n==1 && *t==42); ++deletions; }
static void P4GL_APIENTRY bind(GLenum n,GLuint t) { ++calls; assert(n==GL_TEXTURE_2D); bound=(GLint)t; }
static void P4GL_APIENTRY image(GLenum target,GLint level,GLint internal,GLsizei w,GLsizei h,GLint border,GLenum format,GLenum type,const void *p)
{
    ++calls; ++uploads; assert(target==GL_TEXTURE_2D && bound==42 && alignment==1 && row==0);
    assert(internal==0x8c43 && format==GL_RGBA && type==GL_UNSIGNED_BYTE && !border);
    assert(w==(level?2:4) && h==w); assert(!memcmp(p,pixels,(size_t)w*h*4));
}
static void P4GL_APIENTRY param(GLenum target,GLenum name,GLint v) { ++calls; assert(target==GL_TEXTURE_2D); (void)name; (void)v; }
static void P4GL_APIENTRY store(GLenum n,GLint v) { ++calls; if(n==GL_UNPACK_ALIGNMENT) alignment=v; else row=v; }
static GLenum P4GL_APIENTRY err(void) { ++calls; return error; }
void (P4GL_APIENTRY *p4gl_glGetIntegerv)(GLenum,GLint *)=get;
void (P4GL_APIENTRY *p4gl_glGenTextures)(GLsizei,GLuint *)=gen;
void (P4GL_APIENTRY *p4gl_glDeleteTextures)(GLsizei,const GLuint *)=del;
void (P4GL_APIENTRY *p4gl_glBindTexture)(GLenum,GLuint)=bind;
void (P4GL_APIENTRY *p4gl_glTexImage2D)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void *)=image;
void (P4GL_APIENTRY *p4gl_glTexParameteri)(GLenum,GLenum,GLint)=param;
void (P4GL_APIENTRY *p4gl_glPixelStorei)(GLenum,GLint)=store;
GLenum (P4GL_APIENTRY *p4gl_glGetError)(void)=err;
int main(void)
{
    P4GoldenTexture t={0}; t.storage=pixels; t.level_count=2; t.srgb=1;
    t.levels[0]=(P4GoldenTextureLevel){4,4,pixels}; t.levels[1]=(P4GoldenTextureLevel){2,2,pixels};
    assert(!p4gl_golden_texture_upload(&t) && !calls);
    mode=P4_GAME_MODE_GOLDEN; assert(p4gl_golden_texture_upload(&t)==42);
    assert(uploads==2 && bound==77 && alignment==4 && row==9 && !deletions);
    unsigned before=calls; t.levels[1].width=3; assert(!p4gl_golden_texture_upload(&t) && calls==before);
    t.levels[1].width=2; error=0x0505; assert(!p4gl_golden_texture_upload(&t) && deletions==1);
    assert(bound==77 && alignment==4 && row==9); puts("Golden GL upload/isolation vectors passed"); return 0;
}
