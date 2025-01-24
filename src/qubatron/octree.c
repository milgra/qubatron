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

    int txwth;
    int txhth;
    int levels;
} octree_t;

octree_t    octree_create(v4_t base, int maxlevel);
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

octree_t octree_create(v4_t base, int maxlevel)
{
    octree_t octr;
    octr.basecube = base;
    octr.levels   = maxlevel;
    octr.txwth    = 8192;
    octr.txhth    = 1;
    octr.size     = 8192 / 3;
    octr.octs     = mt_memory_calloc(sizeof(octets_t) * octr.size, NULL, NULL);

    octr.octs[0] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}};
    octr.len     = 1;

    return octr;
}

void octree_delete(octree_t* octr)
{
    REL(octr->octs);
}

void octree_reset(octree_t* octr, v4_t base)
{
    octr->len     = 0;
    octr->octs[0] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}};

    octr->len = 1;
}

octets_t octree_insert_fast(octree_t* octr, size_t index, size_t orind, v3_t pnt)
{
    v4_t     cube   = octr->basecube;
    octets_t result = {0};

    for (int level = 0; level < octr->levels; level++)
    {
	float size = cube.w / 2.0;

	int octet = (int) floor(pnt.x / size) % 2;
	int yi    = (int) floor(pnt.y / size) % 2;
	int zi    = (int) floor(pnt.z / size) % 2;

	if (yi == 0) octet += 2;
	if (zi == 0) octet += 4;

	if (octr->octs[index].oct[octet] == 0)
	{
	    // store subnode in array

	    octr->octs[index].oct[octet] = octr->len;
	    octr->octs[octr->len++]      = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, orind}};

	    // resize array if needed

	    if (octr->len == octr->size)
	    {
		octr->size *= 2;
		octr->txhth = (octr->size * 3) / octr->txwth;
		octr->octs  = mt_memory_realloc(octr->octs, octr->size * sizeof(octets_t));
		if (octr->octs == NULL) mt_log_debug("not enough memory");
		mt_log_debug("octree array size %lu ", octr->size * sizeof(octets_t));
	    }
	}

	cube = (v4_t){
	    cube.x + xsizes[octet] * size,
	    cube.y - ysizes[octet] * size,
	    cube.z - zsizes[octet] * size,
	    size};

	// increase index and length

	index = octr->octs[index].oct[octet];

	// store debug info

	result.oct[level] = octet;
    }

    return result;
}

void octree_insert_fast_octs(octree_t* octr, size_t index, size_t orind, int* octs)
{
    for (int level = 0; level < octr->levels; level++)
    {
	int octet = octs[level];

	if (octr->octs[index].oct[octet] == 0)
	{
	    // store subnode in array

	    octr->octs[index].oct[octet] = octr->len;
	    octr->octs[octr->len++]      = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, orind}};

	    // resize array if needed

	    if (octr->len == octr->size)
	    {
		octr->size *= 2;
		octr->txhth = (octr->size * 3) / octr->txwth;
		octr->octs  = mt_memory_realloc(octr->octs, octr->size * sizeof(octets_t));
		if (octr->octs == NULL)
		    mt_log_debug("not enough memory");
	    }
	}

	// increase index and length

	index = octr->octs[index].oct[octet];
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
