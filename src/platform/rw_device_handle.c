/* RenderWare device-block accessor: retail 00401450 (`_rwDeviceGetHandle`, decomp
 * src/rw/basky.c). That unit is a sky2 unit, excluded by design, so the port cannot
 * compile the original. Without a definition the trap stub returns 0, RwEngineOpen
 * skips EngineOpen, REGISTER never fills the open device, and RwEngineSetVideoMode
 * calls through a NULL fpSystem. The device block itself is the retail static at
 * 0x0070C220 (D_0070C220), which the GL device layer (src/render/p4gl_device.c) serves. */
#include <stdint.h>

#include "p4pc/gmem.h"

void *func_00401450(void)
{
    return P4_G2H(0x0070C220u); /* D_0070C220 */
}
