/* Im2D on OpenGL 3.3 core and OpenGL ES 3.0: the RenderWare immediate-mode 2D draws that the device block
 * exposes at +0x18..+0x24. The retail emitters are SkyIm2DRender* in decomp/src/rw/skyinst.c
 * (FUN_0040AFB0, FUN_0040B230, FUN_0040C0F0, FUN_0040D0A0); the port keeps their addresses
 * as the names of the functions below.
 *
 * Vertices are RwSky2DVertex, 64 bytes: screen-space x/y in pixels, depth, texture coordinates,
 * recipZ, and colour as four floats. A draw samples the raster that RwRenderState TEXTURERASTER
 * names (addressing and filter from the render state); vertex colour modulates the texel.
 * With no raster the draw is untextured. Depth test is off. Blending and the alpha test follow
 * the RpSky ALPHA_1 and TEST_1 state (src/render/p4gl_state.c). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <SDL3/SDL.h>

#include "../platform/pad/pad_sdl.h"
#include "../platform/config/p4cfg.h"
#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_profile.h"
#include "p4gl_readback.h"
#include "p4pc/gmem.h"
#include "p4pc/ps2_libc.h"
#include "p4pc/tables.h"
#include "p4pc/battle_sim.h"

/* RwPrimitiveType values used by the Im2D primitive entry points. */
#define P4GL_PRIM_LINELIST 1
#define P4GL_PRIM_POLYLINE 2
#define P4GL_PRIM_TRILIST 3
#define P4GL_PRIM_TRISTRIP 4
#define P4GL_PRIM_TRIFAN 5

static const char kImVertexSource[] =
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec4 aColor;\n"
    "layout(location = 2) in vec2 aUV;\n"
    "layout(location = 3) in float aRecipZ;\n"
    "#ifdef P4_AA_LINE\n"
    "layout(location = 4) in vec3 aNextPos;\n"
    "layout(location = 5) in vec4 aNextColor;\n"
    "layout(location = 6) in vec2 aNextUV;\n"
    "layout(location = 7) in float aNextRecipZ;\n"
    "flat out vec4 vLine;\n"
    "flat out vec2 vLineBounds;\n"
    "bool endpointExited(vec2 d, bool sx, bool positive) {\n"
    "    float distance = abs(d.x) + abs(d.y);\n"
    "    float major = sx ? d.x : d.y, minor = sx ? d.y : d.x;\n"
    "    return distance >= 0.5 && (positive ? major > 0.0 : major < 0.0)\n"
    "        && (distance > 0.5 || minor >= 0.0);\n"
    "}\n"
    "#endif\n"
    "uniform vec2 uViewport;\n"
    "uniform vec4 uCanvas;\n"
    "out vec4 vColor;\n"
    "out vec3 vUVQ;\n"
    "#ifdef P4_CANVAS_CLIP\n"
    "out vec2 vCanvasPos;\n"
    "#endif\n"
    "void main() {\n"
    "    vec3 pos = aPos;\n"
    "    vec4 colour = aColor;\n"
    "    float q = aRecipZ > 0.0 ? aRecipZ : 1.0;\n"
    "    vec3 uvq = vec3(aUV * q, q);\n"
    "#ifdef P4_AA_LINE\n"
    "    vec2 p0 = floor(aPos.xy * 16.0) / 16.0;\n"
    "    vec2 p1 = floor(aNextPos.xy * 16.0) / 16.0;\n"
    "    vec2 d = p1 - p0;\n"
    "    bool sx = abs(d.x) >= abs(d.y);\n"
    "    float majorDelta = sx ? d.x : d.y;\n"
    "    float start = sx ? p0.x : p0.y, dependent = sx ? p0.y : p0.x;\n"
    "    float slope = majorDelta != 0.0 ? (sx ? d.y : d.x) / majorDelta : 0.0;\n"
    "    vec2 r0 = floor(p0 + 0.5), r1 = floor(p1 + 0.5);\n"
    "    float first = sx ? r0.x : r0.y, last = sx ? r1.x : r1.y;\n"
    "    float direction = majorDelta >= 0.0 ? 1.0 : -1.0;\n"
    "    if (endpointExited(p0 - r0, sx, majorDelta >= 0.0)) first += direction;\n"
    "    if (!endpointExited(p1 - r1, sx, majorDelta >= 0.0)) last -= direction;\n"
    "    float lo = min(first, last), hi = max(first, last);\n"
    "    vLineBounds = majorDelta == 0.0 || direction * (last - first) < 0.0 ? vec2(1.0, 0.0) : vec2(lo, hi);\n"
    "    vLine = vec4(start, dependent, slope, sx ? 1.0 : 0.0);\n"
    "    float dep0 = dependent + slope * (lo - start), dep1 = dependent + slope * (hi - start);\n"
    "    vec2 lower = vec2(lo - 0.5, floor(min(dep0, dep1)) - 0.5);\n"
    "    vec2 upper = vec2(hi + 0.5, ceil(max(dep0, dep1)) + 0.5);\n"
    "    vec2 corner = mix(lower, upper, vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1)));\n"
    "    pos.xy = sx ? corner : corner.yx;\n"
    "    float t = majorDelta != 0.0 ? (corner.x - start) / majorDelta : 0.0;\n"
    "    pos.z = mix(aPos.z, aNextPos.z, t);\n"
    "    colour = mix(aColor, aNextColor, t);\n"
    "    float nextQ = aNextRecipZ > 0.0 ? aNextRecipZ : 1.0;\n"
    "    uvq = mix(uvq, vec3(aNextUV * nextQ, nextQ), t);\n"
    "#endif\n"
    "    /* A GS vertex at an integer coordinate is on a pixel centre (+ 0.5 here), so the edges of a sprite or a letterbox bar run through the\n"
    "     * centres of their boundary pixels. The GS includes a centre on a top or left edge and leaves out one on a bottom or right edge; GL leaves\n"
    "     * the choice to the GPU, and llvmpipe and an Adreno 650 include the bottom edge of the picture instead of the top one (the title's letterbox\n"
    "     * bars came out one row low). p4gl_gl.c probes the rule: P4_TIE_BIAS moves the vertices 1/128 pixel up and left (a few of the subpixel steps GPUs\n"
    "     * snap them to), which puts such a centre on the GS's side. */\n"
    "#ifdef P4_TIE_BIAS\n"
    "    const float kTie = 1.0 / 128.0;\n"
    "#else\n"
    "    const float kTie = 0.0;\n"
    "#endif\n"
    "    /* Sky converts to GS 12.4 after adding XYOFFSET. Within its positive guardband this floors\n"
    "     * screen coordinates, including negative/offscreen ones, before the pixel-centre rule. */\n"
    "    vec2 gsPos = floor(pos.xy * 16.0) / 16.0;\n"
    "    vec2 ndc = vec2((gsPos.x + 0.5 - kTie) / uViewport.x * 2.0 - 1.0, 1.0 - (gsPos.y + 0.5 - kTie) / uViewport.y * 2.0);\n"
    "    if (uCanvas != vec4(1.0, 1.0, 0.0, 0.0)) ndc = ndc * uCanvas.xy + uCanvas.zw;\n"
    "#ifdef P4_CANVAS_CLIP\n"
    "    vCanvasPos = ndc;\n"
    "#endif\n"
    "    /* The vertex z is a GS Z in 0..65535 (larger is nearer); GL depth 0 is near; kept off the far plane. */\n"
    "    float depth = (1.0 - clamp(pos.z, 0.0, 65535.0) / 65535.0) * 0.999;\n"
    "    gl_Position = vec4(ndc, depth * 2.0 - 1.0, 1.0);\n"
    "    /* RwIm2DVertex colour is 0..255 per channel (RenderWare convention, as the game supplies it). */\n"
    "    vColor = colour / 255.0;\n"
    "    /* textureProj divides by q, which gives perspective-correct UVs. recipZ 0 means none. */\n"
    "    vUVQ = uvq;\n"
    "}\n";

/* Rectangle transport is conservative; the fragment shader owns GS coverage.
 * This does not move the GS +.5 sample grid or any continuous 3D geometry. */
static const char kCellVertexSource[] =
    "layout(location=0) in vec4 aRect;\n"
    "layout(location=1) in vec4 aColor0;\n"
    "layout(location=2) in vec4 aColor1;\n"
    "layout(location=3) in vec4 aColor2;\n"
    "layout(location=4) in vec4 aColor3;\n"
    "layout(location=5) in vec3 aUVQ0;\n"
    "layout(location=6) in vec3 aUVQ1;\n"
    "layout(location=7) in vec3 aUVQ2;\n"
    "layout(location=8) in vec3 aUVQ3;\n"
    "layout(location=9) in vec4 aDepth;\n"
    "layout(location=10) in float aCellFlags;\n"
    "uniform vec2 uViewport;\n"
    "uniform vec2 uCellPhysical;\n"
    "uniform vec2 uCellNative;\n"
    "uniform vec4 uCanvas;\n"
    "flat out vec4 vRect;\n"
    "flat out vec4 vCells;\n"
    "flat out vec4 vColor0, vColor1, vColor2, vColor3;\n"
    "flat out vec3 vUVQ0, vUVQ1, vUVQ2, vUVQ3;\n"
    "flat out vec4 vDepth;\n"
    "flat out int vCellFlags;\n"
    "#ifdef P4_CANVAS_CLIP\n"
    "out vec2 vCanvasPos;\n"
    "#endif\n"
    "void main() {\n"
    "    vRect=aRect;\n"
    "    if(uCellNative==uViewport && uCanvas==vec4(1.0,1.0,0.0,0.0)) vCells=ceil(aRect);\n"
    "    else {\n"
    "        vec4 edges=aRect+vec4(0.5);\n"
    "#ifdef P4_TIE_BIAS\n"
    "        edges-=vec4(1.0/128.0);\n"
    "#endif\n"
    "        edges=(edges/uViewport.xyxy-vec4(0.5))*uCanvas.xyxy+vec4(0.5)+uCanvas.zwzw*vec4(0.5,-0.5,0.5,-0.5);\n"
    "        edges=edges*uCellNative.xyxy-vec4(0.5);\n"
    "        vCells=ceil(edges);\n"
    "    }\n"
    "    vColor0=aColor0/255.0; vColor1=aColor1/255.0; vColor2=aColor2/255.0; vColor3=aColor3/255.0;\n"
    "    vUVQ0=vec3(aUVQ0.xy*aUVQ0.z,aUVQ0.z); vUVQ1=vec3(aUVQ1.xy*aUVQ1.z,aUVQ1.z);\n"
    "    vUVQ2=vec3(aUVQ2.xy*aUVQ2.z,aUVQ2.z); vUVQ3=vec3(aUVQ3.xy*aUVQ3.z,aUVQ3.z);\n"
    "    vDepth=(1.0-clamp(aDepth,vec4(0.0),vec4(65535.0))/65535.0)*0.999;\n"
    "    vCellFlags=int(aCellFlags);\n"
    "    vec2 corner=vec2(float(gl_VertexID&1), float(gl_VertexID>>1));\n"
    "    vec2 p=mix(vCells.xy, vCells.zw, corner);\n"
    "    vec2 ndc=vec2(p.x/uCellNative.x*2.0-1.0, 1.0-p.y/uCellNative.y*2.0);\n"
    "    ndc+=(corner*2.0-1.0)*vec2(2.0,-2.0)/uCellPhysical;\n"
    "#ifdef P4_CANVAS_CLIP\n"
    "    vCanvasPos=ndc;\n"
    "#endif\n"
    "    gl_Position=vec4(ndc,vDepth.x*2.0-1.0,1.0);\n"
    "}\n";

