# Override claims

One line per address: `<addr>  <owner>  <what it replaces>`. Claim an address here before writing `overrides/<addr>.c`. One owner per address.

Some source files are shared by multiple overrides:

- `src/viewer/p4pc_viewer.c`: shared viewer and development environment seams.

00100220  port  syscall 0x23 trap (kwlnExitThread)
00100230  port  boot heap base (kwlnProbeHeapBudget)
00100310  port  TIMER0 INTC callback, no-op (kwlnHandleTimerInterrupt)
00100350  port  kwlnInitIop, IOP reset replaced by p4pc_platform_init
00460a80  port  native queue clear plus release of synthetic sprite packs retired behind the main ordering table
0043ece8  port  malloc entry, native pointer bridge to literal retail EE newlib in src/platform/kernel_heap.c + ee_heap.c
0043ed08  port  free entry, native pointer bridge to literal retail EE newlib in src/platform/kernel_heap.c + ee_heap.c
0043ef80  port  _malloc_r (newlib), explicit guest reent and original chunk/bin/reuse implementation in src/platform/ee_heap.c
0043e170  port  _free_r (newlib), explicit guest reent and immediate original coalescing/bin implementation in src/platform/ee_heap.c
0042ba20  port  DIntr (libkernl), port-owned definition in src/platform/ps2_kernel.c
0042ba70  port  EIntr (libkernl), port-owned definition in src/platform/ps2_kernel.c
0043f9c8  port  memset (libc): explicit EE-u32 count boundary over reviewed host byte semantics in src/platform/ee_bytes.c
00100e30  port  kwlnEnableInterrupts, sync/ei dropped
00101350  port  kwlnMainLoop: unchanged native boot/seed/update loop plus inactive-by-default battle-tool update boundary
00174680  port  native scripted-field encounter request plus opt-in first-request observation before field mutation and enemy creation
00192e90  port  native battle task constructor plus opt-in pre-init and post-create observation; preserve setup and task order
001932f0  port  native script encounter request plus opt-in pre-fade and original-getter-result observation; preserve constructor and sequence order
00193840  port  native packed-block copy; port coordinates admission, the shared skill/enemy bank observer and the table-service extent note published with each GP store
001a4800  port  unchanged native READY-to-execution owner plus opt-in copied committed-command observation; free Persona/Analyze remain distinct
001a2b50  port  native AI-to-READY transition plus the p4.battle.decide stage (battle_decide.c) once the command record is released; retail body when no decider is enabled
001b15b0  port  unchanged native queue reorder/admission owner plus opt-in copied ordinary/followup actor admission
001f3010  port  unchanged native target HP/SP/status/death commit plus opt-in requested-versus-effective resource observation, including zero-delta native clamps; port side-ailment facet (port): commit a marked landing and cure-mask clear at status apply, clear on defeat
001f39d0  port  unchanged native action-cost/inventory owner plus opt-in copied actual cost commit; no second native cost calculation
00202e70  port  native selection-input owner plus opt-in party policy input codes; original UI validation/commit handlers remain authoritative
001a1c80  port  native filtered target initialization plus opt-in canonical policy target key before native highlighting; preserve lookAtUnit NULL mirror fix
0042f6f8  port  scePadInit: SDL init, returns 1 on success
0042fb50  port  scePadPortOpen: port 0 slot 0 is the keyboard and gamepad
0042ffa0  port  scePadRead: DualShock 2 packet from SDL snapshot
00430018  port  scePadGetState: stable (6) for open port 0, else 0
00430578  port  scePadSetMainMode: records digital or analog mode
00430210  port  scePadInfoAct: reports zero actuators
004306f8  port  scePadSetActAlign: no actuators, returns 0
00430180  port  scePadGetReqState: reports request complete
00430630  port  scePadSetActDirect: no actuators, refused
0042eae0  port  sceCdInit (trapped as func_0042eae0): mounts P4PC_ISO and P4PC_OVERRIDE_DIR, returns 1 on success
0042f600  port  sceCdMmode: drive mode no-op, returns 1
00428550  port  sceDopen (libkernl filestub): opens a directory listing over the VFS tree
00428618  port  sceDclose (libkernl filestub): closes a directory handle
00428780  port  sceDread (libkernl filestub): next entry as a 0x148-byte sceIoDirent
004288d8  port  sceGetstat (libkernl filestub): stat over the VFS tree, 64-byte sceIoStat
00446ed8  port  vsprintf: shared retail conversion engine in ee_printf.c, native va_list pass-through
0044db98  port  __pack_f (fp-bit): port-owned definition in src/platform/ps2_fpbit.c
0044dc50  port  __unpack_f (fp-bit): port-owned definition in src/platform/ps2_fpbit.c
0044ddf0  port  __unpack_d (fp-bit): port-owned definition in src/platform/ps2_fpbit.c
0044e508  port  __fpcmp_parts_d (fp-bit): port-owned definition in src/platform/ps2_fpbit.c
004010c0  port  fpSystem RenderWare device dispatcher, port-owned definition in src/render/p4gl_device.c (OpenGL 3.3 core)
003f5070  port  fpRenderStateSet (device block +0x10): port-owned definition in src/render/p4gl_state.c
003f5d90  port  fpRenderStateGet (device block +0x14): port-owned definition in src/render/p4gl_state.c
003f6780  port  RpSkySelectDeepZBuffer (basky.c, sky2 driver, excluded): port-owned definition in src/render/p4gl_state.c
003f98a0  port  stdFunc RASTERCREATE (4): port-owned definition in src/render/p4gl_raster.c
003fb830  port  stdFunc RASTERDESTROY (5): port-owned definition in src/render/p4gl_raster.c
003fc5f0  port  stdFunc RASTERLOCK (15): port-owned definition in src/render/p4gl_raster.c
003fd230  port  stdFunc RASTERUNLOCK (16): port-owned definition in src/render/p4gl_raster.c
003fd460  port  stdFunc RASTERLOCKPALETTE (23): port-owned definition in src/render/p4gl_raster.c
003fd520  port  stdFunc RASTERUNLOCKPALETTE (24): port-owned definition in src/render/p4gl_raster.c
003f7a60  port  stdFunc TEXTURESETRASTER (8): port-owned definition in src/render/p4gl_raster.c
003fd630  port  stdFunc RASTERGETMIPLEVELS (28): port-owned definition in src/render/p4gl_raster.c
003f6a10  port  stdFunc CAMERABEGINUPDATE (1): port-owned definition in src/render/p4gl_camera.c
003f73d0  port  stdFunc CAMERAENDUPDATE (10): port-owned definition in src/render/p4gl_camera.c
003f7490  port  stdFunc CAMERACLEAR (21): port-owned definition in src/render/p4gl_camera.c
003f79b0  port  stdFunc RASTERSHOWRASTER (20): port-owned definition in src/render/p4gl_camera.c
0040afb0  port  Im2D RenderLine (device +0x18): port-owned definition in src/render/p4gl_im2d.c
0040b230  port  Im2D RenderTriangle (device +0x1c): port-owned definition in src/render/p4gl_im2d.c
0040c0f0  port  Im2D RenderPrimitive (device +0x20): port-owned definition in src/render/p4gl_im2d.c
0040d0a0  port  Im2D RenderIndexedPrimitive (device +0x24): port-owned definition in src/render/p4gl_im2d.c
00401450  port  _rwDeviceGetHandle (func_00401450, sky2 basky.c excluded): port-owned definition in src/platform/rw_device_handle.c, returns D_0070C220
003e22c0  port  RwResourcesSetArenaSize (decomp C in renderware/plcore/baresour.c; promoted marker was INCLUDE_ASM, so the address is forwarded to it)
003e9eb0  port  RwFrameRegisterPlugin (retail tail-call into the registry add with frameTKList 0x0070B7A0; port-owned forwarder in src/platform/rw_frame_register.c, since the baraster.c marker is in unbuilt src/rw)
001026b0  port  FldShadow_PollIntcBit13 (k_shadow.c): reads modeled GS CSR.FIELD at 0x12001000; original negative field gate retained
004214c0  port  AddIntcHandler (syscall 0x10): port-owned definition func_004214c0 in src/platform/ps2_kernel.c
004214e0  port  RemoveIntcHandler (syscall 0x11): port-owned definition in src/platform/ps2_kernel.c
00422328  port  EnableIntc wrapper (DIntr/_EnableIntc 0x421520/EIntr): port-owned definition func_00422328 in src/platform/ps2_kernel.c
003d5000  port  func_003d5000 (promoted code1_003d): RW-style 4x4 matrix slot table update, error path via _rwerror/RwErrorSet
003c9eb0  port  func_003c9eb0 (promoted code1_003c, 948 bytes): called from kwlnInitSceneRig
0039b8d0  port  func_0039b8d0 (promoted code1_0039, 420 bytes)
00390520  port  func_00390520 (promoted code1_0039, 340 bytes): called from kwlnInitEngine
003999a0  port  func_003999a0 (promoted code1_0039, 356 bytes): called from kwlnInitEngine
00417470  port  func_00417470 (promoted code1_0041, 156 bytes): called from kwlnInitEngine
00415070  port  func_00415070 RxHeapCreate (promoted code1_0041, asm-only in decomp; p2core.c aliases it): called from RwEngineStart path
00414820  port  func_00414820 (promoted code1_0041, asm-only in decomp): RxHeap 8-byte slot pool; called by func_00414930
00414930  port  func_00414930 (promoted code1_0041, asm-only in decomp): RxHeap block carve; called by func_00414ef0
00414ef0  port  func_00414ef0 (promoted code1_0041, asm-only in decomp): RxHeap init; called by func_00415070
003bd160  port  func_003bd160 (promoted code1_003b, 776 bytes): called from kwlnInitEngine
003fb3f0  port  stdFunc SUBRASTER (12): port-owned definition in src/render/p4gl_raster.c (borrowed shadow view, no VRAM)
003fdd10  port  stdFunc NATIVETEXTUREGETSIZE (25): port-owned definition in src/render/p4gl_native.c
003fdf20  port  stdFunc NATIVETEXTUREREAD (26): port-owned definition in src/render/p4gl_native.c
003fddc0  port  stdFunc NATIVETEXTUREWRITE (27): port-owned definition in src/render/p4gl_native.c
003fe1f0  port  stdFunc HINTRENDERF2B (22): port-owned definition in src/render/p4gl_device.c (returns 1)
003c5510  port  func_003c5510 (promoted code1_003c, asm-only): RW plugin 0x502 constructor; trapped and failed _rwPluginRegistryInitObject
003bfd00  port  func_003bfd00 (promoted code1_003b, asm-only): RW plugin 0x504 constructor; trapped and returned 0 from _rwPluginRegistryInitObject
003c95a0  port  func_003c95a0 (port-owned src/platform/rw_plugin_constructors.c; no FUN_ marker in a built unit): RW plugin 0x507 constructor; trap returned 0
003e18c0  port  func_003e18c0 (port-owned src/platform/rw_freelist_thunks.c; no FUN_ marker in a built unit): forwards to RwFreeListForAllUsed (native, bamemory.c); trap was hit from the 0x505 destroy callback
004215e0  port  CreateThread (syscall 0x20): port-owned definition in src/platform/kernel_thread.c
00421600  port  StartThread (syscall 0x22): port-owned definition in src/platform/kernel_thread.c
00421610  port  ExitThread (syscall 0x23): port-owned definition in src/platform/kernel_thread.c
00421620  port  ExitDeleteThread (syscall 0x24): port-owned definition in src/platform/kernel_thread.c
00421650  port  DisableDispatchThread (syscall 0x27): port-owned definition in src/platform/kernel_thread.c
00421660  port  EnableDispatchThread (syscall 0x28): port-owned definition in src/platform/kernel_thread.c
00421670  port  ChangeThreadPriority (syscall 0x29): port-owned definition in src/platform/kernel_thread.c
00421680  port  iChangeThreadPriority (syscall -42): port-owned definition in src/platform/kernel_thread.c
00421690  port  RotateThreadReadyQueue (syscall 0x2b): port-owned definition in src/platform/kernel_thread.c
004216a0  port  iRotateThreadReadyQueue (syscall -44): port-owned definition in src/platform/kernel_thread.c
004216b0  port  ReleaseWaitThread (syscall 0x2d): port-owned definition in src/platform/kernel_thread.c
004216c0  port  iReleaseWaitThread (syscall -46): port-owned definition in src/platform/kernel_thread.c
004216d0  port  GetThreadId (syscall 0x2f): port-owned definition in src/platform/kernel_thread.c
004216e0  port  ReferThreadStatus (syscall 0x30): retail status-bit success result, src/platform/kernel_thread.c
004216f0  port  iReferThreadStatus (syscall -49): shared status-bit success result, src/platform/kernel_thread.c
00421700  port  SleepThread (syscall 0x32): port-owned definition in src/platform/kernel_thread.c
00421710  port  WakeupThread (syscall 0x33): retail target-TID success result, src/platform/kernel_thread.c
00421720  port  iWakeupThread (syscall -52): shared target-TID success result, src/platform/kernel_thread.c
00421730  port  CancelWakeupThread (syscall 0x35): port-owned definition in src/platform/kernel_thread.c
00421740  port  iCancelWakeupThread (syscall -54): port-owned definition in src/platform/kernel_thread.c
00421750  port  SuspendThread (syscall 0x37): retail target-TID success result, src/platform/kernel_thread.c
00421760  port  iSuspendThread (syscall -56): shared target-TID success result, src/platform/kernel_thread.c
00421770  port  ResumeThread (syscall 0x39): retail target-TID success result, src/platform/kernel_thread.c
00421780  port  iResumeThread (syscall -58): shared target-TID success result, src/platform/kernel_thread.c
004217e0  port  CreateSema (syscall 0x40) and 004217f0 DeleteSema (0x41): port-owned definitions in src/platform/kernel_thread.c
00421800  port  SignalSema (syscall 0x42): port-owned definition in src/platform/kernel_thread.c
00421810  port  iSignalSema (syscall -67): port-owned definition in src/platform/kernel_thread.c
00421820  port  WaitSema (syscall 0x44): port-owned definition in src/platform/kernel_thread.c
00421830  port  PollSema (syscall 0x45): port-owned definition in src/platform/kernel_thread.c
003ed960  port  func_003ed960 (basync.c, 248 bytes)
003b7ca0  port  func_003b7ca0 (rpskin.c, 352 bytes)
003cad90  port  func_003cad90 (baworobj.c, 220 bytes)
003f24a0  port  func_003f24a0 (palquant.c, 288 bytes)
003f26c0  port  func_003f26c0 (palquant.c, 160 bytes)
003d6680  port  func_003d6680 (rtanim.c, 260 bytes)
003d6790  port  func_003d6790 (rtanim.c, 280 bytes)
003d68b0  port  func_003d68b0 (rpuvanim.c, 68 bytes)
003d6950  port  func_003d6950 (rpuvanim.c, 172 bytes)
003d6aa0  port  func_003d6aa0 (rpuvanim.c, 204 bytes)
003d6c50  port  func_003d6c50 (rpuvanim.c, 248 bytes)
003d6d50  port  func_003d6d50 (rpuvanim.c, 292 bytes)
003d6e80  port  func_003d6e80 (rpuvanim.c, 268 bytes)
003b8050  port  func_003b8050 (rpskin.c, 596 bytes)
003b7510  port  SkinFindMaxWeights (rpskin.c, 128 bytes): port-owned definition in src/platform/rw_skin_callees.c
003b7590  port  SkinFindNumUsedBones (rpskin.c, 236 bytes): port-owned definition in src/platform/rw_skin_callees.c
003b7680  port  func_003b7680 (rpskin.c, 480 bytes): port-owned definition in src/platform/rw_skin_callees.c
003d6350  port  func_003d6350 (rtanim.c, 644 bytes)
0040fcd0  port  func_0040fcd0 (RpSkyTexCacheRasterLock): port changed it to return 1: the port has no GS texture ring and its rasters lack the sky2 private layout
003f07d0  port  func_003f07d0 (palquant.c, 1084 bytes): palette quantizer, 16/256-colour path
003f17e0  port  func_003f17e0 (palquant.c, 1280 bytes)
003f1ce0  port  func_003f1ce0 (palquant.c, 1972 bytes)
003eff30  port  func_003eff30 (imrascnv.c, 1744 bytes): palquant callee
003f0610  port  func_003f0610 (imrascnv.c, 448 bytes): palquant callee
003f0510  port  func_003f0510 (imrascnv.c, 256 bytes): palquant callee
003f0c10  port  func_003f0c10 (palquant.c, 460 bytes): palquant callee
003f0de0  port  func_003f0de0 (palquant.c, 584 bytes): palquant callee
003f1240  port  func_003f1240 (palquant.c, 360 bytes): palquant callee
003f13b0  port  func_003f13b0 (palquant.c, 356 bytes): palquant callee
003f1520  port  func_003f1520 (palquant.c, 212 bytes): palquant callee
003f1600  port  func_003f1600 (palquant.c, 252 bytes): palquant callee
003f1700  port  func_003f1700 (palquant.c, 216 bytes): palquant callee
003f02d0  port  func_003f02d0 (palquant.c, 824 bytes): palquant callee
003f1030  port  func_003f1030 (palquant.c, 524 bytes): palquant callee
003f0110  port  func_003f0110 (imrascnv.c, 1272 bytes): palquant callee
003f03f0  port  func_003f03f0 (palquant.c, 544 bytes): palquant callee
003cf140  port  func_003cf140 (PS2All nodeBody, DMA producer via func_0040a800): replaced by the GL atomic node
003cf610  port  func_003cf610 (PS2AllNodeBody, 924 bytes): replaced by the GL atomic node
0043f810  port  memcpy (libc): explicit EE-u32 count boundary over reviewed nonoverlapping host byte copies in src/platform/ee_bytes.c
0043f8c0  port  memmove (libc): explicit EE-u32 count boundary over reviewed host overlap-preserving byte copies in src/platform/ee_bytes.c
0044b310  port  fabs on a double (func_0044b310, middleware_inferred_00446108): overrides/0044b310.c
00443010  port  strstr (libc, decomp rename `strstr` -> func_00443010): reviewed host substring semantics in src/platform/ee_bytes.c
003f6440  port  RpSkyRenderStateSet (basky.c, GIF packet writer): port-owned state record in src/render/p4gl_state.c
003f6690  port  RpSkyRenderStateGet (basky.c): reads the port-owned state record in src/render/p4gl_state.c
003fe630  port  _rwDeviceRegisterPlugin (RwEngineInit driver hook; retail pool/registry setup; marker in excluded src/rw/basky.c, so port-owned definition in src/platform/rw_device_register.c)
001f5680  port  func_001f5680 (btlTarget, 76 bytes)
00396e80  port  func_00396e80 (code1_0039, 120 bytes)
003d5710  port  func_003d5710 (code1_003d, 56 bytes)
003cc250  port  func_003cc250 (code1_003c, 104 bytes)
003b5fb0  port  func_003b5fb0 (code1_003b, 104 bytes)
003b6390  port  func_003b6390 (code1_003b, 144 bytes)
003b6420  port  func_003b6420 (code1_003b, 152 bytes)
003b64c0  port  func_003b64c0 (code1_003b, 128 bytes)
0048a810  port  mdlEffectCatmullRomVU: VU0 Catmull-Rom blend in C, result left in vf10
004adb50  port  mdlEffectUpdateModelChildTransforms (effect model update): VU0 colour island via p4_vu0_packed_color_scale2, quaternion store via sqc2 bridge
00463ea0  port  tmxDmaCopy (sdkUttmx): EE DMAC scratchpad copy replaced by memmove
00410420  port  RwIm3DTransform (baim3d.c): keeps the Im3D pool state, copies the vertices to clip space for the GL draw (overrides/00410420.c, src/render/p4gl_im3d.c)
004104d0  port  RwIm3DEnd (baim3d.c): retail pool reset plus dropping the GL vertex copy (overrides/004104d0.c)
00410520  port  RwIm3DRenderIndexedPrimitive (baim3d.c): retail stash and index rounding, GL draw instead of RxPipelineExecute (overrides/00410520.c)
004106a0  port  RwIm3DRenderPrimitive (baim3d.c): retail stash, GL draw instead of RxPipelineExecute (overrides/004106a0.c)
00410800  port  RwIm3DRenderTriangle (baim3d.c): one-triangle GL draw, indices on the stack instead of the RxHeap (overrides/00410800.c)
00410930  port  RwIm3DRenderLine (baim3d.c): one-line GL draw, indices on the stack instead of the RxHeap (overrides/00410930.c)
003a8840  port  func_003a8840 (PS2PTank nodeBody, DMA producer via func_003f3eb0): replaced by the GL particle draw (overrides/003a8840.c, src/render/p4gl_ptank.c)
00489f80  port  mdlEffectEmitMaskedTagPair (GS FRAME_1/CLAMP_1 packet writer): sets the GL mask/clamp state (overrides/00489f80.c, src/render/p4gl_mdleffect.c)
0048a000  port  mdlEffectEmitTagPair (GS FRAME_1/CLAMP_1 packet writer): sets the GL mask/clamp state (overrides/0048a000.c, src/render/p4gl_mdleffect.c)
0048a070  port  mdlEffectEmitValuePacket (GS FRAME_1/CLAMP_1 packet writer): sets the GL mask/clamp state (overrides/0048a070.c, src/render/p4gl_mdleffect.c)
0048a0e0  port  mdlEffectEmitSlotPacket (GS FRAME_1/CLAMP_1 packet writer): sets the GL mask/clamp state (overrides/0048a0e0.c, src/render/p4gl_mdleffect.c)
001852f0  port  FldNoiseEmitTagged (GS FRAME_1/CLAMP_1 packet writer): sets the GL mask/clamp state (overrides/001852f0.c, src/render/p4gl_mdleffect.c)
00185370  port  FldNoiseEmitPlain (GS FRAME_1/CLAMP_1 packet writer): sets the GL mask/clamp state (overrides/00185370.c, src/render/p4gl_mdleffect.c)
003a2290  port  func_003a2290 (rpPTankAtomicRenderCB, PTank atomic render callback): draws the particle arrays in GL, skips instancing and the pipeline (overrides/003a2290.c)
003cc370  port  func_003cc370 (Im3D platform pipeline setup, promoted code1_003c_cw119 copy excluded): port-owned success stub in src/render/p4gl_im3d.c, the GL Im3D path reads no pipes
0043c710  port  calloc entry: literal retail EE low-word multiply and usable-chunk clearing (0043c738), src/platform/kernel_heap.c + ee_heap.c
00441558  port  realloc entry: native pointer / EE u32 size bridge to literal retail 00441580 (null/zero, split/merge/copy branches), src/platform/ee_heap.c
0044dca8  port  __make_fp pack_f wrapper (fp-bit): src/platform/ps2_fpbit.c
0044dcd8  port  fptodp (fp-bit): src/platform/ps2_fpbit.c
0044dd18  port  __pack_d (fp-bit): src/platform/ps2_fpbit.c
0044de80  port  __add_core (fp-bit): src/platform/ps2_fpbit.c
0044e0a8  port  raw-bit double add wrapper: unpack -> retail __add_core -> pack in src/platform/ps2_fpbit.c
0044e100  port  dpsub (fp-bit): src/platform/ps2_fpbit.c
0044e168  port  double multiply wrapper (fp-bit): src/platform/ps2_fpbit.c
0044e1c0  port  __mul_core (fp-bit): src/platform/ps2_fpbit.c
0044e3b8  port  raw-bit double divide wrapper: unpack -> retail __div_core -> pack in src/platform/ps2_fpbit.c
0044e608  port  raw-bit double compare wrapper: unpack -> retail __fpcmp_parts_d in src/platform/ps2_fpbit.c
0044e658  port  litodp signed-word normalization and pack in src/platform/ps2_fpbit.c
0044e708  port  dptoli NaN/zero handling, exponent saturation and fraction truncation in src/platform/ps2_fpbit.c
0044e7a8  port  __make_dp pack_d wrapper (fp-bit): src/platform/ps2_fpbit.c
0044e7d8  port  dptofp (fp-bit): src/platform/ps2_fpbit.c
003df9f0  port  RwMatrixOrthoNormalize: overrides/003df9f0.c
003e0680  port  RwMatrixRotate with precomputed (1 - cos, sin): overrides/003e0680.c
003e0e20  port  RwMatrixTransform (combine modes 0/1/2): overrides/003e0e20.c
00412ca0  port  plugin registry open: overrides/00412ca0.c
00412e90  port  plugin registry find by name: overrides/00412e90.c
00412fb0  port  plugin registry add entries (variadic ids): overrides/00412fb0.c
004125e0  port  plugin registry close: overrides/004125e0.c
00442a80  port  strncat (libc): explicit EE-u32 callback count over reviewed bounded host append semantics in src/platform/ee_bytes.c
00443f18  port  strtok: retail byte tokenizer with active guest reent +0x5c save slot in src/platform/ps2_libc.c
00443f38  port  strtok_r: retail byte tokenizer with four-byte guest save slot in src/platform/ps2_libc.c
00446e88  port  vprintf: shared retail conversion engine with host stdout byte transport
004470f0  port  __ieee754_acos (double bits): literal retail soft-double core in src/platform/ee_dlibm.c
004476b0  port  __ieee754_log (double bits): literal retail soft-double core in src/platform/ee_dlibm.c
00447b88  port  __ieee754_log10 (double bits): literal retail soft-double core in src/platform/ee_dlibm.c
00447d00  port  __ieee754_pow (double bits): literal retail soft-double core in src/platform/ee_dlibm.c
00448a00  port  __ieee754_sqrt (double bits): literal retail integer/soft-double core in src/platform/ee_dlibm.c
00448d10  port  __ieee754_acosf: literal retail core in src/platform/ee_libm.c
00449120  port  __ieee754_asinf: literal retail core in src/platform/ee_libm.c
004494b8  port  __ieee754_atan2f: literal retail core in src/platform/ee_libm.c
0044b340  port  atanf: literal retail core in src/platform/ee_libm.c
004222c0  port  DisableIntc wrapper (code1_0042, excluded unit): src/platform/ps2_kernel.c
00422390  port  DisableDmac wrapper (code1_0042, excluded unit): src/platform/ps2_kernel.c
00422920  port  start a dormant thread with an argument (code1_0042, excluded unit): src/platform/kernel_thread.c
004244c8  port  scePrintf: shared retail conversion engine with host stdout byte transport
0042e7c8  port  sceCdSync on shared logical DVD device; reviewed timing seam, sce_cdvd.c
0042efe0  port  sceCdDiskReady: src/platform/files/sce_cdvd.c
0042f200  port  sceCdRead on shared seek/read-ahead/DMA device, sce_cdvd.c
0042f548  port  sceCdBreak cancels physical request before owner release, sce_cdvd.c
004b55a0  port  mdlEffect table callback +0x10 runner: overrides/004b55a0.c
004b5620  port  mdlEffect table callback +0x14 runner: overrides/004b5620.c
004f9638  port  RSU block byte permutation (a): overrides/004f9638.c
004f96a0  port  RSU block byte permutation (b): overrides/004f96a0.c
004f9770  port  RSU block byte permutation (c): overrides/004f9770.c
004f98a8  port  RSU block byte permutation (d): overrides/004f98a8.c
004f9910  port  RSU block byte permutation (e): overrides/004f9910.c
00413290  port  plugin registry index lookup (callee of 00412fb0): overrides/00413290.c
004147a0  port  plugin registry finalise (callee of 004125e0): overrides/004147a0.c
00413350  port  plugin registry cell find-or-add: overrides/00413350.c
00413b80  port  plugin registry link pass: overrides/00413b80.c
00413e50  port  plugin registry grouping pass: overrides/00413e50.c
004140f0  port  plugin registry group colouring: overrides/004140f0.c
004141e0  port  plugin registry distinct-key walk: overrides/004141e0.c
004142b0  port  plugin registry live-cell array: overrides/004142b0.c
004143f0  port  plugin registry layout: overrides/004143f0.c
004156e0  port  keyed record sort (RW registry): keyed wrapper in src/platform/ps2_libc.c uses retail qsort func_00440bb8
00413410  port  plugin registry cell removal: overrides/00413410.c
00413700  port  plugin registry cell table build: overrides/00413700.c
0042e2b8  port  layer-aware file search (sceCdSearchFile core): src/platform/files/sce_cdvd.c
003b4230  port  func_003b4230 (code1_003b, 168 bytes)
003b65d0  port  func_003b65d0 (code1_003b, 168 bytes)
003b6540  port  func_003b6540 (code1_003b, 136 bytes)
003d0790  port  func_003d0790 (code1_003d, 188 bytes)
003cbf30  port  func_003cbf30 (code1_003c, RpWorld light removal, 220 bytes)
003ce840  port  func_003ce840 (code1_003c, 412 bytes)
0039a910  port  func_0039a910 (code1_0039, jump-table maps, 280 bytes)
003a2770  port  func_003a2770 (code1_003a, 432 bytes)
003a5180  port  func_003a5180 (code1_003a, 248 bytes)
003b4ab0  port  func_003b4ab0 (code1_003b, 344 bytes)
003b4c10  port  func_003b4c10 (code1_003b, 416 bytes)
003b4ff0  port  func_003b4ff0 (code1_003b, 360 bytes)
003b5650  port  func_003b5650 (code1_003b, 464 bytes)
003b5ac0  port  func_003b5ac0 (code1_003b, 296 bytes)
003b5bf0  port  func_003b5bf0 (code1_003b, 304 bytes)
003b6020  port  func_003b6020 (code1_003b, 440 bytes)
003b61e0  port  func_003b61e0 (code1_003b, 424 bytes)
004ed600  port  sj_mem SJMEM_Create: native alias to the re4 function (config/native_names.txt)
004ed710  port  sj_mem SJMEM_Destroy: native alias to the re4 function (config/native_names.txt)
004ed7c8  port  sj_mem SJMEM_GetUuid: native alias to the re4 function (config/native_names.txt)
004ed858  port  sj_mem SJMEM_EntryErrFunc: native alias to the re4 function (config/native_names.txt)
004ed908  port  sj_mem SJMEM_Reset: native alias to the re4 function (config/native_names.txt)
004ed9e8  port  sj_mem SJMEM_GetChunk: native alias to the re4 function (config/native_names.txt)
004edb28  port  sj_mem SJMEM_PutChunk: native alias to the re4 function (config/native_names.txt)
004edc18  port  sj_mem SJMEM_UngetChunk: native alias to the re4 function (config/native_names.txt)
004edd68  port  sj_mem SJMEM_IsGetChunk: native alias to the re4 function (config/native_names.txt)
004edf18  port  sj_rbf SJRBF_Init: native alias to the re4 function (config/native_names.txt)
004edf98  port  sj_rbf SJRBF_Finish: native alias to the re4 function (config/native_names.txt)
004ee140  port  sj_rbf SJRBF_Destroy: native alias to the re4 function (config/native_names.txt)
004ee210  port  sj_rbf SJRBF_Reset: native alias to the re4 function (config/native_names.txt)
004ee310  port  sj_rbf SJRBF_GetChunk: native alias to the re4 function (config/native_names.txt)
004ee510  port  sj_rbf SJRBF_PutChunk: native alias to the re4 function (config/native_names.txt)
004ee6e0  port  sj_rbf SJRBF_UngetChunk: native alias to the re4 function (config/native_names.txt)
004ee910  port  sj_rbf SJRBF_IsGetChunk: native alias to the re4 function (config/native_names.txt)
004f0518  port  SVM lock (type), port-owned src/cri/svm_port.c
004f0578  port  SVM unlock (type), port-owned src/cri/svm_port.c
004f0ef0  port  SVM_ExecSvrFunc, port-owned src/cri/svm_port.c
004f0a60  port  SVM_SetCbSvr wrapper: native alias
004f0b88  port  SVM_DelCbSvr wrapper: native alias
004f17a8  port  RFHN_IsUseImgHn (rofs_hn asm): in-use image-handle entry lookup
004f5878  port  RFMAI_CnvS64toU32plusU32 (rofs_mai asm): split a 64-bit value into two words
004f69d8  port  cvmh_GetSize (rofs_pfs asm): byte-swap two words end for end
004f6a70  port  cvmh_GetTimInfo (rofs_cvmh asm): 7-byte copy
004f7610  port  toUpper (rofs_cvmh asm): ASCII upper-casing
004222b0  port  QueryIntrContext (CP0 SR read): always 0, no interrupt dispatch in the port
0043c6a0  port  func_0043c6a0 (libmc2): integer absolute value
0043dc50  port  func_0043dc50 (libmc2): returns the word at 0x00710070
0044e410  port  __div_core (fp-bit): port-owned definition in src/platform/ps2_fpbit.c, with the other fp-bit cores
00414c10  port  func_00414c10 (RxHeapFree, promoted code1_0041): free a block and merge free neighbours
00414e10  port  func_00414e10 (RxHeapAlloc, promoted code1_0041): allocate from the free entries, growing the heap
00414a30  port  func_00414a30 (RxHeap grow, callee of RxHeapAlloc): new superblock carved into the block list
00414b50  port  func_00414b50 (RxHeap carve, callee of RxHeapAlloc): take or split a free entry
004db708  port  ahxsbf_mult_win0 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004db8f8  port  ahxsbf_mult_win1 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dbae8  port  ahxsbf_mult_win2 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dbcd8  port  ahxsbf_mult_win3 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dbec8  port  ahxsbf_mult_win4 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dc0b8  port  ahxsbf_mult_win5 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dc2a8  port  ahxsbf_mult_win6 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dc498  port  ahxsbf_mult_win7 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dc688  port  ahxsbf_mult_win8 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dc878  port  ahxsbf_mult_win9 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dca68  port  ahxsbf_mult_win10 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dcc58  port  ahxsbf_mult_win11 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dce48  port  ahxsbf_mult_win12 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dd038  port  ahxsbf_mult_win13 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dd228  port  ahxsbf_mult_win14 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004dd418  port  ahxsbf_mult_win15 (AHX synthesis window, cri/ahx_sbf.c asm): 32 clamped 16-term products
004da518  port  AHXDCD_SetExtPrm (cri/ahx_dcd.c asm): 8-byte copy
004da530  port  AHXDCD_GetOutVol (cri/ahx_dcd.c asm)
004da550  port  AHXDCD_GetDrctLvl (cri/ahx_dcd.c asm)
004da570  port  AHXDCD_GetFx (cri/ahx_dcd.c asm)
004da5b8  port  AHXDCD_GetOutPan (cri/ahx_dcd.c asm)
004d65f8  port  adxt_SetLnkSw (cri/adx_tlk.c asm): store plus func_004cec28 tail call
00505078  port  func_00505078 (cri/mpv_hdec.c asm)
00515980  port  func_00515980 (cri/sfd_buf.c asm)
005159a0  port  func_005159a0 (cri/sfd_buf.c asm)
005265a0  port  func_005265a0 (cri/sfd_tim.c asm): time record to ticks
005277a0  port  func_005277a0 (cri/sfd_trn.c asm)
005277e0  port  sftrn_ConnBufTrn (cri/sfd_trn.c asm)
00450dd0  port  sdkDbprtPrint (sdkDbprt debug text, variadic): va_list instead of MWCC builtins
00450e80  port  sdkDbprtPrintColor (sdkDbprt debug text with colour, variadic): va_list instead of MWCC builtins
00426670  port  fileio lock semaphore creation (no-op on the host): src/platform/files/sce_file.c
00426cf0  port  sceOpen under its m2c name: src/platform/files/sce_file.c
00426f80  port  sceClose under its m2c name: src/platform/files/sce_file.c
004270f8  port  sceLseek under its m2c name: src/platform/files/sce_file.c
00427338  port  sceRead under its m2c name: src/platform/files/sce_file.c
004275a8  port  sceWrite under its m2c name: src/platform/files/sce_file.c
00427868  port  fio status call (commands 1-3 answered locally): src/platform/files/sce_file.c
00427dd8  port  path command call (remove, rmdir, deldrv, chdir): src/platform/files/sce_file.c
00428f08  port  sceSync under its m2c name: src/platform/files/sce_file.c
00429d10  port  IOP heap alloc for the sound driver: src/platform/sound/ps2_sndcom.c
00429d90  port  IOP heap free for the sound driver: src/platform/sound/ps2_sndcom.c
00414fd0  port  func_00414fd0 (RxHeapDestroy, promoted code1_0041)
0039b720  port  func_0039b720 (PDS registry: build type-2 derived pipe)
0039b830  port  func_0039b830 (PDS registry destroy)
0039ba80  port  PDSRegistryCreatePipes (func_0039ba80)
0039bb70  port  PDSRegistryGetPipe (func_0039bb70)
0039bbd0  port  func_0039bbd0 (PDS registry: build type-1 pipe from a PS2AllMat spec)
003d3130  port  PS2AllMat node configuration: func_003d3130, func_003d3460, func_003d35f0, func_003d3780 (all four in overrides/003d3130.c)
003d39a0  port  func_003d39a0 (PS2AllMat callback slot setter)
003d30b0  port  func_003d30b0 (PS2AllMat pipelineNodeInit; VU1 layout func_003d2c40/func_003cc720 deliberately not run, see the file)
004125d0  port  RxPipelineNodeGetInitData: p2define.c (now a unit) is the retail code; config/native_names.txt maps the address
00411740  port  StalacTiteAlloc (func_00411740) and 004117b0 StalacMiteAlloc: port-owned definitions in src/platform/rw_p2_stalac.c on the retail words gp-17976/-17972
003cf070  port  func_003cf070 (PS2AllPipelineNodeInit)
003ca5a0  port  func_003ca5a0 (module 0x509 engine-open callback registered by func_003cb900)
003b8e50  port  func_003b8e50 (spline plugin: basis tables)
003b90b0  port  func_003b90b0 (spline plugin: natural-spline matrix)
003bb0d0  port  func_003bb0d0 (spline plugin module 0x102 open)
00398350  port  func_00398350 (module 0x120 open callback)
00396750  port  func_00396750 (HAnim module 0x11E open callback)
003a8500  port  func_003a8500 (module 0x12F record set-up, PS2All pipeline)
003a7160  port  func_003a7160 (module 0x12F record set-up, PS2All pipeline)
003b6e70  port  func_003b6e70 (RpRandom module 0x10C open callback)
003a7f80  port  func_003a7f80 (module 0x12F first-record pipeline)
003a6ba0  port  func_003a6ba0 (module 0x12F second-record pipeline)
003b6f00  port  RpRandomSeed (func_003b6f00)
003b7060  port  RpRandom (func_003b7060)
003954b0  port  func_003954b0 (code1_0039, quaternion to matrix, 240 bytes)
0039ab20  port  func_0039ab20 (code1_0039, 256 bytes)
003a2950  port  func_003a2950 (code1_003a, 152 bytes)
003ddf80  port  RwFread (func_003ddf80, code1_003d, 80 bytes)
003ddfd0  port  RwFwrite (func_003ddfd0, code1_003d, 80 bytes)
003de020  port  RwFseek (func_003de020, code1_003d, 224 bytes)
003dd760  port  func_003dd760 (code1_003d, 112 bytes)
003a4d50  port  func_003a4d50 (code1_003a, SPR-DMA block copy, direct memcpy)
003a4850  port  func_003a4850 (code1_003a, SPR-DMA 16-byte pattern fill)
003a4ab0  port  func_003a4ab0 (code1_003a, SPR-DMA 32-byte pattern fill)
003a4270  port  func_003a4270 (code1_003a, SPR-DMA word fill)
003b31a0  port  rpPTankSkyInsColorPV (code1_003b, 156 bytes)
003b3240  port  rpPTankSkyInsColorPVC (code1_003b, 816 bytes)
003b3570  port  rpPTankSkyInsColorPPC (code1_003b, 784 bytes)
003b3f50  port  rpPTankSkyInsUV4 (code1_003b, 204 bytes)
003b4020  port  rpPTankSkyInsUV2C (code1_003b, 256 bytes)
003b4120  port  func_003b4120 (code1_003b, 264 bytes)
003b4db0  port  func_003b4db0 (code1_003b, 568 bytes)
003b5160  port  rpPTankGENInsPosNCPSPR (func_003b5160, code1_003b, 580 bytes)
003b53b0  port  func_003b53b0 (code1_003b, 660 bytes)
003b5820  port  func_003b5820 (code1_003b, 672 bytes)
003b5d20  port  func_003b5d20 (code1_003b, 644 bytes)
0044b7b0  port  sinf (func_0044b7b0): retail float kernel in src/platform/ee_libm.c
0044b610  port  cosf (func_0044b610): retail float kernel in src/platform/ee_libm.c
004f2798  port  ROFS_SetDir (rofs_if.c, no per-volume list on the host VFS): src/platform/files/sce_cdvd.c
00396890  port  HAnimDestructor (code1_0039, 164 bytes)
00396fb0  port  func_00396fb0 (code1_0039, HAnim hierarchy record create, 368 bytes)
00396680  port  func_00396680 (code1_0039, 208 bytes)
00397390  port  func_00397390 (code1_0039, 196 bytes)
00396940  port  func_00396940 (code1_0039, 252 bytes)
00396a40  port  func_00396a40 (code1_0039, 444 bytes)
0039b2c0  port  func_0039b2c0 (code1_0039, 192 bytes)
00394fd0  port  func_00394fd0 (code1_0039, 304 bytes)
00395100  port  func_00395100 (code1_0039, 400 bytes)
0039ae90  port  func_0039ae90 (code1_0039, 176 bytes)
00394c70  port  func_00394c70 (code1_0039, 256 bytes)
0039ac80  port  func_0039ac80 (code1_0039, 432 bytes)
00396c00  port  func_00396c00 (code1_0039, HAnim link stream read, 640 bytes)
003971d0  port  func_003971d0 (code1_0039, 448 bytes)
0039af40  port  func_0039af40 (code1_0039, 320 bytes)
0039b080  port  func_0039b080 (code1_0039, 400 bytes)
003b42e0  port  _rpPTankGENInstance (func_003b42e0, code1_003b, 400 bytes)
003a9c70  port  _rpPTankSkyClonePipeAllocCallBack (func_003a9c70, code1_003a, 480 bytes)
003bb330  port  func_003bb330 (code1_003b, 112 bytes)
003bb3a0  port  func_003bb3a0 (code1_003b, 256 bytes)
003bb210  port  func_003bb210 (code1_003b, 288 bytes)
003bbbe0  port  func_003bbbe0 (code1_003b, 576 bytes)
003b71b0  port  func_003b71b0 (code1_003b, 224 bytes)
003b7290  port  func_003b7290 (code1_003b, 448 bytes)
004c85a0  port  PC ADXF whole-file VFS open, sce_adxf.c (taken from port with approval)
004c8a60  port  retail ADXF close wrapper, sce_adxf.c
004c9670  port  retail ADXF byte-size wrapper, sce_adxf.c
004c95f8  port  retail ADXF sector-size wrapper, sce_adxf.c
004c9010  port  PC ADXF read with next-scheduler-frame completion, sce_adxf.c
004c9820  port  pure ADXF status wrapper, sce_adxf.c
00455230  port  H_Cdvd_SelectDirectory: remembers the normalized directory for the ADXF open instead of installing a ROFS dir list (overrides/00455230.c)
00401b80  port  func_00401b80 (RpSkyGetDisplayBufferRaster) (basky.c excluded): returns the framebuffer-backed display raster (src/render/p4gl_raster.c); no retail draw-raster getter exists (gp-18172 is read only by sky internals)
00442de8  port  strncpy: explicit EE-u32 callback count over reviewed bounded copy/zero-padding semantics in src/platform/ee_bytes.c
003ce170  port  func_003ce170 (code1_003c, DMA chain pointer->index, 136 bytes)
003ce230  port  func_003ce230 (code1_003c, DMA chain index->pointer, 116 bytes)
003d3ae0  port  func_003d3ae0 (code1_003d, box vs 6 planes, 264 bytes)
003d0140  port  func_003d0140 (code1_003d, 224 bytes)
003a3500  port  func_003a3500 (code1_003a, 368 bytes)
003c65b0  port  func_003c65b0 (code1_003c, 104 bytes)
003dd990  port  func_003dd990 (code1_003d, 192 bytes)
003d5130  port  func_003d5130 (code1_003d, 144 bytes)
003ddca0  port  func_003ddca0 (code1_003d, 380 bytes)
003d8760  port  func_003d8760 (code1_003d, raster row converter, 1172 bytes)
003c4a80  port  func_003c4a80 (code1_003c, 308 bytes)
003c2a80  port  func_003c2a80 (code1_003c, 236 bytes)
003c9300  port  func_003c9300 (code1_003c, 556 bytes)
003caa50  port  func_003caa50 (code1_003c, 468 bytes)
003c3ae0  port  func_003c3ae0 (code1_003c, 472 bytes)
003d53c0  port  func_003d53c0 (code1_003d, animation object stream read, 576 bytes)
003d5330  port  func_003d5330 (code1_003d, 136 bytes)
003d5600  port  func_003d5600 (code1_003d, animation object stream write, 268 bytes)
003be9f0  port  func_003be9f0 (AtomicDefaultRenderCallBack): draws the atomic in GL instead of executing its RxPipeline (overrides/003be9f0.c)
004eeb20  port  SJUNI_Init, src/cri/sj_uni.c
004eeba0  port  SJUNI_Finish, src/cri/sj_uni.c
004eec20  port  SJUNI_Create, src/cri/sj_uni.c
004ed3e8  port  SJ init level up, src/cri/sj_uni.c
004ed420  port  SJ init level down, src/cri/sj_uni.c
004eed60  port  SJUNI joint body, src/cri/sj_uni.c
004eee18  port  SJUNI joint body, src/cri/sj_uni.c
004eeea8  port  SJUNI joint body, src/cri/sj_uni.c
004eef58  port  SJUNI joint body, src/cri/sj_uni.c
004ef060  port  SJUNI joint body, src/cri/sj_uni.c
004ef160  port  SJUNI joint body, src/cri/sj_uni.c
004ef310  port  SJUNI joint body, src/cri/sj_uni.c
004ef488  port  SJUNI joint body, src/cri/sj_uni.c
004ef5f8  port  SJUNI joint body, src/cri/sj_uni.c
00416bc0  port  func_00416bc0 (RtAnim type 2 stream read): overrides/00416bc0.c
00416cf0  port  func_00416cf0 (RtAnim type 2 stream write): overrides/00416cf0.c
00416df0  port  func_00416df0 (RtAnim type 2 stream size): overrides/00416df0.c
00415ad0  port  func_00415ad0 (RtAnimKeyFrameApply, uncompressed): quaternion to matrix
004165a0  port  func_004165a0 (RtAnimKeyFrameBlend, uncompressed): slerp
00415bc0  port  func_00415bc0 (RtAnimKeyFrameInterpolate, compressed): half-float slerp
00417310  port  func_00417310 (RtAnimKeyFrameAdd, uncompressed): quaternion product
00416e10  port  func_00416e10 (RtAnimKeyFrameMulRecip, compressed): in-place multiply by reciprocal
00390290  port  func_00390290 (code1_0039, 52 bytes)
003a2340  port  func_003a2340 (code1_003a, 68 bytes)
003d7ac0  port  func_003d7ac0 (rpuvanim, 100 bytes)
003d7c50  port  func_003d7c50 (code1_003d, 120 bytes)
003d60f0  port  func_003d60f0 (rtanim, 124 bytes)
0039f0e0  port  func_0039f0e0 (code1_0039, 128 bytes)
003c4220  port  func_003c4220 (code1_003c, 136 bytes)
003c9640  port  func_003c9640 (code1_003c, 136 bytes)
003bcbe0  port  func_003bcbe0 (code1_003b, 152 bytes)
003c49a0  port  func_003c49a0 (code1_003c, 152 bytes)
003d7a20  port  func_003d7a20 (rpuvanim, 160 bytes)
003cdfa0  port  func_003cdfa0 (code1_003c, 172 bytes)
003ce2e0  port  func_003ce2e0 (code1_003c, 180 bytes)
003bce50  port  func_003bce50 (code1_003b, 192 bytes)
003962e0  port  func_003962e0 (code1_0039, 200 bytes)
003961f0  port  func_003961f0 (code1_0039, 232 bytes)
003bcc80  port  func_003bcc80 (code1_003b, 208 bytes)
003c8dd0  port  func_003c8dd0 (code1_003c, 224 bytes)
003dd290  port  func_003dd290 (code1_003d, 252 bytes)
003cac30  port  func_003cac30 (code1_003c, 264 bytes)
003c0520  port  func_003c0520 (code1_003c, 276 bytes)
003db550  port  func_003db550 (code1_003d, 212 bytes)
003c0700  port  func_003c0700 (code1_003c, 324 bytes)
003ce050  port  _rpObjectGetMeshCacheResize (func_003ce050, code1_003c, 284 bytes)
003b8d40  port  _rpSkinPipelinesAttach (func_003b8d40, code1_003b, 264 bytes)
003d81d0  port  func_003d81d0 (code1_003d, BMP header writer, 292 bytes)
003dddf0  port  func_003dddf0 (code1_003d, file open by mode string, 300 bytes)
003dcb40  port  RtQuatTransformVectors (func_003dcb40, code1_003d, 300 bytes)
003dc610  port  RtQuatConvertFromMatrix (func_003dc610, code1_003d, 292 bytes)
003bfae0  port  func_003bfae0 (code1_003b, bounding sphere update, 352 bytes)
003c1d40  port  func_003c1d40 (code1_003c, 344 bytes)
00398410  port  func_00398410 (code1_0039, 304 bytes)
003a2bb0  port  func_003a2bb0 (code1_003a, 304 bytes)
003bc880  port  func_003bc880 (code1_003b, 312 bytes)
003c9d40  port  func_003c9d40 (code1_003c, 320 bytes)
003c0f20  port  func_003c0f20 (code1_003c, clump stream read, 2948 bytes)
0051fdc0  port  viBufCreate (Sofdec IPU ring), native in src/movie/sfmpvp.c; 0051fe38 viBufReset folded in
0051ff78  port  viBufBeginPut, native in src/movie/sfmpvp.c
00520068  port  viBufEndPut, native in src/movie/sfmpvp.c
005200b8  port  viBufUnput, native in src/movie/sfmpvp.c
00520138  port  viBufAddDMA (the IPU DMA is a model: decoder consumption plus an 8-quadword FIFO), native in src/movie/sfmpvp.c
00520338  port  viBufStopDMA, native in src/movie/sfmpvp.c
00520438  port  viBufRestartDMA, native in src/movie/sfmpvp.c
00520750  port  viBufDelete, native in src/movie/sfmpvp.c
005207a8  port  viBufFlush, native in src/movie/sfmpvp.c
00520808  port  SFMPVP_IsSceMpegUsed, native in src/movie/sfmpvp.c
00520818  port  func_00520818 (SFMPVP init), native in src/movie/sfmpvp.c
00520860  port  SFMPVP_GetViBufInf, native in src/movie/sfmpvp.c
005208c8  port  func_005208c8 (SFMPVP_Create), native in src/movie/sfmpvp.c
00520ae0  port  SFD_GetSceErrMessage, native in src/movie/sfmpvp.c
00520b58  port  sfmpvp_PutVibuf, native in src/movie/sfmpvp.c
00520cf0  port  SFMPVP_Destroy, native in src/movie/sfmpvp.c
00520dc8  port  SFMPVP_Put, native in src/movie/sfmpvp.c
00520e38  port  SFMPVP_Unput, native in src/movie/sfmpvp.c
00520e98  port  SFMPVP_Flush, native in src/movie/sfmpvp.c
00520ef0  port  copy2area, native in src/movie/sfmpvp.c
00521020  port  SFMPVP_DecodeFrmSj: libmpeg's sceMpegGetPicture/RAW8 replaced by libavcodec (src/movie/sfmpvp.c)
00521168  port  SFMPVP_GetFrameCount, native in src/movie/sfmpvp.c
00521178  port  SFMPVP_SearchPsc, native in src/movie/sfmpvp.c
00521390  port  sfmpvp_ConsumePsc, native in src/movie/sfmpvp.c
005213a8  port  func_005213a8 (three pictures buffered or flushed), native in src/movie/sfmpvp.c
0041e4d8  port  libmpeg sceMpegCreate and the rest of libmpeg (0x0041b9d0..0x00421088, sceMpeg*, _sequence*, _picture*): not called once SFMPVP is native, leave untranslated
00143ba0  port  gsUploadRasterTiles: movie frame written into the raster shadow (overrides/00143ba0.c, src/movie/movie_upload.c)
003bee80  port  func_003bee80 (code1_003b, material block read, 796 bytes)
003a20e0  port  func_003a20e0 (rpPTankValidateFlag, code1_003a, normalizes PTank data flags)
003a2390  port  func_003a2390 (_rpPTankAtomicCreateCustom, code1_003a, builds the PTank atomic and record)
003a3670  port  func_003a3670 (rpPTankSkyAllocCallBack, port layout of clusters and user data, no DMA chains; overrides/003a3670.c)
003a3e90  port  func_003a3e90 (rpPTankSkyCreateCallBack, geometry holder shape only, no instancer selected: the port draws from the render callback; overrides/003a3e90.c)
003c4140  port  func_003c4140 (RpMaterialCreate, port-owned src/platform/rw_plugin_constructors.c; its body is only in the unbuilt code1_003c_cw119.c): forwards to the named RpMaterialCreate
00509268  port  func_00509268 (mwPlyCreateSofdec): native player, src/movie/mwply.c (the movie player is the port's own at the MWPLY API; Sofdec's internals are not run for movies)
0050d4d0  port  func_0050d4d0 (mwPlyGetStat), native in src/movie/mwply.c
005097e8  port  func_005097e8 (mwPlyDestroy), native in src/movie/mwply.c
00508310  port  mwPlyCalcWorkCprmSfd: forwarded to src/movie/mwply.c by config/mirror_fixes.txt
0050a008  port  mwPlySetFrmSync: forwarded by config/mirror_fixes.txt
0050a1c0  port  mwPlyGetCurFrm: forwarded by config/mirror_fixes.txt
0050aae0  port  mwPlyRelCurFrm: forwarded by config/mirror_fixes.txt
0050ffc8  port  mwPlyReleaseLp: forwarded by config/mirror_fixes.txt
0050b3f8  port  mwPlyInitSfdFx: forwarded by config/mirror_fixes.txt (no Sofdec library state)
004561a0  port  H_Cdvd_StartPlayback: opens the movie in src/movie/mwply.c (overrides/004561a0.c)
003963c0  port  func_003963c0 (code1_0039, keyframe subtract, 352 bytes)
00396520  port  func_00396520 (code1_0039, keyframe combine, 340 bytes)
00390350  port  func_00390350 (code1_0039, 432 bytes)
003990a0  port  func_003990a0 (code1_0039, 368 bytes)
00399bf0  port  func_00399bf0 (code1_0039, 376 bytes)
00394d70  port  func_00394d70 (code1_0039, 256 bytes)
00394e70  port  func_00394e70 (code1_0039, 352 bytes)
00395290  port  func_00395290 (code1_0039, 532 bytes)
00398890  port  func_00398890 (code1_0039, 224 bytes)
00399470  port  func_00399470 (code1_0039, 176 bytes)
0039beb0  port  func_0039beb0 (code1_0039, 336 bytes)
00399db0  port  func_00399db0 (code1_0039, 536 bytes)
003985b0  port  func_003985b0 (code1_0039, 720 bytes)
0039c000  port  func_0039c000 (code1_0039, vertex gather, 864 bytes)
003a29f0  port  func_003a29f0 (code1_003a, 224 bytes)
0039c360  port  func_0039c360 (code1_0039, skin matrix palette, 960 bytes)
00398970  port  func_00398970 (code1_0039, collision record writer, 944 bytes)
00398d20  port  func_00398d20 (code1_0039, collision record reader, 896 bytes)
00392a50  port  func_00392a50 (code1_0039, triangle probe walk, 416 bytes)
003955a0  port  func_003955a0 (code1_0039, keyframe slerp by time, 1584 bytes)
00395bd0  port  func_00395bd0 (code1_0039, keyframe slerp by weight, 1568 bytes)
003d5840  port  func_003d5840 (rtanim.c, RtAnimInterpolatorSetCurrentAnim)
003d59d0  port  func_003d59d0 (rtanim.c, RtAnimInterpolatorSubAnimTime)
003d5bc0  port  func_003d5bc0 (rtanim.c, RtAnimInterpolatorAddAnimTime)
003d6230  port  func_003d6230 (rtanim.c, called by mdlReleaseBlendTrack: release a track object)
003ed850  port  func_003ed850 (basync.c): frame hierarchy world-matrix sync
003f25c0  port  func_003f25c0 (palquant.c): free a colour-trie subtree
003d6170  port  func_003d6170 (rtanim.c): find a track entry by name
003d5790  port  func_003d5790 (RtAnimInterpolatorCreate): port-owned in src/platform/rw_rtanim.c (the decomp holds it only in the excluded code1_003d_cw119.c)
00397c40  port  func_00397c40 (code1_0039, RpHAnimHierarchyUpdateMatrices, 1696 bytes)
003dc490  port  func_003dc490 (code1_003d, 128 bytes)
003dc510  port  func_003dc510 (code1_003d, 128 bytes)
003dc590  port  func_003dc590 (code1_003d, 128 bytes)
003dd390  port  func_003dd390 (code1_003d, 160 bytes)
003cc500  port  func_003cc500 (code1_003c, 84 bytes)
003d5750  port  func_003d5750 (code1_003d, 64 bytes)
003d2010  port  func_003d2010 (code1_003d, 192 bytes)
003d8070  port  func_003d8070 (code1_003d, 192 bytes)
003bbea0  port  func_003bbea0 (code1_003b, 224 bytes)
003db360  port  func_003db360 (code1_003d, 224 bytes)
003c66c0  port  func_003c66c0 (code1_003c, 240 bytes)
003ca740  port  func_003ca740 + func_003ca780 + func_003ca7a0 (code1_003c, three thunks, 240 bytes)
003dd430  port  func_003dd430 (code1_003d, 256 bytes)
003c55f0  port  func_003c55f0 (code1_003c, 272 bytes)
003d7b40  port  func_003d7b40 (code1_003d, 272 bytes)
003bc620  port  func_003bc620 (code1_003b, 288 bytes)
003dc370  port  func_003dc370 (code1_003d, 288 bytes)
003bc740  port  func_003bc740 (code1_003b, 320 bytes)
003d51c0  port  func_003d51c0 (code1_003d, 320 bytes)
003cfff0  port  func_003cfff0 (code1_003c, 336 bytes)
003deea0  port  func_003deea0 (code1_003d, 336 bytes)
003bebb0  port  func_003bebb0 (code1_003b, 352 bytes)
003d3780  port  func_003d3780 (code1_003d, 352 bytes)
003de110  port  func_003de110 (code1_003d, 352 bytes)
003a9f40  port  func_003a9f40 (code1_003a, 368 bytes)
003bc150  port  func_003bc150 (code1_003b, name list reader, 688 bytes)
003d72a0  port  func_003d72a0 (rpuvanim.c, animation slot release, 160 bytes)
00418f50  port  func_00418f50 (sceDmaGetChan): channel register block from the 0x0070c410 table, native in src/platform/ps2_dma.c
00418df8  port  func_00418df8 (sceDmaSync wait loop): channels are always idle, native in src/platform/ps2_dma.c
00449680  port  __ieee754_logf (func_00449680): literal retail core in src/platform/ee_libm.c
00431b58  port  func_00431b58 (libmc2 op 3, format): empties the host card folder; src/platform/memcard/ps2_memcard.c
003fd940  port  func_003fd940 (readTEXFromStream, sky native texture raster read): port-owned in src/render/p4gl_native.c, undoes the PSMT8/PSMT4 pre-swizzle and the CLUT
003f6910  port  func_003f6910 (skyReadTexCB, texture plugin 0x110 read): port-owned in src/render/p4gl_native.c
003f6990  port  func_003f6990 (skyWriteTexCB, texture plugin 0x110 write): port-owned in src/render/p4gl_native.c
003dea20  port  func_003dea20 (sdkCdvd device create; callbacks by symbol)
003de280  port  func_003de280 (sdkCdvd device slot method): src/platform/cdvd_device.c
003de2d0  port  func_003de2d0 (sdkCdvd device records free method)
003b8930  port  func_003b8930 (code1_003b, user-data plugin chunk write)
003bbf80  port  func_003bbf80 (code1_003b, user-data array copy)
003bc400  port  func_003bc400 (code1_003b, user-data array stream write)
003d8300  port  func_003d8300 (code1_003d, palettised image chunk header and palette write)
003dcc70  port  func_003dcc70 (code1_003d, slerp setup: angle and scaled quaternions)
003ce560  port  func_003ce560 (code1_003c, _rpGeometryNativeRead, 732 bytes)
003ce3a0  port  func_003ce3a0 (code1_003c, _rpGeometryNativeWrite, 448 bytes)
003ce9e0  port  func_003ce9e0 (code1_003c, _rpWorldSectorNativeWrite, 456 bytes)
003ceeb0  port  func_003ceeb0 (code1_003c, _rpWorldSectorNativeSize, 436 bytes)
003dda50  port  func_003dda50 (code1_003d, 440 bytes)
003a6df0  port  func_003a6df0 (code1_003a, 440 bytes)
003d20d0  port  func_003d20d0 (code1_003d, 368 bytes)
0038fb50  port  func_0038fb50 (code1_0038_cw119, 468 bytes)
003d7490  port  func_003d7490 (rpuvanim.c, 288 bytes)
003d7350  port  func_003d7350 (rpuvanim.c, 304 bytes)
003d75b0  port  func_003d75b0 (rpuvanim.c, 288 bytes)
003d76d0  port  func_003d76d0 (rpuvanim.c, 192 bytes)
003d65e0  port  func_003d65e0 (rpuvanim.c, 160 bytes)
003d6fb0  port  func_003d6fb0 (rpuvanim.c, 272 bytes)
003d77f0  port  func_003d77f0 (rpuvanim.c, 432 bytes)
003dd620  port  func_003dd620 (sdkCdvd device set-name): src/platform/cdvd_device.c
003c0960  port  func_003c0960 (code1_003c, extension chunk writer)
003d8c00  port  func_003d8c00 (code1_003d, BMP file reader)
003e9d50  port  func_003e9d50 (RwFrameRotate: the decomp built it as a 3-argument RwFrameTranslate, dropping the float angle on x86; overrides/003e9d50.c)
00424708  port  SifSetDma (libkernl): EE to IOP RAM copy, src/platform/sound/sdr_mem.c
00430e28  port  sceSdInit (libsd client): resets the sound driver state, src/platform/sound/ps2_sndcom.c
00430f80  port  sceSdRemote (libsd client): 0x8010 accepted, 0x90f0 goes through the wrappers, src/platform/sound/ps2_sndcom.c
0043bb70  port  SPU heap init (sdkSnd driver), src/platform/sound/ps2_sndcom.c
0043bcb8  port  SPU heap alloc (sdkSnd driver), src/platform/sound/ps2_sndcom.c
0043c010  port  sdr client start (sdkSnd driver), src/platform/sound/ps2_sndcom.c
0043c0c0  port  sksound control command wrapper, src/platform/sound/ps2_sndcom.c
0043c180  port  sksound voice transfer wrapper, src/platform/sound/ps2_sndcom.c
0043c230  port  skhsynth create wrapper, src/platform/sound/ps2_sndcom.c
0043c308  port  skhsynth free wrapper, src/platform/sound/ps2_sndcom.c
0043c3b0  port  sksesq create wrapper, src/platform/sound/ps2_sndcom.c
0043c470  port  sksesq free wrapper, src/platform/sound/ps2_sndcom.c
0043c518  port  sksesq play/release wrapper, src/platform/sound/ps2_sndcom.c
0043c5e8  port  sksesq stop wrapper, src/platform/sound/ps2_sndcom.c
004ec788  port  AXRNA_* (the renderer under ADXRNA, replaces PS2RNA), src/platform/sound/axrna.c
00407bd0  port  func_00407bd0 (stdFunc 11 SETRASTERCONTEXT): port-owned in src/render/p4gl_image.c
00407c10  port  func_00407c10 (stdFunc 2 RGBTOPIXEL): port-owned in src/render/p4gl_image.c
00407d10  port  func_00407d10 (stdFunc 3 PIXELTORGB): port-owned in src/render/p4gl_image.c
00407ff0  port  func_00407ff0 (stdFunc 6 IMAGEGETRASTER): port-owned in src/render/p4gl_image.c
00407df0  port  _rwSkyImageFindRasterFormat (stdFunc 9): port-owned in src/render/p4gl_image.c
0040a500  port  _rwSkyRasterSetImage (stdFunc 7): port-owned in src/render/p4gl_image.c
003f67e0  port  RpSkySelectTrueTSClipper: port-owned in src/render/p4gl_image.c
003f67f0  port  RpSkySelectTrueTLClipper: port-owned in src/render/p4gl_image.c
00442948  port  strlen: explicit EE-u32 length result for valid bounded byte strings in src/platform/ee_bytes.c
003db630  port  func_003db630 (code1_003d, texture stream read, level images)
003dbe40  port  func_003dbe40 (code1_003d, texture stream read, native texture form)
003de740  port  func_003de740 (code1_003d, buffered stream seek)
003c8200  port  func_003c8200 (code1_003c, mesh strip header builder)
003b9a40  port  func_003b9a40 (code1_003b, cubic spline control-point fit, retail zero-size recurrence allocation; transferred from port)
003b85e0  port  func_003b85e0 (skin sky weight batch preparation; port-owned src/platform/rw_skin_callees.c)
003b88c0  port  func_003b88c0 (empty: jr ra; port-owned src/platform/rw_skin_callees.c)
003b8ad0  port  func_003b8ad0 (code1_003b, 592 bytes)
003bc9c0  port  func_003bc9c0 (code1_003b, 512 bytes)
003d0540  port  func_003d0540 (code1_003d, 592 bytes)
003c6080  port  func_003c6080 (code1_003c, triangle compare, 512 bytes)
003c67b0  port  func_003c67b0 (code1_003c, 432 bytes)
003c57d0  port  func_003c57d0 (code1_003c, 544 bytes)
003c1ea0  port  func_003c1ea0 (code1_003c, 656 bytes)
003d8500  port  func_003d8500 (code1_003d, 416 bytes)
003db190  port  func_003db190 (code1_003d, 464 bytes)
003de4d0  port  func_003de4d0 (code1_003d, 464 bytes)
003d3bd0  port  func_003d3bd0 (code1_003d, 652 bytes)
003dbbc0  port  func_003dbbc0 (code1_003d, 640 bytes)
003caef0  port  func_003caef0 (code1_003c, 752 bytes)
003d7cf0  port  func_003d7cf0 (code1_003d, 880 bytes)
003c8920  port  func_003c8920 (code1_003c, 720 bytes)
003dc740  port  func_003dc740 (code1_003d, quaternion rotate, 1024 bytes)
003c2630  port  func_003c2630 (code1_003c, 1072 bytes)
003c4390  port  func_003c4390 (code1_003c, 1072 bytes)
004f34b0  port  func_004f34b0 (ROFS_Init) and 004f5bf0 (ROFS_SetOption): host-VFS models in src/platform/files/sce_rofs.c
003c7d90  port  func_003c7d90 (code1_003c, 1136 bytes)
003cfa80  port  func_003cfa80 (code1_003c, 1312 bytes)
003bb5b0  port  func_003bb5b0 (code1_003b, 1248 bytes)
003be180  port  func_003be180 (code1_003b, 1600 bytes)
004c6eb0  port  ADXF device registration under the lock (CVFS AddDev ROFS), src/platform/files/adxf_fsvr.c
004c6ee0  port  ADXF CVFS AddDev ROFS body, adxf_fsvr.c
004c6f08  port  ADXF device removal under the lock, adxf_fsvr.c
004c6f30  port  ADXF CVFS DelDev ROFS body, adxf_fsvr.c
004c6f50  port  ADXF device attach under the lock, adxf_fsvr.c
004c6fa8  port  checks the CVM image and registers the named PC ROFS device, adxf_fsvr.c (coordinated with port)
004c7188  port  ADXF device detach under the lock, adxf_fsvr.c
004c71b8  port  ADXF device detach body, adxf_fsvr.c
004c7238  port  ADXF bytes to sectors, src/platform/files/adxf_partition.c
004c7260  port  ADXF command history ring, adxf_partition.c
004c7340  port  ADXF partition parameter check, adxf_partition.c
004c74f8  port  ADXF_LoadPartitionNw under the lock, adxf_partition.c
004c7560  port  ADXF_LoadPartitionNw body, adxf_partition.c
004c78c8  port  ADXF partition load by name, adxf_partition.c
004c7b80  port  ADXF partition load start, adxf_partition.c
004c7e28  port  ADXF partition load abort, adxf_partition.c
004c7ef8  port  ADXF_GetPtStat under the lock, adxf_partition.c
004c7f30  port  ADXF_GetPtStat unlocked, adxf_partition.c
004c7f80  port  ADXF_GetPtStat body (AFS table of contents parse), adxf_partition.c
004c8770  port  PC ADXF range-window open, adxf_partition.c
004c8950  port  PC ADXF member open, adxf_partition.c
004c8a90  port  PC ADXF close/cancel, adxf_partition.c
004c9068  port  PC ADXF buffer read, adxf_partition.c
004c9858  port  PC ADXF status read, adxf_partition.c
004c9888  port  ADXF partition and member check, adxf_partition.c
004c9a28  port  ADXF archive member lookup, adxf_partition.c
004c9bb8  port  ADXF partition name field, adxf_partition.c
004f8518  port  ROFS CVFS interface getter, src/platform/files/rofs_host.c
004f2408  port  ROFS server entry; host completion is published by its frame alarm, rofs_host.c
004f2460  port  ROFS file size by name, rofs_host.c
004f2548  port  ROFS file size through a pointer, rofs_host.c
004f2948  port  ROFS file exists, rofs_host.c
004f2260  port  guest-storable PC ROFS file-window open, rofs_host.c
004f2370  port  ROFS close cancels/unlinks before free, rofs_host.c
004f85c0  port  ROFS sector seek, rofs_host.c
004f1a08  port  PC ROFS sector position, rofs_host.c
004f2398  port  PC ROFS read with next-frame completion, rofs_host.c
004f23e0  port  ROFS stop cancels its deferred completion, rofs_host.c
004f8648  port  CVFS0/1/2/3 status over the PC read fence, rofs_host.c
004f1ce0  port  PC ROFS transferred-sector position, rofs_host.c
004f86c8  port  ROFS change directory, rofs_host.c
004f28b0  port  ROFS directory info, rofs_host.c
004f8848  port  ROFS option function, rofs_host.c
003d9760  port  func_003d9760 (code1_003d, sphere-triangle test, 2000 bytes)
00394870  port  func_00394870 (code1_0039, 1016 bytes)
00392650  port  func_00392650 (code1_0039, 1024 bytes)
003941e0  port  func_003941e0 (code1_0039, 1668 bytes)
003a1730  port  func_003a1730 (code1_003a, 1296 bytes)
003a1c40  port  func_003a1c40 (code1_003a, 1176 bytes)
00399530  port  func_00399530 (code1_0039, 1128 bytes)
003d59a0  port  func_003d59a0 (RtAnim record keyframe copy): port-owned in src/platform/rw_rtanim_record.c (the marker is only in the excluded code1_003d_cw119.c, so the override never applied; moved from overrides/)
003d5e90  port  func_003d5e90 (RtAnim record node blend): port-owned in src/platform/rw_rtanim_record.c (as 003d59a0)
00390680  port  func_00390680 (code1_0039, segment vs collision BSP, 3448 bytes)
00391400  port  func_00391400 (code1_0039, box vs collision BSP, 2052 bytes)
00391c10  port  func_00391c10 (code1_0039, sphere vs collision BSP, 2616 bytes)
00392bf0  port  func_00392bf0 (code1_0039, ray vs geometry via collision mesh, 3344 bytes)
00393900  port  func_00393900 (code1_0039, sphere vs geometry via collision mesh, 2264 bytes)
00397480  port  func_00397480 (code1_0039, animation hierarchy update, 1972 bytes)
003a93b0  port  func_003a93b0 (code1_003a, instance cull and batch, 1232 bytes)
004cc1e0  port  ADXM_IsSetupThrd, src/cri/cri_glue.c
004cbd28  port  ADXM_WaitVsync, src/cri/cri_glue.c
004bd6a0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd6b8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd720  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd748  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd750  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd758  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd760  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd790  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd850  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bd9f8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bda68  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004bdc20  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0718  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c07f0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0928  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0a40  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0ce0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0d60  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0e00  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0e08  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0e40  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0e50  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0e78  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0ee0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0f08  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0f88  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0fc0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0fd0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0fd8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0fe0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c0fe8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c1030  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c1248  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004c54e8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004ca8b0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb510  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb548  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb5d0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb640  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb728  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb750  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb778  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb790  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb880  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cb988  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cba78  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cbc88  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cbcd8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cbd40  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cbda8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cbe08  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cbf68  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cbff0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cc078  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cc1f0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cc200  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cc218  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004ccbf0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004ccc70  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cd1a0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004cd3f0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004d32e0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004d32f8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004d8830  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004de2e8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004de310  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004de340  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004de3f0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004dfc38  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e0380  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e0398  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e03b0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e0408  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e1200  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e1230  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e12e8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e7280  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004e72a0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004eb318  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004f0ec0  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004f0ed8  port  CRI sdkSndInit chain, src/cri/{acs,adxm,adx_init,svm_port,cri_glue}.c
004d5440  port  CRI alias to the re4 native (config/native_names.txt)
004e17a0  port  CRI alias to the re4 native (config/native_names.txt)
004e1ad8  port  CRI alias to the re4 native (config/native_names.txt)
004e2e28  port  CRI alias to the re4 native (config/native_names.txt)
004ca720  port  CRI alias to the re4 native (config/native_names.txt)
004f0c78  port  CRI alias to the re4 native (config/native_names.txt)
004f0de8  port  CRI alias to the re4 native (config/native_names.txt)
004c5648  port  ADX_ScanInfoCode, port-owned in src/cri/adx_hdr.c (ADX header readers, translated from the asm)
004c5770  port  ADX_DecodeInfo, src/cri/adx_hdr.c
004c5870  port  ADX_DecodeInfoExADPCM2, src/cri/adx_hdr.c
004c58f0  port  ADX_DecodeInfoExVer, src/cri/adx_hdr.c
004c5968  port  ADX_DecodeInfoExIdly, src/cri/adx_hdr.c
004c5aa0  port  ADX_DecodeInfoExLoopEncTime, src/cri/adx_hdr.c
004c5b68  port  ADX_DecodeInfoExLoop, src/cri/adx_hdr.c
004c5dc8  port  ADX_DecodeInfoAinf, src/cri/adx_hdr.c; port retail alias in config/native_names.txt
004c5fe8  port  ADX_DecodeFooter, src/cri/adx_hdr.c
004c6048  port  ADX_CalcHdrInfoLen, src/cri/adx_hdr.c
003d9f30  port  func_003d9f30 (code1_003d, 4692 bytes)
003c70e0  port  func_003c70e0 (code1_003c, 3248 bytes)
003bd6b0  port  func_003bd6b0 (code1_003b, 1616 bytes)
003d0230  port  func_003d0230 (code1_003d, 560 bytes)
0043a978  port  func_0043a978 (libmc2 IOP module load and RPC bind): no IOP, the card is the host folder; no-op so memcardInitBackend can run
004be4a8  port  ACSSND object setup (src/cri/acssnd.c)
004bfba0  port  ACSSND object reset (src/cri/acssnd.c)
004d3850  port  ADXT_Create locked wrapper (src/cri/adxt_wrap.c)
004d3cd8  port  ADXT_Destroy locked wrapper (src/cri/adxt_wrap.c)
004d38a8  port  adxt_Create inner create: PS2 work layout (0x2060-sample output rings, file ring of the rest), retail translation (src/cri/adxt_wrap.c)
004d7320  port  adxt_trap_entry_lps loop-start trap (src/cri/adxt_svr.c)
004d73d8  port  adxt_trap_entry loop-end trap (src/cri/adxt_svr.c)
004d7570  port  adxt_eos_entry stream end-of-sector callback with the +0xc4 stream window (src/cri/adxt_svr.c)
004d7618  port  ADXT output pans from the stream's channel count (src/cri/adxt_svr.c)
004d7680  port  adxt_nlp_trap_entry linked-file end trap (src/cri/adxt_svr.c)
004d7968  port  adxt_stat_decinfo: flush_nsmpl clear, decoder error state 6, ADXT_SetOutVol (src/cri/adxt_svr.c)
004d7d90  port  ADXT PREP step: start at min(2 * maxdecsmpl, 0x4000) (src/cri/adxt_svr.c)
004d7f18  port  ADXT PLAYING step: transfer stays on into PLAYEND_WAIT (src/cri/adxt_svr.c)
004d8000  port  ADXT PLAYEND_WAIT step: silence padding counted in flush_nsmpl (src/cri/adxt_svr.c)
004d8208  port  ADXT PLAYEND step (empty) (src/cri/adxt_svr.c)
004d8550  port  ADXT_ExecRdErrChk read error check (src/cri/adxt_svr.c)
004d85d0  port  ADXT decoder supply termination by input kind (src/cri/adxt_svr.c)
004d8698  port  adxt_ExecHndl per-state dispatch, native ADXT_ExecHndl (src/cri/adxt_svr.c; replaces re4/adx_tsvr.c)
004bd890  port  ACSFDR fader tick (src/cri/acs_step.c)
004c1038  port  ACSVHL highlight state machine (src/cri/acs_step.c)
004bf960  port  ACSSND per-frame step (src/cri/acs_step.c)
004cb2d0  port  ADXM server pass by mode (src/cri/adxm.c)
004cb360  port  ADXM vsync server pass (src/cri/adxm.c)
004cb3f8  port  ADXM all-types server pass (src/cri/adxm.c)
004ccb68  port  ADXM thread wake-ups (src/cri/adxm.c)
004ccc98  port  ADXM vsync server entry (src/cri/adxm.c)
004cca10  port  ADXM vsync interrupt body (src/cri/adxm.c)
004cd170  port  ADXM vsync callback caller (src/cri/adxm.c)
004e5a08  port  dvCiGetInterface (src/cri/adx_init.c)
004e9e50  port  mfCiGetInterface (src/cri/adx_init.c)
004cd470  port  forward to 0x004eb318, the PS2RNA voice configuration (src/cri/adx_init.c)
004bdaf0  port  ACSFDR level inverse (src/cri/acs.c)
004bdd48  port  ACSFDR fade start hook (src/cri/acs.c)
004bdd78  port  ACSFDR fade end hook (src/cri/acs.c)
004e5b10  port  dvCi object server step (src/cri/adx_init.c)
004e5bd8  port  dvCi server (src/cri/adx_init.c)
004e9fb0  port  mfCi server (src/cri/adx_init.c)
004d20f0  port  ADXSTM start inner (src/cri/adxstm_api.c)
004d2128  port  ADXSTM start wrapper (src/cri/adxstm_api.c)
004d2160  port  ADXSTM start inner (src/cri/adxstm_api.c)
004d2b80  port  ADXSTM chunk limits (src/cri/adxstm_api.c)
004d2bd8  port  ADXSTM chunk limits inner (src/cri/adxstm_api.c)
004d2c98  port  ADXSTM sectors per request (src/cri/adxstm_api.c)
004d2ce0  port  ADXSTM sectors per request inner (src/cri/adxstm_api.c)
004d2cf8  port  ADXSTM file size (src/cri/adxstm_api.c)
004d2d30  port  ADXSTM file size inner (src/cri/adxstm_api.c)
004d2d78  port  CVFS name query wrapper (src/cri/adxstm_api.c)
004d2db0  port  CVFS name query forward (src/cri/adxstm_api.c)
004d31e8  port  ADXT input kind wrapper (src/cri/adxstm_api.c)
004c6a70  port  ADX error callback entry (src/cri/adxstm_api.c)
004d1ce8  port  CRI alias to the re4 native (config/native_names.txt)
004d2038  port  CRI alias to the re4 native (config/native_names.txt)
004d20a0  port  CRI alias to the re4 native (config/native_names.txt)
004d23a8  port  CRI alias to the re4 native (config/native_names.txt)
004d2c30  port  CRI alias to the re4 native (config/native_names.txt)
004c8b48  port  CRI alias to the re4 native (config/native_names.txt)
004c9d48  port  CRI alias to the re4 native (config/native_names.txt)
004e1a00  port  cvFsDelDev (src/cri/cvfs_port.c)
004e2240  port  CRI alias to the re4 native (config/native_names.txt)
004e2690  port  CRI alias to the re4 native (config/native_names.txt)
004e2750  port  CRI alias to the re4 native (config/native_names.txt)
004e2360  port  CRI alias to the re4 native (config/native_names.txt)
004bdfa0  port  ACSSBADX play state (src/cri/acssnd.c)
004be318  port  ACSSBAIX track volumes (src/cri/acssnd.c)
004bebf0  port  ACSSND_Stop (src/cri/acssnd.c)
004beec8  port  ACSSND_SetOutVol (src/cri/acssnd.c)
004befd0  port  ACSSND_SetTrackVol (src/cri/acssnd.c)
004bf0e8  port  ACSSND track volume getter (src/cri/acssnd.c)
004bf138  port  ACSSND_SetLpFlg (src/cri/acssnd.c)
004bf240  port  ACSSND_AttachAhx (src/cri/acssnd.c)
004bf350  port  ACSSND_DetachAhx (src/cri/acssnd.c)
004bf438  port  ACSSND_GetActiveAdxt (src/cri/acssnd.c)
004bf498  port  ACSSND_GetNextAdxt (src/cri/acssnd.c)
004bf510  port  ACSSND_GetAdxt (src/cri/acssnd.c)
004bf590  port  ACSSND_GetNumAdxt (src/cri/acssnd.c)
004bf5e0  port  ACSSND_GetActiveAixp (src/cri/acssnd.c)
004bf638  port  ACSSND_GetNextAixp (src/cri/acssnd.c)
004bf6a8  port  ACSSND_GetAixp (src/cri/acssnd.c)
004bf718  port  ACSSND_GetNumAixp (src/cri/acssnd.c)
004bf758  port  ACSSND_CancelFading (src/cri/acssnd.c)
004bfb28  port  ACSSND fade curve (src/cri/acssnd.c)
004bfca0  port  ACSSND_Start (src/cri/acssnd.c)
004bff98  port  ACSSND start parameters (src/cri/acssnd.c)
004c0068  port  ACSSND slot fade in (src/cri/acssnd.c)
004c0118  port  ACSSND slot fade out (src/cri/acssnd.c)
004c01c8  port  ACSSND slot follow-up (src/cri/acssnd.c)
004c0308  port  ACSSND next buffer (src/cri/acssnd.c)
004c03b8  port  ACSSND previous buffer (src/cri/acssnd.c)
004c0450  port  ACSSND frame tail (src/cri/acssnd.c)
004c0518  port  ACSSND level push (src/cri/acssnd.c)
004c05f0  port  ACSSND all idle (src/cri/acssnd.c)
004c0658  port  ACSSND fade hook (src/cri/acssnd.c)
004c12a0  port  ACSVHL state set (src/cri/acssnd.c)
004c12a8  port  ACSVHL fade length (src/cri/acssnd.c)
004c12f8  port  ACSVHL fade length (src/cri/acssnd.c)
004e3a10  port  func_004e3a10 (src/cri/ci_dv.c, 208 bytes)
004e3c78  port  func_004e3c78 (src/cri/ci_dv.c, 272 bytes)
004e4698  port  func_004e4698 (src/cri/ci_dv.c, 88 bytes)
004e4770  port  func_004e4770 (src/cri/ci_dv.c, 136 bytes)
004e47f8  port  func_004e47f8 (src/cri/ci_dv.c, 184 bytes)
004e48b0  port  func_004e48b0 (src/cri/ci_dv.c, 120 bytes)
004e49e8  port  func_004e49e8 (src/cri/ci_dv.c, 120 bytes)
004e58a0  port  func_004e58a0 (src/cri/ci_dv.c, 264 bytes)
004e5ce0  port  func_004e5ce0 (src/cri/ci_dv.c, 136 bytes)
004e5d68  port  dvCiGetFileSize (src/cri/ci_dv.c, 192 bytes)
004e5e80  port  dvCiOpen (src/cri/ci_dv.c, 376 bytes)
004e5ff8  port  dvCiClose (src/cri/ci_dv.c, 88 bytes)
004e6050  port  func_004e6050 (src/cri/ci_dv.c, 152 bytes)
004e60e8  port  func_004e60e8 (src/cri/ci_dv.c, 56 bytes)
004e6120  port  dvCiReqRd (src/cri/ci_dv.c, 424 bytes)
004e62c8  port  func_004e62c8 (src/cri/ci_dv.c, 232 bytes)
004e63b0  port  func_004e63b0 (src/cri/ci_dv.c, 56 bytes)
004e63f0  port  func_004e63f0 (src/cri/ci_dv.c, 80 bytes)
004e6480  port  func_004e6480 (src/cri/ci_dv.c, 16 bytes)
004e6578  port  func_004e6578 (src/cri/ci_dv.c, 200 bytes)
004e7258  port  dvCiIsExistFcache (src/cri/ci_dv.c, 40 bytes)
004e7668  port  func_004e7668 (src/cri/ci_ht.c, 120 bytes)
004e76f8  port  func_004e76f8 (src/cri/ci_ht.c, 160 bytes)
004e77c0  port  htCiGetFileSize (src/cri/ci_ht.c, 440 bytes)
004e7a58  port  htCiOpen (src/cri/ci_ht.c, 376 bytes)
004e7bd0  port  func_004e7bd0 (src/cri/ci_ht.c, 160 bytes)
004e7c70  port  func_004e7c70 (src/cri/ci_ht.c, 152 bytes)
004e7d08  port  func_004e7d08 (src/cri/ci_ht.c, 56 bytes)
004e7d40  port  htCiReqRd (src/cri/ci_ht.c, 400 bytes)
004e7ed0  port  func_004e7ed0 (src/cri/ci_ht.c, 248 bytes)
004e7fc8  port  func_004e7fc8 (src/cri/ci_ht.c, 56 bytes)
004e8008  port  func_004e8008 (src/cri/ci_ht.c, 80 bytes)
004e8058  port  func_004e8058 (src/cri/ci_ht.c, 24 bytes)
004e8110  port  func_004e8110 (src/cri/ci_ht.c, 200 bytes)
004e9420  port  func_004e9420 (src/cri/ci_mf.c, 48 bytes)
004e9480  port  func_004e9480 (src/cri/ci_mf.c, 144 bytes)
004e9510  port  func_004e9510 (src/cri/ci_mf.c, 152 bytes)
004e9640  port  func_004e9640 (src/cri/ci_mf.c, 152 bytes)
004ea128  port  mfCiOpen (src/cri/ci_mf.c, 160 bytes)
004ea1c8  port  mfCiClose (src/cri/ci_mf.c, 72 bytes)
004ea210  port  func_004ea210 (src/cri/ci_mf.c, 192 bytes)
004ea2d0  port  dvCiTell (src/cri/ci_mf.c, 56 bytes)
004ea308  port  mfCiReqRd (src/cri/ci_mf.c, 392 bytes)
004ea490  port  func_004ea490 (src/cri/ci_mf.c, 80 bytes)
004ea4e0  port  func_004ea4e0 (src/cri/ci_mf.c, 56 bytes)
004ea518  port  func_004ea518 (src/cri/ci_mf.c, 56 bytes)
004ea550  port  func_004ea550 (src/cri/ci_mf.c, 128 bytes)
004ea5d0  port  func_004ea5d0 (src/cri/ci_mf.c, 56 bytes)
004ea690  port  mfCiOptFn1 (src/cri/ci_mf.c, 128 bytes)
004ea838  port  func_004ea838 (src/cri/ci_pl.c, 48 bytes)
004ea9c8  port  func_004ea9c8 (src/cri/ci_pl.c, 48 bytes)
004eaa18  port  func_004eaa18 (src/cri/ci_pl.c, 48 bytes)
004eaa88  port  PL2ENC_Encode (src/cri/ci_pl.c, 96 bytes)
004eae58  port  func_004eae58 (src/cri/ci_pl.c, 80 bytes)
004eaf58  port  func_004eaf58 (src/cri/ci_pl.c, 64 bytes)
004ec068  port  func_004ec068 (src/cri/ci_pl.c, 112 bytes)
004ec140  port  func_004ec140 (src/cri/ci_pl.c, 80 bytes)
004ecb40  port  func_004ecb40 (src/cri/ci_pl.c, 104 bytes)
004ecba8  port  func_004ecba8 (src/cri/ci_pl.c, 96 bytes)
004ed180  port  func_004ed180 (src/cri/ci_pl.c, 40 bytes)
004ed240  port  func_004ed240 (src/cri/ci_pl.c, 72 bytes)
00418b18  port  func_00418b18: register or remove the VBLANK_START (INTC 2) handler
004226a8  port  func_004226a8 (interrupt-safe WakeupThread, called by the ADXM vsync pass): iWakeupThread in src/platform/kernel_thread.c
004bd9d0  port  ACSFDR fade curve setter (src/cri/acsfdr_hooks.c)
004bd9d8  port  ACSFDR fade-start hook setter (src/cri/acsfdr_hooks.c)
004bd9e8  port  ACSFDR fade-end hook setter (src/cri/acsfdr_hooks.c)
004d4308  port  CRI alias to the re4 native (config/native_names.txt)
004d43f8  port  CRI alias to the re4 native (config/native_names.txt)
004d4b48  port  CRI alias to the re4 native (config/native_names.txt)
004d4bd0  port  CRI alias to the re4 native (config/native_names.txt)
004d4c58  port  CRI alias to the re4 native (config/native_names.txt)
004d4df0  port  CRI alias to the re4 native (config/native_names.txt)
004d4f58  port  CRI alias to the re4 native (config/native_names.txt)
004d5110  port  CRI alias to the re4 native (config/native_names.txt)
004d51c0  port  CRI alias to the re4 native (config/native_names.txt)
004d5488  port  CRI alias to the re4 native (config/native_names.txt)
004d5b30  port  CRI alias to the re4 native (config/native_names.txt)
004d6050  port  CRI alias to the re4 native (config/native_names.txt)
004d61c0  port  CRI alias to the re4 native (config/native_names.txt)
004d47c8  port  CRI alias to the re4 native (config/native_names.txt)
004d5dc8  port  CRI alias to the re4 native (config/native_names.txt)
004c99a0  port  partition lookup under the lock (src/cri/adxt_start.c)
004d6ec0  port  ADXT start file range (src/cri/adxt_start.c)
004d6fb8  port  ADXT start file (src/cri/adxt_start.c)
004d6f78  port  ADXT start file wrapper (src/cri/adxt_start.c)
004d6d68  port  ADXT_StartAfs (src/cri/adxt_start.c)
004d6d18  port  ADXT_StartAfs wrapper (src/cri/adxt_start.c)
004d7080  port  ADXT_StartMem2 (src/cri/adxt_start.c)
004d7030  port  ADXT_StartMem2 wrapper (src/cri/adxt_start.c)
004d71b0  port  ADXT_StartMemIdx (src/cri/adxt_start.c)
004d7160  port  ADXT_StartMemIdx wrapper (src/cri/adxt_start.c)
004c31b8  port  ADX key derivation (src/cri/adxt_start.c)
004c3d70  port  ADXB_SetExtString (src/cri/adxt_start.c)
004c3dd8  port  default ADX keys (src/cri/adxt_start.c)
004cec00  port  ADXSJD callback pair (src/cri/adxt_start.c)
004cecc8  port  ADXSJD_AdjustSmpl (src/cri/adxt_start.c)
004d5370  port  adxt_GetDataIdFromMem (src/cri/adxt_start.c)
004d3fa8  port  CRI alias to the re4 native (config/native_names.txt)
004d6968  port  CRI alias to the re4 native (config/native_names.txt)
004cd378  port  func_004cd378 (src/cri/ci_ht.c, 120 bytes)
004e36e8  port  func_004e36e8 (src/cri/ci_dv.c, 136 bytes)
004e3770  port  func_004e3770 (src/cri/ci_dv.c, 160 bytes)
004e3810  port  func_004e3810 (src/cri/ci_dv.c, 240 bytes)
004e3900  port  func_004e3900 (src/cri/ci_dv.c, 176 bytes)
004e39b8  port  func_004e39b8 (src/cri/ci_dv.c, 48 bytes)
004e3ae0  port  func_004e3ae0 (src/cri/ci_dv.c, 272 bytes)
004e3c10  port  func_004e3c10 (src/cri/ci_dv.c, 104 bytes)
004e4700  port  func_004e4700 (src/cri/ci_dv.c, 56 bytes)
004e4738  port  func_004e4738 (src/cri/ci_dv.c, 56 bytes)
004e4928  port  func_004e4928 (src/cri/ci_dv.c, 104 bytes)
004e4990  port  func_004e4990 (src/cri/ci_dv.c, 88 bytes)
004e5a38  port  func_004e5a38 (src/cri/ci_dv.c, 48 bytes)
004e5a68  port  func_004e5a68 (src/cri/ci_dv.c, 88 bytes)
004e5e28  port  func_004e5e28 (src/cri/ci_dv.c, 56 bytes)
004e5e60  port  func_004e5e60 (src/cri/ci_dv.c, 32 bytes)
004e6528  port  func_004e6528 (src/cri/ci_dv.c, 16 bytes)
004e6538  port  func_004e6538 (src/cri/ci_dv.c, 32 bytes)
004e6560  port  func_004e6560 (src/cri/ci_dv.c, 24 bytes)
004e6640  port  func_004e6640 (src/cri/ci_dv.c, 168 bytes)
004e6eb0  port  func_004e6eb0 (src/cri/ci_dv.c, 224 bytes)
004e7428  port  func_004e7428 (src/cri/ci_ht.c, 48 bytes)
004e7458  port  func_004e7458 (src/cri/ci_ht.c, 48 bytes)
004e7488  port  func_004e7488 (src/cri/ci_ht.c, 80 bytes)
004e7540  port  func_004e7540 (src/cri/ci_ht.c, 96 bytes)
004e75a0  port  func_004e75a0 (src/cri/ci_ht.c, 200 bytes)
004e7a00  port  func_004e7a00 (src/cri/ci_ht.c, 56 bytes)
004e7a38  port  func_004e7a38 (src/cri/ci_ht.c, 32 bytes)
004e8070  port  func_004e8070 (src/cri/ci_ht.c, 32 bytes)
004e8090  port  func_004e8090 (src/cri/ci_ht.c, 48 bytes)
004e80e0  port  func_004e80e0 (src/cri/ci_ht.c, 48 bytes)
004e82a0  port  func_004e82a0 (src/cri/ci_ht.c, 280 bytes)
004e8b98  port  func_004e8b98 (src/cri/ci_ht.c, 80 bytes)
004e9878  port  func_004e9878 (src/cri/ci_mf.c, 144 bytes)
004e9e20  port  func_004e9e20 (src/cri/ci_mf.c, 24 bytes)
004e9e38  port  func_004e9e38 (src/cri/ci_mf.c, 24 bytes)
004e9e68  port  func_004e9e68 (src/cri/ci_mf.c, 48 bytes)
004e9eb0  port  func_004e9eb0 (src/cri/ci_mf.c, 248 bytes)
004ea040  port  func_004ea040 (src/cri/ci_mf.c, 88 bytes)
004ea098  port  func_004ea098 (src/cri/ci_mf.c, 32 bytes)
004ea0b8  port  func_004ea0b8 (src/cri/ci_mf.c, 112 bytes)
004ea868  port  func_004ea868 (src/cri/ci_pl.c, 24 bytes)
004ea9f8  port  func_004ea9f8 (src/cri/ci_pl.c, 32 bytes)
004eaa48  port  func_004eaa48 (src/cri/ci_pl.c, 64 bytes)
004eaae8  port  func_004eaae8 (src/cri/ci_pl.c, 368 bytes)
004eaea8  port  func_004eaea8 (src/cri/ci_pl.c, 176 bytes)
004eaf98  port  func_004eaf98 (src/cri/ci_pl.c, 136 bytes)
004e36b8  port  func_004e36b8 (src/cri/ci_dv.c, 48 bytes)
004e39e8  port  func_004e39e8 (src/cri/ci_dv.c, 40 bytes)
004e5c68  port  func_004e5c68 (src/cri/ci_dv.c, 120 bytes)
004e6490  port  func_004e6490 (src/cri/ci_dv.c, 152 bytes)
004e6cc0  port  func_004e6cc0 (src/cri/ci_dv.c, 200 bytes)
004e6db0  port  func_004e6db0 (src/cri/ci_dv.c, 64 bytes)
004e6df0  port  func_004e6df0 (src/cri/ci_dv.c, 192 bytes)
004e81d8  port  func_004e81d8 (src/cri/ci_ht.c, 112 bytes)
004e8248  port  func_004e8248 (src/cri/ci_ht.c, 88 bytes)
004e8aa8  port  func_004e8aa8 (src/cri/ci_ht.c, 200 bytes)
004e9e98  port  unsigned-64 conversion tail wrapper (src/cri/ci_mf.c)
004e6778  port  func_004e6778 (src/cri/ci_dv.c, 152 bytes)
004e8428  port  func_004e8428 (src/cri/ci_ht.c, 152 bytes)
00444458  port  unsigned-64 retail integer conversion, guest end pointer (src/platform/ps2_libc.c)
00444210  port  signed-64 retail integer conversion (src/platform/ps2_libc.c)
0043c6b0  port  atoi: signed-64 conversion followed by explicit low-32 result (src/platform/ps2_libc.c)
004426e8  port  strcmp: first differing unsigned-byte difference (src/platform/ee_bytes.c)
00442c30  port  strncmp: bounded first differing unsigned-byte difference (src/platform/ee_bytes.c)
003f6800  port  RpSkyTextureSetMipmapK: port-owned in src/render/p4gl_raster.c (stores TEX1.K in the raster slot)
003f68a0  port  func_003f68a0 (RpSkyTextureSetMipmapL): port-owned in src/render/p4gl_raster.c (stores TEX1.L in the raster slot)
004e66e8  port  func_004e66e8 (src/cri/ci_dv.c, 56 bytes)
004e6720  port  func_004e6720 (src/cri/ci_dv.c, 88 bytes)
004e83b8  port  func_004e83b8 (src/cri/ci_ht.c, 24 bytes)
004e83d0  port  func_004e83d0 (src/cri/ci_ht.c, 88 bytes)
004c9f20  port  func_004c9f20, src/cri/adx_hdr.c (AHX decoder translated from the asm)
004c9fe0  port  func_004c9fe0, src/cri/adx_hdr.c (AHX decoder translated from the asm)
004ca000  port  func_004ca000, src/cri/adx_hdr.c (AHX decoder translated from the asm)
004ca0f0  port  func_004ca0f0, src/cri/adx_hdr.c (AHX decoder translated from the asm)
004ca1b0  port  ADX_DecodeHeader, src/cri/adx_hdr.c (AHX decoder translated from the asm)
004cc9d0  port  func_004cc9d0, src/cri/adx_hdr.c (AHX decoder translated from the asm)
004cc9e8  port  func_004cc9e8, src/cri/adx_hdr.c (AHX decoder translated from the asm)
004d9348  port  func_004d9348, src/cri/ahx.c (AHX decoder translated from the asm)
004d9478  port  func_004d9478, src/cri/ahx.c (AHX decoder translated from the asm)
004d94a8  port  func_004d94a8, src/cri/ahx.c (AHX decoder translated from the asm)
004d9528  port  func_004d9528, src/cri/ahx.c (AHX decoder translated from the asm)
004d9768  port  func_004d9768, src/cri/ahx.c (AHX decoder translated from the asm)
004d97c8  port  func_004d97c8, src/cri/ahx.c (AHX decoder translated from the asm)
004d9878  port  func_004d9878, src/cri/ahx.c (AHX decoder translated from the asm)
004d9880  port  AHXBSR_IsEos, src/cri/ahx.c (AHX decoder translated from the asm)
004d98d8  port  func_004d98d8, src/cri/ahx.c (AHX decoder translated from the asm)
004d9998  port  func_004d9998, src/cri/ahx.c (AHX decoder translated from the asm)
004d9a58  port  func_004d9a58, src/cri/ahx.c (AHX decoder translated from the asm)
004d9a90  port  func_004d9a90, src/cri/ahx.c (AHX decoder translated from the asm)
004d9aa8  port  func_004d9aa8, src/cri/ahx.c (AHX decoder translated from the asm)
004d9ae0  port  func_004d9ae0, src/cri/ahx.c (AHX decoder translated from the asm)
004d9b20  port  func_004d9b20, src/cri/ahx.c (AHX decoder translated from the asm)
004d9cd8  port  func_004d9cd8, src/cri/ahx.c (AHX decoder translated from the asm)
004d9d48  port  func_004d9d48, src/cri/ahx.c (AHX decoder translated from the asm)
004d9d68  port  func_004d9d68, src/cri/ahx.c (AHX decoder translated from the asm)
004d9dc0  port  func_004d9dc0, src/cri/ahx.c (AHX decoder translated from the asm)
004d9e10  port  func_004d9e10, src/cri/ahx.c (AHX decoder translated from the asm)
004d9ef0  port  func_004d9ef0, src/cri/ahx.c (AHX decoder translated from the asm)
004d9f50  port  func_004d9f50, src/cri/ahx.c (AHX decoder translated from the asm)
004da018  port  func_004da018, src/cri/ahx.c (AHX decoder translated from the asm)
004da050  port  func_004da050, src/cri/ahx.c (AHX decoder translated from the asm)
004da1f8  port  func_004da1f8, src/cri/ahx.c (AHX decoder translated from the asm)
004da2d8  port  func_004da2d8, src/cri/ahx.c (AHX decoder translated from the asm)
004da2e8  port  func_004da2e8, src/cri/ahx.c (AHX decoder translated from the asm)
004da448  port  AHXDCD_GetNumChan, src/cri/ahx.c (AHX decoder translated from the asm)
004da468  port  func_004da468, src/cri/ahx.c (AHX decoder translated from the asm)
004da4a0  port  func_004da4a0, src/cri/ahx.c (AHX decoder translated from the asm)
004da4c8  port  AHXDCD_SetExtString, src/cri/ahx.c (AHX decoder translated from the asm)
004da630  port  func_004da630, src/cri/ahx.c (AHX decoder translated from the asm)
004da748  port  func_004da748, src/cri/ahx.c (AHX decoder translated from the asm)
004da768  port  func_004da768, src/cri/ahx.c (AHX decoder translated from the asm)
004da850  port  func_004da850, src/cri/ahx.c (AHX decoder translated from the asm)
004da9d8  port  func_004da9d8, src/cri/ahx.c (AHX decoder translated from the asm)
004dacb8  port  func_004dacb8, src/cri/ahx.c (AHX decoder translated from the asm)
004db250  port  func_004db250, src/cri/ahx.c (AHX decoder translated from the asm)
004db398  port  func_004db398, src/cri/ahx.c (AHX decoder translated from the asm)
004db3c0  port  func_004db3c0, src/cri/ahx.c (AHX decoder translated from the asm)
004db430  port  func_004db430, src/cri/ahx.c (AHX decoder translated from the asm)
004db448  port  func_004db448, src/cri/ahx.c (AHX decoder translated from the asm)
004db460  port  func_004db460, src/cri/ahx.c (AHX decoder translated from the asm)
004db558  port  func_004db558, src/cri/ahx.c (AHX decoder translated from the asm)
004dd608  port  func_004dd608, src/cri/ahx.c (AHX decoder translated from the asm)
004dd620  port  func_004dd620, src/cri/ahx.c (AHX decoder translated from the asm)
004dd678  port  func_004dd678, src/cri/ahx.c (AHX decoder translated from the asm)
004dd718  port  func_004dd718, src/cri/ahx.c (AHX decoder translated from the asm)
004dd768  port  func_004dd768, src/cri/ahx.c (AHX decoder translated from the asm)
004dd840  port  func_004dd840, src/cri/ahx.c (AHX decoder translated from the asm)
004dd880  port  func_004dd880, src/cri/ahx.c (AHX decoder translated from the asm)
004dd9c8  port  func_004dd9c8, src/cri/ahx.c (AHX decoder translated from the asm)
004dda30  port  func_004dda30, src/cri/ahx.c (AHX decoder translated from the asm)
004dda88  port  func_004dda88, src/cri/ahx.c (AHX decoder translated from the asm)
004ddac0  port  func_004ddac0, src/cri/ahx.c (AHX decoder translated from the asm)
004ddc10  port  func_004ddc10, src/cri/ahx.c (AHX decoder translated from the asm)
004ddc88  port  func_004ddc88, src/cri/ahx.c (AHX decoder translated from the asm)
004ddc98  port  func_004ddc98, src/cri/ahx.c (AHX decoder translated from the asm)
004ddca0  port  AHXSJD_SetOutSj, src/cri/ahx.c (AHX decoder translated from the asm)
004ddcb8  port  func_004ddcb8, src/cri/ahx.c (AHX decoder translated from the asm)
004ddd10  port  func_004ddd10, src/cri/ahx.c (AHX decoder translated from the asm)
004ddd40  port  func_004ddd40, src/cri/ahx.c (AHX decoder translated from the asm)
004ddda0  port  func_004ddda0, src/cri/ahx.c (AHX decoder translated from the asm)
004de048  port  func_004de048, src/cri/ahx.c (AHX decoder translated from the asm)
004de088  port  func_004de088, src/cri/ahx.c (AHX decoder translated from the asm)
004de110  port  func_004de110, src/cri/ahx.c (AHX decoder translated from the asm)
004de140  port  func_004de140, src/cri/ahx.c (AHX decoder translated from the asm)
004de148  port  func_004de148, src/cri/ahx.c (AHX decoder translated from the asm)
004de150  port  func_004de150, src/cri/ahx.c (AHX decoder translated from the asm)
004de158  port  func_004de158, src/cri/ahx.c (AHX decoder translated from the asm)
004de160  port  func_004de160, src/cri/ahx.c (AHX decoder translated from the asm)
004de290  port  func_004de290, src/cri/ahx.c (AHX decoder translated from the asm)
004c3178  port  ADX key table use count (src/cri/adxt_start.c)
00422218  port  func_00422218 (SyncDCache): no-op, host cache is coherent; src/platform/ps2_kernel.c
0042f3e0  port  func_0042f3e0 (sceCdSeek): always succeeds; src/platform/files/sce_cdvd.c
0042f4b0  port  func_0042f4b0 (sceCdGetError): 0; src/platform/files/sce_cdvd.c
00429358  port  func_00429358 (sceLseek64): full signed 64-bit offset/result on the host VFS; src/platform/files/sce_file.c
003db490  port  func_003db490 (render job callback, mean mipmap K of a texture chain; src overrides/003db490.c, 192 bytes)
004c9b80  port  func_004c9b80, src/cri/aixp.c (AIX player translated from the asm)
004d3bc0  port  func_004d3bc0, src/cri/aixp.c (AIX player translated from the asm)
004d4170  port  func_004d4170, src/cri/aixp.c (AIX player translated from the asm)
004d4778  port  func_004d4778, src/cri/aixp.c (AIX player translated from the asm)
004de2c0  port  func_004de2c0, src/cri/aixp.c (AIX player translated from the asm)
004de318  port  func_004de318, src/cri/aixp.c (AIX player translated from the asm)
004de4c0  port  aixp_Create, src/cri/aixp.c (AIX player translated from the asm)
004de540  port  aixp_Create3D, src/cri/aixp.c (AIX player translated from the asm)
004de5c0  port  func_004de5c0, src/cri/aixp.c (AIX player translated from the asm)
004de860  port  func_004de860, src/cri/aixp.c (AIX player translated from the asm)
004de998  port  func_004de998, src/cri/aixp.c (AIX player translated from the asm)
004de9f8  port  func_004de9f8, src/cri/aixp.c (AIX player translated from the asm)
004deb38  port  func_004deb38, src/cri/aixp.c (AIX player translated from the asm)
004deb58  port  func_004deb58, src/cri/aixp.c (AIX player translated from the asm)
004dec30  port  func_004dec30, src/cri/aixp.c (AIX player translated from the asm)
004decb8  port  func_004decb8, src/cri/aixp.c (AIX player translated from the asm)
004ded60  port  func_004ded60, src/cri/aixp.c (AIX player translated from the asm)
004dee48  port  func_004dee48, src/cri/aixp.c (AIX player translated from the asm)
004deec0  port  func_004deec0, src/cri/aixp.c (AIX player translated from the asm)
004def18  port  func_004def18, src/cri/aixp.c (AIX player translated from the asm)
004defa0  port  func_004defa0, src/cri/aixp.c (AIX player translated from the asm)
004defe8  port  func_004defe8, src/cri/aixp.c (AIX player translated from the asm)
004df068  port  func_004df068, src/cri/aixp.c (AIX player translated from the asm)
004df098  port  func_004df098, src/cri/aixp.c (AIX player translated from the asm)
004df0c8  port  func_004df0c8, src/cri/aixp.c (AIX player translated from the asm)
004df128  port  func_004df128, src/cri/aixp.c (AIX player translated from the asm)
004df1a0  port  func_004df1a0, src/cri/aixp.c (AIX player translated from the asm)
004df220  port  func_004df220, src/cri/aixp.c (AIX player translated from the asm)
004df260  port  func_004df260, src/cri/aixp.c (AIX player translated from the asm)
004df320  port  AIXP_GetNumSmpl, src/cri/aixp.c (AIX player translated from the asm)
004df3c0  port  AIXP_GetNumTrack, src/cri/aixp.c (AIX player translated from the asm)
004df418  port  AIXP_GetNumPhrase, src/cri/aixp.c (AIX player translated from the asm)
004df470  port  AIXP_GetNumSong, src/cri/aixp.c (AIX player translated from the asm)
004df4c8  port  AIXP_GetByteRate, src/cri/aixp.c (AIX player translated from the asm)
004df5b0  port  func_004df5b0, src/cri/aixp.c (AIX player translated from the asm)
004df708  port  func_004df708, src/cri/aixp.c (AIX player translated from the asm)
004df800  port  func_004df800, src/cri/aixp.c (AIX player translated from the asm)
004df850  port  func_004df850, src/cri/aixp.c (AIX player translated from the asm)
004dfc98  port  func_004dfc98, src/cri/aixp.c (AIX player translated from the asm)
004dfcd8  port  func_004dfcd8, src/cri/aixp.c (AIX player translated from the asm)
004dfd78  port  func_004dfd78, src/cri/aixp.c (AIX player translated from the asm)
004dfdc8  port  func_004dfdc8, src/cri/aixp.c (AIX player translated from the asm)
004dfe38  port  func_004dfe38, src/cri/aixp.c (AIX player translated from the asm)
004dfe60  port  func_004dfe60, src/cri/aixp.c (AIX player translated from the asm)
004dff78  port  func_004dff78, src/cri/aixp.c (AIX player translated from the asm)
004dfff0  port  func_004dfff0, src/cri/aixp.c (AIX player translated from the asm)
004e0058  port  func_004e0058, src/cri/aixp.c (AIX player translated from the asm)
004e00d8  port  func_004e00d8, src/cri/aixp.c (AIX player translated from the asm)
004e0158  port  func_004e0158, src/cri/aixp.c (AIX player translated from the asm)
004e01c0  port  func_004e01c0, src/cri/aixp.c (AIX player translated from the asm)
004e0268  port  func_004e0268, src/cri/aixp.c (AIX player translated from the asm)
004e0308  port  func_004e0308, src/cri/aixp.c (AIX player translated from the asm)
004e0458  port  func_004e0458, src/cri/aixp.c (AIX player translated from the asm)
004e04b0  port  func_004e04b0, src/cri/aixp.c (AIX player translated from the asm)
004e0598  port  func_004e0598, src/cri/aixp.c (AIX player translated from the asm)
004e05d8  port  func_004e05d8, src/cri/aixp.c (AIX player translated from the asm)
004e05e8  port  func_004e05e8, src/cri/aixp.c (AIX player translated from the asm)
004e05f0  port  func_004e05f0, src/cri/aixp.c (AIX player translated from the asm)
004e05f8  port  func_004e05f8, src/cri/aixp.c (AIX player translated from the asm)
004e0600  port  func_004e0600, src/cri/aixp.c (AIX player translated from the asm)
004e0610  port  func_004e0610, src/cri/aixp.c (AIX player translated from the asm)
004e0620  port  func_004e0620, src/cri/aixp.c (AIX player translated from the asm)
004e0628  port  func_004e0628, src/cri/aixp.c (AIX player translated from the asm)
004e0658  port  func_004e0658, src/cri/aixp.c (AIX player translated from the asm)
004e0688  port  func_004e0688, src/cri/aixp.c (AIX player translated from the asm)
004e0690  port  func_004e0690, src/cri/aixp.c (AIX player translated from the asm)
004e06f0  port  func_004e06f0, src/cri/aixp.c (AIX player translated from the asm)
004e0728  port  func_004e0728, src/cri/aixp.c (AIX player translated from the asm)
004e0730  port  func_004e0730, src/cri/aixp.c (AIX player translated from the asm)
004e0778  port  func_004e0778, src/cri/aixp.c (AIX player translated from the asm)
004e0b30  port  func_004e0b30, src/cri/aixp.c (AIX player translated from the asm)
004e0b80  port  func_004e0b80, src/cri/aixp.c (AIX player translated from the asm)
004e0c60  port  func_004e0c60, src/cri/aixp.c (AIX player translated from the asm)
004e0db0  port  func_004e0db0, src/cri/aixp.c (AIX player translated from the asm)
004e1100  port  func_004e1100, src/cri/aixp.c (AIX player translated from the asm)
004d3c08  port  func_004d3c08, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004ea740  port  func_004ea740, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004ea780  port  func_004ea780, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004ea7c8  port  func_004ea7c8, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004ea7f8  port  func_004ea7f8, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004ea880  port  func_004ea880, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004ea8c8  port  func_004ea8c8, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004eac58  port  func_004eac58, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004eacb8  port  func_004eacb8, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004eb020  port  func_004eb020, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004eb158  port  func_004eb158, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004eb180  port  func_004eb180, src/cri/pl2_attach.c (PL2 encoder create/attach translated from the asm)
004d1c58  port  CRI alias to the re4 native ADXSTM_Create (config/native_names.txt)
004d1ff8  port  CRI alias to the re4 native ADXSTM_GetStat (config/native_names.txt)
004d2348  port  CRI alias to the re4 native ADXSTM_EntryEosFunc (config/native_names.txt)
0043f778  port  memcmp: literal retail first differing unsigned-byte difference, with EE-u32 count, in src/platform/ee_bytes.c
003cebb0  port  _rpWorldSectorNativeRead (overrides/003cebb0.c, 768 bytes)
003c9940  port  func_003c9940 (code1_003c asm fallback; C only in the excluded _cw119 copy): registers a geometry block and its size in the RenderWare freelist's list
00276420  port  frFontRasterUploadGlyph: glyph pixels written linear (the retail PSMT4 block swizzle read as streaks), palette copy as retail
00440bb8  port  func_00440bb8 (retail qsort ordering and game libc binding, overrides/00440bb8.c, 2368 bytes)
004258b0 port func_004258b0 (sceSifCallRpc): host model of the CRI_ADXI.IRX server for client 0x00732D30 (ids 0..3, 8..18, 32..42); src/cri/adxi_rpc.c
003e4030  port  func_003e4030 (RwV3dNormalize; plain port C because its C is only in the unbuilt code1_003e_cw119.c: src/platform/rw_normalize3.c, 128 bytes)
003e41e0  port  func_003e41e0 (2D normalize with error report; plain port C, see 003e4030: src/platform/rw_normalize2.c, 192 bytes)
004220d8  port  func_004220d8 (FlushCache range, no-op: host cache is coherent; src/platform/ps2_kernel.c)
00399320  port  func_00399320 (gp plugin word flag, overrides/00399320.c, 32 bytes)
00399450  port  func_00399450 (gp plugin word flag, overrides/00399450.c, 32 bytes)
0039a8a0  port  func_0039a8a0 (pipeline DMA tag word set, overrides/0039a8a0.c, 112 bytes)
0039f050  port  func_0039f050 (pipeline handler dispatch, overrides/0039f050.c, 144 bytes)
003d0850  port  func_003d0850 (texture block alloc/dispatch, overrides/003d0850.c, 224 bytes)
003d86a0  port  func_003d86a0 (scratch walk and sum, overrides/003d86a0.c, 192 bytes)
003ebbf0  port  func_003ebbf0 (RwImageStreamRead at the retail address; plain port C because its marker is in the unbuilt src/rw/baimage.c: src/platform/rw_image_stream.c, 496 bytes)
003d0460  port  func_003d0460 (texture block alloc / dispatch, overrides/003d0460.c, 224 bytes)
003e1ff0  port  func_003e1ff0 (RwResourcesAllocateResEntry: alias of the decomp native in config/native_names.txt)
00449938  port  func_00449938 (__ieee754_powf: literal retail core in src/platform/ee_libm.c)
004c1a68  port  func_004c1a68 (ADXB AC3 set-sji dispatcher; src/cri/adx_ac3_retail.c, 48 bytes)
004c1a98  port  ADXB_SetAc3DecSmpl (asm block in the decomp; src/cri/adx_ac3_retail.c, 32 bytes)
004c1ab8  port  func_004c1ab8 (ADXB AC3 term-supply dispatcher; src/cri/adx_ac3_retail.c, 48 bytes)
0048abd0  port  mdlEffectSamplePackedColor (effect module, VU0 colour ramp)
0048c4e0  port  EffMotionEmitterUpdate (effect module, particle emitter)
0048cdf0  port  mdlEffectUpdateHelixEmitter (effect module, particle emitter)
0048d8c0  port  mdlEffectUpdateOrbitEmitter (effect module, particle emitter)
0048e2f0  port  EffConeEmitterUpdate (effect module, particle emitter)
004903c0  port  mdlEffectUpdateFloorEmitter (effect module, particle emitter)
004a9690  port  effBlurFilterUpdateRectFromScale (effect module)
004aab50  port  effBlurFilterUpdateRectFromScale variant (effect module)
00498f10  port  RingRebuildCircle (effect module)
0049ef50  port  FlashCenteredStripEmitQuads (effect module)
00486400  port  mdlEffectSetChildScales (effect module, child dispatch)
00486330  port  mdlEffectSetChildRotations (effect module, child dispatch)
004861f0  port  mdlEffectSetChildPositions (effect module, child dispatch)
004f8548  port  ROFS volume device interface getter (host), src/platform/files/rofs_host.c
004c6d10  port  ADXF_Init on retail-addressed pool/table storage, sce_adxf.c
004c6de8  port  ADXF_Finish on retail-addressed pool/table storage, sce_adxf.c
004c8400  port  allocate a retail ADXF pool slot, sce_adxf.c
004c8458  port  create ADXF with a normal ADXSTM controller, sce_adxf.c
004c84f0  port  whole-file STM binding and handle metadata, sce_adxf.c
004c85e8  port  inner ADXF_Open, sce_adxf.c
004c8688  port  file-range STM binding and handle metadata, sce_adxf.c
004c89f0  port  release an ADXF-owned stream joint, sce_adxf.c
004c8bd0  port  start a bounded ADXSTM read on a joint, sce_adxf.c
004c8d28  port  caller-owned-joint read entry, sce_adxf.c
004c8e80  port  SJRBF-backed ADXF_ReadNw32, sce_adxf.c
004c90e0  port  blocking ADXF stop, sce_adxf.c
004c9208  port  no-wait ADXF stop, sce_adxf.c
004c92d0  port  publish one ADXSTM result into ADXF, sce_adxf.c
004c9398  port  retail ADXF server wrapper bound to ADXF_ExecServer, sce_adxf.c
004c93c0  port  retail ADXF pool server bound to ADXF_ExecServer, sce_adxf.c
004c9478  port  retail ADXF sector seek, sce_adxf.c
004c95c8  port  retail ADXF sector tell, sce_adxf.c
004c9630  port  retail ADXF sector-size query, sce_adxf.c
004c96a8  port  retail ADXF byte-size query, sce_adxf.c
004c9768  port  retail ADXF requested sectors and first-sector output, sce_adxf.c
004c97f0  port  retail ADXF completed-sector count, sce_adxf.c
004c9c20  port  retail ADXF per-request sector limit, sce_adxf.c
004d1e58  port  blocking ADXSTM bind wrapper, adxstm_api.c
004d1ec8  port  blocking bind pumps ADXT file server, adxstm_api.c
004d21a0  port  bounded ADXSTM start wrapper, adxstm_api.c
004d21e8  port  bounded ADXSTM start body, adxstm_api.c
004d2d38  port  ADXSTM sector count wrapper, adxstm_api.c
004d2d70  port  ADXSTM sector count body supplied only by matched re4/adx_stmc_leaf_d2d70.c; adxstm_api.c wrapper passes its guest address
004d2dc8  port  ADXSTM joint-mode byte wrapper, adxstm_api.c
004d2fa8  port  ADXSTM joint assignment wrapper, adxstm_api.c
004d2fe8  port  ADXSTM joint assignment and buffer policy, adxstm_api.c
004d3228  port  lazy-open settlement wrapper, adxstm_api.c
004d3258  port  lazy-open settlement under the controller server guard, adxstm_api.c
00456150  port  H_Cdvd_ReadSync hardware wait: deterministic kernel/IRQ safepoint, matched CRI status/server order retained; overrides/00456150.c
001f8480  port  btlSoundLoadBanks: generic EE timer/scheduler safepoints in its three synchronous AFS-table wait loops, mirror_fixes.txt; ADXF/partition status and server publication unchanged
004f13c0  port  SRD seek adapter, three live arguments and delegated sector-position return
004f1460  port  SRD get-status adapter, preserves states 0..2 and maps other states to 3
004f14e0  port  CVFS extended-transfer adapter, preserves a nonzero 64-bit count or the signed legacy count
004f9a58  port  SVM unlock tail, native_names alias to the existing SVM_Unlock implementation
004f9a70  port  SRD request-pump tail, native_names alias to the existing func_004e4190 implementation
004f53c0  port  RFTGT error-report dependency of the SRD seek adapter
00513380  port  SFD ADXT decode-information readiness predicate
004e3448  port  CVFS extended-transfer query, option 200 high word plus option 201 low word; src/cri/cvfs_port.c
004f14c8  port  CVFS legacy-transfer delegate, preserve its live handle and signed return in mirror_fixes
00512d90  port  retail SFD silence callback, strict 18-byte scan boundary and 64-bit a_byte at +0x9c8; src/cri/sfd_port.c
00512ea8  port  raw-layout SFD audio-info lookup with untouched failure outputs; src/cri/sfd_port.c
00513dc8  port  retail SEE audio header and signed seek flag; src/cri/sfd_port.c
00517840  port  retail file-header mux version, unknown until valid; src/cri/sfd_port.c
00512730  port  actual SJ free-chunk method wrapper; src/cri/sfd_port.c, shared cri_mem.h interface
00512760  port  actual SJ split/put/unget wrapper, including zero-length path; src/cri/sfd_port.c
005127e8  port  raw SFD terminal audio copy callback, full chunk accounting; src/cri/sfd_port.c
005128a0  port  raw SFD audio/video sample alignment callback; src/cri/sfd_port.c
00512b90  port  SFD work-to-ADXT silence insertion delegate, three live arguments; src/cri/sfd_port.c
00512bb0  port  raw SFD ADX header exclusion and callback transition; src/cri/sfd_port.c
00512c70  port  raw SFD frame-phase search, actual pointer return and error path; src/cri/sfd_port.c
00517ad0  port  raw per-handle/global error provider; preserve retail guest 745a5c rather than the i386 SDK's private host BSS; src/cri/sfd_port.c
00517b40  port  raw first-error record and application callback, including the live second code argument; src/cri/sfd_port.c
00124830  port  titleSelectMenu: native title Config shortcut; retain unrequested title selection behavior
0012c510  port  cmpMainTaskStep: route an accepted field settings request through the native Camp System transition
0012d000  port  cmpMainTaskCreate: associate an accepted field request with its actual child task; port coordinates only the item-tail allocation size
00170b50  port  FldCampTriggerTest: accept settings shortcuts only through the original flag-0x37 and field event gates
00172ba0  port  FldEvent_PartyHasSkill: 0x205 also counts when granted to the protagonist, his current Persona or his equipment, and when granted to an active member or its equipment, as the member's accessory 0x270 (disc ITEMTBL row 0x270 grants 0x205) counts (mirror_fixes)
00232730  port  datCalcHasSkill and every datCalc.c learned/fixed-slot passive search: the static datCalcHasRequestedSkill and datCalcHasFixedSkill (overrides/static) end with the unit's passive grants
0035d0a0  port  cmpConfigDraw: registered PC rows, Controls and bounded native pages using retail bars, font, footer, value controls and scrollbar art
0035dfb0  port  cmpConfigDrawOnOff: project visible logical toggle values/timers onto the original ON/OFF widget without growing the retail work block
0035e820  port  cmpConfigDestroy: native Config sidecar cleanup
0035f0c0  port  cmpConfigStepInput: native Config input/state machine with registered rows, scrolling, Controls and bounded native pages
003627b0  port  cmpSystemMenuUpdate: route an accepted Camp settings request into the original Config child lifecycle
0043ca70  port  raw-bit retail dtoa and its guest-layout bigint closure; src/platform/ee_dtoa.c and ee_decimal.c
00442088  port  sprintf: shared retail conversion engine in ee_printf.c
00440b68  port  printf: shared retail conversion engine with host stdout byte transport
0044ba00  port  raw signed-64 to soft-double conversion required by retail strtod; ps2_fpbit.c
0044bb60  port  raw soft-double to unsigned-64 conversion closure; ps2_fpbit.c
0044bc68  port  raw soft-double to signed-64 conversion required by retail strtod; ps2_fpbit.c
0044e830  port  raw soft-double to unsigned-word conversion closure; ps2_fpbit.c
0010d480  port  packed battle-table completion; freeze skill metadata/elements/text before atomic relocation
001117c0  port  authenticate and stage the actual ITEM input before the unchanged retail copy; publish no expansion early
00464650  port  actual full-image write submission: capture native extension blocks before the memcard task yields
0010e710  port  exact validated full-save restore: commit native extension state only after retail restoration
001113b0  port  actual New Game reset: stage/commit registered native provider defaults around the unchanged retail reset
00465570  port  actual full READ submission: cancel prior logical MC staging before native file searches, never through unrelated reads
00463930  port  tmxDecodeStream: unchanged decoded guest image plus opt-in native source-provenance publication
00243840  port  skill name bank expansion through owned RAW native text; retail bank unchanged
00114cb0  port  legacy skill-icon selector kept for retail art; full-width element art is intercepted at 001138c0/00205170; port coordinates existing skill active/passive predicate facet
00113520  port  equipped skill cost rows; explicit active predicate in mirror_fixes; port facet: each cost row brackets its datCalcSkillCostById call with the shown persona record for the skill cost stage
00138490  port  camp skill deduplication; explicit active predicate in mirror_fixes; port facet: the static cmpSkillListContains (overrides/static) also counts a passive granted to the roster Persona or the protagonist
0013ac30  port  legacy Camp label selector kept for retail art; full-width element art is intercepted at 00138bf0; port coordinates existing skill active predicate facet
001daf40  port  AI skill eligibility; explicit registered/active predicates in mirror_fixes
001db160  port  expanded native AI owned-element scan; port coordinates existing skill active predicate facet
001dd3a0  port  AI damage skill ranking; explicit active predicate in mirror_fixes
001dd570  port  expanded native AI target/element candidates; port coordinates existing registered/active skill predicate facet
001dd920  port  AI recovery skill candidates; explicit active predicate in mirror_fixes
001de000  port  expanded native AI element candidates; port coordinates existing skill active predicate facet
001db740  port  expanded native Analyze/any-known AI facet via scoped mirror recipe; actual enemy/profile observation query, inactive retail/genus behavior unchanged
001b0930  port  one existing action allocation gains explicit scalar AI carrier storage after proven0x458 only in qualified expanded mode; inactive allocation/memset and original base-owner lifetime unchanged
001d94d0  port  source-owned full-width family query/knowledge dispatch; preserve original side/status/filter/early-return policy, no inferred subtype or ambient last action
001dbba0  port  exact candidate-frame target selection and original per-script knowledge policy through typed AI carrier; port side-ailment facet (port): every-ailment exclusion excludes a side-ailing unit (mirror_fixes)
001de800  port  table-condition decision carrier preserves selected positive-condition precedence and exact available candidate permissions
001dea90  port  weighted-command carrier facet; original five-entry weights/RNG/boundary order preserved
001ded30  port  native enemy decision-to-weighted-command/target root with lifetime-owned exact selected-frame carrier
001eb1d0  port  script weighted-command/target root carries exact selected candidate instead of losing family permission into a scalar
001e8bb0  port  legacy script command-family collection retains owning source/target frame and query through returned command selection
001e8cd0  port  legacy script effective-skill collection retains owning source/target frame and query through returned command selection
001de640  port  action-code decoding binds or clears the actual lifetime-owned selected-frame carrier; script decodes pass through this hookable entry; one-shot authored substitution; preserve retail packed command/plan tags
001da230  port  legacy repel profile/family query facet; full typed candidate context for Phys0 and explicit canonical nonphysical profile query
001da270  port  legacy drain profile/family query facet; full typed candidate context for Phys0 and explicit canonical nonphysical profile query
001da2b0  port  legacy null profile/family query facet; full typed candidate context for Phys0 and explicit canonical nonphysical profile query
001da2f0  port  legacy weakness profile/family query facet; full typed candidate context for Phys0 and explicit canonical nonphysical profile query
001db360  port  expanded normal-affinity predicate preserves unknown-first and native temporary/physical gates over exact candidate frames; port facet: the Counter test (mirror_fixes and the predicate body) also sees a granted Counter, Counterstrike or High Counter
001db9f0  port  expanded nonnormal-affinity predicate preserves original observed-only policy over exact candidate frames
001b0a60  port  release explicit one-allocation AI carrier accounting after original unlink and before original base free; inactive lifecycle unchanged, verified001b0b78-001b0bc4
001b0bf0  port  full action-list teardown releases the same appended carrier before original base-owner free; no separate per-action allocation or tail-ownership probing
001debb0  port  bind actual newly created AI script work to its prepared BF and exact source actor; UI/status scripts sharing legacy proc field are not guessed AI contexts
0029cb00  port  cold-split canonical VM loop adds scoped before/after facets only for actual bound AI work, preserving original inactive dispatch/timers/yield/return behavior
0029cf50  port  integer return-slot facet consumes only this VM instance's pending full frame/query tag, clearing stale tags on actual untagged writes
0029cf70  port  float return-slot facet scopes metadata to the actual VM instance; no native command replay, ambient element or no-new-elements state change
0029b1b0  port  single private canonical binary-operation body reused for metadata shadows; original exported operation path and arithmetic remain unchanged
0029bfb0  port  single canonical numeric negation body reused for scoped metadata shadows, no copied math algorithm or extra external hook dispatch
0029c130  port  single canonical numeric logical-not body reused for scoped metadata shadows, original scalar execution remains once
0029d1c0  port  invalidate direct parallel metadata for the actual work-pool slot at the real release seam, never infer ownership from unused VM padding
001e64c0  port  carry the full command-argument frame through AI_GET_CMD_ATTR without a mutable last-selection lookup or s16 semantic-ID truncation
001e0060  port  actual fixed-self base repel query with full VM argument frame and proven basic/available source role before low24 packing
001e0110  port  actual fixed-self base drain query with full VM argument frame and proven basic/available source role before low24 packing
001e01c0  port  actual fixed-self base null predicate, preserving null sense rather than substituting normal
001e0270  port  actual fixed-self base weak query with full-width frame/source classification
001e6930  port  exact-ID normal predicate asked of the actual ID defender; its answer never binds the final targets
001e6b90  port  exact-ID not-normal predicate with original unknown-first policy and full VM argument frame
001e6ea0  port  explicit-VM weak candidate selector with original flag2E policy and one original chosen-target RNG draw
001e6fa0  port  explicit-VM weak selector preserving original100000 exclusion and2E flags
001e7080  port  explicit-VM weak selector preserving2F flags and original chosen target
001e7160  port  full-frame normal target selector, retain original scalar status/ID return and RNG
001e7240  port  full-frame normal selector preserving100000 exclusion and2C flags
001e7320  port  full-frame normal selector preserving2D flags
001e77f0  port  full-frame normal candidate collection before original level/HP ranking, no reranking/replay
001e7950  port  full-frame normal candidates before original pure-status filtering and one random choice; port side-ailment facet (port): a side-ailing unit is not unafflicted (mirror_fixes)
001e7ab0  port  exact no-gate normal predicate/rejected1 before original highest-level choice
001e7c20  port  original count-domain normal/status query, never interpret count as an action ID; port side-ailment facet (port): a side-ailing unit is not unafflicted (mirror_fixes)
001e8090  port  full-frame base repel candidates with original2E flags and chosen-ID return
001e8190  port  full-frame base drain candidates with original2E flags and chosen-ID return
001e8290  port  full-frame base null candidates with original2E flags and chosen-ID return
001e8390  port  full-frame BASE resistance-flag candidates, not wrapper normal
001e8490  port  full-frame weak candidates before unchanged least-HP ranking
001e85c0  port  full-frame weak candidates with original100000 exclusion before unchanged HP ranking
001e86f0  port  full-frame weak candidates preserving2F flags and least-HP tie order
001e8820  port  full-frame weak candidates before unchanged greatest-HP ranking
001e8950  port  full-frame weak candidates with original100000 exclusion before unchanged greatest-HP ranking
001e8a80  port  full-frame weak candidates preserving2F flags and greatest-HP ranking
001e8d80  port  actual basic-attack element getter returns an owned full-frame VM tag before legacy s16 narrowing
001e8eb0  port  full-frame normal candidates before unchanged least-HP ranking
001e8fe0  port  full-frame normal candidates with original100000 exclusion before unchanged least-HP ranking
001e9110  port  full-frame normal candidates preserving2D flags and original least-HP tie order
001e0320  port  explicit-VM ally ANY base-repel query
001e03d0  port  explicit-VM opposite-side ANY base-repel query
001e0480  port  explicit-VM ally ANY base-drain query
001e0530  port  explicit-VM opposite-side ANY base-drain query
001e05e0  port  explicit-VM ally ANY base-null query
001e0690  port  explicit-VM opposite-side ANY base-null query
001e0740  port  explicit-VM ally ANY base-weak query
001e07f0  port  explicit-VM opposite-side ANY base-weak query
001e0fb0  port  ally base-repel ANY with original down exclusion
001e1060  port  opposite-side base-repel ANY with original down exclusion
001e1110  port  ally base-drain ANY with original down exclusion
001e11c0  port  opposite-side base-drain ANY with original down exclusion
001e1270  port  ally base-null ANY with original down exclusion
001e1320  port  opposite-side base-null ANY with original down exclusion
001e13d0  port  ally base-weak ANY with original down exclusion
001e1480  port  opposite-side base-weak ANY with original down exclusion
001e36c0  port  weak ANY query with barrier-before-known gating
001e3770  port  weak ANY query with down exclusion and known gating
001e3820  port  weak ANY query with require-down before known gating
001e3980  port  fixed-self normal query with unknown-first wrapper policy
001e3a30  port  normal ANY query with original unknown shortcut
001e3ae0  port  normal ANY query with defeated/down exclusion
001e3b90  port  normal ANY query with require-down and unknown shortcut
001e4600  port  fixed-self base-resistance query
001e46b0  port  ally base-resistance ANY query
001e4760  port  opposite-side base-resistance ANY query
001db5e0  port  canonical status-normal predicate facet preserves original literal profile policy and full source frame
001db900  port  canonical BASE resistance predicate facet, never inverted into wrapper normal
001def60  port  actual default-basic setter preserves literal8000 and actor-owned basic kind; no paid replacement
001defa0  port  actual encoded command argument0 commits this VM's immutable returned command/frame/query selection
001df040  port  item-granted skill setter binds actual decoded kind3 skill, never treats item id as a skill tag
001df0c0  port  explicit8001 nonattack setter supersedes prior frame/choice metadata
001df100  port  explicit8002 nonattack setter supersedes prior frame/choice metadata
001df140  port  explicit subword/8002 setter supersedes prior attack metadata at actual decode
001df1a0  port  explicit kind9 plan setter clears attack metadata, retaining original plan handling
001df230  port  explicit kind9 plan-with-argument setter clears attack metadata, retaining original plan handling
001df2d0  port  explicit8007 setter supersedes prior frame/choice metadata
001ea220  port  explicit kind9 script action-parameter setter clears only caller-owned attack metadata
001ea2d0  port  actual child-initialization command setter consumes command argument1, not numeric-equal child argument0
001e6a50  port  AI script usable skill query; explicit registered/active predicates in mirror_fixes
001da930  port  condition skill availability; explicit active predicate in mirror_fixes
001e9350  port  battle skill command list; explicit active predicate in mirror_fixes
001e9950  port  original single/group HP scoring carries exact frame, target-local prediction and command return; port coordinates existing skill active predicate facet; canonical amount bridge owned by port
00205170  port  actor-aware battle skill-row icon UI; port coordinates existing skill ID/active/cost/name/description facets
001138c0  port  shared descriptor skill icon: expanded-cohort element art; exact retail call kept on the retail route
00138bf0  port  Camp skill-list element art before the legacy label lookup; retail lookup, coordinates, alpha and scale kept
00116820  port  persona/compendium affinity row: explicit expanded-cohort columns (overrides/00116820.c); retail loop and status layout kept
00219790  port  Analyze affinity row only: explicit expanded-cohort columns with canonical profile affinity and knowledge; retail loop kept. port: ailment page via separate draw (p4_game_elements_ui_native_analyze_ailments) while Square's page is selected
001a24b0  port  btlActionAnalyzeUpdate: Square flips the Analyze row to the target's ailment page (only with ailment columns); entry and target switch reset to the affinity page; mirror_fixes
001efd50  port  target skill classification; explicit active predicate in mirror_fixes; port side-ailment facet (port): an ally losing its turn to a side ailment cannot shield the protagonist (mirror_fixes)
00199d00  port  native opening-motion presentation leaf from the owning frame; port coordinates existing skill active predicate facet
001a7720  port  native skill-update presentation route: formation, target cue and hit-reaction motion use the frame leaf or its declared retail family
00201b00  port  native hit-display ailment class from the owning frame; a new leaf never reaches the legacy element bit
001a3df0  port  native follow-up availability scan over the owning frame (barrier class, null/repel/drain); retail 001d94d0 call unchanged when routing is off
001a43a0  port  native follow-up target filter uses the owning frame affinity, never the legacy element alias
002266b0  port  battle skill voice classification; explicit active predicate in mirror_fixes
0010f8c0  port  skill target property byte; explicit active guard in mirror_fixes
00232a40  port  skill availability; explicit active predicate in mirror_fixes
00238940  port  native status/KO affinity route and the landed-ailment knowledge observe (TablesEnemyKnowledge3's store); port coordinates existing skill registered predicate facet; port facet: the rolled chance passes the status/KO chance stage before the 0..99 clamp and RpRandom; port facet: the stage reports the target's afflictions this battle
001f6930  port  native per-turn ailment recovery decision (btlSoundSelectCureableStatuses); each eligible roll's chance passes the ailment wear-off stage before datCalcRandomBelow(100)
00233bc0  port  native per-stat buff multiplier (datCalcBuffMultiplier); attack, magic, evasion, defense and accuracy pass the buff stage after the rank table, Charge/Concentrate and Lead boost
00232680  port  ailment landing (datCalcSetBadStatus); a landing rage, fear, silence, confusion, poison, exhaustion or enervation counts toward the unit's afflictions this battle while a chance or wear-off modifier is registered
0021e050  port  results-screen battle money (btlXpBaseAward); the team's yen passes the reward stage as btlXpApplyAwards pays it
0021e110  port  battle EXP awards (btlXpPrepareAwards); each recipient's EXP (hero, party member, the protagonist's Personas) passes the reward stage before the level-up preview; port facet: an inactive Persona's Growth also counts when granted to it or the protagonist, once with its slots
0021e9a0  port  battle award payment (btlXpApplyAwards); the team's yen passes the reward stage before datAddMoney; port facet reports the applied awards after money and items are given
002397d0  port  skill status result domain; explicit active predicate in mirror_fixes; port side-ailment facet (port): the single-slot rule (mirror_fixes)
00239e40  port  skill cure domain; explicit active predicate in mirror_fixes
00239f50  port  skill application domain; explicit active predicate in mirror_fixes
0023a1e0  port  skill effect domain; explicit active predicate in mirror_fixes
0023a490  port  skill outcome domain; explicit active predicate in mirror_fixes
0023d8e0  port  native effective element and actor substitution; port coordinates existing skill active predicate facet
0023d9b0  port  skill cost domain; explicit active predicate in mirror_fixes; port enemy-ID bank bound facet; port facet: a nonzero cost passes the skill cost stage after the halving and minimum; port facet: HP/SP cost halving (0x20B/0x20A) also sees the unit's passive grants (mirror_fixes)
0023ddc0  port  skill threshold domain; explicit registered predicate in mirror_fixes
0023df70  port  skill flags domain; explicit active predicate in mirror_fixes
0023e6f0  port  native effective element/hit route; port coordinates existing skill active predicate facet; port facet: a rolled hit's chance passes the hit stage after the 50..99 clamp; side-ailment facet (port): a lost-turn side ailment cannot evade, as Dizzy
002411a0  port  native effective element/hit result route; port coordinates existing skill active predicate facet; port facet: the critical chance passes the hit stage after its 0..100 clamp; side-ailment facet (port): held ailment's crit bonus on an unresisted physical roll
00241bc0  port  native effective element/applicability route; port coordinates existing skill active predicate facet
00241de0  port  native effective element/mode route; port coordinates existing skill active predicate facet
00241f00  port  support skill hit domain; explicit active predicate in mirror_fixes
00242360  port  skill chance domain; explicit active predicate in mirror_fixes; port enemy-ID bank bound facet; port facet: the ailment boost (0x21C) also sees the source's passive grants (mirror_fixes); port side-ailment facet (port): no Dizzy while a side ailment holds (mirror_fixes)
00242990  port  native Light/Dark skill substitution family route and enemy-ID bank bounds; port coordinates existing skill active predicate facet; port facet: each of the eight passive searches ends with the unit's passive grants
00243650  port  skill value domain; explicit active predicate in mirror_fixes; port enemy-ID bank bound facet
00243fa0  port  skill type domain; explicit registered/active predicates in mirror_fixes
002464e0  port  calculation script skill flag; explicit active predicate in mirror_fixes
002019f0  port  skill panel name lookup through canonical skill accessor; unchanged u32 encoded ID
00201f60  port  packet skill panel name lookup through canonical skill accessor
00202340  port  effect counter skill panel name lookup through canonical skill accessor
0013b420  port  skill card description through owned native MSG text; retail bank recipe unchanged
00114e50  port  selected persona skill help through owned native MSG text; caller passes selected skill ID
001f14f0  port  native action/element resolution and expanded enemy-knowledge gate through the knowledge owner's observe API; brackets each resolved hit's HP amount for the final-damage stage (aimed-at unit, repel); passes the target's rolled hit count to that HP amount so effect kind 14 (table-delta split) divides by the hits actually rolled; port coordinates endurance sentinel4096 when expanded, original576 otherwise; port facet: the bracket also carries the command (item, weapon attack, skill) for the damage and recovery stages; side-ailment facet (port): rolls riders after each hit's retail statuses (mirror_fixes)
00313690  port  expanded skill inheritance/mutation grade; unchanged signed retail selector byte
00304410  port  fusion mutation catalog enumeration without 576-row scratch bound; original retail RNG path retained
00312c60  port  inherited skill weights use explicit expanded rate and bounded inheritance group, never full-width element as19-column index
00193a80  port  actual packed-table load start resets only staged skill preparation state, including reloads
00104c50  port  original ITEMTBL builtin publication; retain an already-active immutable base on reload; first expansion only at canonical completion
00105dc0  port  expanded equipment reference side table; preserve retail signed-halfword stores
00106600  port  expanded inventory quantities outside the fixed retail save array
00106620  port  expanded quantity setter; preserve retail truncation/clamp and quest-flag side effects
001067f0  port  expanded-ID owned native names; retail string pointer banks untouched
00106c80  port  metadata-based equipment band for expanded IDs; preserve signed retail comparisons
00106cd0  port  expanded equipment side-reference read before original retail halfword load; one canonical signed original body also serves pure exact-Items actor previews without native source getters
00106d40  port  alternate equipment setter shares the same expanded side references
00110810  port  expanded item availability/state writes outside the retail byte array
00110830  port  expanded item availability/state reads; preserve recovered u8 result ABI
002e24a0  port  global item-list allocation grows output and sort scratch together; exact retail work size retained
002e2590  port  task-owned item-list allocation shares the dynamic native capacity
002e2a10  port  registered-row item-list rebuild with original filtering, state flags and retail qsort
002e3560  port  task-owned item-list rebuild uses the same native consumer implementation
002bc0e0  port  expanded single/paired item descriptions; preserve complete retail bank switch and quantity-derived case
002bc4b0  port  expanded item/category descriptions use native MSG glyphs; exact retail banks retained
002d1590  port  ID-aware inventory/equipped detail queue calls in scoped mirror_fixes entries
002dd3b0  port  new gear detail dispatch uses declared kind; all five description bank-offset consumers retain original IDs
0033c490  port  item-shop detail queue retains expanded ID before retail bank arithmetic
002bdb50  port  shop controller appends metadata-kind-based expanded stock defaults; retail sentinel ranges unchanged
002bdea0  port  clear new equipment/consumable stock flags without indexing retail key-item/global flag banks
002e71c0  port  date-unlock enumeration includes registered new consumables without inventing another ID bank
002e7510  port  field shop setup shares the same expanded stock-default consumer
002011c0  port  append expanded item rows after the complete native battle UI task allocation; original destructor owns all bytes
00207140  port  battle item refresh enumerates declared IDs into bounded task-owned rows; mixed skill loop uses port active helper before its original flags01 check
002055d0  port  scoped mirror item ID/quantity reads select original or appended battle rows
002091f0  port  scoped mirror battle item confirmation reads the same expanded row storage
00232aa0  port  metadata-kind gate for new consumables; retain original granted-skill active flag checks and retail encoded range
002be530  port  scoped shop stock scans append declared equipment and read task-owned unlock rows/full-width recipe requirements
002ca830  port  scoped shop work allocation appends unlock rows without moving original fields or changing its free owner
002dfe00  port  scoped shop unlock cue reads appended rows and logical requirement IDs, not truncated packed dependencies
001312b0  port  scoped equipment submenu reset clears only its appended row scratch after the exact original1598-byte clear
00131730  port  equipment list enumerates declared compatible gear into Camp-task-owned expanded rows
00131a00  port  scoped equipment draw/detail reads share the expanded row base without moving inline retail fields
0013d5d0  port  scoped equipment confirmation selects the same expanded row shown by the native menu
002cdf80  port  scoped saved absolute shop cursors use appended signed16 state when expanded; retail signed-byte stores unchanged
002d8a60  port  scoped sliding-tab row draw reads full saved absolute cursor; visible-row scroll remains the original0..4 byte
00113280  port  scoped new-ID icon draw resolves pinned authored resources; complete retail flag/sprite-bank path remains unchanged
00332370  port  one shared material-message formatter accepts logical full-width dependencies and registered category names; legacy packed entry ABI retained
00332520  port  scoped material hints enumerate declared IDs into appended task rows; logical dependency ID/quantity consumers replace12-bit narrowing
003329e0  port  scoped material-dialogue allocation appends rows beyond the complete180c work; existing3329b0 destructor owns all bytes
002cb6c0  port  scoped purchase-row new-ID icons use the actual primitive queue and authored resource pins; retail banked glyphs unchanged
00230d30  port  scoped weapon attachment uses explicit final right/left native model identities for new equipment; exact retail lookup/path/free ownership retained
002308a0  port  scoped expanded Chie gear resolves its explicit physical model before unchanged native costume/calendar selection; persona model kinds 3 (battle) and 11 (facility) name an added persona's model source
0012e7c0  port  Camp inventory enumeration appends declared kind3+ items in task-owned rows; exact retail300..5ff quantity sequence retained
0012e9d0  port  scoped Camp inventory list/detail draw reads original or appended rows without moving animation fields
0013b9f0  port  scoped Camp inventory group scans share the expanded row base; original flag grouping/wrap preserved
0013c5a0  port  scoped item selection reads the actual expanded row before native action dispatch
0013c700  port  scoped item-use selection and quantity update use the same appended rows as display
00106820  port  scoped metadata pointer accessor requires registered new IDs before returning relocated rows
00106850  port  scoped kind getter validates new-ID membership; retail array expression unchanged
00106880  port  scoped flags getter validates new-ID membership before relocated row access
001068b0  port  scoped primary-value getter validates new-ID membership
001068e0  port  scoped secondary-value getter validates new-ID membership
00106910  port  scoped property getter validates new-ID membership
00106940  port  scoped middle-band getter validates new-ID membership
00106970  port  scoped last-band getter validates new-ID membership
001069a0  port  scoped granted-skill getter validates new-ID membership
001069d0  port  scoped item-value getter validates new-ID membership
00106a00  port  scoped accessory-percent getter validates new-ID membership
00106a30  port  scoped weapon-element getter validates new-ID membership without narrowing u16
00106a60  port  scoped category-mask getter validates new-ID membership
00106a90  port  scoped price getter validates new-ID membership
00106ac0  port  scoped available-month getter validates new-ID membership
00106af0  port  scoped available-day getter validates new-ID membership
00106b20  port  scoped first packed-requirement getter validates new-ID membership
00106b50  port  scoped second packed-requirement getter validates new-ID membership
00106c30  port  scoped category-flag membership getter validates registered new IDs before metadata indexing
00112610  port  scoped new-category badge draw uses its registered pinned resource; retail/noncustom banked frames remain unchanged
002de5a0  port  scoped equipped-item comparison icons resolve side references and authored art; native category selection controls remain intact
002df020  port  scoped quantity-popup item icons retain stable IDs before retail atlas classification
00109e30  port  scoped overlay-first accessory-ID read only; preserve original hero/party physical reads without an overlay; all persona arithmetic remains the future persona owner's scope
002d5040  port  scoped selected/equipped stats-header icons use authored new-ID resources; retail getter/sprite paths and stat arithmetic stay untouched
00235520  port  native element/action amount route, enemy-ID bank bound and the final-damage stage (ordered registered modifiers; Hardmode and Challenge register, never hook); port coordinates existing registered/active skill predicate facet; port facet: party-source persona fields and the recovery stage for formula heals
00244110  port  canonical physical formula shared by original ABI and explicit owning frame/target-query calculation; preserve every term/order/conversion
00244540  port  canonical magic formula shared by original ABI and explicit owning frame/target-query calculation
002447c0  port  canonical power-plus-magic-tier formula with explicit calculation frame transport
00244870  port  canonical hit-rate formula with explicit calculation frame transport; no new preview/policy math
002449c0  port  canonical scaled luck ratio with explicit calculation frame transport
00244ad0  port  canonical pass-term formula with explicit calculation frame transport and original required source predicate
00244b10  port  canonical weighted ratio with explicit calculation frame transport
00244c10  port  canonical inverse status ratio with explicit owning status rider frame
00244cf0  port  canonical scaled physical formula with explicit owning frame/target-query transport
00244e20  port  canonical status ratio of products with explicit owning status rider frame
00244f60  port  original script operand/result stores plus explicit local owning frame/query dispatch; no global last-action slot
00245030  port  native calculation-script element route; port coordinates existing skill active predicate facets
00231af0  port  datUnitInit enemy-ID bound follows the published enemy banks (mirror_fixes); port override facet: an enemy's starting HP/SP pass the unit stage, the override keeps the published bound
00231f80  port  enemy base HP bank bound (mirror_fixes); port override facet: max HP passes the unit stage after the party cap, the override keeps the published bound
00232290  port  enemy base SP bank bound (mirror_fixes); port override facet: max SP passes the unit stage after the party cap, the override keeps the published bound
00232b40  port  enemy stat bank bound (mirror_fixes)
00232c70  port  battle stat (datCalcGetBattleStat); each of the five stats passes the unit stage before Enervation halves it
00235320  port  enemy threshold flag bank bound (mirror_fixes)
0023a6b0  port  enemy affinity id2 bank bound (mirror_fixes)
0023e140  port  enemy skill-list bank bound (mirror_fixes)
0023e2f0  port  enemy bank bound (mirror_fixes)
0023e3e0  port  enemy property bank bound (mirror_fixes)
0023e5b0  port  enemy skill-list bank bound (mirror_fixes)
00243950  port  enemy EXP bank bound (mirror_fixes)
002439c0  port  enemy bank bound (mirror_fixes)
00243a30  port  enemy bank bound (mirror_fixes)
00243ce0  port  enemy bank bound (mirror_fixes)
00243d80  port  enemy MODEL1 bank bound (mirror_fixes)
001f2cc0  port  battle reward enemy-ID bound and scoped defeated-enemy EXP/yen calculator calls (mirror_fixes)
0010f3d0  port  saved knowledge bitmap set limited to retail enemies (mirror_fixes)
0010f420  port  saved knowledge bitmap test limited to retail enemies (mirror_fixes)
0010f460  port  saved collection bits limited to retail enemies (mirror_fixes)
0019b550  port  enemy and persona model-load packet uses an added enemy's or persona's declared retail model source (mirror_fixes)
00219130  port  Analyze view table row follows an added enemy's model source (mirror_fixes)
00109220  port  persona MSG3 name row bound follows the published persona banks (mirror_fixes)
00109280  port  persona PERSONA0 arcana bound follows the published persona banks (mirror_fixes)
00109510  port  persona learnable-skill bound follows the published persona banks (mirror_fixes)
001097c0  port  persona skill-slot bound follows the published persona banks (mirror_fixes)
00109870  port  persona skill-slot bound follows the published persona banks (mirror_fixes)
0010b190  port  persona roster insertion bound follows the published persona banks (mirror_fixes)
0010be60  port  persona growth delta bound follows the published persona banks (mirror_fixes)
0010c5a0  port  persona level-up apply bound follows the published persona banks (mirror_fixes)
0010c750  port  persona base-stat bound follows the published persona banks (mirror_fixes)
0010cad0  port  datPersonaInit bound follows the published persona banks (mirror_fixes)
0011d1f0  port  persona status portrait path (pst_%03x) uses an added persona's declared retail model source (mirror_fixes)
0010fc80  port  compendium clear also clears added personas' rows (mirror_fixes)
0010fcb0  port  compendium lookup uses the persona's own row, added personas included (mirror_fixes)
0010fd40  port  compendium registration writes the persona's own row (mirror_fixes)
0010fde0  port  compendium resolve/seed uses the persona's own row (mirror_fixes)
0010ffa0  port  register-all covers every persona the published banks hold (mirror_fixes)
00110140  port  compendium completion counts added personas (mirror_fixes)
002e4090  port  compendium list scans every persona the published banks hold (mirror_fixes)
002e4610  port  persona-list work sized for every persona the published banks hold (mirror_fixes)
002e5960  port  persona-list clear covers the sized work (mirror_fixes)
002e68b0  port  persona-list sort scratch covers IDs up to 511 (mirror_fixes)
002e6c90  port  persona-list sort scratch covers IDs up to 511 (mirror_fixes)
00310700  port  compendium snapshot appends added personas' rows (mirror_fixes)
00310850  port  compendium snapshot restore covers added personas' rows (mirror_fixes)
003130e0  port  normal-fusion result scan (at-least) also visits added personas (override)
003133b0  port  normal-fusion result scan (at-most) also visits added personas (override)
0036ee60  port  shuffle-time persona pools include added personas (mirror_fixes)
0036c7e0  port  persona card image of an added persona is its model source's (mirror_fixes)
002ecfc0  port  fusion command rows: a fusion kind is omitted only when a p4.authorize "fusion" authorizer denies it; exact retail rows otherwise
001b1d70  port  battle party/enemy action creation: a p4.battle.formation projector may replace the player-side lineup; exact retail loop over the encounter party otherwise
00192b90  port  btlInit: side-ailment state begins with the battle (mirror_fixes)
001b05d0  port  btlActionExitUpdate: a unit's side-ailment state ends before its battle unit is freed (mirror_fixes)
00192cd0  port  btlRelease: every side ailment ends with the battle (mirror_fixes); port' battle_end hook route is unchanged
001f6770  port  btlSoundRollStatusCure: a lost-turn side ailment sends the turn to the bad state (mirror_fixes; port facet)
001a31e0  port  btlActionBadUpdate: a lost-turn side ailment spends the turn before Down recovery and recovers at its end (mirror_fixes; port facet)
001f2eb0  port  btlTargetSetDefeatState: a defeated unit loses its side ailment (mirror_fixes; port facet)
0019ff60  port  btlActionCanFollowUpAfterEnemiesDown: a unit losing its turn to a side ailment neither starts nor joins an All-out Attack (mirror_fixes; port facet)
001ef9c0  port  btlActionPickStatusTarget: a Social Link helper losing its turn to a side ailment does not help (mirror_fixes; port facet)
001eff50  port  btlActionPickCounterTarget: follow-ups need attacker and ally not losing their turn to a side ailment (mirror_fixes; port facet)
001d9b60  port  btlCond_MYBAD (FRBAD/ENBAD): an every-ailment query sees a side ailment (mirror_fixes; port facet)
001d9ca0  port  btlCond_ENBAD_ALL: an every-ailment query sees a side ailment (mirror_fixes; port facet)
001e2550  port  btlAICommandSideIdQueryAllBadStatus: an every-ailment query sees a side ailment (mirror_fixes; port facet)
001e26c0  port  btlAICommandOppSideIdQueryAllBadStatus: an every-ailment query sees a side ailment (mirror_fixes; port facet)
001e9770  port  btlEffectScriptPickLowestHpInRangeUnit: an every-ailment pick sees a side ailment (mirror_fixes; port facet)
001e9f20  port  btlEffectScriptPickHighestHpBadStatusUnit: an every-ailment pick sees a side ailment (mirror_fixes; port facet)
001ea050  port  btlEffectScriptPickLowestHpBadStatusUnit: an every-ailment pick sees a side ailment (mirror_fixes; port facet)
001d69f0  port  btlFormationFormatName: an added skill's battle effect file (skill/%03X.BED) is its effect source's (mirror_fixes)
001f7d90  port  btlSoundSkillSEInit: an added skill's sound bank (skill/%03X.se) is its effect source's (mirror_fixes)
001f7e30  port  btlSoundSkillSEUpdate: the same sound-bank name on a deferred load (mirror_fixes)
002e7ac0  port  Velvet Room menu dispatcher: a p4.authorize "fusion" denial of kind 6 (roster pair fusion, flag 0x1305) returns its entry to the root menu; exact retail dispatch otherwise. + port: a p4.velvet.command (mask test) draws its hint at the idle three-row root (f1 0x18) and opens its page on Square in the Velvet task's own Config child (f0 0x40, unused by retail) until that page closes
0047b0c0  port  mdlReadAuxChunks: unchanged retail RMD chunk reader plus the [resource-model] glTF override seam (overrides/0047b0c0.c, src/platform/models/p4model_build.c); an empty override table takes the retail path
001747d0  port  FldEvent_TriggerUpdate: a port field host slot after the eleven retail trigger pairs (the give.skill choice panel); exact retail controller with no host request
002eb270  port  fclCombineLayoutTransition: the entry lets the native-rows service add or drop a mod card; a list with one is laid out for any count up to ten (p4VelvetLayout); exact retail otherwise
002e90d0  port  fclCombineMenuRoot: card-list rows (0x1A, 0x1C) read through the native-rows side row, the mod card hosts its page in sub-state 0xF1; exact retail rows otherwise
00317900  port  fclCombineDrawEntryCell: entry 9 (the mod card) draws from the native-rows slots; entries 0..8 exactly retail
00318840  port  fclCombineUpdateEntryHighlight: entry 9 from the native-rows slots and extended pack; entries 0..8 exactly retail
00318f30  port  fclCombineMarkRow: row 9 marks the mod card's symbol pair; rows 0..8 exactly retail
003190d0  port  fclCombineRowsBusy: rows read through the native-rows side row; exact retail rows otherwise
001e6d20  port  battle script pick-by-character-id: an absent protagonist's action answers with the leader action the battle recorded, and any other failed lookup publishes -1 instead of faulting; exact retail lookup and result otherwise
001e1530  port  battle AI side-id HP query: with no protagonist action on the player side, his character-1 request asks about the leader-role action's character through the same retail lookup and condition; exact retail lookup and answer otherwise
001e1800  port  battle AI side-id bad-status query: with no protagonist action on the player side, his character-1 request asks about the leader-role action's character through the same retail lookup, status filter and condition; exact retail lookup and answer otherwise
001a06d0  port  turn start: count the selected leader's eligible ordinary turns for Shuffle Time when Yu is absent; retain the retail protagonist count when present; port side-ailment facet (port): btlActionStartInit starts a turn with no earlier side-ailment landing mark (mirror_fixes)
