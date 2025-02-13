#ifndef modelutil_h
#define modelutil_h

#include "model.c"
#include "octree.c"
#include "octree_glc.c"
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

void modelutil_update_skeleton(
    skeleton_glc_t* skelglc,
    octree_glc_t*   octrglc,
    model_t*        model,
    octree_t*       octree,
    float           angle);

void modelutil_punch_hole(
    octree_glc_t* glc,
    octree_t*     octree,
    model_t*      model,
    v3_t          position,
    v3_t          direction);

void modelutil_punch_hole_dyna(
    skeleton_glc_t* skelglc,
    int index, model_t* model,
    v3_t position,
    v3_t direction);

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

    octree_glc_upload_texbuffer(
	octrglc,
	statmod->colors,
	0,
	0,
	statmod->txwth,
	statmod->txhth,
	GL_RGB32F,
	GL_RGB,
	GL_FLOAT,
	octrglc->col1_tex,
	6);

    octree_glc_upload_texbuffer(
	octrglc,
	statmod->normals,
	0,
	0,
	statmod->txwth,
	statmod->txhth,
	GL_RGB32F,
	GL_RGB,
	GL_FLOAT,
	octrglc->nrm1_tex,
	8);

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

    skeleton_glc_alloc_in(skelglc, dynamod->vertexes, dynamod->point_count * 3 * sizeof(GLfloat));
    skeleton_glc_alloc_out(skelglc, NULL, dynamod->point_count * sizeof(GLint) * 12);
}