static const char kImFragmentSource[] =
    "#ifndef P4_RECT_CELLS\n"
    "in vec4 vColor;\n"
    "in vec3 vUVQ;\n"
    "#else\n"
    "flat in vec4 vRect, vCells;\n"
    "flat in vec4 vColor0, vColor1, vColor2, vColor3;\n"
    "flat in vec3 vUVQ0, vUVQ1, vUVQ2, vUVQ3;\n"
    "flat in vec4 vDepth;\n"
    "flat in int vCellFlags;\n"
    "uniform vec2 uCellPhysical;\n"
    "uniform vec2 uCellNative;\n"
    "uniform int uCellContinuousUV;\n"
    "vec4 cellWeights(vec2 p) {\n"
    "    vec2 t=(p-vRect.xy)/(vRect.zw-vRect.xy);\n"
    "    if((vCellFlags&1)!=0) {\n"
    "        if(t.y<=t.x) return vec4(1.0-t.x,t.x-t.y,0.0,t.y);\n"
    "        return vec4(1.0-t.y,0.0,t.y-t.x,t.x);\n"
    "    }\n"
    "    if(t.x+t.y<=1.0) return vec4(1.0-t.x-t.y,t.x,t.y,0.0);\n"
    "    return vec4(0.0,1.0-t.y,1.0-t.x,t.x+t.y-1.0);\n"
    "}\n"
    "#endif\n"
    "#ifdef P4_CANVAS_CLIP\n"
    "in vec2 vCanvasPos;\n"
    "uniform vec4 uCanvasClip;\n"
    "#endif\n"
    "uniform vec4 uCanvas;\n"
    "uniform vec2 uPixelDensity;\n"
    "uniform vec2 uVirtualTextureSize;\n"
    "#if defined(P4_AA_LINE) || defined(P4_RECT_CELLS)\n"
    "uniform vec2 uViewport;\n"
    "#endif\n"
    "#ifdef P4_AA_LINE\n"
    "flat in vec4 vLine;\n"
    "flat in vec2 vLineBounds;\n"
    "uniform vec2 uLinePixelScale;\n"
    "uniform int uLineABE;\n"
    "#endif\n"
    "uniform sampler2D uTexture;\n"
    "uniform int uTextured;\n"
    "uniform int uBorder;\n"
    "uniform int uAlphaFunc;\n"
    "uniform float uAlphaRef;\n"
    "uniform int uAlphaInvert;\n"
    "bool alphaPasses(float a) {\n"
    "    float t = a * 128.0;\n"
    "    if (uAlphaFunc == 8) return false;\n"
    "    if (uAlphaFunc == 2) return t < uAlphaRef;\n"
    "    if (uAlphaFunc == 3) return t <= uAlphaRef;\n"
    "    if (uAlphaFunc == 4) return t == uAlphaRef;\n"
    "    if (uAlphaFunc == 5) return t >= uAlphaRef;\n"
    "    if (uAlphaFunc == 6) return t > uAlphaRef;\n"
    "    if (uAlphaFunc == 7) return t != uAlphaRef;\n"
    "    return true;\n"
    "}\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "#if defined(P4_CANVAS_CLIP) && !defined(P4_RECT_CELLS)\n"
    "    if (any(lessThan(vCanvasPos, uCanvasClip.xy)) || any(greaterThanEqual(vCanvasPos, uCanvasClip.zw))) discard;\n"
    "#endif\n"
    "#ifdef P4_RECT_CELLS\n"
    "    vec2 physical=vec2(gl_FragCoord.x,uCellPhysical.y-gl_FragCoord.y);\n"
    "    vec2 cell=floor(physical*uCellNative/uCellPhysical);\n"
    "    if(all(lessThanEqual(uCellNative,vec2(32768.0))) && all(lessThanEqual(uCellPhysical,vec2(32768.0)))) {\n"
    "        ivec2 size=ivec2(uCellPhysical), pixel=ivec2(gl_FragCoord.xy);\n"
    "        pixel.y=size.y-1-pixel.y;\n"
    "        cell=vec2(((pixel*2+ivec2(1))*ivec2(uCellNative))/(size*2));\n"
    "    }\n"
    "    if(any(lessThan(cell,vCells.xy)) || any(greaterThanEqual(cell,vCells.zw))) discard;\n"
    "#ifdef P4_CANVAS_CLIP\n"
    "    vec2 nativeNdc=(cell+vec2(0.5))/uCellNative*vec2(2.0,-2.0)+vec2(-1.0,1.0);\n"
    "    if(any(lessThan(nativeNdc,uCanvasClip.xy)) || any(greaterThanEqual(nativeNdc,uCanvasClip.zw))) discard;\n"
    "#endif\n"
    "    bool identity=uCanvas==vec4(1.0,1.0,0.0,0.0);\n"
    "    vec4 weights=vec4(0.0);\n"
    "    if((vCellFlags&6)!=6 || (uTextured!=0 && uCellContinuousUV==0)) {\n"
    "        vec2 samplePoint=cell;\n"
    "        if(!identity || uCellNative!=uViewport) {\n"
    "            samplePoint=(cell+vec2(0.5))*uViewport/uCellNative;\n"
    "            if(!identity) samplePoint=(samplePoint-uViewport*0.5-uCanvas.zw*vec2(1.0,-1.0)*uViewport*0.5)/uCanvas.xy+uViewport*0.5;\n"
    "            samplePoint-=vec2(0.5);\n"
    "        }\n"
    "        weights=cellWeights(samplePoint);\n"
    "    }\n"
    "    vec4 c=(vCellFlags&4)!=0 ? vColor0 : vColor0*weights.x+vColor1*weights.y+vColor2*weights.z+vColor3*weights.w;\n"
    "    gl_FragDepth=(vCellFlags&2)!=0 ? gl_FragCoord.z : dot(vDepth,weights);\n"
    "#else\n"
    "    vec4 c = vColor;\n"
    "#endif\n"
    "    if (uTextured != 0) {\n"
    "        /* The half-pixel shift puts a nearest sample exactly on a texel boundary, where float error picks\n"
    "         * either side: a 1/64 texel bias picks the GS's (the later) texel. */\n"
    "#ifdef P4_RECT_CELLS\n"
    "        /* Preserve the original virtual-centre interpolation phase for continuous frame/camera copies. */\n"
    "        vec4 uvWeights=weights;\n"
    "        if(uCellContinuousUV!=0) {\n"
    "            vec2 p=physical*uViewport/uCellPhysical;\n"
    "            if(!identity) p=(p-uViewport*0.5-uCanvas.zw*vec2(1.0,-1.0)*uViewport*0.5)/uCanvas.xy+uViewport*0.5;\n"
    "            uvWeights=cellWeights(p-vec2(0.5));\n"
    "        }\n"
    "        vec3 uvq=vUVQ0*uvWeights.x+vUVQ1*uvWeights.y+vUVQ2*uvWeights.z+vUVQ3*uvWeights.w;\n"
    "#else\n"
    "        vec3 uvq = vUVQ;\n"
    "#ifdef P4_TIE_BIAS\n"
    "        /* the vertex shader moved the quad 1/128 pixel up and left (x - 1/128, GS y - 1/128, which is GL y + 1/128): the interpolants are affine in the\n"
    "         * screen, so the unmoved value at this pixel is the moved one less that much of their slopes */\n"
    "        uvq -= (1.0 / 128.0) * (dFdx(uvq) * uPixelDensity.x - dFdy(uvq) * uPixelDensity.y);\n"
    "#endif\n"
    "#endif\n"
    "        vec2 texels = uVirtualTextureSize.x > 0.0 ? uVirtualTextureSize : vec2(textureSize(uTexture, 0));\n"
    "        uvq.xy += uvq.z / (texels * 64.0);\n"
    "        vec4 tx;\n"
    "        if (uBorder != 0) {\n"
    "            tx = p4SampleBorder(uTexture, uvq.xy / uvq.z, uBorder);\n"
    "        } else {\n"
    "            tx = textureProj(uTexture, uvq);\n"
    "        }\n"
    "        /* A frame-buffer feedback draw (TEX0.TCC = 0 in the GS) reads colour only: the frame's alpha channel holds blend\n"
    "         * state, not a picture. */\n"
    "        if (uTextured == 2) {\n"
    "            tx.a = 1.0;\n"
    "        }\n"
    "        c *= tx;\n"
    "    }\n"
    "#ifdef P4_AA_LINE\n"
    "    vec2 pixel = vec2(gl_FragCoord.x * uLinePixelScale.x, uViewport.y - gl_FragCoord.y * uLinePixelScale.y);\n"
    "    if (uCanvas != vec4(1.0, 1.0, 0.0, 0.0)) pixel = (pixel - uViewport * 0.5 - uCanvas.zw * vec2(1.0, -1.0) * uViewport * 0.5) / uCanvas.xy + uViewport * 0.5;\n"
    "    pixel = floor(pixel);\n"
    "    float major = vLine.w != 0.0 ? pixel.x : pixel.y;\n"
    "    float minor = vLine.w != 0.0 ? pixel.y : pixel.x;\n"
    "    float coverage = 1.0 - abs(minor - (vLine.y + vLine.z * (major - vLine.x)));\n"
    "    if (major < vLineBounds.x || major > vLineBounds.y || coverage <= 0.0) discard;\n"
    "    /* GS AA1 substitutes coverage only for full source alpha when ABE is set.\n"
    "     * The software-GS edge coverage is 16 bits, converted to the GS 7-bit factor. */\n"
    "    if (uLineABE == 0 || c.a >= 1.0) c.a = floor(min(coverage, 1.0) * 65535.0 / 512.0) / 128.0;\n"
    "#endif\n"
    "    if (uAlphaFunc != 0 && alphaPasses(c.a) == (uAlphaInvert != 0)) {\n"
    "        discard;\n"
    "    }\n"
    "#ifdef P4_ROUND_BIAS\n"
    "    c.rgb = max(c.rgb - 0.5 / 255.0, 0.0);\n"
    "#endif\n"
    "    fragColor = c;\n"
    "}\n";

/* DATE and AA1-line variants share the material/alpha-test fragment path. Line
 * expansion is compiled out of ordinary draws, including their vertex fetches. */
typedef struct ImProgram {
    GLuint program;
    GLint viewport;
    GLint canvas;
    GLint canvasClip;
    GLint pixelDensity;
    GLint virtualTextureSize;
    GLint cellPhysical;
    GLint cellNative;
    GLint cellContinuousUV;
    GLint textured;
    GLint alphaFunc;
    GLint alphaRef;
    GLint alphaInvert;
    GLint sampler;
    GLint date;
    GLint border;
    GLint linePixelScale;
    GLint lineABE;
} ImProgram;

