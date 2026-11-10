/* kwlnMainLoop (00101350): same native initialization, real seed routines,
 * and update order. Tool observation is inactive in ordinary game launches.
 * The boundary is kwlnRunFrame, never raster presentation or guessed turns. */
extern int p4_battle_sim_active;
extern void p4_battle_sim_before_update(void);
extern void p4_battle_sim_after_update(int completed);

void kwlnMainLoop(void)
{
    kwlnInitIop();
    kwlnInitEngine();
    kwlnInitSubsystems();
    /* Overrides follow mirror fixes: preserve the accepted TIMER0 boot seam. */
    RpRandomSeed(p4pc_timer0_boot_count());
    RpRandomSeedMT(p4pc_timer0_boot_count());
    for (;;) {
        s32 completed;
        if (p4_battle_sim_active) p4_battle_sim_before_update();
        completed = kwlnRunFrame();
        if (p4_battle_sim_active) p4_battle_sim_after_update(completed);
    }
}
