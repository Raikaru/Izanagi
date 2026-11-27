/* Which source tree a binary was built from: the git commit, and whether the tree had uncommitted changes.
 *
 * tools/build_id.py writes the one translation unit that defines these (a generated p4_build_id.c in the build's generated directory, listed
 * with the port sources by CMake and by the zig build). The id comes from the source tree and nothing else, never the time or the build
 * directory, so one commit built twice is byte-identical; a dirty tree is reported as such because the commit alone would not reproduce it.
 * It is printed in the first lines of p4pc.log and in every native crash report, so a log or a crash says which build it came from.
 * Outside a git tree (a CI container's checkout, which has no .git) the commit comes from P4PC_BUILD_ID or P4PC_CHECK_SHA, else "unknown". */
#ifndef P4PC_BUILD_ID_H
#define P4PC_BUILD_ID_H

/* "49eee0d109ab" (12 hex of the commit), "49eee0d109ab-dirty", or "unknown". */
const char *p4_build_id(void);
/* The full 40-hex commit, or "unknown". */
const char *p4_build_commit(void);
/* 1 when the tree had uncommitted changes (tracked edits, or untracked source files), else 0. */
int p4_build_dirty(void);

#endif