static ImProgram sProgs[16];
static GLuint sProgram;
static GLint sViewportLocation;
static GLint sCanvasLocation;
static GLint sCanvasClipLocation;
static GLint sPixelDensityLocation;
static GLint sVirtualTextureSizeLocation;
static GLint sCellPhysicalLocation;
static GLint sCellNativeLocation;
static GLint sCellContinuousUVLocation;
static GLint sTexturedLocation;
static GLint sAlphaFuncLocation;
static GLint sAlphaRefLocation;
static GLint sAlphaInvertLocation;
static GLint sSamplerLocation;
static GLint sDateLocation;
static GLint sBorderLocation;
static GLint sLinePixelScaleLocation;
static GLint sLineABELocation;
static GLuint sDestTexture;
static int sDestW;
static int sDestH;
static GLuint sVao;
static GLuint sVbo;
static GLuint sEbo;
static GLuint sLineVao;
static P4Sky2Vertex *sLineVertices;
static size_t sLineCapacity;
static int sReady;   /* set once the program and buffers exist */
static int sFailed;  /* set after a failed setup; draws then fail without retrying */

/* TL, TR, BL, BR retain the authored diagonal and attributes. One instanced
 * envelope shades each native GS sample across its complete physical cell. */
typedef struct CellRect {
    float bounds[4], color[4][4], uvq[4][3], depth[4];
    float flags; /* diagonal TL-BR, constant Z, constant colour */
} CellRect;
typedef struct CellCandidate {
    float bounds[4];
    uint32_t corner[4];
    unsigned diagonal;
} CellCandidate;
typedef struct CellRun {
    int cells;
    uint32_t first, count;
} CellRun;
static CellRect *sCells;
static CellRun *sCellRuns;
static uint32_t *sCellIndices;
static size_t sCellCapacity, sCellRunCapacity, sCellIndexCapacity, sCellBase = SIZE_MAX;
static uint32_t sCellCount, sCellRunCount, sCellIndexCount;
static GLuint sCellVao, sCellVbo, sCellEbo;

static void cell_triangle(GLenum mode, const uint16_t *indices, uint32_t triangle, uint32_t out[3])
{
    if (mode == GL_TRIANGLES) {
        out[0] = triangle * 3; out[1] = out[0] + 1; out[2] = out[0] + 2;
    } else if (mode == GL_TRIANGLE_FAN) {
        out[0] = 0; out[1] = triangle + 1; out[2] = triangle + 2;
    } else {
        out[0] = triangle; out[1] = triangle + 1; out[2] = triangle + 2;
    }
    if (indices) for (unsigned i = 0; i < 3; i++) out[i] = indices[out[i]];
}

static int cell_same_attributes(const P4Sky2Vertex *a, const P4Sky2Vertex *b)
{
    const float aq = a->recipZ > 0 ? a->recipZ : 1, bq = b->recipZ > 0 ? b->recipZ : 1;
    return a->z == b->z && a->u == b->u && a->v == b->v && aq == bq &&
           a->r == b->r && a->g == b->g && a->b == b->b && a->a == b->a;
}

static int cell_rectangle(GLenum mode, const P4Sky2Vertex *vertices, const uint16_t *indices,
                          uint32_t triangle, CellCandidate *out)
{
    uint32_t refs[6], masks[2] = {0, 0}, seen = 0;
    float x[6], y[6];
    cell_triangle(mode, indices, triangle, refs);
    cell_triangle(mode, indices, triangle + 1, refs + 3);
    for (unsigned i = 0; i < 6; i++) {
        x[i] = floorf(vertices[refs[i]].x * 16.0f) / 16.0f;
        y[i] = floorf(vertices[refs[i]].y * 16.0f) / 16.0f;
        if (i == 0) {
            out->bounds[0] = out->bounds[2] = x[i];
            out->bounds[1] = out->bounds[3] = y[i];
        } else {
            if (x[i] < out->bounds[0]) out->bounds[0] = x[i];
            if (y[i] < out->bounds[1]) out->bounds[1] = y[i];
            if (x[i] > out->bounds[2]) out->bounds[2] = x[i];
            if (y[i] > out->bounds[3]) out->bounds[3] = y[i];
        }
    }
    if (!(out->bounds[0] < out->bounds[2] && out->bounds[1] < out->bounds[3])) return 0;
    for (unsigned i = 0; i < 6; i++) {
        unsigned corner, bit;
        if ((x[i] != out->bounds[0] && x[i] != out->bounds[2]) ||
            (y[i] != out->bounds[1] && y[i] != out->bounds[3])) return 0;
        corner = (x[i] == out->bounds[2]) | ((y[i] == out->bounds[3]) << 1);
        bit = 1u << corner;
        if (masks[i / 3] & bit) return 0;
        masks[i / 3] |= bit;
        if (seen & bit) {
            if (!cell_same_attributes(&vertices[refs[i]], &vertices[out->corner[corner]])) return 0;
        } else {
            out->corner[corner] = refs[i];
            seen |= bit;
        }
    }
    if (seen != 15 || ((masks[0] & masks[1]) != 6 && (masks[0] & masks[1]) != 9)) return 0;
    out->diagonal = (masks[0] & masks[1]) == 9;
    return 1;
}

static void *cell_grow(void *buffer, size_t *capacity, size_t needed, size_t elementSize)
{
    const size_t maximum = (size_t)PTRDIFF_MAX / elementSize;
    size_t next = *capacity ? *capacity : 16;
    void *grown;
    if (needed <= *capacity) return buffer;
    if (needed > maximum) return NULL;
    while (next < needed) {
        if (next > maximum / 2) { next = needed; break; }
        next *= 2;
    }
    grown = realloc(buffer, next * elementSize);
    if (grown) *capacity = next;
    return grown;
}

static int cell_run(int cells, uint32_t first, uint32_t count)
{
    if (sCellRunCount && sCellRuns[sCellRunCount - 1].cells == cells) {
        sCellRuns[sCellRunCount - 1].count += count;
    } else {
        CellRun *grown = cell_grow(sCellRuns, &sCellRunCapacity, (size_t)sCellRunCount + 1, sizeof *grown);
        if (!grown) return 0;
        sCellRuns = grown;
        sCellRuns[sCellRunCount++] = (CellRun){cells, first, count};
    }
    return 1;
}

static int cell_append_rect(const P4Sky2Vertex *vertices, const CellCandidate *candidate)
{
    CellRect *grown = cell_grow(sCells, &sCellCapacity, (size_t)sCellCount + 1, sizeof *grown);
    CellRect *rect;
    unsigned flags = candidate->diagonal | 6u;
    if (!grown) return 0;
    sCells = grown;
    rect = &sCells[sCellCount];
    memcpy(rect->bounds, candidate->bounds, sizeof rect->bounds);
    for (unsigned i = 0; i < 4; i++) {
        const P4Sky2Vertex *v = &vertices[candidate->corner[i]];
        rect->depth[i] = v->z;
        rect->color[i][0] = v->r; rect->color[i][1] = v->g;
        rect->color[i][2] = v->b; rect->color[i][3] = v->a;
        rect->uvq[i][0] = v->u; rect->uvq[i][1] = v->v;
        rect->uvq[i][2] = v->recipZ > 0 ? v->recipZ : 1;
        if (i) {
            if (rect->depth[i] != rect->depth[0]) flags &= ~2u;
            if (v->r != rect->color[0][0] || v->g != rect->color[0][1] ||
                v->b != rect->color[0][2] || v->a != rect->color[0][3]) flags &= ~4u;
        }
    }
    rect->flags = (float)flags;
    if (!cell_run(1, sCellCount, 1)) return 0;
    sCellCount++;
    return 1;
}

static int cell_append_triangle(GLenum mode, const uint16_t *indices, uint32_t triangle)
{
    uint32_t *grown;
    if (sCellIndexCount > INT_MAX - 3) return 0;
    grown = cell_grow(sCellIndices, &sCellIndexCapacity, (size_t)sCellIndexCount + 3, sizeof *grown);
    if (!grown) return 0;
    sCellIndices = grown;
    cell_triangle(mode, indices, triangle, &sCellIndices[sCellIndexCount]);
    if (!cell_run(0, sCellIndexCount, 3)) return 0;
    sCellIndexCount += 3;
    return 1;
}

/* No allocation/upload when a draw has no rectangles. Keep source ordering for
 * mixed lists/strips/fans, and the original draw-log/skip identity. */
static int cell_build(GLenum mode, const P4Sky2Vertex *vertices, const uint16_t *indices, uint32_t count)
{
    const uint32_t triangles = mode == GL_TRIANGLES ? count / 3 : count > 2 ? count - 2 : 0;
    uint32_t first = 0, triangle;
    CellCandidate candidate;
    while (first + 1 < triangles && !cell_rectangle(mode, vertices, indices, first, &candidate)) first++;
    if (first + 1 >= triangles) return 0;
    sCellCount = sCellRunCount = sCellIndexCount = 0;
    for (triangle = 0; triangle < first; triangle++)
        if (!cell_append_triangle(mode, indices, triangle)) goto failed;
    if (!cell_append_rect(vertices, &candidate)) goto failed;
    for (triangle = first + 2; triangle < triangles;) {
        if (triangle + 1 < triangles && cell_rectangle(mode, vertices, indices, triangle, &candidate)) {
            if (!cell_append_rect(vertices, &candidate)) goto failed;
            triangle += 2;
        } else {
            if (!cell_append_triangle(mode, indices, triangle++)) goto failed;
        }
    }
    return 1;
failed:
    fprintf(stderr, "p4gl: cannot allocate logical-cell rectangle transport\n");
    return -1;
}

static void cell_bind(uint32_t first)
{
    static const GLint sizes[11] = {4,4,4,4,4,3,3,3,3,4,1};
    static const size_t offsets[11] = {
        offsetof(CellRect, bounds), offsetof(CellRect, color[0]), offsetof(CellRect, color[1]),
        offsetof(CellRect, color[2]), offsetof(CellRect, color[3]), offsetof(CellRect, uvq[0]),
        offsetof(CellRect, uvq[1]), offsetof(CellRect, uvq[2]), offsetof(CellRect, uvq[3]),
        offsetof(CellRect, depth), offsetof(CellRect, flags)
    };
    if (!sCellVao) {
        p4gl_glGenVertexArrays(1, &sCellVao);
        p4gl_glGenBuffers(1, &sCellVbo);
        p4gl_glGenBuffers(1, &sCellEbo);
        p4gl_glBindVertexArray(sCellVao);
        for (GLuint i = 0; i < 11; i++) {
            p4gl_glEnableVertexAttribArray(i);
            p4gl_glVertexAttribDivisor(i, 1);
        }
    } else p4gl_glBindVertexArray(sCellVao);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, sCellVbo);
    if (sCellBase != first) {
        for (GLuint i = 0; i < 11; i++)
            p4gl_glVertexAttribPointer(i, sizes[i], GL_FLOAT, GL_FALSE, sizeof(CellRect),
                                      (const void *)((size_t)first * sizeof(CellRect) + offsets[i]));
        sCellBase = first;
    }
}

