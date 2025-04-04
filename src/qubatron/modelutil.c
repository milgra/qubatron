#ifndef modelutil_h
#define modelutil_h

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
    v3_t            direction);

void modelutil_punch_hole_dyna(
    octree_glc_t*   octrglc,
    skeleton_glc_t* skelglc,
    particle_glc_t* partglc,
    model_t*        partmod,
    int index, model_t* model,
    v3_t position,
    v3_t direction,
    v4_t tlf);

void modelutil_update_particle(
    particle_glc_t* partglc,
    model_t*        partmod,
    int             levels,
    float           basesize,
    uint32_t        frames);

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
    snprintf(pntpath, PATH_MAX, "%sres/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%sres/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%sres/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%sres/%s.rng", base_path, scenepath);
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
    snprintf(pntpath, PATH_MAX, "%sres/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%sres/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%sres/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%sres/%s.rng", base_path, scenepath);
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

typedef struct _tempcubes_t
{
    char arr[30][30][30];
} tempcubes_t;

int modelutil_zeroes(tempcubes_t* cubes, int x, int y, int z, int size)
{
    int result = 0;
    for (int cx = x - 1; cx < x + 2; cx++)
    {
	for (int cy = y - 1; cy < y + 2; cy++)
	{
	    for (int cz = z - 1; cz < z + 2; cz++)
	    {
		if (cx > 0 && cx < size && cy > 0 && cy < size && cz > 0 && cz < size)
		{
		    if (cubes->arr[cx][cy][cz] == 0)
		    {
			result++;
		    }
		}
	    }
	}
    }
    return result;
}

int grid_fill_half(tempcubes_t* cubes, int x, int y, int z, int size)
{
    int result = 0;
    for (int cx = x - 1; cx < x + 2; cx++)
    {
	for (int cy = y - 1; cy < y + 2; cy++)
	{
	    for (int cz = z - 1; cz < z + 2; cz++)
	    {
		if (cx > 0 && cx < size && cy > 0 && cy < size && cz > 0 && cz < size)
		{
		    if (cubes->arr[cx][cy][cz] == 0)
		    {
			result++;
		    }
		}
	    }
	}
    }
    return result;
}

