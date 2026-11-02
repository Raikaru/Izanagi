/* Registers (or, with 0, removes) the handler for INTC cause 2, VBLANK_START (retail 0x00418b18). The state
 * is two words in the block func_004176a0 returns: the current handler at +8 and the AddIntcHandler handle
 * at +12. A previous handler is disabled and removed first. Returns the previous handler. kwlnInitSubsystems
 * registers kwlnEnableInterrupts, the sound server's per-vblank callback. */
extern unsigned char *func_004176a0(void);
extern int func_004222c0(int cause);
extern int RemoveIntcHandler(int cause, int handle);
extern int func_004214c0(int cause, int (*handler)(int), int next);
extern int func_00422328(int cause);

int func_00418b18(int handler) {
    int *state = (int *)func_004176a0();
    int previous = state[2];

    if (handler == 0) {
        func_004222c0(2);
        RemoveIntcHandler(2, state[3]);
        state[2] = 0;
        state[3] = 0;
        return previous;
    }
    if (previous != 0) {
        func_004222c0(2);
        RemoveIntcHandler(2, state[3]);
    }
    state[2] = handler;
    state[3] = func_004214c0(2, (int (*)(int))handler, -1);
    func_00422328(2);
    return previous;
}