static GLuint compile_stage(GLenum stage, const char *source, int features)
{
    static const char *const defines[] = {
        "", "#define P4_AA_LINE\n", "#define P4_CANVAS_CLIP\n",
        "#define P4_AA_LINE\n#define P4_CANVAS_CLIP\n",
        "#define P4_RECT_CELLS\n", "#define P4_RECT_CELLS\n#define P4_AA_LINE\n",
        "#define P4_RECT_CELLS\n#define P4_CANVAS_CLIP\n",
        "#define P4_RECT_CELLS\n#define P4_CANVAS_CLIP\n#define P4_AA_LINE\n"
    };
    char *combined;
    size_t length, prefixLength;
    GLuint shader;

    if (features == 0) {
        return p4gl_gl_compile_shader(stage, source, "Im2D");
    }
    prefixLength = strlen(defines[features]);
    length = strlen(source) + 1;
    combined = malloc(prefixLength + length);
    if (combined == NULL) {
        return 0;
    }
    memcpy(combined, defines[features], prefixLength);
    memcpy(combined + prefixLength, source, length);
    shader = p4gl_gl_compile_shader(stage, combined, (features & 2) ? "Im2D clipped canvas" : "Im2D AA1 line");
    free(combined);
    return shader;
}

static int link_program(GLuint vertexShader, GLuint fragmentShader, ImProgram *out)
{
    GLint linked = GL_FALSE;

    out->program = p4gl_glCreateProgram();
    p4gl_glAttachShader(out->program, vertexShader);
    p4gl_glAttachShader(out->program, fragmentShader);
    p4gl_glLinkProgram(out->program);
    p4gl_glGetProgramiv(out->program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        fprintf(stderr, "p4gl: Im2D program link failed\n");
        return 0;
    }
    out->viewport = p4gl_glGetUniformLocation(out->program, "uViewport");
    out->canvas = p4gl_glGetUniformLocation(out->program, "uCanvas");
    out->canvasClip = -1;
    out->pixelDensity = p4gl_glGetUniformLocation(out->program, "uPixelDensity");
    out->virtualTextureSize = p4gl_glGetUniformLocation(out->program, "uVirtualTextureSize");
    out->cellPhysical = p4gl_glGetUniformLocation(out->program, "uCellPhysical");
    out->cellNative = p4gl_glGetUniformLocation(out->program, "uCellNative");
    out->cellContinuousUV = p4gl_glGetUniformLocation(out->program, "uCellContinuousUV");
    out->textured = p4gl_glGetUniformLocation(out->program, "uTextured");
    out->alphaFunc = p4gl_glGetUniformLocation(out->program, "uAlphaFunc");
    out->alphaRef = p4gl_glGetUniformLocation(out->program, "uAlphaRef");
    out->alphaInvert = p4gl_glGetUniformLocation(out->program, "uAlphaInvert");
    out->sampler = p4gl_glGetUniformLocation(out->program, "uTexture");
    out->date = p4gl_glGetUniformLocation(out->program, "uDate");
    out->border = p4gl_glGetUniformLocation(out->program, "uBorder");
    out->linePixelScale = p4gl_glGetUniformLocation(out->program, "uLinePixelScale");
    out->lineABE = p4gl_glGetUniformLocation(out->program, "uLineABE");
    p4gl_glUseProgram(out->program);
    p4gl_glUniform1i(out->sampler, 0);
    if (out->date >= 0) {
        p4gl_glUniform1i(p4gl_glGetUniformLocation(out->program, "uDest"), 1);
    }
    return 1;
}

static int build_variant_program(ImProgram *out, int date, int line, int clip, int cells);

static int select_program(int date, int line, int clip, int cells)
{
    ImProgram *p = &sProgs[(date != 0) | ((line != 0) << 1) | ((clip != 0) << 2) | ((cells != 0) << 3)];
    if ((clip || cells) && p->program == 0) {
        if (sFailed || !build_variant_program(p, date, line, clip, cells)) {
            sFailed = 1;
            return 0;
        }
    }

    sProgram = p->program;
    sViewportLocation = p->viewport;
    sCanvasLocation = p->canvas;
    sCanvasClipLocation = p->canvasClip;
    sPixelDensityLocation = p->pixelDensity;
    sVirtualTextureSizeLocation = p->virtualTextureSize;
    sCellPhysicalLocation = p->cellPhysical;
    sCellNativeLocation = p->cellNative;
    sCellContinuousUVLocation = p->cellContinuousUV;
    sTexturedLocation = p->textured;
    sAlphaFuncLocation = p->alphaFunc;
    sAlphaRefLocation = p->alphaRef;
    sAlphaInvertLocation = p->alphaInvert;
    sSamplerLocation = p->sampler;
    sDateLocation = p->date;
    sBorderLocation = p->border;
    sLinePixelScaleLocation = p->linePixelScale;
    sLineABELocation = p->lineABE;
    return 1;
}

/* The fragment source with the destination alpha test added: the frame buffer's alpha MSB (GS alpha 0x80 is 1.0 here) must
 * equal DATM, read from a copy of the target on texture unit 1. */
static char *date_fragment_source(void)
{
    static const char kUniforms[] = "uniform int uTextured;\nuniform sampler2D uDest;\nuniform int uDate;\n";
    static const char kTest[] =
        "    if (uDate != 0) {\n"
        "        bool set = texelFetch(uDest, ivec2(gl_FragCoord.xy), 0).a >= 0.99;\n"
        "        if (set != (uDate == 2)) {\n"
        "            discard;\n"
        "        }\n"
        "    }\n"
        "    fragColor = c;\n";
    const char *u = strstr(kImFragmentSource, "uniform int uTextured;\n");
    const char *f = strstr(kImFragmentSource, "    fragColor = c;\n");
    char *out;
    size_t n;

    if (u == NULL || f == NULL) {
        return NULL;
    }
    n = strlen(kImFragmentSource) + sizeof kUniforms + sizeof kTest;
    out = malloc(n);
    if (out == NULL) {
        return NULL;
    }
    snprintf(out, n, "%.*s%s%.*s%s", (int)(u - kImFragmentSource), kImFragmentSource, kUniforms,
             (int)(f - (u + strlen("uniform int uTextured;\n"))), u + strlen("uniform int uTextured;\n"), kTest);
    {
        /* the tail after the replaced statement */
        size_t have = strlen(out);

        snprintf(out + have, n - have, "%s", f + strlen("    fragColor = c;\n"));
    }
    return out;
}

/* Native programs stay unchanged; clipped/cell variants are compiled on demand. */
static int build_variant_program(ImProgram *out, int date, int line, int clip, int cells)
{
    char *dateSource = date ? date_fragment_source() : NULL;
    const int features = (line ? 1 : 0) | (clip ? 2 : 0) | (cells ? 4 : 0);
    GLuint vertex, fragment;
    int ok;

    if (date && dateSource == NULL) return 0;
    vertex = compile_stage(GL_VERTEX_SHADER, cells ? kCellVertexSource : kImVertexSource, features);
    fragment = compile_stage(GL_FRAGMENT_SHADER, date ? dateSource : kImFragmentSource, features);
    free(dateSource);
    ok = vertex != 0 && fragment != 0 && link_program(vertex, fragment, out);
    if (vertex != 0) p4gl_glDeleteShader(vertex);
    if (fragment != 0) p4gl_glDeleteShader(fragment);
    if (ok) {
        out->canvasClip = p4gl_glGetUniformLocation(out->program, "uCanvasClip");
    } else if (out->program != 0) {
        p4gl_glDeleteProgram(out->program);
        out->program = 0;
    }
    return ok;
}

/* Builds the Im2D program and its VAO/VBO/EBO on first use. Needs the GL context current. */
static int im2d_init(void)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    if (sReady) {
        return 1;
    }
    if (sFailed) {
        return 0;
    }

    vertexShader = compile_stage(GL_VERTEX_SHADER, kImVertexSource, 0);
    fragmentShader = compile_stage(GL_FRAGMENT_SHADER, kImFragmentSource, 0);
    if (vertexShader == 0 || fragmentShader == 0) {
        sFailed = 1;
        return 0;
    }

    if (!link_program(vertexShader, fragmentShader, &sProgs[0])) {
        sFailed = 1;
        return 0;
    }
    {
        char *dateSource = date_fragment_source();
        GLuint dateFragment = dateSource != NULL ? compile_stage(GL_FRAGMENT_SHADER, dateSource, 0) : 0;

        free(dateSource);
        if (dateFragment == 0 || !link_program(vertexShader, dateFragment, &sProgs[1])) {
            sFailed = 1;
            return 0;
        }
    }
    {
        char *dateSource = date_fragment_source();
        GLuint lineVertex = compile_stage(GL_VERTEX_SHADER, kImVertexSource, 1);
        GLuint lineFragment = compile_stage(GL_FRAGMENT_SHADER, kImFragmentSource, 1);
        GLuint lineDate = dateSource != NULL ? compile_stage(GL_FRAGMENT_SHADER, dateSource, 1) : 0;

        free(dateSource);
        if (lineVertex == 0 || lineFragment == 0 || lineDate == 0 ||
            !link_program(lineVertex, lineFragment, &sProgs[2]) ||
            !link_program(lineVertex, lineDate, &sProgs[3])) {
            sFailed = 1;
            return 0;
        }
    }
    select_program(0, 0, 0, 0);

    p4gl_glGenVertexArrays(1, &sVao);
    p4gl_glGenBuffers(1, &sVbo);
    p4gl_glGenBuffers(1, &sEbo);
    p4gl_glBindVertexArray(sVao);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    p4gl_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sEbo);
    p4gl_glEnableVertexAttribArray(0);
    p4gl_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(P4Sky2Vertex),
                               (const void *)offsetof(P4Sky2Vertex, x));
    p4gl_glEnableVertexAttribArray(1);
    p4gl_glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(P4Sky2Vertex),
                               (const void *)offsetof(P4Sky2Vertex, r));
    p4gl_glEnableVertexAttribArray(2);
    p4gl_glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(P4Sky2Vertex),
                               (const void *)offsetof(P4Sky2Vertex, u));
    p4gl_glEnableVertexAttribArray(3);
    p4gl_glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(P4Sky2Vertex),
                               (const void *)offsetof(P4Sky2Vertex, recipZ));

    /* Divisors belong to this separate VAO; ordinary triangle draws retain divisor zero. */
    p4gl_glGenVertexArrays(1, &sLineVao);
    p4gl_glBindVertexArray(sLineVao);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    for (GLuint attribute = 0; attribute < 8; attribute++) {
        p4gl_glEnableVertexAttribArray(attribute);
        p4gl_glVertexAttribDivisor(attribute, 1);
    }

    sReady = 1;
    return 1;
}

