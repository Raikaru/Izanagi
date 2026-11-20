/* SPDX-License-Identifier: MIT */
/* Izanagi RenderWare clean layer: axis-aligned bounding boxes (docs/rw_clean.md, phase 1).
 *
 * A box is its supremum (the per-axis maximum) followed by its infimum (the minimum). */
#include "rwc.h"

/* Starts the box at the first point and widens it by the others; an empty list leaves the box as it was. */
// FUN_003E5110
RwBBox *RwBBoxCalculate(RwBBox *box, const RwV3d *points, RwInt32 count)
{
    RwInt32 i;

    if (count <= 0)
        return box;
    box->sup = points[0];
    box->inf = points[0];
    for (i = 1; i < count; i++) {
        const RwV3d *p = &points[i];

        if (p->x > box->sup.x) box->sup.x = p->x;
        if (p->y > box->sup.y) box->sup.y = p->y;
        if (p->z > box->sup.z) box->sup.z = p->z;
        if (p->x < box->inf.x) box->inf.x = p->x;
        if (p->y < box->inf.y) box->inf.y = p->y;
        if (p->z < box->inf.z) box->inf.z = p->z;
    }
    return box;
}

RwBBox *RwBBoxInitialize(RwBBox *box, const RwV3d *point)
{
    box->sup = *point;
    box->inf = *point;
    return box;
}

RwBBox *RwBBoxAddPoint(RwBBox *box, const RwV3d *p)
{
    if (p->x > box->sup.x) box->sup.x = p->x;
    if (p->y > box->sup.y) box->sup.y = p->y;
    if (p->z > box->sup.z) box->sup.z = p->z;
    if (p->x < box->inf.x) box->inf.x = p->x;
    if (p->y < box->inf.y) box->inf.y = p->y;
    if (p->z < box->inf.z) box->inf.z = p->z;
    return box;
}

RwBool RwBBoxContainsPoint(const RwBBox *box, const RwV3d *p)
{
    return p->x >= box->inf.x && p->x <= box->sup.x && p->y >= box->inf.y && p->y <= box->sup.y && p->z >= box->inf.z
           && p->z <= box->sup.z;
}
