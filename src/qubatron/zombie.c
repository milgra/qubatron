#ifndef zombie_h
#define zombie_h

#include "model.c"
#include "mt_line_3d.c"
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

typedef struct _zombie_sizes_t
{
    float headneck;
    float neckhip;
    float sholelbo;
    float elbohand;
    float legknee;
    float kneefoot;
} zombie_sizes_t;

typedef struct _zombie_bones_t
{
    v4_t arr[POINT_COUNT];
} zombie_bones_t;

typedef struct _zombie_t
{
    zombie_bones_t oribones;
    zombie_bones_t newbones;
    zombie_parts_t newparts;
    zombie_sizes_t orisizes;

    int actleg;

    v4_t lfp; // left foot pivot
    v4_t rfp; // right foot pivot

    v4_t pos;
    v4_t dir;

    int      mode;   // 0 walk 1 ragdoll
    mass_t** masses; // mass points for ragdoll physics
    dres_t** dreses;
} zombie_t;

zombie_t       zombie_init(v4_t pos, v4_t dir, octree_t* statoctr);
void           zombie_update(zombie_t* zombie, octree_t* statoctr, model_t* statmod, float lighta, float angle, v4_t pos, v4_t dir, float speed);
void           zombie_init_ragdoll(zombie_t* zombie);
void           zombie_init_walk(zombie_t* zombie);
void           zombie_shoot(zombie_t* cc, v3_t pos, v3_t dir, v3_t hit);

#endif

#if __INCLUDE_LEVEL__ == 0

// points on original model
// x y z weight
// height is 163.0

zombie_parts_t zombie_parts = {

    .head = (v4_t){43.0, 160.0, 26.0, 13.0},
    .neck = (v4_t){43.0, 135.0, 22.0, 18.0},
    .hip  = (v4_t){43.0, 80.0, 22.0, 18.0},

    .rshol = (v4_t){65.0, 135.0, 26.0, 15.0},
    .relbo = (v4_t){70.0, 112.0, 26.0, 9.0},
    .rhand = (v4_t){100.0, 55.0, 28.0, 9.0},

    .lshol = (v4_t){25.0, 135.0, 26.0, 15.0},
    .lelbo = (v4_t){10.0, 112.0, 22.0, 9.0},
    .lhand = (v4_t){-10.0, 55.0, 28.0, 9.0},

    .rleg  = (v4_t){55.0, 80.0, 20.0, 13.0},
    .rknee = (v4_t){60.0, 47.0, 15.0, 20.0},
    .rfoot = (v4_t){72.0, 5.0, 17.0, 18.0},

    .lleg  = (v4_t){32.0, 80.0, 20.0, 14.0},
    .lknee = (v4_t){26.0, 47.0, 15.0, 20.0},
    .lfoot = (v4_t){14.0, 5.0, 17.0, 18.0}

};

