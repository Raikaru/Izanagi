# Trap stubs with no retail address

These names are called by the compiled decomp but no symbol list gives them a retail address, so
`tools/asm_worklist.py` cannot list them. Each is classified below with the files that reference it.
Owners other than PortPromotedB keep the stub until that lane ports or removes the caller.

| Name | Owner | Referenced from | Evidence |
|---|---|---|---|
| ADXF_GetFnameRangeEx | PortCriC | cri/libadxe/adx_fs.c, cri/libadxe/adx_tlk2.c, cri/re4/mwsfdply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| ADXM_IsSetupThrd | PortCriC | cri/re4/adx_inis.c, cri/re4/mwsfdsvr.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| ADXSJD_GetDecDtLen | PortCriC | cri/libadxe/adx_sjd.c, cri/re4/adx_tlk.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| ADXSJD_SetKeyString | PortCriC | cri/re4/adx_tlk.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| ADXSTM_GetNumSctIbuf | PortCriC | cri/re4/adx_tlk.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| ADXT_DestroyAll | PortCriC | cri/libadxe/adx_tlk.c, cri/re4/adx_inis.c, cri/re4/adx_t.h | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| ARAlloc | translated (PortPromotedB) | cri/re4/rna_res.c | GameCube SDK / PowerPC intrinsic with no PS2 retail code; defined in src/platform/gc_compat.c |
| ARFree | translated (PortPromotedB) | cri/re4/rna_res.c | GameCube SDK / PowerPC intrinsic with no PS2 retail code; defined in src/platform/gc_compat.c |
| AXRNA_ExecServer | PortCriC | cri/re4/adx_rnaa.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| AXRNA_Finish | PortCriC | cri/re4/adx_rnaa.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| AXRNA_GetNumData | PortCriC | cri/re4/adx_rnaa.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| AXRNA_GetNumRoom | PortCriC | cri/re4/adx_rnaa.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| AXRNA_Init | PortCriC | cri/re4/adx_rnaa.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| AXRNA_SetNumChan | PortCriC | cri/re4/adx_rnaa.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| AXRNA_SetStmHdInfo | PortCriC | cri/re4/adx_rnaa.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| CFT_MakeArgb8888Alp3110Tbl | PortCriC | cri/re4/sfx_alp.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| CFT_MakeArgb8888Alp3211Tbl | PortCriC | cri/re4/sfx_alp.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| CFT_MakeArgb8888AlpLumiTbl | PortCriC | cri/re4/sfx_alp.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| CFT_Ycc420plnToArgb8888 | PortCriC | cri/re4/sfx_YCC420PLN_to_ARGB8888PLN.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| CFT_Ycc420plnToArgb8888Init | PortCriC | cri/re4/sfx.h, cri/re4/sfx_lib.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| CFT_Ycc420plnToY84C44 | PortCriC | cri/re4/sfx.h, cri/re4/sfx_YCC420PLN_to_Y84C44.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPSGET_Finish | PortCriC | cri/re4/mps_lib.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPSGET_Init | PortCriC | cri/re4/mps_lib.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPS_GetLastSysHd | PortCriC | cri/re4/mps.h, cri/re4/sfd_mps.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPS_GetPketHd | PortCriC | cri/re4/mps.h, cri/re4/sfd_mps.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPS_GetSysHd | PortCriC | cri/re4/mps.h, cri/re4/sfd_mps.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPVFRM_Init | PortCriC | cri/re4/mpv_lib.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPV_DecodeFrmSj | PortCriC | cri/re4/mpv.h, cri/re4/mpv_hdec.c, cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPV_GetBitRate | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPV_GetLinkFlg | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPV_GetPicAtr | PortCriC | cri/re4/mpv.h, cri/re4/mpv_hdec.c, cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPV_GetVbvBufSiz | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MPV_SkipFrmSj | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MWSFD_GetPauseBdr | PortCriC | cri/re4/mwsfdlib.c, cri/re4/mwsfdply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| MWSFD_GetUsePicUsr | PortCriC | cri/re4/mwsfdcre.c, cri/re4/mwsfdfrm.c, cri/re4/mwsfdlib.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| RxHeapRealloc | PortAsmC (RW core) | renderware/p2/p2core.c, renderware/p2/p2heap.c | RW P2 heap realloc, sibling of the RxHeapAlloc/RxHeapFree rows PortAsmC owns |
| SFD_EntrySeek | PortCriC | cri/re4/sfd.h, cri/re4/sfd_ply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFD_GetPaStat | PortCriC | cri/re4/mwsfdply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFD_IsNextFrmReady | PortCriC | cri/re4/mwsfdfrm.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFD_SetByteRate | PortCriC | cri/re4/sfd.h, cri/re4/sfd_ply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFD_SetFileSize | PortCriC | cri/re4/sfd.h, cri/re4/sfd_ply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFD_SetSeekPos | PortCriC | cri/re4/sfd.h, cri/re4/sfd_ply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFD_SetTotTime | PortCriC | cri/re4/sfd.h, cri/re4/sfd_ply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_AllocFrm | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_EndDrawFrm | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_EndRefFrm | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_FreeFrm | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_GetNumFrm | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_HoldFrm | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_IsTermDec | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_RefStbyFrm | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_SearchFrmObj | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_SearchVfrmData | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_SetGopStat | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_StbyFrm | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFMPVF_TermDec | PortCriC | cri/re4/sfd_mpv.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFSEE_ExecServer | PortCriC | cri/re4/sfd.h, cri/re4/sfd_ply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFSEE_FixAvPlay | PortCriC | cri/re4/sfd.h, cri/re4/sfd_ply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFSEE_InitHn | PortCriC | cri/re4/sfd.h, cri/re4/sfd_ply.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_CnvFrmZ16 | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_CnvFrmZ32 | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_GetCnvBottomUp | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_GetCompoMode | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_GetOutZoffset | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_GetOutZscale | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_SetCnvBottomUp | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_SetOutZoffset | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SFX_SetOutZscale | PortCriC | cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SJUNI_Finish | PortCriC | cri/libadxe/adx_inis.c, cri/libadxe/sj_uni.c, cri/re4/adx_inis.c, cri/re4/sj_uni.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| SJUNI_Init | PortCriC | cri/libadxe/adx_inis.c, cri/libadxe/sj_uni.c, cri/re4/adx_inis.c, cri/re4/mwsfdlib.c, cri/re4/sj_uni.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| __cvt_fp2unsigned | translated (PortPromotedB) | cri/re4/adx_tlk.c, cri/re4/cftfx.c | GameCube SDK / PowerPC intrinsic with no PS2 retail code; defined in src/platform/gc_compat.c |
| __dcbt | translated (PortPromotedB) | cri/re4/mpv_mc.c, cri/re4/mpv_mcy.c | GameCube SDK / PowerPC intrinsic with no PS2 retail code; defined in src/platform/gc_compat.c |
| __dcbz | translated (PortPromotedB) | cri/re4/mpv_umc.c | GameCube SDK / PowerPC intrinsic with no PS2 retail code; defined in src/platform/gc_compat.c |
| __lwbrx | translated (PortPromotedB) | cri/re4/mpv_mc.c | GameCube SDK / PowerPC intrinsic with no PS2 retail code; defined in src/platform/gc_compat.c |
| __rlwimi | translated (PortPromotedB) | cri/re4/mpv_mc.c | GameCube SDK / PowerPC intrinsic with no PS2 retail code; defined in src/platform/gc_compat.c |
| __rlwinm | translated (PortPromotedB) | cri/re4/mpv_mc.c | GameCube SDK / PowerPC intrinsic with no PS2 retail code; defined in src/platform/gc_compat.c |
| _rpGeometryNativeSize | PortAsmC (RW core) | renderware/world/baworobj.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| _rpGeometryNativeWrite | PortAsmC (RW core) | renderware/world/baworobj.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| _rpWorldPipeAttach | PortAsmC (RW core) | renderware/world/baworobj.c, renderware/worldp2/bapipew.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| _rpWorldPipelineOpen | PortAsmC (RW core) | renderware/world/baworld.c, renderware/worldp2/bapipew.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| _rpWorldSectorNativeRead | PortAsmC (RW core) | renderware/world/baworobj.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| _rpWorldSectorNativeSize | PortAsmC (RW core) | renderware/world/baworobj.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| _rpWorldSectorNativeWrite | PortAsmC (RW core) | renderware/world/baworobj.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| _rwpathisabsolute | PortAsmC (RW core) | renderware/core/baimage.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| _rxWorldDevicePluginAttach | PortAsmC (RW core) | renderware/world/baworld.c | RenderWare function with no name or address in the decomp symbol lists; the retail image holds its own copy that has not been located by name |
| mwPlyEntryFnameCore | PortCriC | cri/re4/mwsfdsl.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| mwPlyEntryFnameSub | PortCriC | cri/re4/mwsfdsl.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| mwPlyReleaseSub | PortCriC | cri/re4/mwsfdsl.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| mwPlyStartSub | PortCriC | cri/re4/mwsfdsl.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| mwSfdDestroy | PortCriC | cri/re4/mwsfdcre.c, cri/re4/mwsfdply.c, cri/re4/mwsfdsfx.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
| mwlSfdSleepDecSvr | PortCriC | cri/re4/mwsfdply.c, cri/re4/mwsfdsvr.c | CRI library API with no retail symbol: the PS2 image has no code for it; referenced only from GameCube-origin CRI sources |
