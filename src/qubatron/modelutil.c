#ifndef modelutil_h
#define modelutil_h

#include "dust_glc.c"
#include "model.c"
#include "octree.c"
#include "octree_glc.c"
#include "particle_glc.c"
#include "skeleton_glc.c"

void modelutil_load_test(
    char*           base_path,
    octree_glc_t*   octrglc,
    skeleton_glc_t* skelglc,
    octree_t*       statoctr,
    octree_t*       dynaoctr,
    model_t*        statmod,
    model_t*        dynamod);

void modelutil_load_flat(
    char*           base_path,
    octree_glc_t*   octrglc,
    skeleton_glc_t* skelglc,
    octree_t*       statoctr,
    octree_t*       dynaoctr,
    model_t*        statmod,
    model_t*        dynamod);

void modelutil_punch_hole(
    octree_glc_t*   glc,
    particle_glc_t* partglc,
    model_t*        partmod,
    octree_t*       octree,
    model_t*        statmod,
    model_t*        dynamod,
    v3_t            position,
    v3_t            direction,
    long            dynacount,
    int             guntype);

void modelutil_punch_hole_dyna(
    octree_glc_t*   octrglc,
    skeleton_glc_t* skelglc,
    particle_glc_t* partglc,
    model_t*        partmod,
    int             index,
    model_t*        model,
    v3_t            position,
    v3_t            direction,
    v4_t            tlf,
    long            dynacount,
    int             guntype);

void modelutil_update_particle(
    particle_glc_t* partglc,
    model_t*        partmod,
    model_t*        dynamod,
    int             levels,
    float           basesize,
    uint32_t        frames,
    octree_t*       statoctr,
    model_t*        statmod,
    octree_glc_t*   octrglc);

void modelutil_update_dust(
    dust_glc_t* partglc,
    model_t*    dustmod,
    model_t*    dynamod,
    int         levels,
    float       basesize,
    uint32_t    frames,
    v3_t        campos);

#endif

#if __INCLUDE_LEVEL__ == 0

void modelutil_load_test(
    char*           base_path,
    octree_glc_t*   octrglc,
    skeleton_glc_t* skelglc,
    octree_t*       statoctr,
    octree_t*       dynaoctr,
    model_t*        statmod,
    model_t*        dynamod)
{
    // load testscene with 5 points

    int point_count = 5;

    GLfloat points[3 * 8192] = {
	10.0, 690.0, 10.0,
	10.0, 340.0, 10.0,
	10.0, 340.0, 690.0,
	10.0, 10.0, 10.0,
	690.0, 10.0, 690.0};

    GLfloat normals[3 * 8192] = {
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0};

    GLfloat colors[3 * 8192] = {
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0};

    statmod->vertexes    = mt_memory_alloc(3 * 8192, NULL, NULL);
    statmod->normals     = mt_memory_alloc(3 * 8192, NULL, NULL);
    statmod->colors      = mt_memory_alloc(3 * 8192, NULL, NULL);
    statmod->point_count = 5;

    memcpy(statmod->vertexes, points, 3 * 8192);
    memcpy(statmod->normals, normals, 3 * 8192);
    memcpy(statmod->colors, colors, 3 * 8192);

    for (int index = 0; index < point_count * 3; index += 3)
    {
	octree_insert_point(
	    statoctr,
	    0,
	    index / 3,
	    (v3_t){points[index], points[index + 1], points[index + 2]},
	    NULL);
    }

    octree_glc_upload_texbuffer_data(
	octrglc,
	statmod->colors,                            // buffer
	GL_FLOAT,                                   // data type
	statmod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	0,                                          // start offset
	statmod->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_STATIC_COLOR);            // buffer type

    octree_glc_upload_texbuffer_data(
	octrglc,
	statmod->normals,                           // buffer
	GL_FLOAT,                                   // data type
	statmod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	0,                                          // start offset
	statmod->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_STATIC_NORMAL);           // buffer type

    octree_glc_upload_texbuffer_data(
	octrglc,
	statoctr->octs,                     // buffer
	GL_INT,                             // data type
	statoctr->len * sizeof(GLint) * 12, // size
	sizeof(GLint) * 4,                  // itemsize
	0,                                  // start offset
	statoctr->len * sizeof(GLint) * 12, // end offset
	OCTREE_GLC_BUFFER_STATIC_OCTREE);   // buffer type

    octree_glc_upload_texbuffer_data(
	octrglc,
	dynaoctr->octs,                     // buffer
	GL_INT,                             // data type
	dynaoctr->len * sizeof(GLint) * 12, // size
	sizeof(GLint) * 4,                  // itemsize
	0,                                  // start offset
	dynaoctr->len * sizeof(GLint) * 12, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_OCTREE);  // buffer type
}