void modelutil_update_skeleton(skeleton_glc_t* skelglc, octree_glc_t* octrglc, model_t* model, octree_t* octree, float angle)
{
    skeleton_glc_update(
	skelglc,
	angle,
	model->point_count,
	octree->levels,
	octree->basecube.w);

    // add modified point coords by compute shader

    octree_reset(
	octree,
	octree->basecube);

    for (int index = 0; index < model->point_count; index++)
    {
	octree_insert_path(
	    octree,
	    0,
	    index,
	    &skelglc->octqueue[index * 12]); // 48 bytes stride 12 int
    }

    octree_glc_upload_texbuffer(
	octrglc,
	octree->octs,
	0,
	0,
	octree->txwth,
	octree->txhth,
	GL_RGBA32I,
	GL_RGBA_INTEGER,
	GL_INT,
	octrglc->oct2_tex, 11);
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

void modelutil_punch_hole(octree_glc_t* glc, octree_t* octree, model_t* model, v3_t position, v3_t direction)
{
    // check collosion between direction vector and static and dynamic voxels

    int index = octree_trace_line(octree, position, direction);

    mt_log_debug("***SHOOT***");
    mt_log_debug("voxel index %i", index);

    v3_t pt  = (v3_t){model->vertexes[index * 3], model->vertexes[index * 3 + 1], model->vertexes[index * 3 + 2]};
    v3_t nrm = (v3_t){model->normals[index * 3], model->normals[index * 3 + 1], model->normals[index * 3 + 2]};
    v3_t col = (v3_t){model->colors[index * 3], model->colors[index * 3 + 1], model->colors[index * 3 + 2]};

    int minind = 0;
    int maxind = 0;

    // cover all grid points inside a sphere, starting with box

    int division = 2;
    for (int i = 0; i < octree->levels - 1; i++) division *= 2;

    float step = octree->basecube.w / (float) division;

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

		    octree_remove_point(octree, (v3_t){pt.x + dx, pt.y + dy, pt.z + dz}, &orind, &octind);

		    if (octind > 0)
		    {
			cubes.arr[cx + size][cy + size][cz + size] = 2;
			if (dx * dx + dy * dy + dz * dz > ((size - 2) * step) * ((size - 2) * step)) cubes.arr[cx + size][cy + size][cz + size] = 3;

			if (minind == 0) minind = octind;
			if (maxind == 0) maxind = octind;

			if (octind < minind) minind = octind;
			if (octind > maxind) maxind = octind;
		    }
		}
	    }
	}
    }

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
	int sy = octree_line_index_for_octet_index(octree, minind);
	int ey = octree_line_index_for_octet_index(octree, maxind) + 1;
	int si = octree_octet_index_for_line_index(octree, sy);
	int di = octree_rgba32idata_index_for_octet_index(octree, si);

	GLint* data = (GLint*) octree->octs;

	mt_log_debug("delete hole, updating textbuffer at sy %i ey %i si %i di %i", sy, ey, si, di);

	octree_glc_upload_texbuffer(
	    glc,
	    data + di,
	    0,
	    sy,
	    octree->txwth,
	    ey - sy,
	    GL_RGBA32I,
	    GL_RGBA_INTEGER,
	    GL_INT,
	    glc->oct1_tex,
	    10);

	// hole is created, push touched circle inside wall

	int minind = 0;
	int maxind = 0;

	int modind = model->point_count;

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
			    pt.x + dx + nrm.x * -10.0 * step,
			    pt.y + dy + nrm.y * -10.0 * step,
			    pt.z + dz + nrm.z * -10.0 * step};

			model_add_point(model, cp, nrm, col);

			int modind = -1;
			octree_insert_point(
			    octree,
			    0,
			    model->point_count - 1,
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
				pt.x + dx + nrm.x * -5.0 * step,
				pt.y + dy + nrm.y * -5.0 * step,
				pt.z + dz + nrm.z * -5.0 * step};

			    model_add_point(model, cp, nrm, col);

			    int modind = -1;
			    octree_insert_point(
				octree,
				0,
				model->point_count - 1,
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

	mt_log_debug("points added, new octree length %i new model length %i", octree->len, model->point_count);

	sy = octree_line_index_for_octet_index(octree, minind);
	ey = octree_line_index_for_octet_index(octree, octree->len) + 1;
	si = octree_octet_index_for_line_index(octree, sy);
	di = octree_rgba32idata_index_for_octet_index(octree, si);

	mt_log_debug("adding sphere, updating textbuffer at sy %i ey %i si %i di %i", sy, ey, si, di);

	int modsy = model_line_index_for_point_index(model, modind);
	int modey = model_line_index_for_point_index(model, model->point_count) + 1;
	int modsi = model_point_index_for_line_index(model, modsy);
	int moddi = model_data_index_for_point_index(model, modsi);

	mt_log_debug("updating model at sy %i ey %i si %i di %i", modsy, modey, modsi, moddi);

	octree_glc_upload_texbuffer(
	    glc,
	    model->colors + moddi,
	    0,
	    modsy,
	    model->txwth,
	    modey - modsy,
	    GL_RGB32F,
	    GL_RGB,
	    GL_FLOAT,
	    glc->col1_tex,
	    6);

	octree_glc_upload_texbuffer(
	    glc,
	    model->normals + moddi,
	    0,
	    modsy,
	    model->txwth,
	    modey - modsy,
	    GL_RGB32F,
	    GL_RGB,
	    GL_FLOAT,
	    glc->nrm1_tex,
	    8);

	data = (GLint*) octree->octs;

	octree_glc_upload_texbuffer(
	    glc,
	    data,
	    0,
	    0,
	    octree->txwth,
	    octree->txhth,
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

void modelutil_punch_hole_dyna(skeleton_glc_t* skelglc, int index, model_t* model, v3_t position, v3_t direction)
{
    float ox = model->vertexes[index * 3];
    float oy = model->vertexes[index * 3 + 1];
    float oz = model->vertexes[index * 3 + 2];

    int cnt = 0;
    for (int i = 0; i < model->point_count * 3; i++)
    {
	float x = model->vertexes[i];
	float y = model->vertexes[i + 1];
	float z = model->vertexes[i + 2];

	float dx = x - ox;
	float dy = y - oy;
	float dz = z - oz;

	if (dx * dx + dy * dy + dz * dz < 100.0)
	{
	    model->vertexes[i]     = 0.0;
	    model->vertexes[i + 1] = 0.0;
	    model->vertexes[i + 2] = 0.0;
	    cnt++;
	}
    }

    mt_log_debug("punch hole dyna, index %i, x %f y %f z %f zeroed point count %i", index, cnt, ox, oy, oz);

    skeleton_glc_alloc_in(skelglc, model->vertexes, model->point_count * 3 * sizeof(GLfloat));
}

#endif
