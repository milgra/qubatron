#ifndef modelutil_h
#define modelutil_h

#include "model.c"
#include "octree.c"
#include "octree_glc.c"

void modelutil_punch_hole(octree_glc_t* glc, octree_t* octree, model_t* model, v3_t position, v3_t direction);

#endif

#if __INCLUDE_LEVEL__ == 0

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
	int sy  = (minind * 3) / 8192;
	int ey  = (maxind * 3) / 8192 + 1;
	int noi = sy * 8192 * 4;

	GLint* data = (GLint*) octree->octs;

	mt_log_debug("UPDATING HOLE");
	mt_log_debug("minind %i maxind %i", minind, maxind);
	mt_log_debug("sy %i ey %i", sy, ey);
	mt_log_debug("noi %i", noi);

	octree_glc_upload_texbuffer(
	    glc,
	    data + noi,
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
			    pt.x + dx + nrm.x * -4.0 * step,
			    pt.y + dy + nrm.y * -4.0 * step,
			    pt.z + dz + nrm.z * -4.0 * step};

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
				pt.x + dx + nrm.x * -2.0 * step,
				pt.y + dy + nrm.y * -2.0 * step,
				pt.z + dz + nrm.z * -2.0 * step};

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

	sy  = (minind * 3) / 8192;
	ey  = (octree->len * 3) / 8192 + 1;
	noi = sy * 8192 * 4;

	mt_log_debug("MOVING VOXELS");
	mt_log_debug("minind %i size %i", minind, octree->size);
	mt_log_debug("sy %i ey %i", sy, ey);
	mt_log_debug("noi %i", noi);

	octree_glc_upload_texbuffer(glc, model->colors, 0, 0, model->txwth, model->txhth, GL_RGB32F, GL_RGB, GL_FLOAT, glc->col1_tex, 6);
	octree_glc_upload_texbuffer(glc, model->normals, 0, 0, model->txwth, model->txhth, GL_RGB32F, GL_RGB, GL_FLOAT, glc->nrm1_tex, 8);

	if (data != (GLint*) octree->octs)
	{
	    mt_log_debug("OCTREE BACKING BUFFER RESIZED!!!");
	}

	data = (GLint*) octree->octs;

	octree_glc_upload_texbuffer(
	    glc,
	    data + noi,
	    0,
	    sy,
	    octree->txwth,
	    ey - sy,
	    GL_RGBA32I,
	    GL_RGBA_INTEGER,
	    GL_INT,
	    glc->oct1_tex,
	    10);
    }
}

#endif