void modelutil_load_flat(
    char*           base_path,
    octree_glc_t*   octrglc,
    skeleton_glc_t* skelglc,
    octree_t*       statoctr,
    octree_t*       dynaoctr,
    model_t*        statmod,
    model_t*        dynamod)
{
    char pntpath[PATH_MAX];
    char nrmpath[PATH_MAX];
    char colpath[PATH_MAX];
    char rngpath[PATH_MAX];

    char* scenepath = "abandoned.ply";
    snprintf(pntpath, PATH_MAX, "%s%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%s%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%s%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%s%s.rng", base_path, scenepath);
    #ifdef EMSCRIPTEN
    snprintf(pntpath, PATH_MAX, "%sresasm/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%sresasm/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%sresasm/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%sresasm/%s.rng", base_path, scenepath);
    #endif

    model_load_flat(statmod, pntpath, colpath, nrmpath, rngpath);

    octets_t fstpath;
    octets_t lstpath;

    for (int index = 0; index < statmod->point_count * 3; index += 3)
    {
	octets_t path =
	    octree_insert_point(
		statoctr,
		0,
		index / 3,
		(v3_t){statmod->vertexes[index], statmod->vertexes[index + 1], statmod->vertexes[index + 2]},
		NULL);

	if (index == 0)
	    fstpath = path;
	else if (index == (statmod->point_count - 3))
	    lstpath = path;
    }

    mt_log_debug("STATIC MODEL");
    model_log_vertex_info(statmod, 0);
    octree_log_path(fstpath, 0);
    model_log_vertex_info(statmod, statmod->point_count - 1);
    octree_log_path(lstpath, statmod->point_count - 1);

    // upload static model

    octree_glc_upload_texbuffer_data(
	octrglc,
	statmod->colors,                            // buffer
	GL_FLOAT,                                   // data type
	statmod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	0,                                          // start offset
	statmod->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_STATIC_COLOR);            // buffer type

    octree_glc_upload_texbuffer_data(
	octrglc,
	statmod->normals,                           // buffer
	GL_FLOAT,                                   // data type
	statmod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	0,                                          // start offset
	statmod->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_STATIC_NORMAL);           // buffer type

    octree_glc_upload_texbuffer_data(
	octrglc,
	statoctr->octs,                     // buffer
	GL_INT,                             // data type
	statoctr->len * sizeof(GLint) * 12, // size
	sizeof(GLint) * 4,                  // itemsize
	0,                                  // start offset
	statoctr->len * sizeof(GLint) * 12, // end offset
	OCTREE_GLC_BUFFER_STATIC_OCTREE);   // buffer type

    scenepath = "zombie.ply";
    snprintf(pntpath, PATH_MAX, "%s%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%s%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%s%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%s%s.rng", base_path, scenepath);
    #ifdef EMSCRIPTEN
    snprintf(pntpath, PATH_MAX, "%sresasm/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%sresasm/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%sresasm/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%sresasm/%s.rng", base_path, scenepath);
    #endif
    model_load_flat(dynamod, pntpath, colpath, nrmpath, rngpath);

    for (int index = 0; index < dynamod->point_count * 3; index += 3)
    {
	octets_t path =
	    octree_insert_point(
		dynaoctr,
		0,
		index / 3,
		(v3_t){dynamod->vertexes[index], dynamod->vertexes[index + 1], dynamod->vertexes[index + 2]},
		NULL);
	if (index == 0)
	    fstpath = path;
	else if (index == (dynamod->point_count - 3))
	    lstpath = path;
    }

    mt_log_debug("DYNAMIC MODEL");
    model_log_vertex_info(dynamod, 0);
    model_log_vertex_info(dynamod, dynamod->point_count - 1);
    octree_log_path(fstpath, 0);
    octree_log_path(lstpath, dynamod->point_count - 3);

    // upload dynamic model

    octree_glc_upload_texbuffer_data(
	octrglc,
	dynamod->colors,                            // buffer
	GL_FLOAT,                                   // data type
	dynamod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	0,                                          // start offset
	dynamod->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_COLOR);           // buffer type

    octree_glc_upload_texbuffer_data(
	octrglc,
	dynamod->normals,                           // buffer
	GL_FLOAT,                                   // data type
	dynamod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	0,                                          // start offset
	dynamod->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_NORMAL);          // buffer type

    octree_glc_upload_texbuffer_data(
	octrglc,
	dynaoctr->octs,                     // buffer
	GL_INT,                             // data type
	dynaoctr->len * sizeof(GLint) * 12, // size
	sizeof(GLint) * 4,                  // itemsize
	0,                                  // start offset
	dynaoctr->len * sizeof(GLint) * 12, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_OCTREE);  // buffer type

    // upload actor to skeleton modifier

    skeleton_glc_alloc_in(skelglc, dynamod->vertexes, dynamod->normals, dynamod->point_count * 3 * sizeof(GLfloat));
    skeleton_glc_alloc_out(skelglc, NULL, dynamod->point_count * sizeof(GLint) * 12, dynamod->point_count * 3 * sizeof(GLfloat));
}

