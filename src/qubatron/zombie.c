#ifndef zombie_h
#define zombie_h

#include "mt_quat.c"
#include "mt_vector_4d.c"
#include "physics.c"
#include <string.h>

#define POINT_COUNT 20

typedef struct _zombie_parts_t
{
    v4_t head;
    v4_t neck;
    v4_t hip;
    v4_t lshol;
    v4_t rshol;
    v4_t lelbo;
    v4_t relbo;
    v4_t lhand;
    v4_t rhand;
    v4_t lleg;
    v4_t rleg;
    v4_t lknee;
    v4_t rknee;
    v4_t lfoot;
    v4_t rfoot;
} zombie_parts_t;

typedef struct _zombie_bones_t
{
    v4_t arr[POINT_COUNT];
} zombie_bones_t;

typedef struct _zombie_t
{
    zombie_bones_t oribones;
    zombie_bones_t newbones;
    zombie_parts_t newparts;

    v4_t lfp; // left foot pivot
    v4_t rfp; // right foot pivot
} zombie_t;

zombie_t       zombie_init();
void           zombie_update(zombie_t* zombie, float langle, float dir, v4_t pos);

#endif

#if __INCLUDE_LEVEL__ == 0

// points on original model
// x y z weight

zombie_parts_t zombie_parts = {

    .head = (v4_t){54.0, 200.0, 26.0, 20.0},
    .neck = (v4_t){54.0, 170.0, 26.0, 23.0},
    .hip  = (v4_t){52.0, 120.0, 26.0, 22.0},

    .lshol = (v4_t){33.0, 170.0, 26.0, 14.0},
    .lelbo = (v4_t){13.0, 110.0, 26.0, 14.0},
    .lhand = (v4_t){-10.0, 65.0, 28.0, 3.0},

    .rshol = (v4_t){75.0, 170.0, 26.0, 14.0},
    .relbo = (v4_t){85.0, 150.0, 26.0, 11.0},
    .rhand = (v4_t){120.0, 75.0, 32.0, 3.0},

    .lleg  = (v4_t){45.0, 120.0, 26.0, 20.0},
    .lknee = (v4_t){25.0, 60.0, 26.0, 26.0},
    .lfoot = (v4_t){19.0, -10.0, 17.0, 8.0},

    .rleg  = (v4_t){68.0, 110.0, 26.0, 20.0},
    .rknee = (v4_t){80.0, 60.0, 26.0, 26.0},
    .rfoot = (v4_t){92.0, -10.0, 17.0, 8.0}

};

zombie_t zombie_init()
{
    zombie_t res;

    res.oribones = (zombie_bones_t){
	.arr = {
	    zombie_parts.head,
	    zombie_parts.neck,
	    zombie_parts.neck,
	    zombie_parts.hip,
	    zombie_parts.rshol,
	    zombie_parts.relbo,
	    zombie_parts.relbo,
	    zombie_parts.rhand,
	    zombie_parts.lshol,
	    zombie_parts.lelbo,
	    zombie_parts.lelbo,
	    zombie_parts.lhand,
	    zombie_parts.rleg,
	    zombie_parts.rknee,
	    zombie_parts.rknee,
	    zombie_parts.rfoot,
	    zombie_parts.lleg,
	    zombie_parts.lknee,
	    zombie_parts.lknee,
	    zombie_parts.lfoot}};

    res.newbones = res.oribones;
    res.newparts = zombie_parts;

    return res;
}

