#ifndef zombie_h
#define zombie_h

#include "mt_quat.c"
#include "mt_vector_4d.c"
#include <string.h>

typedef struct _zombie_parts_t
{
    v4_t head;
    v4_t neck;
    v4_t hip;
    v4_t lleg;
    v4_t rleg;
    v4_t lfoot;
    v4_t rfoot;
    v4_t larm;
    v4_t rarm;
    v4_t lhand;
    v4_t rhand;
} zombie_parts_t;

typedef struct _zombie_bones_t
{
    v4_t arr[12];
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

    .larm  = (v4_t){5.0, 170.0, 22.0, 10.0},
    .lhand = (v4_t){-10.0, 70.0, 22.0, 3.0},

    .rarm  = (v4_t){75.0, 170.0, 26.0, 14.0},
    .rhand = (v4_t){120.0, 70.0, 26.0, 3.0},

    .lleg  = (v4_t){38.0, 120.0, 22.0, 6.0},
    .lfoot = (v4_t){8.0, -10.0, 22.0, 8.0},

    .rleg  = (v4_t){70.0, 120.0, 22.0, 6.0},
    .rfoot = (v4_t){100.0, -10.0, 22.0, 8.0}

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
	    zombie_parts.rarm,
	    zombie_parts.rhand,
	    zombie_parts.larm,
	    zombie_parts.lhand,
	    zombie_parts.rleg,
	    zombie_parts.rfoot,
	    zombie_parts.lleg,
	    zombie_parts.lfoot}};

    res.newparts = zombie_parts;

    res.newparts.hip   = v4_add(res.newparts.hip, (v4_t){250.0, 0.0, 600.0, 0.0});
    res.newparts.head  = v4_add(res.newparts.hip, (v4_t){2.0, 86.0, 0.0, 0.0});
    res.newparts.neck  = v4_add(res.newparts.hip, (v4_t){2.0, 50.0, 0.0, 0.0});
    res.newparts.larm  = v4_add(res.newparts.hip, (v4_t){-47.0, 50.0, 0.0, 0.0});
    res.newparts.lhand = v4_add(res.newparts.hip, (v4_t){-62.0, -50.0, 0.0, 0.0});
    res.newparts.rarm  = v4_add(res.newparts.hip, (v4_t){53.0, 50.0, 0.0, 0.0});
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
    /* v4_t headv = (v4_t){2.0, 86.0 - feetdist + cos(lighta) * 2.0, 0.0 + sin(lighta) * 3.0, 0.0}; */
    /* v4_t neckv = (v4_t){2.0, 50.0 + cos(lighta) * 2.0, 0.0 - sin(lighta) * 3.0, 0.0}; */
    v4_t larmv = v4_xyzw(quat_rotate(back_rot, (v3_t){-47.0, 50.0, 0.0}));
    v4_t lhndv = v4_xyzw(quat_rotate(back_rot, (v3_t){-62.0 + sin(lighta) * 15.0, -50.0, 30.0}));
    v4_t rarmv = v4_xyzw(quat_rotate(back_rot, (v3_t){53.0, 50.0, 0.0}));
    v4_t rhndv = v4_xyzw(quat_rotate(back_rot, (v3_t){68.0 + cos(lighta) * 15.0, -50.0, 30.0}));
    v4_t llegv = v4_xyzw(quat_rotate(back_rot, (v3_t){-20.0, 0.0, 1.0}));
    v4_t rlegv = v4_xyzw(quat_rotate(back_rot, (v3_t){20.0, 0.0, 1.0}));

    v4_t headv = v4_sub(zombie_parts.head, zombie_parts.hip);
    headv.w    = 0.0;
    headv.z += sinf(lighta) * 5.0;
    v4_t neckv = v4_sub(zombie_parts.neck, zombie_parts.hip);
    neckv.w    = 0.0;

    rarmv = v4_sub(zombie_parts.rarm, zombie_parts.hip);
    rarmv = v4_xyzw(quat_rotate(back_rot, v4_xyz(rarmv)));

    rarmv.w = 0.0;
    /* v4_t rhndv = v4_sub(zombie_parts.rhand, zombie_parts.hip); */
    /* rhndv.w    = 0.0; */

    zombie->newparts.hip   = v4_add(pos, hipv);
    zombie->newparts.head  = v4_add(zombie->newparts.hip, headv);
    zombie->newparts.neck  = v4_add(zombie->newparts.hip, neckv);
    zombie->newparts.larm  = v4_add(zombie->newparts.hip, larmv);
    zombie->newparts.lhand = v4_add(zombie->newparts.hip, lhndv);
    zombie->newparts.rarm  = v4_add(zombie->newparts.hip, rarmv);
    zombie->newparts.rhand = v4_add(zombie->newparts.hip, rhndv);
    zombie->newparts.lleg  = v4_add(zombie->newparts.hip, llegv);
    zombie->newparts.rleg  = v4_add(zombie->newparts.hip, rlegv);
    zombie->newparts.lfoot = v4_add(zombie->newparts.lfoot, ldir);
    zombie->newparts.rfoot = v4_add(zombie->newparts.rfoot, rdir);

    // move lfoot and rfoot towards pivots, move other points according to these positions

    zombie->newbones = (zombie_bones_t){
	.arr = {
	    zombie->newparts.head,
	    zombie->newparts.neck,
	    zombie->newparts.neck,
	    zombie->newparts.hip,
	    zombie->newparts.rarm,
	    zombie->newparts.rhand,
	    zombie->newparts.larm,
	    zombie->newparts.lhand,
	    zombie->newparts.rleg,
	    zombie->newparts.rfoot,
	    zombie->newparts.lleg,
	    zombie->newparts.lfoot}};
}

void zombie_ragdoll()
{
    // setup distance resolvers according to current positions
}

#endif