int SameSide(v3_t v1, v3_t v2, v3_t v3, v3_t v4, v3_t p)
{
    v3_t  normal = v3_cross(v3_sub(v2, v1), v3_sub(v3, v1));
    float dotV4  = v3_dot(normal, v3_sub(v4, v1));
    float dotP   = v3_dot(normal, v3_sub(p, v1));
    return ((dotV4 > 0.0 && dotP > 0.0) || (dotV4 <= 0.0 && dotP <= 0.0));
}

int PointInTetrahedron(v3_t v1, v3_t v2, v3_t v3, v3_t v4, v3_t p)
{
    return SameSide(v1, v2, v3, v4, p) &&
	   SameSide(v2, v3, v4, v1, p) &&
	   SameSide(v3, v4, v1, v2, p) &&
	   SameSide(v4, v1, v2, v3, p);
}

void modelutil_punch_hole(
    octree_glc_t*   glc,
    particle_glc_t* partglc,
    model_t*        partmod,
    octree_t*       statoctr,
    model_t*        statmod,
    model_t*        dynamod,
    v3_t            position,
    v3_t            direction,
    long            dynacount,
    int             guntype)
{
    // check collosion between direction vector and static and dynamic voxels

    int index = octree_trace_line(statoctr, position, direction, NULL);

    mt_log_debug("***SHOOT***");
    mt_log_debug("voxel index %i", index);

    v3_t pnt = (v3_t){statmod->vertexes[index * 3], statmod->vertexes[index * 3 + 1], statmod->vertexes[index * 3 + 2]};
    v3_t nrm = (v3_t){statmod->normals[index * 3], statmod->normals[index * 3 + 1], statmod->normals[index * 3 + 2]};
    v3_t col = (v3_t){statmod->colors[index * 3], statmod->colors[index * 3 + 1], statmod->colors[index * 3 + 2]};

    mt_log_debug("pnt %f %f %f", pnt.x, pnt.y, pnt.z);

    // iterate through a subcube in the voxel grid to create the hole

    int minmodi = 0;
    int maxmodi = 0;

    int octind = -1; // fucked up, modify!
    int modind = -1; // fucked up, modify!

    float dist = (guntype + 1) * 5.0 + (guntype + 1) * 5.0 * ((float) (rand() % 100) / 100.0);
    float half = dist / 2.0;

    int division = 2;
    for (int i = 0; i < statoctr->levels - 1; i++) division *= 2;
    float step   = statoctr->basecube.w / (float) division;
    int   pntlen = (int) (dist / step);
    v3_t  pntarr[pntlen * pntlen * pntlen];
    int   modarr[pntlen * pntlen * pntlen];
    int   pntind = 0;

    float r1 = 0.5 + 0.5 * ((float) (rand() % 100) / 100.0);
    float r2 = 0.5 + 0.5 * ((float) (rand() % 100) / 100.0);
    float r3 = 0.5 + 0.5 * ((float) (rand() % 100) / 100.0);
    float r4 = 0.5 + 0.5 * ((float) (rand() % 100) / 100.0);

    v3_t p1 = pnt;
    p1      = v3_sub(p1, (v3_t){r1 * half, r1 * half, r1 * half});
    v3_t p2 = pnt;
    p2      = v3_sub(p2, (v3_t){r2 * half, r2 * half, r2 * half});
    v3_t p3 = pnt;
    p3      = v3_add(p3, (v3_t){r3 * half, r3 * half, r3 * half});
    v3_t p4 = pnt;
    p4      = v3_add(p3, (v3_t){r4 * half, r4 * half, r4 * half});

    // remove points first

    for (float cx = pnt.x - half; cx < pnt.x + half; cx += step)
    {
	for (float cy = pnt.y - half; cy < pnt.y + half; cy += step)
	{
	    for (float cz = pnt.z - half; cz < pnt.z + half; cz += step)
	    {
		float dx = pnt.x - cx;
		float dy = pnt.y - cy;
		float dz = pnt.z - cz;

		if (dx * dx + dy * dy + dz * dz < half * half)
		/* if (PointInTetrahedron(p1, p2, p3, p4, (v3_t){cx, cy, cz}) == 1) */
		{
		    octind = -1;
		    modind = -1;
		    octind = -1;

		    octree_remove_point(statoctr, (v3_t){pnt.x + dx, pnt.y + dy, pnt.z + dz}, &modind, &octind);

		    if (octind >= 0)
		    {
			octree_glc_upload_texbuffer_data(
			    glc,
			    statoctr->octs,                     // buffer
			    GL_INT,                             // data type
			    statoctr->len * sizeof(GLint) * 12, // size
			    sizeof(GLint) * 4,                  // itemsize
			    octind * sizeof(GLint) * 12,        // start offset
			    (octind + 1) * sizeof(GLint) * 12,  // end offset
			    OCTREE_GLC_BUFFER_STATIC_OCTREE);   // buffer type

			// store min and max model and octet index for partial upload to gpu

			if (minmodi == 0) minmodi = modind;
			if (maxmodi == 0) maxmodi = modind;

			if (modind < minmodi) minmodi = modind;
			if (modind > maxmodi) maxmodi = modind;

			v3_t npnt = (v3_t){pnt.x + dx, pnt.y + dy, pnt.z + dz};
			v3_t nnrm = (v3_t){statmod->normals[modind * 3], statmod->normals[modind * 3 + 1], statmod->normals[modind * 3 + 2]};
			v3_t ncol = (v3_t){statmod->colors[modind * 3], statmod->colors[modind * 3 + 1], statmod->colors[modind * 3 + 2]};

			// move original points into the wall/body

			float rat = (half - sqrtf(dx * dx + dy * dy + dz * dz)) / half;
			rat *= -10.0;

			float x = pnt.x + dx + nnrm.x * rat;
			float y = pnt.y + dy + nnrm.y * rat;
			float z = pnt.z + dz + nnrm.z * rat;

			modarr[pntind]   = modind;
			pntarr[pntind++] = (v3_t){x, y, z};

			// add points for density, avoid holes
			// !!! possible buffer overrun

			modarr[pntind]   = modind;
			pntarr[pntind++] = (v3_t){x + step, y + step, z + step};
		    }
		}
	    }
	}
    }

    // add points

    for (int i = 0; i < pntind; i++)
    {
	v3_t curr          = pntarr[i];
	int  modind        = modarr[i];
	int  octindarr[13] = {0};
	octree_insert_point(statoctr, 0, modind, curr, octindarr);

	v3_t nnrm = (v3_t){statmod->normals[modind * 3], statmod->normals[modind * 3 + 1], statmod->normals[modind * 3 + 2]};
	v3_t ncol = (v3_t){statmod->colors[modind * 3], statmod->colors[modind * 3 + 1], statmod->colors[modind * 3 + 2]};

	for (int j = 0; j < 13; j++)
	{
	    if (octindarr[j] > 0)
	    {
		int octind = octindarr[j];
		octree_glc_upload_texbuffer_data(
		    glc,
		    statoctr->octs,                     // buffer
		    GL_INT,                             // data type
		    statoctr->len * sizeof(GLint) * 12, // size
		    sizeof(GLint) * 4,                  // itemsize
		    octind * sizeof(GLint) * 12,        // start offset
		    (octind + 1) * sizeof(GLint) * 12,  // end offset
		    OCTREE_GLC_BUFFER_STATIC_OCTREE);   // buffer type
	    }
	}

	// rotate normal based on distance

	statmod->colors[modind * 3] += 0.2; // fuck these multipliers, use a setter inside model !!!
	statmod->colors[modind * 3 + 1] += 0.2;
	statmod->colors[modind * 3 + 2] += 0.2;

	statmod->normals[modind * 3]     = pnt.x - curr.x; // fuck these multipliers, use a setter inside model !!!
	statmod->normals[modind * 3 + 1] = pnt.y - curr.y;
	statmod->normals[modind * 3 + 2] = pnt.z - curr.z;

	// add particle to particle model

	v3_t speed = (v3_t){
	    nnrm.x + -0.6 + 1.2 * (float) (rand() % 100) / 100.0,
	    nnrm.y + -0.6 + 1.2 * (float) (rand() % 100) / 100.0,
	    nnrm.z + -0.6 + 1.2 * (float) (rand() % 100) / 100.0,
	};
	speed = v3_scale(speed, 10.0);

	model_add_point(partmod, curr, speed, ncol);
    }

    octree_glc_upload_texbuffer_data(
	glc,
	statmod->colors,                            // buffer
	GL_FLOAT,                                   // data type
	statmod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	minmodi * sizeof(GLfloat) * 3,              // start offset
	maxmodi * sizeof(GLfloat) * 3,              // end offset
	OCTREE_GLC_BUFFER_STATIC_COLOR);            // buffer type

    octree_glc_upload_texbuffer_data(
	glc,
	statmod->normals,                           // buffer
	GL_FLOAT,                                   // data type
	statmod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	minmodi * sizeof(GLfloat) * 3,              // start offset
	maxmodi * sizeof(GLfloat) * 3,              // end offset
	OCTREE_GLC_BUFFER_STATIC_NORMAL);           // buffer type

    // setup particle output buffer

    particle_glc_alloc_out(partglc, NULL, partmod->point_count * 3 * sizeof(GLfloat));

    // add particle vertexes, normals and colors to combined model

    long point_count = dynamod->point_count;
    // append particle model after zombie and dust
    dynamod->point_count = dynacount; // dirty hack, create a combined model!!!
    model_append(dynamod, partmod);

    octree_glc_upload_texbuffer_data(
	glc,
	dynamod->colors,                            // buffer
	GL_FLOAT,                                   // data type
	dynamod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	0,                                          // start offset
	dynamod->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_COLOR);           // buffer type

    octree_glc_upload_texbuffer_data(
	glc,
	dynamod->normals,                           // buffer
	GL_FLOAT,                                   // data type
	dynamod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	0,                                          // start offset
	dynamod->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_NORMAL);          // buffer type

    dynamod->point_count = point_count; // restore zombie point count
}