void zombie_update(zombie_t* zombie, float lighta, float dir, v4_t pos)
{
    v4_t rot = quat_from_axis_angle(v3_normalize(v3_sub(v4_xyz(zombie->newparts.hip), v4_xyz(zombie->newparts.neck))), dir);

    v4_t ldir = v4_scale(v4_sub(zombie->lfp, zombie->newparts.lfoot), 0.2);
    v4_t rdir = v4_scale(v4_sub(zombie->rfp, zombie->newparts.rfoot), 0.2);

    v4_t hipv  = (v4_t){0.0, 120.0, 0.0, dir};
    v3_t headv = v4_xyz(v4_sub(zombie_parts.head, zombie_parts.hip));
    v3_t neckv = v4_xyz(v4_sub(zombie_parts.neck, zombie_parts.hip));

    v3_t rsholv = v4_xyz(v4_sub(zombie_parts.rshol, zombie_parts.hip));
    v3_t relbov = v4_xyz(v4_sub(zombie_parts.relbo, zombie_parts.hip));
    v3_t rhndv  = v4_xyz(v4_sub(zombie_parts.rhand, zombie_parts.hip));

    v3_t lhndv  = v4_xyz(v4_sub(zombie_parts.lhand, zombie_parts.hip));
    v3_t lsholv = v4_xyz(v4_sub(zombie_parts.lshol, zombie_parts.hip));
    v3_t lelbov = v4_xyz(v4_sub(zombie_parts.lelbo, zombie_parts.hip));

    v3_t rlegv  = v4_xyz(v4_sub(zombie_parts.rleg, zombie_parts.hip));
    v3_t rkneev = v4_xyz(v4_sub(zombie_parts.rknee, zombie_parts.hip));

    v3_t llegv  = v4_xyz(v4_sub(zombie_parts.lleg, zombie_parts.hip));
    v3_t lkneev = v4_xyz(v4_sub(zombie_parts.lknee, zombie_parts.hip));

    headv.z += sinf(lighta) * 5.0;
    neckv.z += cosf(lighta) * 5.0;

    rsholv = quat_rotate(rot, rsholv);
    rsholv.z += cosf(lighta) * 5.0;
    relbov = quat_rotate(rot, relbov);
    rhndv.z += sinf(lighta) * 20.0;
    rhndv.y += sinf(lighta) * 20.0;

    lsholv = quat_rotate(rot, lsholv);
    lsholv.z += cosf(lighta) * 5.0;
    lelbov = quat_rotate(rot, lelbov);
    lelbov.z += cosf(lighta) * 5.0;
    lhndv.z += sinf(lighta) * 20.0;
    lhndv.y += sinf(lighta) * 20.0;

    rlegv  = quat_rotate(rot, rlegv);
    rkneev = quat_rotate(rot, rkneev);

    llegv  = quat_rotate(rot, llegv);
    lkneev = quat_rotate(rot, lkneev);

    zombie->newparts.hip  = v4_add(pos, hipv);
    zombie->newparts.head = v4_add(zombie->newparts.hip, v4_xyzw(headv));
    zombie->newparts.neck = v4_add(zombie->newparts.hip, v4_xyzw(neckv));

    zombie->newparts.rshol = v4_add(zombie->newparts.hip, v4_xyzw(rsholv));
    zombie->newparts.relbo = v4_add(zombie->newparts.hip, v4_xyzw(relbov));
    zombie->newparts.rhand = v4_add(zombie->newparts.hip, v4_xyzw(rhndv));

    zombie->newparts.lshol = v4_add(zombie->newparts.hip, v4_xyzw(lsholv));
    zombie->newparts.lelbo = v4_add(zombie->newparts.hip, v4_xyzw(lelbov));
    zombie->newparts.lhand = v4_add(zombie->newparts.hip, v4_xyzw(lhndv));

    zombie->newparts.rleg  = v4_add(zombie->newparts.hip, v4_xyzw(rlegv));
    zombie->newparts.rknee = v4_add(zombie->newparts.hip, v4_xyzw(rkneev));
    zombie->newparts.rfoot = v4_add(zombie->newparts.rfoot, rdir);

    zombie->newparts.lleg  = v4_add(zombie->newparts.hip, v4_xyzw(llegv));
    zombie->newparts.lknee = v4_add(zombie->newparts.hip, v4_xyzw(lkneev));
    zombie->newparts.lfoot = v4_add(zombie->newparts.lfoot, ldir);

    // move lfoot and rfoot towards pivots, move other points according to these positions

    zombie->newbones = (zombie_bones_t){
	.arr = {
	    zombie->newparts.head,
	    zombie->newparts.neck,
	    zombie->newparts.neck,
	    zombie->newparts.hip,
	    zombie->newparts.rshol,
	    zombie->newparts.relbo,
	    zombie->newparts.relbo,
	    zombie->newparts.rhand,
	    zombie->newparts.lshol,
	    zombie->newparts.lelbo,
	    zombie->newparts.lelbo,
	    zombie->newparts.lhand,
	    zombie->newparts.rleg,
	    zombie->newparts.rknee,
	    zombie->newparts.rknee,
	    zombie->newparts.rfoot,
	    zombie->newparts.lleg,
	    zombie->newparts.lknee,
	    zombie->newparts.lknee,
	    zombie->newparts.lfoot}};
}

void zombie_ragdoll(zombie_t* zombie)
{
    // crate mass points from model points

    int      point_count = sizeof(zombie_parts) / sizeof(v4_t);
    v4_t*    points      = (v4_t*) &zombie->newparts;
    mass_t** masses      = mt_memory_calloc(sizeof(mass_t*) * point_count, NULL, NULL);

    for (int i = 0; i < point_count; i++)
    {
	v4_t point = points[i];
	masses[i]  = mass_alloc(v4_xyz(point), 5.0, 1.0, 1.0);
    }

    // create distance resolvers

    dres_t** dresarr = mt_memory_calloc(sizeof(dres_t*) * 12, NULL, NULL);

    dresarr[0] = dres_alloc(masses[0], masses[1], v3_length(v4_xyz(v4_sub(points[1], points[0]))), 1.0);
}

#endif
