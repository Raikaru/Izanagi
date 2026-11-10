#ifndef P4PC_EE_DFLOAT_H
#define P4PC_EE_DFLOAT_H

/* Native floating-register ABI only. Raw retail GPR-bit entry declarations
 * live in ee_dlibm.h and must not collide with the decomp's u64 typedef. */
double p4pc_ee_dsqrt(double x);
double p4pc_ee_pow(double x, double y);
double p4pc_ee_log(double x);
double p4pc_ee_log10(double x);
double p4pc_ee_acos(double x);

#endif