void modelutil_punch_hole(octree_glc_t* glc, particle_glc_t* partglc, model_t* partmod, octree_t* statoctr, model_t* statmod, model_t* dynamod, v3_t position, v3_t direction)
{
    // check collosion between direction vector and static and dynamic voxels

    int index = octree_trace_line(statoctr, position, direction, NULL);

    mt_log_debug("***SHOOT***");
    mt_log_debug("voxel index %i", index);

    v3_t pnt = (v3_t){statmod->vertexes[index * 3], statmod->vertexes[index * 3 + 1], statmod->vertexes[index * 3 + 2]};
    v3_t nrm = (v3_t){statmod->normals[index * 3], statmod->normals[index * 3 + 1], statmod->normals[index * 3 + 2]};
    v3_t col = (v3_t){statmod->colors[index * 3], statmod->colors[index * 3 + 1], statmod->colors[index * 3 + 2]};

    int mini = 0;
    int maxi = 0;

    int minind = 0;
    int maxind = 0;
    int orind  = 0;
    int octind = 0;
    int modind = -1; // fucked up, modify!

    float dist = 10.0 + (float) (rand() % 50) / 5.0;

    for (int i = 0; i < statmod->point_count * 3; i += 3)
    {
	float x = statmod->vertexes[i];
	float y = statmod->vertexes[i + 1];
	float z = statmod->vertexes[i + 2];

	float dx = x - pnt.x;
	float dy = y - pnt.y;
	float dz = z - pnt.z;

	if (dx * dx + dy * dy + dz * dz < dist)
	{
	    if (mini == 0) mini = i / 3;
	    if (maxi == 0) maxi = i / 3;

	    if (i < mini) mini = i / 3;
	    if (i > maxi) maxi = i / 3;

	    /* v3_t npnt = (v3_t){pnt.x + dx, pnt.y + dy, pnt.z + dz}; */
	    v3_t nnrm = (v3_t){statmod->normals[i], statmod->normals[i + 1], statmod->normals[i + 2]};
	    v3_t ncol = (v3_t){statmod->colors[i], statmod->colors[i + 1], statmod->colors[i + 2]};

	    float rat = -(dist - (dx * dx + dy * dy + dz * dz)) / 2.0;

	    octree_remove_point(statoctr, (v3_t){x, y, z}, &orind, &octind);

	    if (minind == 0) minind = octind;
	    if (maxind == 0) maxind = octind;

	    if (octind < minind) minind = octind;
	    if (octind > maxind) maxind = octind;

	    x += nnrm.x * rat;
	    y += nnrm.y * rat;
	    z += nnrm.z * rat;

	    octree_insert_point(statoctr, 0, i / 3, (v3_t){x, y, z}, &modind);

	    if (maxind == 0) maxind = modind;
	    if (modind > maxind) maxind = modind; // not working!!!

	    // rotate normal based on distance

	    statmod->colors[i] += 0.1;
	    statmod->colors[i + 1] += 0.1;
	    statmod->colors[i + 2] += 0.1;

	    // add particle to particle model

	    v3_t speed = (v3_t){
		nnrm.x + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
		nnrm.y + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
		nnrm.z + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
	    };
	    speed = v3_scale(speed, (float) (rand() % 1000) / 20.0);

	    model_add_point(partmod, (v3_t){x, y, z}, speed, ncol);
	}
    }

    octree_glc_upload_texbuffer_data(
	glc,
	statmod->colors,                            // buffer
	GL_FLOAT,                                   // data type
	statmod->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                        // itemsize
	mini * sizeof(GLfloat) * 3,                 // start offset
	maxi * sizeof(GLfloat) * 3,                 // end offset
	OCTREE_GLC_BUFFER_STATIC_COLOR);            // buffer type

    octree_glc_upload_texbuffer_data(
	glc,
	statoctr->octs,                     // buffer
	GL_INT,                             // data type
	statoctr->len * sizeof(GLint) * 12, // size
	sizeof(GLint) * 4,                  // itemsize
	minind * sizeof(GLint) * 12,        // start offset
	maxind * sizeof(GLint) * 12,        // end offset
	OCTREE_GLC_BUFFER_STATIC_OCTREE);   // buffer type

    // setup particle output buffer

    particle_glc_alloc_out(partglc, NULL, partmod->point_count * 3 * sizeof(GLfloat));

    // add particle vertexes, normals and colors to combined model

    /* memcpy(dynamod->vertexes + dynamod->point_count * 3, partmod->vertexes, partmod->point_count * 3 * sizeof(GLfloat)); */
    /* memcpy(dynamod->normals + dynamod->point_count * 3, partmod->normals, partmod->point_count * 3 * sizeof(GLfloat)); */
    /* memcpy(dynamod->colors + dynamod->point_count * 3, partmod->colors, partmod->point_count * 3 * sizeof(GLfloat)); */
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
    v4_t            tlf)
{
    float ox = model->vertexes[index * 3];
    float oy = model->vertexes[index * 3 + 1];
    float oz = model->vertexes[index * 3 + 2];

    float dist = 10.0 + (float) (rand() % 50) / 5.0;

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

	    v3_t nnrm = (v3_t){model->normals[i], model->normals[i + 1], model->normals[i + 2]};
	    v3_t ncol = (v3_t){model->colors[i], model->colors[i + 1], model->colors[i + 2]};

	    float rat = -(dist - (dx * dx + dy * dy + dz * dz)) / 2.0;

	    model->vertexes[i] += nnrm.x * rat;
	    model->vertexes[i + 1] += nnrm.y * rat;
	    model->vertexes[i + 2] += nnrm.z * rat;

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
	    speed = v3_scale(speed, (float) (rand() % 1000) / 20.0);

	    model_add_point(partmod, (v3_t){x, y, z}, speed, ncol);
	}
    }

    skeleton_glc_alloc_in(skelglc, model->vertexes, model->normals, model->point_count * 3 * sizeof(GLfloat));

    octree_glc_upload_texbuffer_data(
	octrglc,
	model->colors,                            // buffer
	GL_FLOAT,                                 // data type
	model->point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                      // itemsize
	minind * sizeof(GLfloat),                 // start offset
	maxind * sizeof(GLfloat),                 // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_COLOR);         // buffer type

    /* octree_glc_upload_texbuffer_data( */
    /* 	octrglc, */
    /* 	model->normals,                           // buffer */
    /* 	GL_FLOAT,                                 // data type */
    /* 	model->point_count * sizeof(GLfloat) * 3, // size */
    /* 	sizeof(GLfloat) * 3,                      // itemsize */
    /* 	minind * sizeof(GLfloat) * 3,             // start offset */
    /* 	maxind * sizeof(GLfloat) * 3,             // end offset */
    /* OCTREE_GLC_BUFFER_DYNAMIC_NORMAL);        // buffer type */

    // setup particle output buffer

    particle_glc_alloc_out(partglc, NULL, partmod->point_count * 3 * sizeof(GLfloat));

    // add particle vertexes, normals and colors to combined model

    /* memcpy(model->vertexes + model->point_count * 3, partmod->vertexes, partmod->point_count * 3 * sizeof(GLfloat)); */
    /* memcpy(model->normals + model->point_count * 3, partmod->normals, partmod->point_count * 3 * sizeof(GLfloat)); */
    /* memcpy(model->colors + model->point_count * 3, partmod->colors, partmod->point_count * 3 * sizeof(GLfloat)); */
}

void modelutil_update_particle(particle_glc_t* partglc, model_t* partmod, int levels, float basesize, uint32_t frames)
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

    if (frames % 20 == 0)
    {
	int fincount = 0;
	for (int i = 0; i < partmod->point_count * 3; i += 3)
	{
	    if (partmod->normals[i] == 0.0 && partmod->normals[i + 1] == 0.0 && partmod->normals[i + 2] == 0.0)
		++fincount;
	}
	mt_log_debug("fincount %i points %i", fincount, partmod->point_count);

	if (fincount == partmod->point_count)
	{
	    /* dynamod->point_count -= partmod->point_count; */
	    partmod->point_count = 0;
	}
    }
}

#endif
