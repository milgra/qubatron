#ifndef octree_h
#define octree_h

// #include <GLES2/gl2.h>
#include "mt_log.c"
#include "mt_memory.c"
#include "mt_vector_3d.c"
#include "mt_vector_4d.c"
#include <GL/glew.h>

typedef struct octets_t
{
    GLint oct[12]; // 8 octets, 1 orignal index, 3 for padding
} octets_t;

typedef struct octree_t
{
    v4_t      basecube;
    octets_t* octs;
    size_t    size;
    size_t    len;
    size_t    leaves;
} octree_t;

octree_t    octree_create(size_t size, v4_t base);
void        octree_delete(octree_t* arr);
void        octree_reset(octree_t* arr, v4_t base);
void        octree_insert_fast(octree_t* arr, size_t arrind, size_t orind, v3_t pnt, bool* leaf);
void        octree_insert_fast_octs(octree_t* arr, size_t arrind, size_t orind, int* octs, bool* leaf);

#endif

#if __INCLUDE_LEVEL__ == 0

// octets
//     4 5
//     6 7
// 0 1
// 2 3

static const float xsizes[8] = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
static const float ysizes[8] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0};
static const float zsizes[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};

octree_t octree_create(size_t size, v4_t base)
{
    octree_t arr;
    arr.size     = size;
    arr.octs     = mt_memory_alloc(sizeof(octets_t) * arr.size, NULL, NULL);
    arr.len      = 0;
    arr.leaves   = 0;
    arr.basecube = base;

    arr.octs[0] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}};

    arr.len = 1;

    return arr;
}

void octree_delete(octree_t* arr)
{
    REL(arr->octs);
}

void octree_reset(octree_t* arr, v4_t base)
{
    arr->len     = 0;
    arr->leaves  = 0;
    arr->octs[0] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}};

    arr->len = 1;
}

/* inserts new cube for a point creating the intermediate octree */

void octree_insert_fast(octree_t* arr, size_t arrind, size_t orind, v3_t pnt, bool* leaf)
{
    int  levels = 12;
    v4_t cube   = arr->basecube;

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

	if (arr->octs[arrind].oct[octet] == 0)
	{
	    // store subnode in array

	    arr->octs[arrind].oct[octet] = arr->len;
	    arr->octs[arr->len++]        = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, orind}};

	    // resize array if needed

	    if (arr->len == arr->size)
	    {
		arr->size *= 2;
		arr->octs = mt_memory_realloc(arr->octs, arr->size * sizeof(octets_t));
		if (arr->octs == NULL) mt_log_debug("not enough memory");
	    }

	    if (level == levels - 1)
	    {
		arr->leaves++;
		*leaf = true;
	    }
	}

	cube = (v4_t){
	    cube.x + xsizes[octet] * size,
	    cube.y - ysizes[octet] * size,
	    cube.z - zsizes[octet] * size,
	    size};

	// increase index and length

	arrind = arr->octs[arrind].oct[octet];
    }
}

void octree_insert_fast_octs(octree_t* arr, size_t arrind, size_t orind, int* octs, bool* leaf)
{
    int levels = 12;

    for (int level = 0; level < levels; level++)
    {
	int octet = octs[level];

	if (arr->octs[arrind].oct[octet] == 0)
	{
	    // store subnode in array

	    arr->octs[arrind].oct[octet] = arr->len;
	    arr->octs[arr->len++]        = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, orind}};

	    // resize array if needed

	    if (arr->len == arr->size)
	    {
		arr->size *= 2;
		arr->octs = mt_memory_realloc(arr->octs, arr->size * sizeof(octets_t));
		if (arr->octs == NULL) mt_log_debug("not enough memory");
	    }

	    if (level == levels - 1)
	    {
		arr->leaves++;
		*leaf = true;
	    }
	}

	// increase index and length

	arrind = arr->octs[arrind].oct[octet];
    }
}

#endif
