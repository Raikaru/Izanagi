#include "p4pc/golden_encounter_rates.h"
int32_t p4_golden_encounter_total(const uint8_t *row, unsigned d, uint16_t mode, int exclude, int boost)
{
    if (!row || d>1) return 0;
    int n=(int8_t)row[d], s=(int8_t)row[3+d], r=(int8_t)row[6+d];
    int m=boost==1 ? 100 : 1;
    if (mode==3) return m*s ? m*s : n;
    if (mode==4) return r ? r : n;
    return n + (exclude ? 0 : ((mode>0 ? m*s : 0) + (mode>=2 ? r : 0)));
}
int32_t p4_golden_encounter_projected_total(const uint8_t *row, unsigned d, uint16_t level,
    int exclude, int boost, int suppress)
{
    if (!row || d>1) return 0;
    int m=suppress ? 0 : (boost==1 ? 100 : 1);
    return (int8_t)row[d] + (exclude ? 0 : ((level>0 ? m*(int8_t)row[2+d] : 0) +
                                          (level>=2 ? (int8_t)row[4+d] : 0)));
}
int p4_golden_encounter_matrix(const uint8_t *raw, size_t size, const uint8_t *projected, size_t bytes,
    unsigned *golden_cells, unsigned *projected_cells)
{
    if (!raw || size!=68u*252u || !projected || bytes!=68u*248u || !golden_cells || !projected_cells) return -1;
    unsigned gc=0, pc=0;
    static const uint16_t modes[6]={0,1,2,3,4,65535};
    for (unsigned i=0;i<68;++i) for (unsigned d=0;d<2;++d) {
        const uint8_t *g=raw+i*252u, *p=projected+i*248u;
        for (unsigned c=0;c<3;++c) if (g[3*c+d]!=p[2*c+d]) return -1;
        /* Nonnegative signed rates plus positive normal are the pinned-data
         * premise, not a repair for invalid all-zero or negative input. */
        if ((int8_t)g[d]<=0 || (int8_t)g[3+d]<0 || (int8_t)g[6+d]<0) return -1;
        for (int f=0;f<2;++f) for (int b=0;b<2;++b) {
            for (unsigned mode=0;mode<6;++mode) for (unsigned retry=0;retry<2;++retry) {
                /* Retry is after drawing and cannot affect the total. E2
                 * liveness is deliberately not claimed by this matrix. */
                if (p4_golden_encounter_total(g,d,modes[mode],f,b)<=0) return -1;
                ++gc;
            }
            for (int e=0;e<2;++e) for (unsigned level=0;level<3;++level) {
                if (p4_golden_encounter_projected_total(p,d,(uint16_t)level,f,b,e)<=0) return -1;
                ++pc;
            }
        }
    }
    *golden_cells=gc; *projected_cells=pc; return 0;
}
