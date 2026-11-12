#ifndef P4GL_YUV_SHADER_H
#define P4GL_YUV_SHADER_H

/* Exact pinned FFmpeg7.1.1 converter contracts, exhaustively checked by
 * movie_colour_test. Android's dependency build disables assembler on every ABI;
 * its generic C table arithmetic differs from both native SIMD kernels. */
#if defined(__ANDROID__) || defined(P4GL_TEST_GENERIC_C)
#define P4GL_YUV_INTEGER_SUPPORTED 1
#define P4GL_YUV_MODEL 2
#elif defined(__aarch64__) || defined(_M_ARM64)
#define P4GL_YUV_INTEGER_SUPPORTED 1
#define P4GL_YUV_MODEL 1
#define P4GL_YUV_PRODUCT_SHIFT 15
#define P4GL_YUV_OUTPUT_SHIFT 1
#define P4GL_YUV_OUTPUT_ROUND 1
#elif defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define P4GL_YUV_INTEGER_SUPPORTED 1
#define P4GL_YUV_MODEL 0
#define P4GL_YUV_PRODUCT_SHIFT 16
#define P4GL_YUV_OUTPUT_SHIFT 0
#define P4GL_YUV_OUTPUT_ROUND 0
#else
/* Other CPU backends retain the existing exact CPU conversion, not an
 * unqualified approximation of their library's arithmetic. */
#define P4GL_YUV_INTEGER_SUPPORTED 0
#define P4GL_YUV_MODEL 0
#define P4GL_YUV_PRODUCT_SHIFT 16
#define P4GL_YUV_OUTPUT_SHIFT 0
#define P4GL_YUV_OUTPUT_ROUND 0
#endif

#define P4GL_YUV_STRING_VALUE_(x) #x
#define P4GL_YUV_STRING_VALUE(x) P4GL_YUV_STRING_VALUE_(x)
static const char kP4YuvIntegerFragment[] =
    "uniform sampler2D uY;\n"
    "uniform sampler2D uU;\n"
    "uniform sampler2D uV;\n"
    "out vec4 oColor;\n"
    "void main() {\n"
    "    ivec2 p = ivec2(gl_FragCoord.xy);\n"
    "    ivec2 q = p >> 1;\n"
    "    int y = int(texelFetch(uY, p, 0).r * 255.0 + 0.5);\n"
#if P4GL_YUV_MODEL == 2
    "    int u = int(texelFetch(uU, q, 0).r * 255.0 + 0.5);\n"
    "    int v = int(texelFetch(uV, q, 0).r * 255.0 + 0.5);\n"
    "    ivec3 offset = ivec3(((v * 89830) >> 16) - 175,\n"
    "                         ((u * -22049) >> 16) + ((v * -45756) >> 16) + 134,\n"
    "                         ((u * 113537) >> 16) - 221);\n"
    "    ivec3 rgb = clamp(((ivec3(y) + offset) * 76309 - ivec3(1304898)) >> 16, ivec3(0), ivec3(255));\n"
#else
    "    int u = (int(texelFetch(uU, q, 0).r * 255.0 + 0.5) - 128) * 8;\n"
    "    int v = (int(texelFetch(uV, q, 0).r * 255.0 + 0.5) - 128) * 8;\n"
    "    const int productShift = " P4GL_YUV_STRING_VALUE(P4GL_YUV_PRODUCT_SHIFT) ";\n"
    "    int yy = ((y * 8 - 128) * 9539) >> productShift;\n"
    "    ivec3 rgb = ivec3(yy + ((v * 13075) >> productShift),\n"
    "                      yy + ((u * -3209) >> productShift) + ((v * -6660) >> productShift),\n"
    "                      yy + ((u * 16525) >> productShift));\n"
    "    rgb = clamp((rgb + ivec3(" P4GL_YUV_STRING_VALUE(P4GL_YUV_OUTPUT_ROUND) ")) >> " P4GL_YUV_STRING_VALUE(P4GL_YUV_OUTPUT_SHIFT) ", ivec3(0), ivec3(255));\n"
#endif
    /* Stay strictly inside the integer's UNORM8 bin under both truncation
     * and round-to-nearest, including llvmpipe's inherited EE float mode. */
    "    oColor = vec4((vec3(rgb) + vec3(0.25)) * (1.0 / 255.0), 1.0);\n"
    "}\n";
#undef P4GL_YUV_STRING_VALUE
#undef P4GL_YUV_STRING_VALUE_
#endif
