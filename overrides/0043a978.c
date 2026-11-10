/* libmc2 start-up (retail 0x0043a978): resets the IOP, loads the memory card modules and binds the
 * RPC channel to them (sceSifBindRpc retry loops). The PC port has no IOP; the card is the host folder
 * behind the sceMc2 model in src/platform/memcard/ps2_memcard.c, which needs no start-up beyond the
 * func_004316a8 / func_00431928 calls that memcardInitBackend makes next. */
void func_0043a978(void) {
}