void modelutil_punch_hole_dyna(
    octree_glc_t*   octrglc,
    skeleton_glc_t* skelglc,
    particle_glc_t* partglc,
    model_t*        partmod,
    int             index,
    model_t*        model,
    v3_t            position,
    v3_t            direction,
    v4_t            tlf,
    long            dynacount,
    int             guntype)
{
    float ox = model->vertexes[index * 3];
    float oy = model->vertexes[index * 3 + 1];
    float oz = model->vertexes[index * 3 + 2];

    float dist = (guntype + 1) * 5.0 + (guntype + 1) * 5.0 * ((float) (rand() % 100) / 100.0);

    int minind = 0;
    int maxind = 0;

    for (int i = 0; i < model->point_count * 3; i += 3)
    {
	float x = model->vertexes[i];
	float y = model->vertexes[i + 1];
	float z = model->vertexes[i + 2];

	float dx = x - ox;
	float dy = y - oy;
	float dz = z - oz;

	if (dx * dx + dy * dy + dz * dz < dist)
	{
	    if (minind == 0) minind = i;
	    if (maxind == 0) maxind = i;

	    if (i < minind) minind = i;
	    if (i > maxind) maxind = i;

	    v3_t npnt = (v3_t){tlf.x + dx, tlf.y + dy, tlf.z + dz};
	    v3_t nnrm = (v3_t){model->normals[i], model->normals[i + 1], model->normals[i + 2]};
	    v3_t ncol = (v3_t){model->colors[i], model->colors[i + 1], model->colors[i + 2]};

	    float rat = -(dist - (dx * dx + dy * dy + dz * dz)) / 2.0;

	    model->vertexes[i] += nnrm.x * rat;
	    model->vertexes[i + 1] += nnrm.y * rat;
	    model->vertexes[i + 2] += nnrm.z * rat;

	    model->normals[i]     = ox - model->vertexes[i];
	    model->normals[i + 1] = oy - model->vertexes[i + 1];
	    model->normals[i + 2] = oz - model->vertexes[i + 2];

	    // rotate normal based on distance

	    model->colors[i] = 1.0;
	    /* model->colors[i + 1] = 0.0; */
	    /* model->colors[i + 2] = 0.0; */

	    // add particle to particle model

	    v3_t speed = (v3_t){
		nnrm.x + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
		nnrm.y + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
		nnrm.z + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
	    };
	    speed  = v3_scale(speed, (float) (rand() % 1000) / 100.0);
	    ncol.x = 1.0;

	    model_add_point(partmod, npnt, speed, ncol);
	}
    }

    skeleton_glc_alloc_in(skelglc, model->vertexes, model->normals, model->point_count * 3 * sizeof(GLfloat));

    /* octree_glc_upload_texbuffer_data( */
    /* 	octrglc, */
    /* 	model->colors,                            // buffer */
    /* 	GL_FLOAT,                                 // data type */
    /* 	model->point_count * sizeof(GLfloat) * 3, // size */
    /* 	sizeof(GLfloat) * 3,                      // itemsize */
    /* 	minind * sizeof(GLfloat),                 // start offset */
    /* 	maxind * sizeof(GLfloat),                 // end offset */
    /* 	OCTREE_GLC_BUFFER_DYNAMIC_COLOR);         // buffer type */

    // setup particle output buffer

    particle_glc_alloc_out(partglc, NULL, partmod->point_count * 3 * sizeof(GLfloat));

    // add particle vertexes, normals and colors to combined model

    long point_count = model->point_count;
    // append particle model after zombie and dust
    model->point_count = dynacount; // dirty hack, create a combined model!!!
    model_append(model, partmod);

    octree_glc_upload_texbuffer_data(
	octrglc,
	model->colors,                            // buffer
	GL_FLOAT,                                 // data type
	model->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                      // itemsize
	0,                                        // start offset
	model->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_COLOR);         // buffer type

    octree_glc_upload_texbuffer_data(
	octrglc,
	model->normals,                           // buffer
	GL_FLOAT,                                 // data type
	model->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                      // itemsize
	0,                                        // start offset
	model->point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_NORMAL);        // buffer type

    model->point_count = point_count; // restore zombie point count
}

