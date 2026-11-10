#ifndef P4PC_PLATFORM_FILES_ADXF_HOST_H
#define P4PC_PLATFORM_FILES_ADXF_HOST_H

#include <stddef.h>
#include <stdint.h>

/* PC-speed VFS window handles. Accepted reads remain busy until the next
 * scheduler frame boundary; CRI/ROFS callers retain their normal publication
 * steps. Volume 0 is the main volume and volume 1 is BTL. */
typedef struct {
    signed char used;
    signed char stat;
    short pad;
    int fd;
    long long base;
    long long size;
    long long pos;
    int completion_alarm;
} P4AdxfHandle;
void p4pc_cdvd_set_dir(const char *dir, int volume);
const char *p4pc_cdvd_dir(int volume);
const char *p4pc_cdvd_dir_of_blob(const void *blob, int *volume);
void *func_004c85a0(const char *name, int mode);
void func_004c8a60(void *handle);
int func_004c9670(void *handle);
int func_004c95f8(void *handle);
int func_004c9010(void *handle, int sectors, void *buffer);
int func_004c9820(void *handle);
void *func_004f8518(void);
void *func_004f8548(void);
void func_004f2408(void);
int func_004f2460(const char *fname);
int func_004f2548(const char *fname, int *size);
int func_004f2948(const char *fname);
void *func_004f2260(const char *fname, void *dir, int rw);
void func_004f2370(void *handle);
int func_004f85c0(void *handle, int pos, int type);
int func_004f1a08(void *handle);
int func_004f2398(void *handle, int sectors, void *buffer);
void func_004f23e0(void *handle);
int func_004f8648(void *handle);
int func_004f1ce0(void *handle);
int func_004f86c8(const char *dirname);
int func_004f28b0(const char *name, void *info, int count);
int func_004f8848(void *handle, int option, int a, int b);
void p4pc_adxf_cancel(P4AdxfHandle *h);
void func_004c7260(int command, int phase, int a, int b, int c);
int func_004c7238(int bytes);
int func_004c7340(unsigned int ptid, void *ptinfo);
int p4pc_adxf_resolve_name(const char *name, const void *blob, char *out, size_t cap);
void *func_004c8770(const char *fname, void *dir, int start, int length);
void func_004c8a90(void *handle);
int func_004c9888(unsigned int ptid, int member);
int func_004c9a28(int ptid, int member, char *name, int *dir, int *start, int *length);
void *func_004c9bb8(int ptid);
void *func_004c8950(int ptid, int member);
int func_004c9858(void *handle);
int p4pc_adxf_read(P4AdxfHandle *h, int sectors, void *buffer);
int func_004c9068(void *handle, int sectors, void *buffer);
int func_004c7e28(void);
int func_004c7b80(int ptid, int parent, int member, const char *fname, void *dir, int start, int length,
                  unsigned char *ptinfo, unsigned char *work, int work_size, int type);
int func_004c78c8(int ptid, const char *fname, void *dir, unsigned char *ptinfo, unsigned char *work, int work_size);
int func_004c7560(int ptid, const char *fname, void *dir, unsigned char *ptinfo);
int func_004c74f8(int ptid, const char *fname, void *dir, unsigned char *ptinfo);
int func_004c7f80(int ptid);
int func_004c7f30(int ptid);
int func_004c7ef8(int ptid);
int func_004c6ee0(void);
int func_004c6f30(void);
int func_004c6eb0(void);
int func_004c6f08(void);
int func_004c6fa8(const char *name, const char *image, void *arg);
int func_004c6f50(const char *name, const char *dir, void *arg);
void func_004c71b8(const char *name);
int func_004c7188(const char *name);
#endif
