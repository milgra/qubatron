#ifndef zombie_h
#define zombie_h

#include "mt_quat.c"
#include "mt_vector_4d.c"
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

    res.newparts = zombie_parts;

    res.newparts.hip   = v4_add(res.newparts.hip, (v4_t){250.0, 0.0, 600.0, 0.0});
    res.newparts.head  = v4_add(res.newparts.hip, (v4_t){2.0, 86.0, 0.0, 0.0});
    res.newparts.neck  = v4_add(res.newparts.hip, (v4_t){2.0, 50.0, 0.0, 0.0});
    res.newparts.lshol = v4_add(res.newparts.hip, (v4_t){-47.0, 50.0, 0.0, 0.0});
    res.newparts.lhand = v4_add(res.newparts.hip, (v4_t){-62.0, -50.0, 0.0, 0.0});
    res.newparts.rshol = v4_add(res.newparts.hip, (v4_t){53.0, 50.0, 0.0, 0.0});
    res.newparts.rhand = v4_add(res.newparts.hip, (v4_t){68.0, -50.0, 0.0, 0.0});
    res.newparts.lleg  = v4_add(res.newparts.hip, (v4_t){-20.0, 0.0, 1.0, 0.0});
    res.newparts.lfoot = v4_add(res.newparts.hip, (v4_t){-45.0, -130.0, 31.0, 0.0});
    res.newparts.rleg  = v4_add(res.newparts.hip, (v4_t){20.0, 0.0, 1.0, 0.0});
    res.newparts.rfoot = v4_add(res.newparts.hip, (v4_t){45.0, -130.0, 1.0, 0.0});

    res.lfp = res.newparts.lfoot;
    res.rfp = res.newparts.rfoot;

    return res;
}

void zombie_update(zombie_t* zombie, float lighta, float dir, v4_t pos)
{
    v4_t back_rot = quat_from_axis_angle(v3_normalize(v3_sub(v4_xyz(zombie->newparts.hip), v4_xyz(zombie->newparts.neck))), dir);

    v4_t ldir = v4_scale(v4_sub(zombie->lfp, zombie->newparts.lfoot), 0.2);
    v4_t rdir = v4_scale(v4_sub(zombie->rfp, zombie->newparts.rfoot), 0.2);

    float feetdist = v3_length(v4_xyz(v4_sub(zombie->newparts.lfoot, zombie->newparts.rfoot))) / 10.0;

    v4_t hipv = (v4_t){0.0, 120.0, 0.0, dir};

    v4_t headv = v4_sub(zombie_parts.head, zombie_parts.hip);

    headv.w = 0.0;
    headv.z += sinf(lighta) * 5.0;

    v4_t neckv = v4_sub(zombie_parts.neck, zombie_parts.hip);
    neckv.z += cosf(lighta) * 5.0;
    neckv.w = 0.0;

    v4_t rsholv = v4_sub(zombie_parts.rshol, zombie_parts.hip);
    rsholv      = v4_xyzw(quat_rotate(back_rot, v4_xyz(rsholv)));
    rsholv.z += cosf(lighta) * 5.0;
    rsholv.w = 0.0;

    v4_t relbov = v4_sub(zombie_parts.relbo, zombie_parts.hip);
    relbov      = v4_xyzw(quat_rotate(back_rot, v4_xyz(relbov)));
    relbov.w    = 0.0;

    v4_t rhndv = v4_sub(zombie_parts.rhand, zombie_parts.hip);
    rhndv.z += sinf(lighta) * 20.0;
    rhndv.y += sinf(lighta) * 20.0;
    rhndv.w = 0.0;

    v4_t lhndv = v4_sub(zombie_parts.lhand, zombie_parts.hip);
    lhndv.z += sinf(lighta) * 20.0;
    lhndv.y += sinf(lighta) * 20.0;
    lhndv.w = 0.0;

    v4_t lsholv = v4_sub(zombie_parts.lshol, zombie_parts.hip);
    lsholv      = v4_xyzw(quat_rotate(back_rot, v4_xyz(lsholv)));
    lsholv.z += cosf(lighta) * 5.0;
    lsholv.w = 0.0;

    v4_t lelbov = v4_sub(zombie_parts.lelbo, zombie_parts.hip);
    lelbov      = v4_xyzw(quat_rotate(back_rot, v4_xyz(lelbov)));
    lelbov.z += cosf(lighta) * 5.0;
    lelbov.w = 0.0;

    v4_t rlegv = v4_sub(zombie_parts.rleg, zombie_parts.hip);
    rlegv      = v4_xyzw(quat_rotate(back_rot, v4_xyz(rlegv)));
    rlegv.w    = 0.0;

    v4_t rkneev = v4_sub(zombie_parts.rknee, zombie_parts.hip);
    rkneev      = v4_xyzw(quat_rotate(back_rot, v4_xyz(rkneev)));
    rkneev.w    = 0.0;

    v4_t llegv = v4_sub(zombie_parts.lleg, zombie_parts.hip);
    llegv      = v4_xyzw(quat_rotate(back_rot, v4_xyz(llegv)));
    llegv.w    = 0.0;

    v4_t lkneev = v4_sub(zombie_parts.lknee, zombie_parts.hip);
    lkneev      = v4_xyzw(quat_rotate(back_rot, v4_xyz(lkneev)));
    lkneev.w    = 0.0;

    zombie->newparts.hip   = v4_add(pos, hipv);
    zombie->newparts.head  = v4_add(zombie->newparts.hip, headv);
    zombie->newparts.neck  = v4_add(zombie->newparts.hip, neckv);
    zombie->newparts.lshol = v4_add(zombie->newparts.hip, lsholv);
    zombie->newparts.lelbo = v4_add(zombie->newparts.hip, lelbov);
    zombie->newparts.lhand = v4_add(zombie->newparts.hip, lhndv);
    zombie->newparts.rshol = v4_add(zombie->newparts.hip, rsholv);
    zombie->newparts.relbo = v4_add(zombie->newparts.hip, relbov);
    zombie->newparts.rhand = v4_add(zombie->newparts.hip, rhndv);
    zombie->newparts.lleg  = v4_add(zombie->newparts.hip, llegv);
    zombie->newparts.rleg  = v4_add(zombie->newparts.hip, rlegv);
    zombie->newparts.lknee = v4_add(zombie->newparts.hip, lkneev);
    zombie->newparts.rknee = v4_add(zombie->newparts.hip, rkneev);
    zombie->newparts.lfoot = v4_add(zombie->newparts.lfoot, ldir);
    zombie->newparts.rfoot = v4_add(zombie->newparts.rfoot, rdir);

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

void zombie_ragdoll()
{
    // setup distance resolvers according to current positions
}

#endif
