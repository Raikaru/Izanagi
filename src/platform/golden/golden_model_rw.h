/* Existing PS2/RenderWare host seams, as used by p4model_build.c. */
#ifndef P4_GOLDEN_MODEL_RW_H
#define P4_GOLDEN_MODEL_RW_H
#include <stdint.h>
void *RpClumpCreate(void);
int32_t RpClumpDestroy(void *clump);
void *RpClumpAddAtomic(void *clump, void *atomic);
void *RwFrameCreate(void);
void *RwFrameAddChild(void *parent, void *child);
void *RwFrameRemoveChild(void *child);
int32_t RwFrameDestroyHierarchy(void *frame);
void *RwFrameUpdateObjects(void *frame);
void *func_003e9cb0(void *frame, const void *matrix, int32_t combine);
void *func_003c2630(int32_t vertices, int32_t triangles, uint32_t format);
void *func_003c22f0(void *geometry);
int32_t func_003c2a80(void *geometry);
const void *RpGeometryTriangleSetVertexIndices(const void *geometry, void *triangle, uint16_t a, uint16_t b, uint16_t c);
void *RpGeometryTriangleSetMaterial(void *geometry, void *triangle, void *material);
const void *RpMorphTargetCalcBoundingSphere(const void *morph, float sphere[4]);
void *RpMaterialCreate(void);
int32_t RpMaterialDestroy(void *material);
void *RpMaterialSetTexture(void *material, void *texture);
void *func_003c00e0(void);
void *func_003c1b90(void *atomic, void *frame);
void *func_003c0210(void *atomic, void *geometry, uint32_t flags);
int32_t func_003c02e0(void *atomic);
void *RwRasterCreate(int32_t width, int32_t height, int32_t depth, uint32_t flags);
uint8_t *RwRasterLock(void *raster, uint8_t level, int32_t mode);
void *RwRasterUnlock(void *raster);
void func_003ec330(void *raster);
void *func_003ef2e0(void *raster);
void func_003ef470(void *texture, const void *name);
int32_t func_003ef3a0(void *texture);
void mdlDrawVisibleAtomics(void *clump);
int32_t RpSkyRenderStateGet(int32_t state, void *out);
int32_t RpSkyRenderStateSet(int32_t state, uint32_t value);
#endif