/* Maps an RwPrimitiveType to a GL mode. Returns 0 for types Im2D does not draw here
 * (POINTLIST and NAPRIMTYPE). */
static GLenum im2d_mode(int32_t primType)
{
    switch (primType) {
    case P4GL_PRIM_LINELIST:
        return GL_LINES;
    case P4GL_PRIM_POLYLINE:
        return GL_LINE_STRIP;
    case P4GL_PRIM_TRILIST:
        return GL_TRIANGLES;
    case P4GL_PRIM_TRISTRIP:
        return GL_TRIANGLE_STRIP;
    case P4GL_PRIM_TRIFAN:
        return GL_TRIANGLE_FAN;
    default:
        fprintf(stderr, "p4gl: Im2D primitive type %d not supported\n", (int)primType);
        return 0;
    }
}

/* Vertex count valid for a u16 index buffer: 1..65536. */
static int vertex_count_ok(int32_t numVertices)
{
    return numVertices > 0 && numVertices <= 0x10000;
}

static int index_ok(int32_t index, int32_t numVertices)
{
    return index >= 0 && index < numVertices;
}

/* RwTextureAddressMode (decomp/include/rw/sky2/rwplcore.h:5385-5393) to a GL wrap mode.
 * Unset (native, 0) is wrap, the RenderWare default. The border colour is not set, so border
 * takes GL's default transparent black. A context with no GL_CLAMP_TO_BORDER (OpenGL ES 3.0 without
 * GL_EXT_texture_border_clamp, p4gl_gl_border_native) samples such a texture clamped to the edge and the
 * fragment shader does the border itself: p4gl_texture_border_mode gives it the mode (p4SampleBorder in p4gl_gl.c). */
static GLint address_mode(uint32_t mode)
{
    switch (mode) {
    case 2:
        return GL_MIRRORED_REPEAT;
    case 3:
        return GL_CLAMP_TO_EDGE;
    case 4:
        return GL_CLAMP_TO_BORDER;
    default:
        return GL_REPEAT;
    }
}

/* RwTextureFilterMode (rwplcore.h:5306-5316) to a GL filter. The device keeps one mip level,
 * so the mip modes fall back to linear. Unset is linear, the RenderWare default. */
static GLint filter_mode(uint32_t mode)
{
    return mode == 1 ? GL_NEAREST : GL_LINEAR;
}

static int sBorderMode;

/* Wrap code of a RwTextureAddressMode for p4SampleBorder: 0 repeat, 1 mirror, 2 clamp to edge, 3 border. */
static int border_code(uint32_t mode)
{
    return mode == 2u ? 1 : mode == 3u ? 2 : mode == 4u ? 3 : 0;
}

/* The p4SampleBorder mode (the shader's uBorder) of the texture the last p4gl_texture_bind bound, 0 when the hardware samples it. */
int p4gl_texture_border_mode(void)
{
    return sBorderMode;
}

/* Atomics take the sampler from RwTexture.filterAddressing, not the immediate-mode
 * render states left by a previous sprite. Both paths share the GL/border setup. */
int p4gl_texture_bind_sampler(uint32_t rasterValue, uint32_t sampler)
{
    uint32_t addressU = (sampler >> 8) & 15u;
    uint32_t addressV = (sampler >> 12) & 15u;
    const uint32_t filter = sampler & 255u;
    GLuint texture;

    texture = p4gl_raster_texture((const P4RwRaster *)P4_G2H(rasterValue));
    if (texture == 0) {
        return 0;
    }


    /* A camera-texture raster (the shadow map the floor quad samples) is clamped: its surroundings are not the picture. */
    if ((((const P4RwRaster *)P4_G2H(rasterValue))->cType & 0x0Fu) == 5u) {
        addressU = 3u;
        addressV = 3u;
    }

    /* A GS CLAMP_1 the game wrote itself (mdlEffect) replaces the render-state addressing. GS modes
     * 1 (clamp) and 2 (region clamp) are RenderWare clamp (3); 0 and 3 repeat (1). */
    {
        uint32_t wms;
        uint32_t wmt;

        if (p4gl_gs_clamp_get(&wms, &wmt)) {
            addressU = (wms == 1u || wms == 2u) ? 3u : 1u;
            addressV = (wmt == 1u || wmt == 2u) ? 3u : 1u;
        }
    }

    sBorderMode = 0;
    p4gl_glBindTexture(GL_TEXTURE_2D, texture);
    if ((addressU == 4u || addressV == 4u) && !p4gl_gl_border_native()) {
        static int told;

        if (!told) {
            fprintf(stderr, "p4gl: texture border addressing, emulated in the shader\n");
            told = 1;
        }
        sBorderMode = border_code(addressU) | (border_code(addressV) << 2) |
                      (filter_mode(filter) == GL_LINEAR ? 16 : 0);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, address_mode(addressU));
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, address_mode(addressV));
    }
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_mode(filter));
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mode(filter));
    return 1;
}

/* Immediate-mode draws retain the game's address and filter render states. */
int p4gl_texture_bind(uint32_t rasterValue)
{
    uint32_t u = p4gl_render_state(P4GL_RS_TEXTUREADDRESSU);
    uint32_t v = p4gl_render_state(P4GL_RS_TEXTUREADDRESSV);

    if (u == 0) {
        u = p4gl_render_state(P4GL_RS_TEXTUREADDRESS);
    }
    if (v == 0) {
        v = p4gl_render_state(P4GL_RS_TEXTUREADDRESS);
    }
    return p4gl_texture_bind_sampler(rasterValue,
                                    p4gl_render_state(P4GL_RS_TEXTUREFILTER) | (u << 8) | (v << 12));
}

typedef struct ImTextureState {
    int textured, border, continuous;
    float virtualWidth, virtualHeight;
} ImTextureState;

/* Binds the raster that TEXTURERASTER names, with its addressing and filter. A zero raster
 * draws untextured. Returns 0 when the named raster has no GL texture yet. */
static int im2d_prepare_texture(ImTextureState *out)
{
    uint32_t rasterValue = p4gl_render_state(P4GL_RS_TEXTURERASTER);
    *out = (ImTextureState){0};
    if (rasterValue == 0) return 1;
    if (!p4gl_texture_bind(rasterValue)) {
        fprintf(stderr, "p4gl: Im2D textured draw of a raster with no GL texture\n");
        return 0;
    }
    {
        const P4RwRaster *raster = (const P4RwRaster *)P4_G2H(rasterValue);
        const int frame = p4gl_raster_is_frame(raster);
        const int camera = (raster->cType & 15u) == 5u;
        out->virtualWidth = frame || camera ? (GLfloat)raster->width : 0;
        out->virtualHeight = frame || camera ? (GLfloat)raster->height : 0;
        out->continuous = frame || camera;
        out->textured = frame && p4gl_sky_alpha_uses_source() ? 2 : 1;
        out->border = p4gl_texture_border_mode();
    }
    return 1;
}

/* Virtual coordinates never follow the window or physical render resolution. */
static int sTargetOverrideW;
static int sTargetOverrideH;
static int sTargetPixelW;
static int sTargetPixelH;

void p4gl_target_override(int width, int height, int pixelWidth, int pixelHeight)
{
    sTargetOverrideW = width;
    sTargetOverrideH = height;
    sTargetPixelW = pixelWidth;
    sTargetPixelH = pixelHeight;
}

int p4gl_target_pixel_size(int *width, int *height)
{
    uint32_t pw = 0, ph = 0;

    if (sTargetOverrideW > 0 && sTargetOverrideH > 0) {
        *width = sTargetPixelW;
        *height = sTargetPixelH;
        return *width > 0 && *height > 0;
    }
    p4gl_readback_pixel_size(&pw, &ph);
    if (pw > 0 && ph > 0) {
        *width = (int)pw;
        *height = (int)ph;
        return 1;
    }
    SDL_Window *window = p4pc_pad_sdl_window();
    if (window == NULL && p4_battle_sim_renderless) {
        /* The sink's coordinate extent is virtual; pixel readback is forbidden. */
        *width = 640;
        *height = 448;
        return 1;
    }
    return window != NULL && SDL_GetWindowSizeInPixels(window, width, height) && *width > 0 && *height > 0;
}

int p4gl_target_size(int *width, int *height)
{
    if (sTargetOverrideW > 0 && sTargetOverrideH > 0) {
        *width = sTargetOverrideW;
        *height = sTargetOverrideH;
        return 1;
    }
    uint32_t w = 0, h = 0;
    p4gl_readback_virtual_size(&w, &h);
    if (w == 0 && p4pc_pad_sdl_window() == NULL && !p4_battle_sim_renderless) return 0;
    *width = w ? (int)w : 640;
    *height = h ? (int)h : 448;
    return 1;
}

/* Draw scopes are attached to ordering-table producers, not guessed from a
 * texture's contents or the current game sequence. In particular world-space
 * billboards and the field's multi-strip filters already use the camera's
 * widened view matrix and must not receive the centred UI transform again. */
static int sLayout = P4GL_LAYOUT_CENTER;
enum { MODEL_FIT_GENERIC = 1, MODEL_FIT_BATTLE = 2 };
static int sModelFit;
static int sMovieFit = -1;
typedef struct P4LayoutScope {
    int layout, modelFit;
} P4LayoutScope;
static P4LayoutScope sLayouts[64];
static unsigned sLayoutDepth;

float p4gl_layout_model_scale(void)
{
    static float previousRatio = 1.0f, fit = 1.0f;
    if (!sModelFit) return 1.0f;
    const float ratio = p4gl_aspect_width();
    if (ratio != previousRatio) {
        fit = ratio > 1.0f ? 1.0f / ratio : 1.0f;
        previousRatio = ratio;
    }
    return fit;
}

/* Panels/dividers cover uniformly, but their portrait masks and image sampling
 * are separate: widening a mask must not enlarge the face painted inside it. */
int p4gl_layout_model_material(const void *material)
{
    extern uint32_t mdlGetFrameNameUserData(void *);
    if (!sModelFit || material == NULL || p4gl_aspect_width() <= 1.0f)
        return P4GL_MODEL_FIT;
    const uint32_t nameAddress = mdlGetFrameNameUserData((void *)material);
    if (nameAddress == 0) return P4GL_MODEL_FIT;
    const char *name = (const char *)P4_G2H(nameAddress);
    /* These common node names belong to battle cut-ins only. Generic circular
     * communication panels must not inherit a full-width background policy. */
    if (sModelFit == MODEL_FIT_BATTLE &&
        (func_004426e8(name, "bord") == 0 || func_004426e8(name, "back") == 0 ||
         func_004426e8(name, "flash") == 0 || func_004426e8(name, "kasan01") == 0 ||
         func_004426e8(name, "kasan02") == 0))
        return P4GL_MODEL_FILL_X;
    if (func_00442c30(name, "haikei_", 7) == 0 ||
        func_004426e8(name, "koma_moyou") == 0 || func_004426e8(name, "siro_line") == 0 ||
        (func_00442c30(name, "koma_flash_", 11) == 0 &&
         name[11] >= 'a' && name[11] <= 'd' && name[12] == '\0'))
        return P4GL_MODEL_COVER;
    const char *part = NULL;
    int first = P4GL_MODEL_PORTRAIT_A;
    if (func_00442c30(name, "koma_", 5) == 0) part = name + 5;
    else if (func_00442c30(name, "megane_kiran_", 13) == 0) {
        part = name + 13;
        first = P4GL_MODEL_GLINT_A;
    }
    if (part != NULL && part[0] >= 'a' && part[0] <= 'd' && part[1] == '\0')
        return first + part[0] - 'a';
    return P4GL_MODEL_FIT;
}

