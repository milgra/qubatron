#ifndef zombie_h
#define zombie_h

#include "mt_quat.c"
#include "mt_vector_4d.c"
#include "octree.c"
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

    int      mode;   // 0 walk 1 ragdoll
    mass_t** masses; // mass points for ragdoll physics
    dres_t** dreses;
} zombie_t;

zombie_t       zombie_init();
void           zombie_update(zombie_t* zombie, octree_t* statoctr, float langle, float dir, v4_t pos);
void           zombie_init_ragdoll(zombie_t* zombie);

#endif

#if __INCLUDE_LEVEL__ == 0

// points on original model
// x y z weight

zombie_parts_t zombie_parts = {

    .head = (v4_t){54.0, 200.0, 26.0, 20.0},
    .neck = (v4_t){54.0, 170.0, 26.0, 23.0},
    .hip  = (v4_t){52.0, 120.0, 26.0, 22.0},

    .rshol = (v4_t){75.0, 170.0, 26.0, 14.0},
    .relbo = (v4_t){85.0, 150.0, 26.0, 11.0},
    .rhand = (v4_t){120.0, 75.0, 32.0, 3.0},

    .lshol = (v4_t){33.0, 170.0, 26.0, 14.0},
    .lelbo = (v4_t){13.0, 110.0, 26.0, 14.0},
    .lhand = (v4_t){-10.0, 65.0, 28.0, 3.0},

    .rleg  = (v4_t){68.0, 110.0, 26.0, 20.0},
    .rknee = (v4_t){80.0, 60.0, 26.0, 26.0},
    .rfoot = (v4_t){92.0, 10.0, 17.0, 8.0},

    .lleg  = (v4_t){45.0, 120.0, 26.0, 20.0},
    .lknee = (v4_t){25.0, 60.0, 26.0, 26.0},
    .lfoot = (v4_t){19.0, 10.0, 17.0, 8.0}

};

zombie_t zombie_init()
{
    zombie_t res = {0};

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

void zombie_update(zombie_t* zombie, octree_t* statoctr, float lighta, float dir, v4_t pos)
{
    if (zombie->mode == 0) // walk
    {
	v4_t rot = quat_from_axis_angle(v3_normalize(v3_sub(v4_xyz(zombie->newparts.hip), v4_xyz(zombie->newparts.neck))), dir);

	v4_t ldir = v4_scale(v4_sub(zombie->lfp, zombie->newparts.lfoot), 0.2);
	v4_t rdir = v4_scale(v4_sub(zombie->rfp, zombie->newparts.rfoot), 0.2);

	v4_t hipv  = (v4_t){0.0, 140.0, 0.0, dir};
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
    }
    else // ragdoll
    {
	// update masses

	int point_count = sizeof(zombie_parts) / sizeof(v4_t);
	point_count     = 1;

	for (int i = 0; i < point_count; i++)
	{
	    // add gravity to basis
	    zombie->masses[i]->basis = v3_add(zombie->masses[i]->basis, (v3_t){0.0, -1.0, 0.0});
	}

	// update distance resolvers

	/* for (int i = 0; i < 14; i++) */
	/* { */
	/*     dres_update(zombie->dreses[i], 1.0); */
	/* } */

	// check collosion

	for (int i = 0; i < point_count; i++)
	{
	    // first check if movement vector crosses the level somewhere
	    v4_t topleft = {0};
	    int  orind   = octree_trace_line(statoctr, zombie->masses[i]->trans, zombie->masses[i]->basis, &topleft);

	    if (orind > 0)
	    {
		// move back mass to intersection point and mirror
	    }
	    else
	    {
		// no intersection of movement
		// check proximity of level voxels from mass center

		// check all three axises
		int xind = octree_trace_line(statoctr, zombie->masses[i]->trans, (v3_t){1.0, 0.0, 0.0}, &topleft);
		int yind = octree_trace_line(statoctr, zombie->masses[i]->trans, (v3_t){0.0, 1.0, 0.0}, &topleft);
		int zind = octree_trace_line(statoctr, zombie->masses[i]->trans, (v3_t){0.0, 0.0, 1.0}, &topleft);

		// in case of intersection move back mass to intersection point and mirror
	    }

	    /* v3_t ptoi = v3_sub(v4_xyz(topleft), zombie->masses[i]->trans); */
	    /* float dot  = v3_dot(v3_normalize(zombie->masses[i]->basis), v3_normalize(ptoi)); */

	    /* v3_log(zombie->masses[i]->basis); */
	    /* v3_log(ptoi); */
	    /* mt_log_debug("dot %f", dot); */

	    /* if (dot > 0.0) */
	    /* { */
	    /*     float dist = v3_length(ptoi); */

	    /*     /\* mt_log_debug("ORI %i DST %f", orind, dist); *\/ */
	    /*     if (dist < 10.0) */
	    /*     { */
	    /* 	zombie->masses[i]->basis.y = 11.0; */
	    /*     } */
	    /* } */
	}

	// finally do movement

	for (int i = 0; i < point_count; i++)
	{
	    zombie->masses[i]->trans = v3_add(zombie->masses[i]->trans, zombie->masses[i]->basis);
	}

	// update newparts based on masspoints

	v4_t* points = (v4_t*) &zombie->newparts;

	for (int i = 0; i < point_count; i++)
	{
	    points[i] = v4_xyzw(zombie->masses[i]->trans);
	}
    }

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

void zombie_init_ragdoll(zombie_t* zombie)
{
    zombie->mode = 1;
    // crate mass points from model points

    int      point_count = sizeof(zombie_parts) / sizeof(v4_t);
    v4_t*    points      = (v4_t*) &zombie->newparts;
    mass_t** masses      = mt_memory_calloc(sizeof(mass_t*) * point_count, NULL, NULL);

    for (int i = 0; i < point_count; i++)
    {
	v4_t point = points[i];
	masses[i]  = mass_alloc(v4_xyz(point), 15.0, 1.0, 0.95);
    }

    // create distance resolvers

    dres_t** dreses = mt_memory_calloc(sizeof(dres_t*) * 14, NULL, NULL);

    dreses[0] = dres_alloc(masses[0], masses[1], 0.9); // head to neck
    dreses[1] = dres_alloc(masses[1], masses[2], 0.9); // neck to hip

    dreses[2] = dres_alloc(masses[1], masses[3], 0.9); // neck to rshoulder
    dreses[3] = dres_alloc(masses[3], masses[4], 0.9); // rshoulder to relbow
    dreses[4] = dres_alloc(masses[4], masses[5], 0.9); // relbow to rhand

    dreses[5] = dres_alloc(masses[1], masses[6], 0.9); // neck to lshoulder
    dreses[6] = dres_alloc(masses[6], masses[7], 0.9); // lshoulder to lelbow
    dreses[7] = dres_alloc(masses[7], masses[8], 0.9); // lelbow to rhand

    dreses[8]  = dres_alloc(masses[2], masses[9], 0.9);   // hip to rleg
    dreses[9]  = dres_alloc(masses[9], masses[10], 0.9);  // rleg to rknee
    dreses[10] = dres_alloc(masses[10], masses[11], 0.9); // rknee to rfoot

    dreses[11] = dres_alloc(masses[2], masses[12], 0.9);  // hip to lleg
    dreses[12] = dres_alloc(masses[12], masses[13], 0.9); // lleg to lknee
    dreses[13] = dres_alloc(masses[13], masses[14], 0.9); // lknee to lfoot

    zombie->masses = masses;
    zombie->dreses = dreses;
}

#endif
