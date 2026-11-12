/* RwResourcesSetArenaSize (renderware/plcore/baresour.c). The decomp already has this
 * function as C; the promoted unit kept INCLUDE_ASM, so no definition carried the retail
 * symbol. This forwards the address to the C definition. The retail flag at gp-18556 is
 * resourcesModule.numInstances, and the arena size is RWSRCGLOBAL(resArenaInitSize). */
extern s32 RwResourcesSetArenaSize(u32 size);

s32 func_003e22c0(u32 arg0) {
    return RwResourcesSetArenaSize(arg0);
}