/* i386 callback words can contain a compiled host address; x64 keeps retail
 * code addresses in guest memory. Resolve each i386 producer once, then keep
 * the hot ordering-table path allocation-free. Unknown port callbacks inherit
 * their enclosing layout. This is a cache, so saturation can safely replace
 * an entry rather than imposing a limit on the game's callback count. */
static uint32_t layout_callback_id(uint32_t callback)
{
    if (callback == 0 || (callback >= 0x00100000u && callback < 0x02000000u)) return callback;
#ifndef P4_GUEST64
    static struct { uint32_t host, retail; } cache[1024];
    const unsigned first = ((callback >> 4) * 2654435761u) >> 22;
    unsigned slot = first;
    uint32_t retail = 0;

    for (unsigned n = 0; n < 1024; n++, slot = (slot + 1u) & 1023u) {
        if (cache[slot].host == callback) return cache[slot].retail;
        if (cache[slot].host == 0) break;
    }
    for (size_t i = 0; i < p4pc_native_count; i++) {
        if ((uintptr_t)p4pc_native_table[i].fn == (uintptr_t)callback) {
            retail = p4pc_native_table[i].addr;
            break;
        }
    }
    if (cache[slot].host != 0) slot = first;
    cache[slot].host = callback;
    cache[slot].retail = retail;
    return retail;
#else
    return P4_FN_H2G(P4_FN_G2H(callback));
#endif
}

/* Render-only hidden HUD scopes (mod HUD groups, src/platform/mods/
 * game_services.c): every draw inside a hidden ordering-table bucket or node
 * is dropped by p4gl_gl_suppress_draw while the node itself still runs, so
 * the game's glyph/sprite lists are consumed and released as usual. Every
 * layout begin opens one scope and p4gl_layout_end closes it, with or
 * without widescreen; queue and node begins say whether theirs is hidden. */
static const uint32_t *sHudQueues;
static const uint32_t (*sHudCallbacks)[2];
static unsigned sHudQueueCount, sHudCallbackCount, sHudDepth, sHudOverflow;
static uint8_t sHudStack[64];
static int sHudPending;

/* The caller keeps both arrays alive until the next call. */
void p4gl_hud_hidden_set(const uint32_t *queues, unsigned queueCount, const uint32_t (*callbacks)[2],
                         unsigned callbackCount)
{
    sHudQueues = queues;
    sHudQueueCount = queueCount;
    sHudCallbacks = callbacks;
    sHudCallbackCount = callbackCount;
}

static int hud_active(void) { return sHudQueueCount || sHudCallbackCount || sHudDepth || sHudOverflow; }

static int hud_queue_hidden(uint32_t queue)
{
    for (unsigned i = 0; i < sHudQueueCount; i++)
        if (sHudQueues[i] == queue) return 1;
    return 0;
}

static int hud_callback_hidden(uint32_t callback)
{
    for (unsigned i = 0; i < sHudCallbackCount; i++)
        if (callback >= sHudCallbacks[i][0] && callback <= sHudCallbacks[i][1]) return 1;
    return 0;
}

static void hud_push(int hidden)
{
    /* Scopes nested deeper than the stack keep their parent's state. */
    if (sHudDepth == sizeof sHudStack) {
        ++sHudOverflow;
        return;
    }
    const int state = (sHudDepth && sHudStack[sHudDepth - 1]) || hidden;
    sHudStack[sHudDepth++] = (uint8_t)state;
    p4gl_gl_suppress_draw(state);
}

static void hud_pop(void)
{
    if (sHudOverflow) {
        --sHudOverflow;
        return;
    }
    if (sHudDepth == 0) return;
    --sHudDepth;
    p4gl_gl_suppress_draw(sHudDepth ? sHudStack[sHudDepth - 1] : 0);
}

void p4gl_layout_begin(uint32_t callback, unsigned command)
{
    if (hud_active()) hud_push(sHudPending);
    sHudPending = 0;
    if (p4gl_aspect_width() == 1.0f) return;
    callback = layout_callback_id(callback);
    if (sLayoutDepth == sizeof sLayouts / sizeof sLayouts[0]) {
        fprintf(stderr, "p4gl: widescreen draw nesting exceeds ordering-table stack\n");
        abort();
    }
    sLayouts[sLayoutDepth++] = (P4LayoutScope){sLayout, sModelFit};
    if ((command >= 21 && command <= 27) ||
        (callback >= 0x00480000u && callback < 0x00500000u) ||
        callback == 0x001221a0u || /* shared flat/wipe/radial full-screen fade */
        callback == 0x00160680u || callback == 0x001607e0u) {
        sLayout = P4GL_LAYOUT_FILL;
    } else if (callback == 0x00122a40u || callback == 0x002167f0u ||
               callback == 0x00363540u || callback == 0x002aa450u) {
        sLayout = P4GL_LAYOUT_RIGHT; /* date/weather, party portraits and loading corner */
    } else if (callback >= 0x0020b1a0u && callback <= 0x0020b5b0u) {
        sLayout = P4GL_LAYOUT_LEFT; /* command wheel and its list/grid */
    }
}

void p4gl_layout_push(int layout)
{
    p4gl_layout_begin(0, 0);
    if (p4gl_aspect_width() != 1.0f) {
        sLayout = layout;
        sModelFit = 0;
    }
}

/* Retail layers 0x4a..0x53 are the field banner and minimap. The map's
 * opening state callback supplies its mode before the tile/symbol layers:
 * compact map is edge-anchored; expanded map remains a centred layout. */
static int sCompactMap;

void p4gl_layout_queue_begin(uint32_t queue)
{
    if (p4gl_aspect_width() == 1.0f && !hud_active()) return;
    sHudPending = hud_active() && hud_queue_hidden(queue);
    p4gl_layout_begin(0, 0);
    if (p4gl_aspect_width() == 1.0f) return;
    if (queue == 0x00794c30u) sCompactMap = 0;
    if (queue == 0x00794930u || queue == 0x007943c0u || queue == 0x00794960u)
        sLayout = P4GL_LAYOUT_FILL; /* all fog/noise strips and colour-filter grid cells */
    else if (queue == 0x00794c60u ||
             (sCompactMap && queue >= 0x00794c90u && queue <= 0x00794e10u))
        sLayout = P4GL_LAYOUT_LEFT;
    if (queue == 0x00795050u ||
        (queue >= 0x00796520u && queue <= 0x00796640u && (queue - 0x00796520u) % 0x30u == 0)) {
        /* All-out/skill and generic cut-in tables are authored screen models,
         * not world geometry. Foreground art keeps its original proportions;
         * material-level background geometry fills the wider canvas. */
        sModelFit = queue == 0x00795050u ? MODEL_FIT_BATTLE : MODEL_FIT_GENERIC;
        p4gl_atomic_model_scope_begin();
    }
}

void p4gl_layout_node_begin(const void *node)
{
    const uint8_t *packet = (const uint8_t *)node;
    uint32_t callback, argument;
    uint16_t command;
    if (p4gl_aspect_width() == 1.0f && !hud_active()) return;
    memcpy(&callback, packet + 8, sizeof callback);
    memcpy(&argument, packet + 16, sizeof argument);
    memcpy(&command, packet + 24, sizeof command);
    callback = layout_callback_id(callback);
    sHudPending = hud_active() && hud_callback_hidden(callback);
    p4gl_layout_begin(callback, command);
    if (p4gl_aspect_width() == 1.0f) return;
    if (callback == 0x002add10u && argument != 0)
        sCompactMap = ((const uint8_t *)P4_G2H(argument))[0xb8] == 0;
}

void p4gl_layout_end(void)
{
    hud_pop();
    if (p4gl_aspect_width() == 1.0f) return;
    if (sLayoutDepth == 0) {
        fprintf(stderr, "p4gl: unbalanced widescreen ordering-table draw\n");
        abort();
    }
    const P4LayoutScope *scope = &sLayouts[--sLayoutDepth];
    sLayout = scope->layout;
    sModelFit = scope->modelFit;
}

/* Every photograph/brightness pass, including the depth-10 transition copy,
 * uses the same uniform cover; title logo, silhouettes and text are untouched. */
void p4gl_title_photo_draw(float depth, uint32_t color, uint8_t alpha, int32_t entry,
                          int32_t offset, void *pack, int32_t states)
{
    extern int32_t shdSpriteDrawNow(float, float, float, int32_t, uint8_t,
                                  int32_t, int32_t, void *, int32_t);
    p4gl_layout_push(P4GL_LAYOUT_ZOOM);
    shdSpriteDrawNow(-1.0f, -1.0f, depth, (int32_t)color, alpha, entry, offset, pack, states);
    p4gl_layout_end();
}

