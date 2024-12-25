// model container

#ifndef model_h
#define model_h

#include <GL/glew.h>

#include "mt_log.c"
#include "mt_memory.c"
#include "mt_vector_3d.c"
#include "rply.h"
#include <stdbool.h>

typedef struct model_t
{
    GLfloat* vertexes;
    GLfloat* colors;
    GLfloat* normals;
    long     point_count;
    long     leaf_count;
    long     index;
    long     ind;
    long     cnt;
    v3_t     offset;
} model_t;

model_t model_init(char* path, v3_t offset);

#endif

#if __INCLUDE_LEVEL__ == 0

float minpx, maxpx, minpy, maxpy, minpz, maxpz, mindx, mindy, mindz, lx, ly, lz;
float px, py, pz, cx, cy, cz, nx, ny, nz;

static int model_vertex_cb(p_ply_argument argument)
{
    long eol;

    model_t* model;
    ply_get_argument_user_data(argument, (void**) &model, &eol);

    if (model->ind == 0) px = (ply_get_argument_value(argument) - model->offset.x);
    if (model->ind == 1) pz = (ply_get_argument_value(argument) - model->offset.y);
    if (model->ind == 2) py = (ply_get_argument_value(argument) - model->offset.z);

    if (model->ind == 3) cx = ply_get_argument_value(argument);
    if (model->ind == 4) cy = ply_get_argument_value(argument);
    if (model->ind == 5) cz = ply_get_argument_value(argument);

    if (model->ind == 6) nx = ply_get_argument_value(argument);
    if (model->ind == 7) ny = ply_get_argument_value(argument);
    if (model->ind == 8)
    {
	nz = ply_get_argument_value(argument);

	model->vertexes[model->index]     = px;
	model->vertexes[model->index + 1] = py;
	model->vertexes[model->index + 2] = pz;

	model->normals[model->index]     = nx;
	model->normals[model->index + 1] = nz;
	model->normals[model->index + 2] = ny;
	model->normals[model->index + 3] = 0.0;

	model->colors[model->index]     = cx / 255.0;
	model->colors[model->index + 1] = cy / 255.0;
	model->colors[model->index + 2] = cz / 255.0;
	model->colors[model->index + 3] = 1.0;

	model->index += 4;

	model->ind = -1;

	if (model->cnt == 0)
	{
	    minpx = px;
	    minpy = py;
	    minpz = pz;
	    maxpx = px;
	    maxpy = py;
	    maxpz = pz;
	    mindx = 1000.0;
	    mindy = 1000.0;
	    mindz = 1000.0;
	}
	else
	{
	    if (px < minpx) minpx = px;
	    if (py < minpy) minpy = py;
	    if (pz < minpz) minpz = pz;
	    if (px > maxpx) maxpx = px;
	    if (py > maxpy) maxpy = py;
	    if (pz > maxpz) maxpz = pz;
	    if (px > 0.0 && fabs(px - lx) < mindx) mindx = px - lx;
	    if (py > 0.0 && fabs(py - ly) < mindy) mindy = py - ly;
	    if (pz > 0.0 && fabs(pz - lz) < mindz) mindz = pz - lz;
	}

	lx = px;
	ly = py;
	lz = pz;

	if (model->cnt % 1000000 == 0) printf("progress %f\n", (float) model->cnt / (float) model->point_count);

	model->cnt++;
    }

    model->ind++;

    /* if (model->cnt < 1000) */
    /* { */
    /* 	printf("%li : %g", ind, ply_get_argument_value(argument)); */
    /* 	if (eol) */
    /* 	    printf("\n"); */
    /* 	else */
    /* 	    printf(" "); */
    /* } */
    return 1;
}

// init test model
/* bool leaf = false; */
/* octree_insert1(&cubearr, 0, (v3_t){110.0, 790.0, -110.0}, (v4_t){110.0, 790.0, -110.0, 0.0}, (v4_t){1.0, 1.0, 1.0, 1.0}, &leaf); */
/* octree_insert1(&cubearr, 0, (v3_t){110.0, 440.0, -110.0}, (v4_t){110.0, 790.0, -110.0, 0.0}, (v4_t){1.0, 1.0, 1.0, 1.0}, &leaf); */

model_t model_init(char* path, v3_t offset)
{
    model_t model = {0};

    model.offset = offset;

    p_ply ply = ply_open(path, NULL, 0, NULL);

    if (!ply) mt_log_debug("cannot read %s", path);
    if (!ply_read_header(ply)) mt_log_debug("cannot read ply header");

    model.point_count = ply_set_read_cb(ply, "vertex", "x", model_vertex_cb, &model, 0);
    ply_set_read_cb(ply, "vertex", "y", model_vertex_cb, &model, 0);
    ply_set_read_cb(ply, "vertex", "z", model_vertex_cb, &model, 1);
    ply_set_read_cb(ply, "vertex", "red", model_vertex_cb, &model, 0);
    ply_set_read_cb(ply, "vertex", "green", model_vertex_cb, &model, 0);
    ply_set_read_cb(ply, "vertex", "blue", model_vertex_cb, &model, 1);
    ply_set_read_cb(ply, "vertex", "nx", model_vertex_cb, &model, 0);
    ply_set_read_cb(ply, "vertex", "ny", model_vertex_cb, &model, 0);
    ply_set_read_cb(ply, "vertex", "nz", model_vertex_cb, &model, 1);

    mt_log_debug("cloud point count : %lu", model.point_count);

    model.vertexes = mt_memory_alloc(sizeof(GLfloat) * model.point_count * 4, NULL, NULL);
    model.normals  = mt_memory_alloc(sizeof(GLfloat) * model.point_count * 4, NULL, NULL);
    model.colors   = mt_memory_alloc(sizeof(GLfloat) * model.point_count * 4, NULL, NULL);

    if (!ply_read(ply)) return model;
    ply_close(ply);

    mt_log_debug("minpx %f maxpx %f minpy %f maxpy %f minpz %f maxpz %f mindx %f mindy %f mindz %f\n", minpx, maxpx, minpy, maxpy, minpz, maxpz, mindx, mindy, mindz);

    return model;
}

#endif
