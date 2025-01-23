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

    int txwth;
    int txhth;
} octree_t;

octree_t    octree_create(v4_t base);
void        octree_delete(octree_t* octr);
void        octree_reset(octree_t* octr, v4_t base);
octets_t    octree_insert_fast(octree_t* octr, size_t arrind, size_t orind, v3_t pnt);
void        octree_insert_fast_octs(octree_t* octr, size_t arrind, size_t orind, int* octs);
void        octree_log_path(octets_t o, size_t index);

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

int octree_levels = 11;

octree_t octree_create(v4_t base)
{
    octree_t octr;
    octr.txwth    = 8192;
    octr.txhth    = 1;
    octr.size     = 8192 / 3;
    octr.octs     = mt_memory_calloc(sizeof(octets_t) * octr.size, NULL, NULL);
    octr.len      = 0;
    octr.leaves   = 0;
    octr.basecube = base;

    octr.octs[0] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}};

    octr.len = 1;

    return octr;
}

void octree_delete(octree_t* octr)
{
    REL(octr->octs);
}

void octree_reset(octree_t* octr, v4_t base)
{
    octr->len     = 0;
    octr->leaves  = 0;
    octr->octs[0] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}};

    octr->len = 1;
}

/* inserts new cube for a point creating the intermediate octree */

octets_t octree_insert_fast(octree_t* octr, size_t arrind, size_t orind, v3_t pnt)
{
    v4_t     cube   = octr->basecube;
    octets_t result = {0};

    for (int level = 0; level < octree_levels; level++)
    {
	// get octet index

	float size = cube.w / 2.0;

	int octet = (int) floor(pnt.x / size) % 2;
	int yi    = (int) floor(pnt.y / size) % 2;
	int zi    = (int) floor(pnt.z / size) % 2;

	if (yi == 0) octet += 2;
	if (zi == 0) octet += 4;

	/* printf("pnt %f %f %f level %i size %f octet %i\n", pnt.x, pnt.y, pnt.z, level, size, octet); */

	if (octr->octs[arrind].oct[octet] == 0)
	{
	    // store subnode in array

	    octr->octs[arrind].oct[octet] = octr->len;
	    octr->octs[octr->len++]       = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, orind}};

	    // resize array if needed

	    if (octr->len == octr->size)
	    {
		octr->size *= 2;
		octr->txhth = (octr->size * 3) / octr->txwth;
		octr->octs  = mt_memory_realloc(octr->octs, octr->size * sizeof(octets_t));
		if (octr->octs == NULL) mt_log_debug("not enough memory");
		mt_log_debug("octree array size %lu ", octr->size * sizeof(octets_t));
	    }

	    // zero out dirt if we octree was reset before
	    /* octr->octs[octr->len + 1] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}}; */

	    if (level == octree_levels - 1) octr->leaves++;
	}

	cube = (v4_t){
	    cube.x + xsizes[octet] * size,
	    cube.y - ysizes[octet] * size,
	    cube.z - zsizes[octet] * size,
	    size};

	// increase index and length

	arrind = octr->octs[arrind].oct[octet];

	// store debug info

	result.oct[level] = octet;
    }

    return result;
}

void octree_insert_fast_octs(octree_t* octr, size_t arrind, size_t orind, int* octs)
{
    /* printf("adding %i %i %i\n", octs[0], octs[1], octs[2]); */

    for (int level = 0; level < octree_levels; level++)
    {
	int octet = octs[level];

	/* printf("level %i arrind %i octet %i length %i size %i\n", level, arrind, octet, octr->len, octr->size); */

	if (octr->octs[arrind].oct[octet] == 0)
	{
	    // store subnode in array

	    octr->octs[arrind].oct[octet] = octr->len;
	    octr->octs[octr->len++]       = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, orind}};

	    // resize array if needed

	    if (octr->len == octr->size)
	    {
		octr->size *= 2;
		octr->txhth = (octr->size * 3) / octr->txwth;
		octr->octs  = mt_memory_realloc(octr->octs, octr->size * sizeof(octets_t));
		if (octr->octs == NULL)
		    mt_log_debug("not enough memory");
	    }

	    // zero out dirt if we octree was reset before
	    /* octr->octs[octr->len + 1] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}}; */

	    if (level == octree_levels - 1) octr->leaves++;
	}

	// increase index and length

	arrind = octr->octs[arrind].oct[octet];
    }
}

void octree_log_path(octets_t o, size_t index)
{
    mt_log_debug(
	"octet path for index %li : %i %i %i %i %i %i %i %i %i %i %i %i",
	index,
	o.oct[0],
	o.oct[1],
	o.oct[2],
	o.oct[3],
	o.oct[4],
	o.oct[5],
	o.oct[6],
	o.oct[7],
	o.oct[8],
	o.oct[9],
	o.oct[10],
	o.oct[11]

    );
}

#endif