static int im2d_canvas(const P4Sky2Vertex *vertices, int32_t count, int width, int height,
                       float canvas[4], float bounds[4])
{
    const float ratio = p4gl_aspect_width();
    float sx = 1.0f, sy = 1.0f, dx = 0.0f;
    int layout = sLayout;
    const uint32_t raster = p4gl_render_state(P4GL_RS_TEXTURERASTER);
    int clip = 0;

    if (width == 640 && height == 448 && sMovieFit >= P4CFG_MOVIE_ZOOM) {
        float left = vertices[0].x, right = left;
        float top = vertices[0].y, bottom = top;
        for (int32_t i = 1; i < count; i++) {
            if (vertices[i].x < left) left = vertices[i].x;
            if (vertices[i].x > right) right = vertices[i].x;
            if (vertices[i].y < top) top = vertices[i].y;
            if (vertices[i].y > bottom) bottom = vertices[i].y;
        }
        const float halfWidth = (right - left) / 640.0f;
        const float halfHeight = (bottom - top) / 448.0f;
        if (halfWidth > 0.0f && halfHeight > 0.0f) {
            if (sMovieFit == P4CFG_MOVIE_STRETCH) {
                sx = 1.0f / halfWidth;
                sy = 1.0f / halfHeight;
            } else {
                sx = ratio > 1.0f ? 1.0f / ratio : 1.0f;
                sy = ratio < 1.0f ? ratio : 1.0f;
                const float scaledWidth = halfWidth * sx, scaledHeight = halfHeight * sy;
                const float zoom = 1.0f / (scaledWidth < scaledHeight ? scaledWidth : scaledHeight);
                sx *= zoom;
                sy *= zoom;
            }
            canvas[0] = sx;
            canvas[1] = sy;
            canvas[2] = -(left + right - 640.0f) / 640.0f * sx;
            canvas[3] = (top + bottom - 448.0f) / 448.0f * sy;
            const uint32_t nativeWidth = p4gl_aspect_native_width();
            canvas[2] += 1.0f / (float)nativeWidth - sx / 640.0f;
            canvas[3] += (sy - 1.0f) / 448.0f;
            return 0; /* Crop at the full target, not at the centred UI canvas. */
        }
    }
    if (ratio != 1.0f && width == 640 && height == 448) {
        /* Frame reads are a complete target, even when split into strips or
         * displaced for feedback/bloom. An untextured 640-wide primitive is
         * a screen clear, fade or letterbox bar, not a stretched UI sprite. */
        if (layout == P4GL_LAYOUT_ZOOM) {
            sx = ratio < 1.0f ? 1.0f / ratio : 1.0f;
            sy = ratio > 1.0f ? ratio : 1.0f;
            layout = P4GL_LAYOUT_FILL;
        } else if (raster != 0 && p4gl_raster_is_frame(P4_G2H(raster))) {
            layout = P4GL_LAYOUT_FILL;
        } else {
            int fullWidthSolid = 0;
            if (raster == 0 && sMovieFit < 0) {
                float left = vertices[0].x, right = left;
                for (int32_t i = 1; i < count; i++) {
                    if (vertices[i].x < left) left = vertices[i].x;
                    if (vertices[i].x > right) right = vertices[i].x;
                }
                fullWidthSolid = left <= 0.0f && right >= 640.0f;
            }
            if (fullWidthSolid) {
                layout = P4GL_LAYOUT_FILL;
            } else if (sModelFit) {
                sx = sy = p4gl_layout_model_scale();
                clip = 1;
                layout = P4GL_LAYOUT_FILL; /* screen-model projection is already applied */
            }
        }
        if (layout != P4GL_LAYOUT_FILL) {
            clip = 1;
            sx = ratio > 1.0f ? 1.0f / ratio : 1.0f;
            sy = ratio < 1.0f ? ratio : 1.0f;
            if (layout == P4GL_LAYOUT_LEFT) dx = sx - 1.0f;
            if (layout == P4GL_LAYOUT_RIGHT) dx = 1.0f - sx;
        }
        if (raster == 0 && layout == P4GL_LAYOUT_FILL && !clip && sTargetOverrideW == 0) {
            const uint32_t nativeWidth = p4gl_aspect_native_width();
            static uint32_t previousWidth;
            static float solidOffset;
            if (nativeWidth != previousWidth) {
                previousWidth = nativeWidth;
                solidOffset = 1.0f / (float)nativeWidth - 1.0f / 640.0f;
            }
            /* Expand geometry, not the GS half-cell bias. Otherwise x=0
             * moves past native cell zero at wider aspects. No UV changes. */
            dx = solidOffset;
        }
    }
    canvas[0] = sx; canvas[1] = sy; canvas[2] = dx; canvas[3] = 0;
    if (clip) {
        const float halfX = sx;
        const float halfY = ratio < 1.0f ? ratio : 1.0f;
        bounds[0] = dx - halfX; bounds[1] = -halfY;
        bounds[2] = dx + halfX; bounds[3] = halfY;
    }
    return clip;
}

/* Prepares a draw with the destination alpha test: the frame buffer as it is, a copy of the current target on texture unit 1. */
static void im2d_dest_alpha(int width, int height)
{
    p4gl_glActiveTexture(GL_TEXTURE1);
    if (sDestTexture == 0) {
        p4gl_glGenTextures(1, &sDestTexture);
    }
    p4gl_glBindTexture(GL_TEXTURE_2D, sDestTexture);
    if (sDestW != width || sDestH != height) {
        p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        sDestW = width;
        sDestH = height;
    }
    p4gl_glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
    p4gl_glActiveTexture(GL_TEXTURE0);
}

/* P4PC_GS_LOG=<first flip>,<last flip>: one line per Im2D draw of those presented frames, in the terms of the PCSX2 GS-dump draw list (the retail
 * draw list): the primitive, the screen box of its vertices, z, the texture (a frame raster, or the raster's address), the raw ALPHA_1 and TEST_1 values the game
 * set, the Z test / write state and the frame mask, so that a scene's post-processing passes can be matched against the PCSX2 GS dump of the same scene. */
static void im2d_gs_log(GLenum mode, const P4Sky2Vertex *v, int32_t n)
{
    float box[4] = { 1e9f, 1e9f, -1e9f, -1e9f };
    float rgba[4];
    uint32_t tex;
    int32_t i;

    if (!p4gl_gs_log_on() || v == NULL) {
        return;
    }
    for (i = 0; i < n && i < 64; i++) {
        if (v[i].x < box[0]) box[0] = v[i].x;
        if (v[i].x > box[2]) box[2] = v[i].x;
        if (v[i].y < box[1]) box[1] = v[i].y;
        if (v[i].y > box[3]) box[3] = v[i].y;
    }
    rgba[0] = v[0].r; rgba[1] = v[0].g; rgba[2] = v[0].b; rgba[3] = v[0].a;
    tex = p4gl_render_state(P4GL_RS_TEXTURERASTER);
    p4gl_gs_log_draw("im2d", (unsigned)mode, (int)n, box, (uint32_t)(int32_t)v[0].z, rgba, tex, p4gl_sky_alpha_raw(), p4gl_sky_test_raw(),
                     (unsigned)p4gl_render_state_or(P4GL_RS_ZTESTENABLE, 1u), (unsigned)p4gl_render_state_or(P4GL_RS_ZWRITEENABLE, 1u));
}

/* Each line instance reads both endpoints from the original VBO. Only a genuinely indexed
 * multi-line draw needs a gather; a single indexed line uses two offsets without copying. */
static int im2d_line_layout(GLenum mode, const P4Sky2Vertex **vertices, int32_t *count,
                           const uint16_t *indices, int32_t numIndices)
{
    static const GLint sizes[4] = {3, 4, 2, 1};
    static const size_t offsets[4] = {
        offsetof(P4Sky2Vertex, x), offsetof(P4Sky2Vertex, r),
        offsetof(P4Sky2Vertex, u), offsetof(P4Sky2Vertex, recipZ)
    };
    size_t first = 0, next = sizeof(P4Sky2Vertex);
    GLsizei stride = (GLsizei)sizeof(P4Sky2Vertex) * (mode == GL_LINES ? 2 : 1);

    if (indices != NULL && numIndices == 2) {
        first = (size_t)indices[0] * sizeof(P4Sky2Vertex);
        next = (size_t)indices[1] * sizeof(P4Sky2Vertex);
    } else if (indices != NULL) {
        if ((size_t)numIndices > sLineCapacity) {
            size_t capacity = sLineCapacity != 0 ? sLineCapacity : 64;
            P4Sky2Vertex *grown;

            while (capacity < (size_t)numIndices) capacity *= 2;
            grown = realloc(sLineVertices, capacity * sizeof *grown);
            if (grown == NULL) return 0;
            sLineVertices = grown;
            sLineCapacity = capacity;
        }
        for (int32_t i = 0; i < numIndices; i++) sLineVertices[i] = (*vertices)[indices[i]];
        *vertices = sLineVertices;
        *count = numIndices;
    }
    p4gl_glBindVertexArray(sLineVao);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    for (GLuint attribute = 0; attribute < 4; attribute++) {
        p4gl_glVertexAttribPointer(attribute, sizes[attribute], GL_FLOAT, GL_FALSE, stride,
                                  (const void *)(first + offsets[attribute]));
        p4gl_glVertexAttribPointer(attribute + 4, sizes[attribute], GL_FLOAT, GL_FALSE, stride,
                                  (const void *)(next + offsets[attribute]));
    }
    return 1;
}

static int im2d_material(int width, int height, int pw, int ph, int nw, int nh, const float canvas[4],
                        const float bounds[4], int clip, int line, int cells,
                        int alphaFunc, float alphaRef, const ImTextureState *texture)
{
    if (!select_program(p4gl_sky_date() != 0, line, clip, cells)) return 0;
    p4gl_glUseProgram(sProgram);
    p4gl_glUniform2f(sViewportLocation, (GLfloat)width, (GLfloat)height);
    p4gl_glUniform4f(sCanvasLocation, canvas[0], canvas[1], canvas[2], canvas[3]);
    if (cells) {
        p4gl_glUniform2f(sCellPhysicalLocation, (GLfloat)pw, (GLfloat)ph);
        p4gl_glUniform2f(sCellNativeLocation, (GLfloat)nw, (GLfloat)nh);
    } else {
        p4gl_glUniform2f(sPixelDensityLocation, (float)pw / width * canvas[0],
                        (float)ph / height * canvas[1]);
    }
    if (clip) p4gl_glUniform4f(sCanvasClipLocation, bounds[0], bounds[1], bounds[2], bounds[3]);
    if (line) {
        p4gl_glUniform2f(sLinePixelScaleLocation, (GLfloat)width / pw, (GLfloat)height / ph);
        p4gl_glUniform1i(sLineABELocation, p4gl_render_state_or(P4GL_RS_VERTEXALPHAENABLE, 0u) != 0u);
    }
    p4gl_glUniform1i(sTexturedLocation, texture->textured);
    p4gl_glUniform1i(sBorderLocation, texture->border);
    if (texture->textured)
        p4gl_glUniform2f(sVirtualTextureSizeLocation, texture->virtualWidth, texture->virtualHeight);
    if (cells) p4gl_glUniform1i(sCellContinuousUVLocation, texture->continuous);
    if (p4gl_sky_date()) p4gl_glUniform1i(sDateLocation, p4gl_sky_date());
    p4gl_glUniform1i(sAlphaFuncLocation, alphaFunc);
    p4gl_glUniform1f(sAlphaRefLocation, alphaRef);
    p4gl_glUniform1i(sAlphaInvertLocation, 0);
    return 1;
}

/* Upload once, then replay the same source-ordered runs for AFAIL. No GS log,
 * skip identity, DATE snapshot, or alpha-failure ordering is split by transport. */
