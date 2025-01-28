// model container

#ifndef model_h
#define model_h

#include <GL/glew.h>

#include "mt_log.c"
#include "mt_memory.c"
#include "mt_vector_3d.c"
#include "mt_vector_4d.c"
#include <stdbool.h>

typedef struct model_t
{
    GLfloat* vertexes;
    GLfloat* colors;
    GLfloat* normals;
    int*     ranges;
    long     point_count;
    long     ind;
    long     cnt;
    v3_t     offset;

    int    txwth;
    int    txhth;
    size_t buffs;
    int    comps;
} model_t;

model_t model_init();
void    model_delete(model_t* model);
void    model_add_point(model_t* model, v3_t vertex, v3_t normal, v3_t color);

void model_load_flat(model_t* model, char* vertex_path, char* color_path, char* normal_path, char* range_path);
void model_log_vertex_info(model_t* model, size_t index);

#endif

#if __INCLUDE_LEVEL__ == 0

model_t model_init()
{
    model_t model = {0};
    model.txwth   = 8192;
    model.comps   = 3;
    return model;
}

void model_load_flat(model_t* model, char* pntpath, char* colpath, char* nrmpath, char* rngpath)
{
    // build up range array also ( x,y,z ranges)

    FILE* pntfile = fopen(pntpath, "rb");
    FILE* nrmfile = fopen(nrmpath, "rb");
    FILE* colfile = fopen(colpath, "rb");

    fseek(pntfile, 0, SEEK_END);
    long point_count = ftell(pntfile) / sizeof(float);
    fseek(pntfile, 0, SEEK_SET);

    model->point_count = point_count / 3;
    model->txhth       = (int) ceilf((float) model->point_count / (float) model->txwth);
    model->buffs       = model->txwth * model->txhth * model->comps * sizeof(GLfloat);

    model->vertexes = mt_memory_alloc(model->buffs, NULL, NULL);
    model->normals  = mt_memory_alloc(model->buffs, NULL, NULL);
    model->colors   = mt_memory_alloc(model->buffs, NULL, NULL);

    if (!pntfile || !nrmfile || !colfile)
    {
	printf("Unable to open file!\n");
	return;
    }

    fread(model->vertexes, sizeof(float), point_count, pntfile);
    fread(model->normals, sizeof(float), point_count, nrmfile);
    fread(model->colors, sizeof(float), point_count, colfile);

    fclose(pntfile);
    fclose(nrmfile);
    fclose(colfile);

    FILE* rngfile = fopen(rngpath, "rb");

    if (!rngfile)
    {
	printf("Unable to open file!\n");
	return;
    }

    fseek(rngfile, 0, SEEK_END);
    long range_size = ftell(rngfile);
    fseek(rngfile, 0, SEEK_SET);

    model->ranges = mt_memory_alloc(range_size, NULL, NULL);
    fread(model->ranges, sizeof(int), range_size / sizeof(int), rngfile);

    fclose(rngfile);

    mt_log_debug("loading flat model data\n%s\n%s\n%s, point count %lu range count %lu", pntpath, colpath, nrmpath, point_count, range_size / sizeof(int));

    /* for (int i = 0; i < 300; i += 3) */
    /* 	mt_log_debug("pt %f %f %f", model->vertexes[i], model->vertexes[i + 1], model->vertexes[i + 2]); */
}

void model_delete(model_t* model)
{
    REL(model->vertexes);
    REL(model->normals);
    REL(model->colors);
}

void model_log_vertex_info(model_t* model, size_t index)
{
    v3_t vertex;
    v3_t normal;
    v3_t color;

    vertex.x = model->vertexes[index];
    vertex.y = model->vertexes[index + 1];
    vertex.z = model->vertexes[index + 2];

    normal.x = model->normals[index];
    normal.y = model->normals[index + 1];
    normal.z = model->normals[index + 2];

    color.x = model->colors[index];
    color.y = model->colors[index + 1];
    color.z = model->colors[index + 2];

    mt_log_debug(
	"\npoint %lu\nvertex %f %f %f\nnormal %f %f %f\ncolor %f %f %f",
	index,
	vertex.x, vertex.y, vertex.z,
	normal.x, normal.y, normal.z,
	color.x, color.y, color.z);
}

void model_add_point(model_t* model, v3_t vertex, v3_t normal, v3_t color)
{
    /* if (model->index + 3 > model->point_count - 1) */
    /* { */
    /* 	model->point_count *= 2; */
    /* 	model->vertexes = mt_memory_realloc(model->vertexes, sizeof(GLfloat) * model->point_count * 3); */
    /* 	model->normals  = mt_memory_realloc(model->normals, sizeof(GLfloat) * model->point_count * 3); */
    /* 	model->colors   = mt_memory_realloc(model->colors, sizeof(GLfloat) * model->point_count * 3); */
    /* } */

    model->point_count += 1;

    model->txhth = (int) ceilf((float) model->point_count / (float) model->txwth);
    model->buffs = model->txwth * model->txhth * model->comps * sizeof(GLfloat);

    model->vertexes = mt_memory_realloc(model->vertexes, model->buffs);
    model->normals  = mt_memory_realloc(model->normals, model->buffs);
    model->colors   = mt_memory_realloc(model->colors, model->buffs);

    int index = model->point_count - 1;

    model->vertexes[index]     = vertex.x;
    model->vertexes[index + 1] = vertex.y;
    model->vertexes[index + 2] = vertex.z;

    model->normals[index]     = normal.x;
    model->normals[index + 1] = normal.y;
    model->normals[index + 2] = normal.z;

    model->colors[index]     = color.x;
    model->colors[index + 1] = color.y;
    model->colors[index + 2] = color.z;
}

#endif
