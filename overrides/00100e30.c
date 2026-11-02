/* PS2 interrupt-enable callback: calls func_004ccb38(0), then `sync; ei`. The port has
 * no interrupt controller, so the `sync; ei` pair is dropped. The callback still
 * registers through func_00418b18 (kwlnInitSubsystems passes it). */
s32 kwlnEnableInterrupts(void) {
    func_004ccb38(0);
    return 0;
}