static int32_t im2d_draw(GLenum mode, const P4Sky2Vertex *vertices, int32_t numVertices,
                         const uint16_t *indices, int32_t numIndices)
{
    int width = 0, height = 0, pw, ph, nw, nh;
    int fbOnly, cells = 0, ok = 1;
    int32_t fbFunc = 0;
    float fbRef = 0.0f;
    const int line = mode == GL_LINES || mode == GL_LINE_STRIP;
    const int32_t count = indices != NULL ? numIndices : numVertices;
    const int32_t lineCount = line ? (mode == GL_LINES ? count / 2 : count - 1) : 0;
    const P4Sky2Vertex *uploadVertices = vertices;
    int32_t uploadCount = numVertices;
    float canvas[4], bounds[4];
    int clip;
    ImTextureState texture;
    unsigned configured = 0, inverted = 0;
    int current = -1;

    /* The VBO and destination-alpha copy are refreshed by each actual draw. Nothing here
     * advances guest state; texture uploads and camera/target lifetimes are handled outside. */
    if (!p4gl_gl_prepare_draw()) {
        return 1;
    }
    if (p4gl_profile_pass != NULL) {
        const uint32_t raster = p4gl_render_state(P4GL_RS_TEXTURERASTER);
        p4gl_profile_pass(sMovieFit >= 0 ? P4GL_PROFILE_MOVIE :
            raster != 0 && p4gl_raster_is_frame(P4_G2H(raster)) ? P4GL_PROFILE_FEEDBACK : P4GL_PROFILE_UI);
    }
    if (!im2d_init() || !p4gl_target_size(&width, &height)) return 0;
    im2d_gs_log(mode, vertices, numVertices);
    if (p4gl_gs_skip()) return 1;
    if (line && lineCount <= 0) return 1;
    {
        float bb[4] = {1e9f, 1e9f, -1e9f, -1e9f}, color[4];
        for (int32_t i = 0; i < numVertices; i++) {
            if (vertices[i].x < bb[0]) bb[0] = vertices[i].x;
            if (vertices[i].y < bb[1]) bb[1] = vertices[i].y;
            if (vertices[i].x > bb[2]) bb[2] = vertices[i].x;
            if (vertices[i].y > bb[3]) bb[3] = vertices[i].y;
        }
        color[0] = vertices[numVertices - 1].r; color[1] = vertices[numVertices - 1].g;
        color[2] = vertices[numVertices - 1].b; color[3] = vertices[numVertices - 1].a;
        if (!p4gl_drawlog("im2d", (int)mode, count, bb, color, NULL)) return 1;
    }
    clip = im2d_canvas(vertices, numVertices, width, height, canvas, bounds);
    pw = width; ph = height;
    p4gl_target_pixel_size(&pw, &ph);
    nw = width; nh = height;
    if (!(sTargetOverrideW > 0 && sTargetOverrideH > 0) && width == 640 && height == 448)
        nw = (int)p4gl_aspect_native_width();
    if (!line && (pw != nw || ph != nh)) {
        cells = cell_build(mode, vertices, indices, (uint32_t)count);
        if (cells < 0) return 0;
    }
    if (line) {
        if (!im2d_line_layout(mode, &uploadVertices, &uploadCount, indices, numIndices)) return 0;
    } else p4gl_glBindVertexArray(sVao);
    if (!cells || sCellIndexCount) {
        p4gl_glBindBuffer(GL_ARRAY_BUFFER, sVbo);
        p4gl_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)uploadCount * sizeof(P4Sky2Vertex),
                          uploadVertices, GL_STREAM_DRAW);
    }
    if (cells) {
        cell_bind(0);
        p4gl_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sCellCount * sizeof(CellRect), sCells, GL_STREAM_DRAW);
        if (sCellIndexCount) {
            p4gl_glBindVertexArray(sVao);
            p4gl_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sCellEbo);
            p4gl_glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)sCellIndexCount * sizeof(uint32_t),
                              sCellIndices, GL_STREAM_DRAW);
        }
    } else if (!line && indices) {
        p4gl_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sEbo);
        p4gl_glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)numIndices * sizeof(uint16_t), indices, GL_STREAM_DRAW);
    }
    if (!im2d_prepare_texture(&texture)) return 0;
    if (p4gl_sky_date()) im2d_dest_alpha(pw, ph);
    p4gl_sky_apply_blend();
    fbOnly = p4gl_gs_alpha_color_fail(p4gl_sky_test_raw(), &fbFunc, &fbRef);
    p4gl_depth_apply_values(p4gl_render_state_or(P4GL_RS_ZTESTENABLE, 1u) != 0u,
                            !line && p4gl_render_state_or(P4GL_RS_ZWRITEENABLE, 1u) != 0u);
    p4gl_glDisable(GL_CULL_FACE);
    p4gl_frame_mask_apply();
    for (int pass = 0; pass < (fbOnly ? 2 : 1) && ok; pass++) {
        const int alphaFunc = fbOnly ? fbFunc : p4gl_sky_alpha_func();
        const float alphaRef = fbOnly ? fbRef : p4gl_sky_alpha_ref();
        if (pass) {
            p4gl_glDepthMask(GL_FALSE);
            if (fbOnly == 3) p4gl_frame_mask_rgb_only();
        }
        if (cells) {
            for (uint32_t i = 0; i < sCellRunCount; i++) {
                const CellRun *run = &sCellRuns[i];
                const unsigned bit = 1u << run->cells;
                if (!(configured & bit)) {
                    if (!im2d_material(width, height, pw, ph, nw, nh, canvas, bounds, clip, 0, run->cells,
                                       alphaFunc, alphaRef, &texture)) { ok = 0; break; }
                    configured |= bit;
                    current = run->cells;
                } else if (current != run->cells) {
                    if (!select_program(p4gl_sky_date() != 0, 0, clip, run->cells)) { ok = 0; break; }
                    p4gl_glUseProgram(sProgram);
                    current = run->cells;
                }
                if (pass && !(inverted & bit)) {
                    p4gl_glUniform1i(sAlphaInvertLocation, 1);
                    inverted |= bit;
                }
                if (run->cells) {
                    cell_bind(run->first);
                    p4gl_glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)run->count);
                } else {
                    p4gl_glBindVertexArray(sVao);
                    p4gl_glDrawElements(GL_TRIANGLES, (GLsizei)run->count, GL_UNSIGNED_INT,
                                       (const void *)((size_t)run->first * sizeof(uint32_t)));
                }
            }
        } else {
            if (!pass) {
                if (!im2d_material(width, height, pw, ph, nw, nh, canvas, bounds, clip, line, 0,
                                   alphaFunc, alphaRef, &texture)) { ok = 0; break; }
            } else p4gl_glUniform1i(sAlphaInvertLocation, 1);
            if (line) p4gl_glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, lineCount);
            else if (!indices) p4gl_glDrawArrays(mode, 0, numVertices);
            else p4gl_glDrawElements(mode, numIndices, GL_UNSIGNED_SHORT, NULL);
        }
    }
    if (fbOnly && ok) p4gl_glUniform1i(sAlphaInvertLocation, 0);
    p4gl_frame_mask_restore();
    p4gl_depth_restore();
    return ok;
}


/* RwIm2DRenderLine (SkyIm2DRenderLine, FUN_0040AFB0): one line between two vertices. */
int32_t func_0040afb0(uint8_t *vertices, int32_t numVertices, int32_t vert1, int32_t vert2)
{
    uint16_t indices[2];

    if (vertices == NULL || !vertex_count_ok(numVertices)
        || !index_ok(vert1, numVertices) || !index_ok(vert2, numVertices)) {
        return 0;
    }
    indices[0] = (uint16_t)vert1;
    indices[1] = (uint16_t)vert2;
    return im2d_draw(GL_LINES, (const P4Sky2Vertex *)vertices, numVertices, indices, 2);
}

/* RwIm2DRenderTriangle (SkyIm2DRenderTriangle, FUN_0040B230): one triangle. */
int32_t func_0040b230(uint8_t *vertices, int32_t numVertices, int32_t vert1, int32_t vert2,
                      int32_t vert3)
{
    uint16_t indices[3];

    if (vertices == NULL || !vertex_count_ok(numVertices)
        || !index_ok(vert1, numVertices) || !index_ok(vert2, numVertices)
        || !index_ok(vert3, numVertices)) {
        return 0;
    }
    indices[0] = (uint16_t)vert1;
    indices[1] = (uint16_t)vert2;
    indices[2] = (uint16_t)vert3;
    return im2d_draw(GL_TRIANGLES, (const P4Sky2Vertex *)vertices, numVertices, indices, 3);
}

/* RwIm2DRenderPrimitive (SkyIm2DRenderPrimitive, FUN_0040C0F0): unindexed primitive list. */
int32_t func_0040c0f0(int32_t primType, uint8_t *vertices, int32_t numVertices)
{
    GLenum mode = im2d_mode(primType);

    if (mode == 0 || vertices == NULL || numVertices <= 0) {
        return 0;
    }
    return im2d_draw(mode, (const P4Sky2Vertex *)vertices, numVertices, NULL, 0);
}

/* The Sofdec task submits through this seam, so movie policy cannot affect
 * dialogue portraits, event letterboxes or an unrelated 640-wide texture. */
int32_t p4gl_movie_draw(int32_t primitive, uint8_t *vertices, int32_t count)
{
    const int oldMovieFit = sMovieFit, oldLayout = sLayout, oldModelFit = sModelFit;
    sMovieFit = p4cfg_video()->movieFit;
    sLayout = P4GL_LAYOUT_CENTER;
    sModelFit = 0;
    const int32_t result = func_0040c0f0(primitive, vertices, count);
    sMovieFit = oldMovieFit;
    sLayout = oldLayout;
    sModelFit = oldModelFit;
    return result;
}

/* RwIm2DRenderIndexedPrimitive (SkyIm2DRenderIndexedPrimitive, FUN_0040D0A0): indexed
 * primitive list. Indices are u16 (RwImVertexIndex); every index must be below numVertices. */
int32_t func_0040d0a0(int32_t primType, uint8_t *vertices, int32_t numVertices,
                      uint16_t *indices, int32_t numIndices)
{
    GLenum mode = im2d_mode(primType);
    int32_t i;

    if (mode == 0 || vertices == NULL || indices == NULL || numIndices <= 0
        || !vertex_count_ok(numVertices)) {
        return 0;
    }
    for (i = 0; i < numIndices; ++i) {
        if (!index_ok((int32_t)indices[i], numVertices)) {
            fprintf(stderr, "p4gl: Im2D index %u out of range (%d vertices)\n",
                    (unsigned)indices[i], (int)numVertices);
            return 0;
        }
    }
    return im2d_draw(mode, (const P4Sky2Vertex *)vertices, numVertices, indices, numIndices);
}

/* Writes the Im2D entry points into a device block. The core reads them from the block
 * REGISTER copies (see p4gl_device.c), so this runs before every copy. */
void p4gl_im2d_install(P4RwDevice *device)
{
    device->fpIm2DRenderLine = P4_FN_H2G(func_0040afb0);
    device->fpIm2DRenderTriangle = P4_FN_H2G(func_0040b230);
    device->fpIm2DRenderPrimitive = P4_FN_H2G(func_0040c0f0);
    device->fpIm2DRenderIndexedPrimitive = P4_FN_H2G(func_0040d0a0);
}

void p4gl_im2d_context_lost(void)
{
    memset(sProgs, 0, sizeof sProgs);
    sProgram = 0;
    sDestTexture = 0;
    sDestW = sDestH = 0;
    sVao = sLineVao = sVbo = sEbo = 0;
    free(sLineVertices);
    sLineVertices = NULL;
    sLineCapacity = 0;
    sCellVao = sCellVbo = sCellEbo = 0;
    free(sCells); free(sCellRuns); free(sCellIndices);
    sCells = NULL; sCellRuns = NULL; sCellIndices = NULL;
    sCellCapacity = sCellRunCapacity = sCellIndexCapacity = 0;
    sCellCount = sCellRunCount = sCellIndexCount = 0;
    sCellBase = SIZE_MAX;
    sReady = sFailed = 0;
}