zombie_t zombie_init(v4_t pos, v4_t dir, octree_t* statoctr)
{
    zombie_t zombie = {0};

    zombie.oribones = (zombie_bones_t){
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

    zombie.orisizes.headneck = v3_length(v4_xyz(v4_sub(zombie_parts.head, zombie_parts.neck)));
    zombie.orisizes.neckhip  = v3_length(v4_xyz(v4_sub(zombie_parts.neck, zombie_parts.hip)));
    zombie.orisizes.sholelbo = v3_length(v4_xyz(v4_sub(zombie_parts.rshol, zombie_parts.relbo)));
    zombie.orisizes.elbohand = v3_length(v4_xyz(v4_sub(zombie_parts.relbo, zombie_parts.rhand)));
    zombie.orisizes.legknee  = v3_length(v4_xyz(v4_sub(zombie_parts.rleg, zombie_parts.rknee)));
    zombie.orisizes.kneefoot = v3_length(v4_xyz(v4_sub(zombie_parts.rknee, zombie_parts.rfoot)));

    zombie.newbones = zombie.oribones;
    zombie.newparts = zombie_parts;

    zombie.pos = pos;
    zombie.dir = dir;

    // detect leg position

    v4_t leftrotq = quat_from_axis_angle(v3_normalize((v3_t){0.0, 1.0, 0.0}), -M_PI / 2.0);
    v4_t rghtrotq = quat_from_axis_angle(v3_normalize((v3_t){0.0, 1.0, 0.0}), M_PI / 2.0);

    zombie.lfp = v4_add(pos, v4_xyzw(quat_rotate(leftrotq, v3_resize(v4_xyz(dir), 35.0))));
    zombie.rfp = v4_add(pos, v4_xyzw(quat_rotate(rghtrotq, v3_resize(v4_xyz(dir), 35.0))));

    // move points to starting position

    v4_t hipv  = (v4_t){0.0, 120.0, 0.0, 0.0};
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

    zombie.newparts.hip  = v4_add(pos, hipv);
    zombie.newparts.head = v4_add(zombie.newparts.hip, v4_xyzw(headv));
    zombie.newparts.neck = v4_add(zombie.newparts.hip, v4_xyzw(neckv));

    zombie.newparts.rshol = v4_add(zombie.newparts.hip, v4_xyzw(rsholv));
    zombie.newparts.relbo = v4_add(zombie.newparts.hip, v4_xyzw(relbov));
    zombie.newparts.rhand = v4_add(zombie.newparts.hip, v4_xyzw(rhndv));

    zombie.newparts.lshol = v4_add(zombie.newparts.hip, v4_xyzw(lsholv));
    zombie.newparts.lelbo = v4_add(zombie.newparts.hip, v4_xyzw(lelbov));
    zombie.newparts.lhand = v4_add(zombie.newparts.hip, v4_xyzw(lhndv));

    zombie.newparts.rleg  = v4_add(zombie.newparts.hip, v4_xyzw(rlegv));
    zombie.newparts.rknee = v4_add(zombie.newparts.hip, v4_xyzw(rkneev));
    zombie.newparts.rfoot = zombie.rfp;

    zombie.newparts.lleg  = v4_add(zombie.newparts.hip, v4_xyzw(llegv));
    zombie.newparts.lknee = v4_add(zombie.newparts.hip, v4_xyzw(lkneev));
    zombie.newparts.lfoot = zombie.lfp;

    zombie.newbones = (zombie_bones_t){
	.arr = {
	    zombie.newparts.head,
	    zombie.newparts.neck,
	    zombie.newparts.neck,
	    zombie.newparts.hip,
	    zombie.newparts.rshol,
	    zombie.newparts.relbo,
	    zombie.newparts.relbo,
	    zombie.newparts.rhand,
	    zombie.newparts.lshol,
	    zombie.newparts.lelbo,
	    zombie.newparts.lelbo,
	    zombie.newparts.lhand,
	    zombie.newparts.rleg,
	    zombie.newparts.rknee,
	    zombie.newparts.rknee,
	    zombie.newparts.rfoot,
	    zombie.newparts.lleg,
	    zombie.newparts.lknee,
	    zombie.newparts.lknee,
	    zombie.newparts.lfoot}};

    // crate mass points from model points

    int      point_count = sizeof(zombie_parts) / sizeof(v4_t);
    v4_t*    points      = (v4_t*) &zombie.newparts;
    mass_t** masses      = mt_memory_calloc(sizeof(mass_t*) * point_count, NULL, NULL);

    for (int i = 0; i < point_count; i++)
    {
	v4_t point = points[i];
	masses[i]  = mass_alloc(v4_xyz(point), 20.0, 1.0, 0.8);
    }

    // create distance resolvers

    dres_t** dreses = mt_memory_calloc(sizeof(dres_t*) * 18, NULL, NULL);

    dreses[0] = dres_alloc(masses[3], masses[4], 0.99);  // rshoulder to llshoulder
    dreses[1] = dres_alloc(masses[9], masses[10], 0.99); // rleg to lleg

    dreses[2] = dres_alloc(masses[3], masses[9], 0.99);  // rshoulder to rleg
    dreses[3] = dres_alloc(masses[4], masses[10], 0.99); // lshoulder to lleg

    dreses[4] = dres_alloc(masses[3], masses[10], 0.99); // rshoulder to lleg
    dreses[5] = dres_alloc(masses[4], masses[9], 0.99);  // lshoulder to rleg

    dreses[6] = dres_alloc(masses[4], masses[6], 0.99); // rshoulder to relbow
    dreses[7] = dres_alloc(masses[6], masses[8], 0.99); // relbow to rhand

    dreses[8] = dres_alloc(masses[3], masses[5], 0.99); // lshoulder to lelbow
    dreses[9] = dres_alloc(masses[5], masses[7], 0.99); // lelbow to lhand

    dreses[10] = dres_alloc(masses[3], masses[0], 0.99); // lshoulder to head
    dreses[11] = dres_alloc(masses[4], masses[0], 0.99); // rshoulder to head

    dreses[12] = dres_alloc(masses[9], masses[0], 0.99);  // lleg to head
    dreses[13] = dres_alloc(masses[10], masses[0], 0.99); // rleg to head

    dreses[14] = dres_alloc(masses[9], masses[11], 0.99);  // lleg to lknee
    dreses[15] = dres_alloc(masses[11], masses[13], 0.99); // lknee to lfoot

    dreses[16] = dres_alloc(masses[10], masses[12], 0.99); // rleg to rknee
    dreses[17] = dres_alloc(masses[12], masses[14], 0.99); // rknee to rfoot

    zombie.masses = masses;
    zombie.dreses = dreses;

    return zombie;
}

void zombie_update(zombie_t* zombie, octree_t* statoctr, model_t* statmod, float lighta, float angle, v4_t pos, v4_t dir, float speed)
{
    zombie->pos = pos;
    zombie->dir = dir;

    int point_count = sizeof(zombie_parts) / sizeof(v4_t);

    if (zombie->mode == 0) // walk
    {
	zombie_parts_t newparts = zombie->newparts;

	// LEG POSITIONS FIRST

	v4_t l90rot_q = quat_from_axis_angle(v3_normalize((v3_t){0.0, 1.0, 0.0}), -M_PI / 2.0);
	v4_t r90rot_q = quat_from_axis_angle(v3_normalize((v3_t){0.0, 1.0, 0.0}), M_PI / 2.0);

	v4_t l90dir_v = v4_xyzw(quat_rotate(l90rot_q, v3_resize(v4_xyz(dir), 20.0)));
	v4_t r90dir_v = v4_xyzw(quat_rotate(r90rot_q, v3_resize(v4_xyz(dir), 20.0)));

	v3_t stepf = v3_resize(v4_xyz(dir), 40.0);
	v3_t stepb = v3_resize(v4_xyz(dir), 5.0);

	v4_t tlf;
	v4_t newlfp;
	v4_t newrfp;

	if (zombie->actleg == 0) // right leg
	{
	    newlfp = v4_add(v4_add(pos, v4_xyzw(stepf)), l90dir_v);
	    newlfp.y += 100.0;
	    octree_trace_line(statoctr, v4_xyz(newlfp), (v3_t){0.0, -100.0, 0.0}, &tlf);
	    newlfp = tlf;

	    /* if (v3_length(v4_xyz(v4_sub(pos, zombie->lfp))) > 30.0) */
	    /* { */
	    /* 	newlfp.y += 15.0; */
	    /* } */
	    zombie->lfp = v4_add(zombie->lfp, v4_scale(v4_sub(newlfp, zombie->lfp), 0.1));
	}
	else
	{
	    newrfp = v4_add(v4_add(pos, v4_xyzw(stepf)), r90dir_v);
	    newrfp.y += 100.0;
	    octree_trace_line(statoctr, v4_xyz(newrfp), (v3_t){0.0, -100.0, 0.0}, &tlf);
	    newrfp = tlf;

	    /* if (v3_length(v4_xyz(v4_sub(pos, zombie->rfp))) > 30.0) */
	    /* { */
	    /* 	newrfp.y += 15.0; */
	    /* } */
	    zombie->rfp = v4_add(zombie->rfp, v4_scale(v4_sub(newrfp, zombie->rfp), 0.1));
	}

	// if leg distance is too big switch legs

	v4_t footv = v4_sub(zombie->rfp, zombie->lfp);

	if (v3_length(v4_xyz(footv)) > 60.0)
	{
	    zombie->actleg = 1 - zombie->actleg;
	}

	newparts.lfoot = zombie->lfp;
	newparts.rfoot = zombie->rfp;

	// HIP POSITION

	v4_t hip = v4_add(pos, v4_xyzw(v3_resize(v4_xyz(dir), 15.0)));
	hip.y    = (newparts.rfoot.y + newparts.lfoot.y) / 2.0 + zombie->orisizes.legknee + zombie->orisizes.kneefoot - 8.0;

	newparts.hip = hip;

	// LEG AND KNEE

	v4_t anglerot_q = quat_from_axis_angle((v3_t){0.0, 1.0, 0.0}, -angle);

	v3_t rlegdirv = v4_xyz(v4_sub(zombie_parts.rleg, zombie_parts.hip));
	v3_t llegdirv = v4_xyz(v4_sub(zombie_parts.lleg, zombie_parts.hip));

	newparts.rleg = v4_add(hip, v4_xyzw(quat_rotate(anglerot_q, rlegdirv)));
	newparts.lleg = v4_add(hip, v4_xyzw(quat_rotate(anglerot_q, llegdirv)));

	// calculate knee positions
	// knee is between leg and foot perpendicular to half

	v4_t rfootdirv = v4_sub(newparts.rleg, newparts.rfoot);
	v4_t rleghalf  = v4_add(newparts.rfoot, v4_scale(rfootdirv, 0.5));
	v3_t rkneeaxis = v3_cross(v4_xyz(rfootdirv), v4_xyz(dir));
	v4_t rkneerot  = quat_from_axis_angle(v3_normalize(rkneeaxis), M_PI / 2.0);
	v3_t rkneedist = v3_resize(v4_xyz(rfootdirv), zombie->orisizes.legknee + zombie->orisizes.kneefoot - v3_length(v4_xyz(rfootdirv)));
	v4_t rknee     = v4_add(rleghalf, v4_xyzw(quat_rotate(rkneerot, rkneedist)));

	v4_t lfootdirv = v4_sub(newparts.lleg, newparts.lfoot);
	v4_t lleghalf  = v4_add(newparts.lfoot, v4_scale(lfootdirv, 0.5));
	v3_t lkneeaxis = v3_cross(v4_xyz(lfootdirv), v4_xyz(dir));
	v4_t lkneerot  = quat_from_axis_angle(v3_normalize(lkneeaxis), M_PI / 2.0);
	v3_t lkneedist = v3_resize(v4_xyz(lfootdirv), zombie->orisizes.legknee + zombie->orisizes.kneefoot - v3_length(v4_xyz(lfootdirv)));
	v4_t lknee     = v4_add(lleghalf, v4_xyzw(quat_rotate(lkneerot, lkneedist)));

	newparts.rknee = rknee;
	newparts.lknee = lknee;

	// HEAD AND NECK

	v3_t headdirv = v4_xyz(v4_sub(zombie_parts.head, zombie_parts.neck));
	v3_t neckdirv = v4_xyz(v4_sub(zombie_parts.neck, zombie_parts.hip));

	v4_t  footm  = v4_sub(newparts.rfoot, newparts.lfoot);
	float nangle = acos(v3_dot(v3_normalize(v4_xyz(footm)), v3_normalize(v4_xyz(dir))));

	neckdirv = v3_add(neckdirv, v3_resize(v4_xyz(l90dir_v), cos(nangle) * -2.6));
	neckdirv = v3_add(neckdirv, v3_resize(v4_xyz(dir), cos(nangle) * 10.0));

	headdirv = v3_add(headdirv, v3_resize(v4_xyz(l90dir_v), sin(nangle) * -2.6));
	headdirv = v3_add(headdirv, v3_resize(v4_xyz(dir), sin(nangle) * 10.0));

	newparts.neck = v4_add(newparts.hip, v4_xyzw(neckdirv));
	newparts.head = v4_add(newparts.neck, v4_xyzw(headdirv));

	// ARMS

	v4_t rsholdir = v4_sub(zombie_parts.rshol, zombie_parts.neck);
	v4_t lsholdir = v4_sub(zombie_parts.lshol, zombie_parts.neck);

	newparts.rshol = v4_add(newparts.neck, v4_xyzw(quat_rotate(anglerot_q, v4_xyz(rsholdir))));
	newparts.lshol = v4_add(newparts.neck, v4_xyzw(quat_rotate(anglerot_q, v4_xyz(lsholdir))));

	newparts.rhand = v4_add(v4_add(newparts.lfoot, (v4_t){0.0, 80.0, 0.0, 0.0}), v4_xyzw(v3_resize(v4_xyz(r90dir_v), 45.0)));
	newparts.lhand = v4_add(v4_add(newparts.rfoot, (v4_t){0.0, 80.0, 0.0, 0.0}), v4_xyzw(v3_resize(v4_xyz(l90dir_v), 45.0)));

	newparts.rhand = v4_add(newparts.rhand, v4_xyzw(v3_resize(v4_xyz(dir), 5.0)));
	newparts.lhand = v4_add(newparts.lhand, v4_xyzw(v3_resize(v4_xyz(dir), 5.0)));

	// calculate elbow positions
	// elbow is between shoulder and hand perpendicular to half

	v4_t rhanddirv = v4_sub(newparts.rhand, newparts.rshol);
	v4_t rarmhalf  = v4_add(newparts.rshol, v4_xyzw(v3_resize(v4_xyz(rhanddirv), zombie->orisizes.sholelbo)));
	v3_t relboaxis = v3_cross(v4_xyz(rhanddirv), v4_xyz(dir));
	v4_t relborot  = quat_from_axis_angle(v3_normalize(relboaxis), -M_PI / 2.0);
	v3_t relbodist = v3_resize(v4_xyz(rhanddirv), 5.0);
	v4_t relbo     = v4_add(rarmhalf, v4_xyzw(quat_rotate(relborot, relbodist)));

	newparts.relbo = relbo;

	v4_t lhanddirv = v4_sub(newparts.lhand, newparts.lshol);
	v4_t larmhalf  = v4_add(newparts.lshol, v4_xyzw(v3_resize(v4_xyz(lhanddirv), zombie->orisizes.sholelbo)));
	v3_t lelboaxis = v3_cross(v4_xyz(lhanddirv), v4_xyz(dir));
	v4_t lelborot  = quat_from_axis_angle(v3_normalize(lelboaxis), -M_PI / 2.0);
	v3_t lelbodist = v3_resize(v4_xyz(lhanddirv), 5.0);
	v4_t lelbo     = v4_add(larmhalf, v4_xyzw(quat_rotate(lelborot, lelbodist)));

	newparts.lelbo = lelbo;

	newparts.hip.w   = angle;
	newparts.head.w  = angle;
	newparts.neck.w  = angle;
	newparts.rleg.w  = angle;
	newparts.lleg.w  = angle;
	newparts.rknee.w = angle;
	newparts.lknee.w = angle;
	newparts.rfoot.w = angle;
	newparts.lfoot.w = angle;
	newparts.rshol.w = angle;
	newparts.lshol.w = angle;
	newparts.relbo.w = angle;
	newparts.lelbo.w = angle;
	newparts.rhand.w = angle;
	newparts.lhand.w = angle;

	v4_t* points = (v4_t*) &newparts;

	for (int i = 0; i < point_count; i++)
	{
	    zombie->masses[i]->trans = v4_xyz(points[i]);
	}

	for (int i = 0; i < point_count; i++)
	{
	    zombie->masses[i]->basis = v3_sub(v4_xyz(points[i]), zombie->masses[i]->trans);
	    if (v3_length(zombie->masses[i]->basis) > 10.0) zombie->masses[i]->basis = v3_resize(zombie->masses[i]->basis, 10.0);
	}
    }
    else // ragdoll
    {
	// update masses

	int point_count = sizeof(zombie_parts) / sizeof(v4_t);

	// add gravity to basis

	for (int i = 0; i < point_count; i++)
	    zombie->masses[i]->basis = v3_add(zombie->masses[i]->basis, (v3_t){0.0, -0.8, 0.0});

	// update distance resolvers

	for (int i = 0; i < 18; i++)
	    dres_update(zombie->dreses[i], 1.0);

	// check collosion

	for (int i = 0; i < point_count; i++)
	{
	    v4_t  batl, x1tl, x2tl, y1tl, y2tl, z1tl, z2tl;
	    float balen = 100.0, x1len = 100.0, x2len = 100.0, y1len = 100.0, y2len = 100.0, z1len = 100.0, z2len = 100.0;

	    v3_t currtrans = zombie->masses[i]->trans;
	    v3_t nexttrans = v3_add(currtrans, zombie->masses[i]->basis);

	    int baind = octree_trace_line(statoctr, currtrans, zombie->masses[i]->basis, &batl); // basis intersection

	    int x1ind = octree_trace_line(statoctr, nexttrans, (v3_t){10.0, 0.0, 0.0}, &x1tl); // horizotnal dirs
	    int x2ind = octree_trace_line(statoctr, nexttrans, (v3_t){-10.0, 0.0, 0.0}, &x2tl);

	    int y1ind = octree_trace_line(statoctr, nexttrans, (v3_t){0.0, 10.0, 0.0}, &y1tl); // vertical dirs
	    int y2ind = octree_trace_line(statoctr, nexttrans, (v3_t){0.0, -10.0, 0.0}, &y2tl);

	    int z1ind = octree_trace_line(statoctr, nexttrans, (v3_t){0.0, 0.0, 10.0}, &z1tl); // depth dirs
	    int z2ind = octree_trace_line(statoctr, nexttrans, (v3_t){0.0, 0.0, -10.0}, &z2tl);

	    if (baind > 0) balen = v3_length(v3_sub(v4_xyz(batl), currtrans));
	    if (x1ind > 0) x1len = v3_length(v3_sub(v4_xyz(x1tl), nexttrans));
	    if (x2ind > 0) x2len = v3_length(v3_sub(v4_xyz(x2tl), nexttrans));
	    if (y1ind > 0) y1len = v3_length(v3_sub(v4_xyz(y1tl), nexttrans));
	    if (y2ind > 0) y2len = v3_length(v3_sub(v4_xyz(y2tl), nexttrans));
	    if (z1ind > 0) z1len = v3_length(v3_sub(v4_xyz(z1tl), nexttrans));
	    if (z2ind > 0) z2len = v3_length(v3_sub(v4_xyz(z2tl), nexttrans));

	    int collosion = 0;

	    if (balen < v3_length(zombie->masses[i]->basis)) // movement bases goes through obstacle
	    {
		// mirror basis on normal

		v3_t normal = (v3_t){
		    statmod->normals[baind * 3],
		    statmod->normals[baind * 3 + 1],
		    statmod->normals[baind * 3 + 2]};

		// project basis negate onto normal

		v3_t newbasis            = v3_scale(zombie->masses[i]->basis, -1.0);
		v3_t onnormal            = l3_project_point((v3_t){0.0, 0.0, 0.0}, normal, newbasis);
		newbasis                 = v3_add(onnormal, v3_sub(onnormal, newbasis));
		zombie->masses[i]->basis = v3_scale(newbasis, 0.8);
		collosion                = 1;
	    }
	    else if (x1len < 20.0 || x2len < 20.0 || y1len < 20.0 || y2len < 20.0 || z1len < 20.0 || z2len < 20.0) // radius intersection
	    {
		/* mt_log_debug("RADIUS IS %.2f %.2f %.2f %.2f %.2f %.2f", x1len, x2len, y1len, y2len, z1len, z2len); */

		int   indarr[6] = {x1ind, x2ind, y1ind, y2ind, z1ind, z2ind};
		float lenarr[6] = {x1len, x2len, y1len, y2len, z1len, z2len};

		v3_t newbasis = (v3_t){0.0, 0.0, 0.0};

		for (int j = 0; j < 6; j++)
		{
		    if (lenarr[j] < 20.0)
		    {
			// mirror basis on normal

			v3_t normal = (v3_t){
			    statmod->normals[indarr[j] * 3],
			    statmod->normals[indarr[j] * 3 + 1],
			    statmod->normals[indarr[j] * 3 + 2]};

			// project basis negate onto normal

			v3_t antibasis = v3_scale(zombie->masses[i]->basis, -1.0);
			v3_t onnormal  = l3_project_point((v3_t){0.0, 0.0, 0.0}, normal, antibasis);
			if (newbasis.x == 0.0 && newbasis.y == 0.0 && newbasis.z == 0.0)
			    newbasis = v3_add(onnormal, v3_sub(onnormal, antibasis));
			else
			    newbasis = v3_scale(v3_add(newbasis, v3_add(onnormal, v3_sub(onnormal, antibasis))), 0.5);
		    }
		}

		zombie->masses[i]->basis = v3_scale(newbasis, 0.6);
		collosion                = 1;
	    }

	    // do movement if no collosion

	    if (collosion == 0)
		zombie->masses[i]->trans = v3_add(zombie->masses[i]->trans, zombie->masses[i]->basis);
	}
    }

    v4_t* points = (v4_t*) &zombie->newparts;

    for (int i = 0; i < point_count; i++)
    {
	points[i]   = v4_add(points[i], v4_xyzw(v3_scale(v3_sub(zombie->masses[i]->trans, v4_xyz(points[i])), 0.3)));
	points[i].w = angle;
    }

    if (zombie->mode == 1) // ragdoll
    {
	points[1]   = v4_add(points[3], v4_scale(v4_sub(points[4], points[3]), 0.5));  // static neck
	points[2]   = v4_add(points[9], v4_scale(v4_sub(points[10], points[9]), 0.5)); // static hip
	points[1].w = angle;
	points[2].w = angle;
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

    for (int i = 0; i < 18; i++)
	dres_resetdistance(zombie->dreses[i]);
}

void zombie_init_walk(zombie_t* zombie)
{
    zombie->mode = 0;
}

void zombie_shoot(zombie_t* zombie, v3_t pos, v3_t dir, v3_t hit)
{
    // move closest masspoints

    int point_count = sizeof(zombie_parts) / sizeof(v4_t);

    for (int i = 0; i < point_count; i++)
    {
	v3_t dirv = v3_sub(hit, zombie->masses[i]->trans);

	if (v3_length(dirv) < 40.0)
	{
	    float ratio              = 1.0 - v3_length(dirv) / 40.0;
	    zombie->masses[i]->basis = v3_resize(dir, 15.0 * ratio);
	}
	else
	{
	    zombie->masses[i]->basis = (v3_t){0.0, 0.0, 0.0};
	}
    }
}

#endif
