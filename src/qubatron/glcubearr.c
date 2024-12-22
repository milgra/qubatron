#ifndef glcubearr_h
#define glcubearr_h

// #include <GLES2/gl2.h>
#include "mt_log.c"
#include "mt_memory.c"
#include "mt_vector_3d.c"
#include <GL/glew.h>

typedef struct glvec4_t
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
} glvec4_t;

typedef struct glcube_t
{
    GLint ind;
    GLint oct[11]; // 8 octets, we need 11 for std430 padding
} glcube_t;

typedef struct glcubearr_t
{
    glvec4_t  basecube;
    glcube_t* cubes;
    size_t    size;
    size_t    len;
    size_t    leaves;
} glcubearr_t;

glcubearr_t glcubearr_create(size_t size, glvec4_t base);
void        glcubearr_delete(glcubearr_t* arr);
void        glcubearr_reset(glcubearr_t* arr, glvec4_t base);
void        glcubearr_insert_fast(glcubearr_t* arr, size_t arrind, size_t orind, v3_t pnt, bool* leaf);
void        glcubearr_insert_fast_octs(glcubearr_t* arr, size_t arrind, size_t orind, int* octs, bool* leaf);

#endif

#if __INCLUDE_LEVEL__ == 0

static const float glcube_res = 0.5;

// octets
//     4 5
//     6 7
// 0 1
// 2 3

static const float xsizes[8] = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
static const float ysizes[8] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0};
static const float zsizes[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};

glcubearr_t glcubearr_create(size_t size, glvec4_t base)
{
    glcubearr_t arr;
    arr.size     = size;
    arr.cubes    = mt_memory_alloc(sizeof(glcube_t) * arr.size, NULL, NULL);
    arr.len      = 0;
    arr.leaves   = 0;
    arr.basecube = base;

    arr.cubes[0] = (glcube_t){
	0,
	{0, 0, 0, 0, 0, 0, 0, 0}};

    arr.len = 1;

    return arr;
}

void glcubearr_delete(glcubearr_t* arr)
{
    REL(arr->cubes);
}

void glcubearr_reset(glcubearr_t* arr, glvec4_t base)
{
    arr->len      = 0;
    arr->leaves   = 0;
    arr->cubes[0] = (glcube_t){
	0,
	{0, 0, 0, 0, 0, 0, 0, 0}};

    arr->len = 1;
}

/* inserts new cube for a point creating the intermediate octree */

void glcubearr_insert_fast(glcubearr_t* arr, size_t arrind, size_t orind, v3_t pnt, bool* leaf)
{
    int      levels = 12;
    glvec4_t cube   = arr->basecube;

    for (int level = 0; level < levels; level++)
    {
	// get octet index

	float size = cube.w / 2.0;

	int octet = (int) floor(pnt.x / size) % 2;
	int yi    = (int) floor(pnt.y / size) % 2;
	int zi    = (int) floor(-1.0 * pnt.z / size) % 2;

	if (yi == 0) octet += 2;
	if (zi == 1) octet += 4;

	/* printf("level %i size %f octet %i %i %i %i\n", level, size, octet, xi, yi, zi); */

	if (arr->cubes[arrind].oct[octet] == 0)
	{
	    // store subnode in array

	    arr->cubes[arrind].oct[octet] = arr->len;
	    arr->cubes[arr->len++]        = (glcube_t){orind, {0, 0, 0, 0, 0, 0, 0, 0}};

	    // resize array if needed

	    if (arr->len == arr->size)
	    {
		arr->size *= 2;
		arr->cubes = mt_memory_realloc(arr->cubes, arr->size * sizeof(glcube_t));
		if (arr->cubes == NULL) mt_log_debug("not enough memory");
	    }

	    if (level == levels - 1)
	    {
		arr->leaves++;
		*leaf = true;
	    }
	}

	cube = (glvec4_t){
	    cube.x + xsizes[octet] * size,
	    cube.y - ysizes[octet] * size,
	    cube.z - zsizes[octet] * size,
	    size};

	// increase index and length

	arrind = arr->cubes[arrind].oct[octet];
    }
}

void glcubearr_insert_fast_octs(glcubearr_t* arr, size_t arrind, size_t orind, int* octs, bool* leaf)
{
    int levels = 12;

    for (int level = 0; level < levels; level++)
    {
	int octet = octs[level];

	if (arr->cubes[arrind].oct[octet] == 0)
	{
	    // store subnode in array

	    arr->cubes[arrind].oct[octet] = arr->len;
	    arr->cubes[arr->len++]        = (glcube_t){orind, {0, 0, 0, 0, 0, 0, 0, 0}};

	    // resize array if needed

	    if (arr->len == arr->size)
	    {
		arr->size *= 2;
		arr->cubes = mt_memory_realloc(arr->cubes, arr->size * sizeof(glcube_t));
		if (arr->cubes == NULL) mt_log_debug("not enough memory");
	    }

	    if (level == levels - 1)
	    {
		arr->leaves++;
		*leaf = true;
	    }
	}

	// increase index and length

	arrind = arr->cubes[arrind].oct[octet];
    }
}

#endif
