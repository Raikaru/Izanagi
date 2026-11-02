/* Local static-library configuration for miniz 3.0.2. */
#ifndef P4PC_MINIZ_EXPORT_H
#define P4PC_MINIZ_EXPORT_H
#define MINIZ_EXPORT
/* Only low-level codecs/checksums are used; never export zlib aliases or ZIP APIs. */
#define MINIZ_NO_ZLIB_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#define MINIZ_NO_ARCHIVE_APIS
#endif