void modelutil_update_particle(
    particle_glc_t* partglc,
    model_t*        partmod,
    model_t*        dynamod,
    int             levels,
    float           basesize,
    uint32_t        frames,
    octree_t*       statoctr,
    model_t*        statmod,
    octree_glc_t*   octrglc)
{
    // upload latest position and speed data

    particle_glc_alloc_in(partglc, partmod->vertexes, partmod->normals, partmod->point_count * 3 * sizeof(GLfloat));

    // update simulation

    particle_glc_update(partglc, partmod->point_count, levels, basesize);

    // store new position and speed data

    memcpy(partmod->vertexes, partglc->pos_out, partmod->point_count * 3 * sizeof(GLfloat));
    memcpy(partmod->normals, partglc->spd_out, partmod->point_count * 3 * sizeof(GLfloat));

    /* mt_log_debug("particle step"); */
    /* mt_log_debug("%f %f %f", partglc->pos_out[0], partglc->pos_out[1], partglc->pos_out[2]); */
    /* mt_log_debug("%f %f %f", partglc->spd_out[0], partglc->spd_out[1], partglc->spd_out[2]); */

    // check simulation end every second

    if (frames % 60 == 0)
    {
	int fincount = 0;
	for (int i = 0; i < partmod->point_count * 3; i += 3)
	{
	    if (partmod->normals[i] < -900.0)
		++fincount;
	    else
	    {
		/* mt_log_debug("SPD %f %f %f POS %f %f %f", partmod->normals[i], partmod->normals[i + 1], partmod->normals[i + 2], partmod->vertexes[i], partmod->vertexes[i + 1], partmod->vertexes[i + 2]); */
	    }
	}

	mt_log_debug("rem %i", partmod->point_count - fincount);

	if (fincount == partmod->point_count)
	{
	    // find matching octets in static model, set color
	    for (int i = 0; i < partmod->point_count * 3; i += 3)
	    {
		v3_t pnt = (v3_t){partmod->vertexes[i], partmod->vertexes[i + 1], partmod->vertexes[i + 2]};
		v3_t col = (v3_t){partmod->colors[i], partmod->colors[i + 1], partmod->colors[i + 2]};

		int modind = 0;
		int octind = 0;

		octree_model_index(statoctr, pnt, &modind, &octind);

		// set color in static

		statmod->colors[modind * 3]     = col.x;
		statmod->colors[modind * 3 + 1] = col.y;
		statmod->colors[modind * 3 + 2] = col.z;

		octree_glc_upload_texbuffer_data(
		    octrglc,
		    statmod->colors,                            // buffer
		    GL_FLOAT,                                   // data type
		    statmod->point_count * sizeof(GLfloat) * 3, // size
		    sizeof(GLfloat) * 3,                        // itemsize
		    modind * sizeof(GLfloat) * 3,               // start offset
		    (modind + 1) * sizeof(GLfloat) * 3,         // end offset
		    OCTREE_GLC_BUFFER_STATIC_COLOR);            // buffer type
	    }

	    /* dynamod->point_count -= partmod->point_count; */
	    mt_log_debug("dynamod point count %i", dynamod->point_count);
	    partmod->point_count = 0;
	}
    }
}

void modelutil_update_dust(dust_glc_t* dustglc, model_t* dustmod, model_t* dynamod, int levels, float basesize, uint32_t frames, v3_t campos)
{
    // upload latest position and speed data

    dust_glc_alloc_in(dustglc, dustmod->vertexes, dustmod->normals, dustmod->point_count * 3 * sizeof(GLfloat));

    // update simulation

    dust_glc_update(dustglc, dustmod->point_count, levels, basesize, campos);

    // store new position and speed data

    memcpy(dustmod->vertexes, dustglc->pos_out, dustmod->point_count * 3 * sizeof(GLfloat));
    memcpy(dustmod->normals, dustglc->spd_out, dustmod->point_count * 3 * sizeof(GLfloat));

    /* mt_log_debug("dusticle step"); */
    /* mt_log_debug("%f %f %f", dustglc->pos_out[0], dustglc->pos_out[1], dustglc->pos_out[2]); */
    /* mt_log_debug("%f %f %f", dustglc->spd_out[0], dustglc->spd_out[1], dustglc->spd_out[2]); */
}

#endif
