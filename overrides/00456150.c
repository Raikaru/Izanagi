/* Native execution safepoint at the synchronous CDVD hardware-wrapper seam.
 * Retail can receive timer interrupts while this loop executes; native C has no
 * MMIO/kernel call here otherwise. Advance only the kernel/device clock: its
 * real alarm handlers and ready EE threads perform CRI service, not this wait.
 */
extern void p4pc_poll_devices(void);

void H_Cdvd_ReadSync(HCdvd *cdvd)
{
    while (true) {
        if (!H_Cdvd_IsFileLoaded(cdvd)) {
            H_Cdvd_UpdateRequests();
            p4pc_poll_devices();
        } else {
            break;
        }
    }
}
