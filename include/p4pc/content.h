/* Built-in content services: no Lua runtime or enabled mod is required. */
#ifndef P4PC_CONTENT_H
#define P4PC_CONTENT_H
/* Called before asset reads. With no P4PC_TEXTURE_DUMP it performs no discovery,
 * filesystem access, allocation or worker creation. Returns 0/-1. */
int p4_content_init(void);
/* Native Config/tools use the same dump service. Empty root is disabled. */
int p4_content_dump_start(const char *root);
/* Joins the writer, unregisters native observers, releases attribution metadata.
 * Returns -1 for an incomplete dump; safe after an explicit earlier finish. */
int p4_content_finish(void);
#endif
