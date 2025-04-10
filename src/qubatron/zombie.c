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

typedef struct _zombie_bones_t
{
    v4_t arr[POINT_COUNT];
} zombie_bones_t;

typedef struct _zombie_t
{
    zombie_bones_t oribones;
    zombie_bones_t newbones;
    zombie_parts_t newparts;

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

    return zombie;
}

void zombie_update(zombie_t* zombie, octree_t* statoctr, model_t* statmod, float lighta, float angle, v4_t pos, v4_t dir, float speed)
{
    mt_log_debug("angle %f pos dir", angle);
    v3_log(v4_xyz(pos));
    v3_log(v4_xyz(dir));

    zombie->pos = pos;
    zombie->dir = dir;

    // check how far position is from leg positions

    v4_t leftrotq = quat_from_axis_angle(v3_normalize((v3_t){0.0, 1.0, 0.0}), -M_PI / 2.0);
    v4_t leftdirp = v4_add(pos, v4_xyzw(quat_rotate(leftrotq, v3_resize(v4_xyz(dir), 35.0))));

    v3_t llegp = l3_project_point(v4_xyz(pos), v4_xyz(leftdirp), v4_xyz(zombie->lfp));
    v3_t rlegp = l3_project_point(v4_xyz(pos), v4_xyz(leftdirp), v4_xyz(zombie->rfp));

    v3_t llegv = v3_sub(llegp, v4_xyz(zombie->lfp));
    v3_t rlegv = v3_sub(rlegp, v4_xyz(zombie->rfp));

    float lleglen = v3_length(llegv);
    float rleglen = v3_length(rlegv);

    if (speed > 0.0)
    {
	if (zombie->actleg == 0 && lleglen > 10.0)
	{
	    // step forward with left leg
	    zombie->rfp    = v4_add(v4_xyzw(rlegp), v4_xyzw(v3_resize(rlegv, 10.0)));
	    zombie->actleg = 1;
	}
	else if (zombie->actleg == 1 && rleglen > 10.0)
	{
	    // step forward with left leg
	    zombie->lfp    = v4_add(v4_xyzw(llegp), v4_xyzw(v3_resize(llegv, 10.0)));
	    zombie->actleg = 0;
	}
    }
    else if (speed < 0.0)
    {
	if (zombie->actleg == 0 && lleglen > 10.0)
	{
	    // step backward with left leg
	    zombie->rfp    = v4_add(v4_xyzw(rlegp), v4_xyzw(v3_resize(rlegv, 10.0)));
	    zombie->actleg = 1;
	}
	else if (zombie->actleg == 1 && rleglen > 10.0)
	{
	    // step forward with left leg
	    zombie->lfp    = v4_add(v4_xyzw(llegp), v4_xyzw(v3_resize(llegv, 10.0)));
	    zombie->actleg = 0;
	}
    }

    // move toward lft with half stepping

    zombie->newparts.lfoot = v4_add(zombie->newparts.lfoot, v4_scale(v4_sub(zombie->lfp, zombie->newparts.lfoot), 0.4));
    zombie->newparts.rfoot = v4_add(zombie->newparts.rfoot, v4_scale(v4_sub(zombie->rfp, zombie->newparts.rfoot), 0.4));

    v4_t hipv   = (v4_t){0.0, 120.0 + sinf(lighta) * 10.0, 0.0, angle};
    v3_t rlegpv = v4_xyz(v4_sub(zombie_parts.rleg, zombie_parts.hip));
    v3_t llegpv = v4_xyz(v4_sub(zombie_parts.lleg, zombie_parts.hip));

    zombie->newparts.hip  = v4_add(pos, hipv);
    zombie->newparts.rleg = v4_add(zombie->newparts.hip, v4_xyzw(rlegpv));
    zombie->newparts.lleg = v4_add(zombie->newparts.hip, v4_xyzw(llegpv));

    // calculate knee positions

    v4_t rlegconn  = v4_sub(zombie->newparts.rleg, zombie->newparts.rfoot);
    v4_t rleghalf  = v4_add(zombie->newparts.rfoot, v4_scale(rlegconn, 0.5));
    v3_t rkneeaxis = v3_cross(v4_xyz(rlegconn), v4_xyz(dir));
    v4_t rkneerot  = quat_from_axis_angle(v3_normalize(rkneeaxis), M_PI / 2.0);
    v3_t rkneedist = v3_resize(v4_xyz(rlegconn), 130.0 - v3_length(v4_xyz(rlegconn)));
    v4_t rknee     = v4_add(rleghalf, v4_xyzw(quat_rotate(rkneerot, rkneedist)));

    v4_t llegconn  = v4_sub(zombie->newparts.lleg, zombie->newparts.lfoot);
    v4_t lleghalf  = v4_add(zombie->newparts.lfoot, v4_scale(llegconn, 0.5));
    v3_t lkneeaxis = v3_cross(v4_xyz(llegconn), v4_xyz(dir));
    v4_t lkneerot  = quat_from_axis_angle(v3_normalize(lkneeaxis), M_PI / 2.0);
    v3_t lkneedist = v3_resize(v4_xyz(llegconn), 130.0 - v3_length(v4_xyz(llegconn)));
    v4_t lknee     = v4_add(lleghalf, v4_xyzw(quat_rotate(lkneerot, lkneedist)));

    // knee is between leg and foot perpendicular to half

    zombie->newparts.rknee = rknee;
    zombie->newparts.lknee = lknee;

    zombie->newparts.hip.w   = angle;
    zombie->newparts.rleg.w  = angle;
    zombie->newparts.lleg.w  = angle;
    zombie->newparts.rknee.w = angle;
    zombie->newparts.lknee.w = angle;

    // final skeleton

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

    return;

    if (zombie->mode == 0) // walk
    {
	v4_t rot = quat_from_axis_angle(v3_normalize(v3_sub(v4_xyz(zombie->newparts.hip), v4_xyz(zombie->newparts.neck))), angle);

	v4_t ldir = v4_scale(v4_sub(zombie->lfp, zombie->newparts.lfoot), 0.2);
	v4_t rdir = v4_scale(v4_sub(zombie->rfp, zombie->newparts.rfoot), 0.2);

	ldir.y += 2.0;
	rdir.y += 2.0;

	v4_t hipv  = (v4_t){0.0, 120.0, 0.0, angle};
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

	// add gravity to basis

	for (int i = 0; i < point_count; i++)
	    zombie->masses[i]->basis = v3_add(zombie->masses[i]->basis, (v3_t){0.0, -1.0, 0.0});

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

	// update newparts based on masspoints

	v4_t* points = (v4_t*) &zombie->newparts;

	for (int i = 0; i < point_count; i++)
	{
	    points[i]   = v4_xyzw(zombie->masses[i]->trans);
	    points[i].w = angle;
	}

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
    // crate mass points from model points

    int      point_count = sizeof(zombie_parts) / sizeof(v4_t);
    v4_t*    points      = (v4_t*) &zombie->newparts;
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

    zombie->masses = masses;
    zombie->dreses = dreses;
}

void zombie_init_walk(zombie_t* zombie)
{
    zombie->mode = 0;
}

#endif
