/* Retail 003e9eb0: the stripped body of RwFrameRegisterPlugin (decomp src/renderware/core/baframe.c).
 * The retail code loads the frame plugin registry (frameTKList, 0x0070B7A0) into a0 and tail-calls
 * _rwPluginRegistryAddPlugin (retail 003e3870). The decomp's own C for it is in baframe.c, which is
 * built, but the retail address has no `// FUN_` marker in a built unit (the only marker is in the
 * unbuilt src/rw/baraster.c), so it cannot be aliased. Forward to the decomp's C; it keeps the RWAPI
 * assertions that the retail tail call omits. */
#include <stdint.h>

/* The plugin callbacks are function pointers (RwPluginObjectConstructor, Destructor, CopyConstructor). */
extern int32_t RwFrameRegisterPlugin(int32_t size, uint32_t pluginID, void *(*constructCB)(void *, int32_t, int32_t),
                                     void *(*destructCB)(void *, int32_t, int32_t),
                                     void *(*copyCB)(void *, const void *, int32_t, int32_t));

int32_t func_003e9eb0(int32_t size, uint32_t pluginID, void *(*constructCB)(void *, int32_t, int32_t),
                      void *(*destructCB)(void *, int32_t, int32_t), void *(*copyCB)(void *, const void *, int32_t, int32_t))
{
    return RwFrameRegisterPlugin(size, pluginID, constructCB, destructCB, copyCB);
}
