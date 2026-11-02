/* Native content-service lifecycle. The package/VFS service selects providers;
 * this observer only retains their existing identity for asset importers. */
#include "p4pc/content.h"
#include "p4pc/content_source.h"
#include "p4pc/vfs_origin.h"
#include "p4pc/platform.h"
#include "p4pc/guest.h"
#include "../render/p4gl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int initialized, active, exit_registered, result;

static void observe_read(const P4VfsOrigin *origin, const void *bytes, size_t size,
                          int64_t file_offset, int64_t disc_offset, void *user)
{
    P4ContentSource source;
    (void)file_offset; (void)disc_offset; (void)user;
    /* These snapshots can later be verified safely only while the storage is
     * managed by the guest heap or an explicit native importer lifetime. */
    if (!p4_guest_asset_buffer_contains(bytes,size) && !p4_content_host_read_contains(bytes,size)) {
        p4_content_source_forget(bytes,size);
        return;
    }
    memset(&source,0,sizeof source);
    source.origin=origin->kind==P4_VFS_DISC?P4_TEXTURE_DISC:P4_TEXTURE_OVERRIDE;
    source.rank=origin->rank;
    snprintf(source.provider,sizeof source.provider,"%s",origin->provider?origin->provider:"");
    /* Raw sector reads intentionally have no invented container/file key. */
    snprintf(source.key,sizeof source.key,"%s",origin->key?origin->key:"");
    p4_content_source_mark(bytes,size,&source);
}
static void finish_at_exit(void) { p4_content_finish(); }

int p4_content_dump_start(const char *root)
{
    if (!root || !*root) return 0;
    if (active) return -1;
    if (p4_content_sources_enable(1,p4_guest_asset_buffer_contains)!=0) goto fail;
    if (p4pc_heap_observer(p4_content_source_forget)!=0) goto source_fail;
    if (vfs_read_observer_register(observe_read,NULL)!=0) goto heap_fail;
    if (p4_texture_dump_start(root,0)!=0) goto observer_fail;
    if (!exit_registered) {
        if (atexit(finish_at_exit)!=0) {
            p4_texture_dump_finish();
            goto observer_fail;
        }
        exit_registered=1;
    }
    active=1; result=0;
    return 0;
observer_fail:
    vfs_read_observer_remove(observe_read,NULL);
heap_fail:
    p4pc_heap_observer(NULL);
source_fail:
    p4_content_sources_enable(0,NULL);
fail:
    result=-1;
    fprintf(stderr,"content: cannot activate original texture dumping; previous observers retained on conflict\n");
    return -1;
}
int p4_content_init(void)
{
    if (initialized) return result;
    initialized=1;
    return p4_content_dump_start(getenv("P4PC_TEXTURE_DUMP"));
}
int p4_content_finish(void)
{
    if (!active) return result;
    result=p4_texture_dump_finish();
    if (vfs_read_observer_remove(observe_read,NULL)!=0) result=-1;
    if (p4pc_heap_observer(NULL)!=0) result=-1;
    p4gl_raster_content_reset();
    p4_content_sources_enable(0,NULL);
    active=0;
    return result;
}
