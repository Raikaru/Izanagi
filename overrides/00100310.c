/* PS2 interrupt enable: `sync; ei`, plus a write to the INTC mask register at
 * 0x10000010 when the callback number is 9. Interrupts and the INTC belong to the
 * platform layer, and the PC port has neither, so this does nothing. The callback
 * still registers (kwlnInitIop passes this function to func_004214c0). */
s32 kwlnHandleTimerInterrupt(s32 arg0) {
    (void)arg0;
    return 0;
}
