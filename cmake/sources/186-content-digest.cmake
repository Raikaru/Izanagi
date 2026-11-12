# Shared native SHA only: no package loader, renderer or capture worker.
target_sources(p4port PRIVATE src/platform/content_digest.c)
