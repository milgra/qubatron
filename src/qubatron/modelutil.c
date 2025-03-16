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
    statmod->txwth       = 8192;
    statmod->txhth       = (int) ceilf((float) statmod->point_count / (float) statmod->txwth);

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

    octree_glc_upload_texbuffer(
	octrglc,
	normals,
	0,
	0,
	8192,
	1,
	GL_RGB32F,
	GL_RGB,
	GL_FLOAT,
	octrglc->nrm1_tex,
	8);

    octree_glc_upload_texbuffer(
	octrglc,
	colors,
	0,
	0,
	8192,
	1,
	GL_RGB32F,
	GL_RGB,
	GL_FLOAT,
	octrglc->col1_tex,
	6);

    octree_glc_upload_texbuffer(
	octrglc,
	statoctr->octs,
	0,
	0,
	statoctr->txwth,
	statoctr->txhth,
	GL_RGBA32I,
	GL_RGBA_INTEGER,
	GL_INT,
	octrglc->oct1_tex,
	10);

    octree_glc_upload_texbuffer(
	octrglc,
	dynaoctr->octs,
	0,
	0,
	dynaoctr->txwth,
	dynaoctr->txhth,
	GL_RGBA32I,
	GL_RGBA_INTEGER,
	GL_INT,
	octrglc->oct2_tex,
	11);
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

    mt_log_debug("txwth %i txhth %i", statmod->txwth, statmod->txhth);

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

    octree_glc_upload_texbuffer(octrglc, dynamod->colors, 0, 0, dynamod->txwth, dynamod->txhth, GL_RGB32F, GL_RGB, GL_FLOAT, octrglc->col2_tex, 7);
    octree_glc_upload_texbuffer(octrglc, dynamod->normals, 0, 0, dynamod->txwth, dynamod->txhth, GL_RGB32F, GL_RGB, GL_FLOAT, octrglc->nrm2_tex, 9);
    octree_glc_upload_texbuffer(octrglc, dynaoctr->octs, 0, 0, dynaoctr->txwth, dynaoctr->txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, octrglc->oct2_tex, 11);

    // upload actor to skeleton modifier

    skeleton_glc_alloc_in(skelglc, dynamod->vertexes, dynamod->normals, dynamod->point_count * 3 * sizeof(GLfloat));
    skeleton_glc_alloc_out(skelglc, NULL, dynamod->point_count * sizeof(GLint) * 12, dynamod->buffs);
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

    int minind = 0;
    int maxind = 0;

    // cover all grid points inside a sphere, starting with box

    int division = 2;
    for (int i = 0; i < statoctr->levels - 1; i++) division *= 2;

    float step = statoctr->basecube.w / (float) division;

    int         size  = 15;
    tempcubes_t cubes = {0};

    for (int cx = -size; cx < size; cx++)
    {
	for (int cy = -size; cy < size; cy++)
	{
	    for (int cz = -size; cz < size; cz++)
	    {
		float dx = cx * step;
		float dy = cy * step;
		float dz = cz * step;

		if (dx * dx + dy * dy + dz * dz < (size * step) * (size * step))
		{
		    /* cubes.arr[cx + size][cy + size][cz + size] = 1; */

		    int orind  = 0;
		    int octind = 0;

		    octree_remove_point(statoctr, (v3_t){pnt.x + dx, pnt.y + dy, pnt.z + dz}, &orind, &octind);

		    if (octind > 0)
		    {
			v3_t npnt = (v3_t){statmod->vertexes[orind * 3], statmod->vertexes[orind * 3 + 1], statmod->vertexes[orind * 3 + 2]};
			v3_t nnrm = (v3_t){statmod->normals[orind * 3], statmod->normals[orind * 3 + 1], statmod->normals[orind * 3 + 2]};
			v3_t ncol = (v3_t){statmod->colors[orind * 3], statmod->colors[orind * 3 + 1], statmod->colors[orind * 3 + 2]};

			cubes.arr[cx + size][cy + size][cz + size] = 2;
			if (dx * dx + dy * dy + dz * dz > ((size - 2) * step) * ((size - 2) * step)) cubes.arr[cx + size][cy + size][cz + size] = 3;

			if (minind == 0) minind = octind;
			if (maxind == 0) maxind = octind;

			if (octind < minind) minind = octind;
			if (octind > maxind) maxind = octind;

			// add particle to particle model

			v3_t speed = (v3_t){
			    nnrm.x + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
			    nnrm.y + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
			    nnrm.z + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
			};
			speed = v3_scale(speed, (float) (rand() % 1000) / 20.0);

			model_add_point(partmod, npnt, speed, (v3_t){1.0, 1.0, 1.0});

			speed = (v3_t){
			    nnrm.x + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
			    nnrm.y + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
			    nnrm.z + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
			};
			speed = v3_scale(speed, (float) (rand() % 1000) / 20.0);

			model_add_point(partmod, npnt, speed, (v3_t){1.0, 1.0, 1.0});
		    }
		}
	    }
	}
    }

    // setup particle output buffer

    particle_glc_alloc_out(partglc, NULL, partmod->point_count * 3 * sizeof(GLfloat));
    memcpy(dynamod->vertexes + dynamod->point_count * 3, partmod->vertexes, partmod->point_count * 3 * sizeof(GLfloat));
    memcpy(dynamod->normals + dynamod->point_count * 3, partmod->normals, partmod->point_count * 3 * sizeof(GLfloat));
    memcpy(dynamod->colors + dynamod->point_count * 3, partmod->colors, partmod->point_count * 3 * sizeof(GLfloat));
    // dynamod->point_count += partmod->point_count;
    octree_glc_upload_texbuffer(glc, dynamod->colors, 0, 0, dynamod->txwth, dynamod->txhth, GL_RGB32F, GL_RGB, GL_FLOAT, glc->col2_tex, 7);
    octree_glc_upload_texbuffer(glc, dynamod->normals, 0, 0, dynamod->txwth, dynamod->txhth, GL_RGB32F, GL_RGB, GL_FLOAT, glc->nrm2_tex, 9);

    // add vertex, color and normal info to dynamic model temporarily

    /* for (int x = 0; x < 2 * size; x++) */
    /* { */
    /* 	printf("x : %i\n", x); */
    /* 	for (int y = 0; y < 2 * size; y++) */
    /* 	{ */
    /* 	    for (int z = 0; z < 2 * size; z++) */
    /* 	    { */
    /* 		printf("%i ", cubes.arr[x][y][z]); */
    /* 	    } */
    /* 	    printf("\n"); */
    /* 	} */
    /* 	printf("\n"); */
    /* } */

    if (minind > 0)
    {
	int sy = octree_line_index_for_octet_index(statoctr, minind);
	int ey = octree_line_index_for_octet_index(statoctr, maxind) + 1;
	int si = octree_octet_index_for_line_index(statoctr, sy);
	int di = octree_rgba32idata_index_for_octet_index(statoctr, si);

	GLint* data = (GLint*) statoctr->octs;

	mt_log_debug("delete hole, updating textbuffer at sy %i ey %i si %i di %i", sy, ey, si, di);

	octree_glc_upload_texbuffer(
	    glc,
	    data + di,
	    0,
	    sy,
	    statoctr->txwth,
	    ey - sy,
	    GL_RGBA32I,
	    GL_RGBA_INTEGER,
	    GL_INT,
	    glc->oct1_tex,
	    10);

	// hole is created, push touched circle inside wall

	int minind = 0;
	int maxind = 0;

	int modind = statmod->point_count;

	for (int cx = -size; cx < size; cx++)
	{
	    for (int cy = -size; cy < size; cy++)
	    {
		for (int cz = -size; cz < size; cz++)
		{
		    float dx = cx * step;
		    float dy = cy * step;
		    float dz = cz * step;

		    if (cubes.arr[cx + size][cy + size][cz + size] > 1)
		    {
			v3_t cp = (v3_t){
			    pnt.x + dx + nrm.x * -10.0 * step,
			    pnt.y + dy + nrm.y * -10.0 * step,
			    pnt.z + dz + nrm.z * -10.0 * step};

			model_add_point(statmod, cp, nrm, col);

			int modind = -1;
			octree_insert_point(
			    statoctr,
			    0,
			    statmod->point_count - 1,
			    cp,
			    &modind);

			if (modind > -1)
			{
			    if (minind == 0) minind = modind;
			    if (maxind == 0) maxind = modind;

			    if (modind < minind) minind = modind;
			    if (modind > maxind) maxind = modind;
			}

			if (cubes.arr[cx + size][cy + size][cz + size] == 3)
			{
			    // create another layer of points
			    v3_t cp = (v3_t){
				pnt.x + dx + nrm.x * -5.0 * step,
				pnt.y + dy + nrm.y * -5.0 * step,
				pnt.z + dz + nrm.z * -5.0 * step};

			    model_add_point(statmod, cp, nrm, col);

			    int modind = -1;
			    octree_insert_point(
				statoctr,
				0,
				statmod->point_count - 1,
				cp,
				&modind);

			    if (modind > -1)
			    {
				if (minind == 0) minind = modind;
				if (maxind == 0) maxind = modind;

				if (modind < minind) minind = modind;
				if (modind > maxind) maxind = modind;
			    }
			}
		    }
		}
	    }
	}

	mt_log_debug("points added, new octree length %i new model length %i", statoctr->len, statmod->point_count);

	sy = octree_line_index_for_octet_index(statoctr, minind);
	ey = octree_line_index_for_octet_index(statoctr, statoctr->len) + 1;
	si = octree_octet_index_for_line_index(statoctr, sy);
	di = octree_rgba32idata_index_for_octet_index(statoctr, si);

	mt_log_debug("adding sphere, updating textbuffer at sy %i ey %i si %i di %i", sy, ey, si, di);

	int modsy = model_line_index_for_point_index(statmod, modind);
	int modey = model_line_index_for_point_index(statmod, statmod->point_count) + 1;
	int modsi = model_point_index_for_line_index(statmod, modsy);
	int moddi = model_data_index_for_point_index(statmod, modsi);

	mt_log_debug("updating model at sy %i ey %i si %i di %i", modsy, modey, modsi, moddi);

	octree_glc_upload_texbuffer(
	    glc,
	    statmod->colors + moddi,
	    0,
	    modsy,
	    statmod->txwth,
	    modey - modsy,
	    GL_RGB32F,
	    GL_RGB,
	    GL_FLOAT,
	    glc->col1_tex,
	    6);

	octree_glc_upload_texbuffer(
	    glc,
	    statmod->normals + moddi,
	    0,
	    modsy,
	    statmod->txwth,
	    modey - modsy,
	    GL_RGB32F,
	    GL_RGB,
	    GL_FLOAT,
	    glc->nrm1_tex,
	    8);

	data = (GLint*) statoctr->octs;

	octree_glc_upload_texbuffer(
	    glc,
	    data,
	    0,
	    0,
	    statoctr->txwth,
	    statoctr->txhth,
	    GL_RGBA32I,
	    GL_RGBA_INTEGER,
	    GL_INT,
	    glc->oct1_tex,
	    10);

	/* octree_glc_upload_texbuffer( */
	/*     glc, */
	/*     data + di, */
	/*     0, */
	/*     sy, */
	/*     octree->txwth, */
	/*     ey - sy, */
	/*     GL_RGBA32I, */
	/*     GL_RGBA_INTEGER, */
	/*     GL_INT, */
	/*     glc->oct1_tex, */
	/*     10); */
    }
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

    int cnt = 0;
    for (int i = 0; i < model->point_count * 3; i += 3)
    {
	float x = model->vertexes[i];
	float y = model->vertexes[i + 1];
	float z = model->vertexes[i + 2];

	float dx = x - ox;
	float dy = y - oy;
	float dz = z - oz;

	if (dx * dx + dy * dy + dz * dz < 10.0)
	{
	    v3_t npnt = (v3_t){tlf.x + dx, tlf.y + dy, tlf.z + dz};
	    v3_t nnrm = (v3_t){model->normals[i], model->normals[i + 1], model->normals[i + 2]};
	    v3_t ncol = (v3_t){model->colors[i], model->colors[i + 1], model->colors[i + 2]};

	    // add particle to particle model

	    v3_t speed = (v3_t){
		nnrm.x + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
		nnrm.y + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
		nnrm.z + -0.3 + 0.6 * (float) (rand() % 100) / 100.0,
	    };
	    speed = v3_scale(speed, (float) (rand() % 1000) / 20.0);

	    mt_log_debug("adding %f %f %f norm %f %f %f", npnt.x, npnt.y, npnt.z, speed.x, speed.y, speed.z);

	    model_add_point(partmod, npnt, speed, (v3_t){1.0, 0.0, 0.0});

	    model->vertexes[i]     = 0.0;
	    model->vertexes[i + 1] = 0.0;
	    model->vertexes[i + 2] = 0.0;
	    cnt++;
	}
    }

    mt_log_debug("punch hole dyna, index %i, x %f y %f z %f zeroed point count %i", index, cnt, ox, oy, oz);

    skeleton_glc_alloc_in(skelglc, model->vertexes, model->normals, model->point_count * 3 * sizeof(GLfloat));

    // setup particle output buffer

    particle_glc_alloc_out(partglc, NULL, partmod->point_count * 3 * sizeof(GLfloat));
    memcpy(model->vertexes + model->point_count * 3, partmod->vertexes, partmod->point_count * 3 * sizeof(GLfloat));
    memcpy(model->normals + model->point_count * 3, partmod->normals, partmod->point_count * 3 * sizeof(GLfloat));
    memcpy(model->colors + model->point_count * 3, partmod->colors, partmod->point_count * 3 * sizeof(GLfloat));

    // model->point_count += partmod->point_count;
    octree_glc_upload_texbuffer(octrglc, model->colors, 0, 0, model->txwth, model->txhth, GL_RGB32F, GL_RGB, GL_FLOAT, octrglc->col2_tex, 7);
    octree_glc_upload_texbuffer(octrglc, model->normals, 0, 0, model->txwth, model->txhth, GL_RGB32F, GL_RGB, GL_FLOAT, octrglc->nrm2_tex, 9);
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
	    partmod->point_count = 0;
	}
    }
}

#endif
