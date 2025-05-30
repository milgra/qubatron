#ifndef octree_h
#define octree_h

#include "mt_log.c"
#include "mt_memory.c"
#include "mt_vector_3d.c"
#include "mt_vector_4d.c"
#include <GL/glew.h>
#include <float.h>

typedef struct octets_t
{
    GLint oct[12]; // 8 octets, 1 point index for octets, 3 for padding
} octets_t;

typedef struct octree_t
{
    v4_t      basecube;
    octets_t* octs;
    size_t    size;
    size_t    len;
    int       levels;
} octree_t;

octree_t    octree_create(v4_t base, int maxlevel);
octree_t    octree_clone(octree_t* tree);
void        octree_delete(octree_t* tree);
void        octree_reset(octree_t* tree, v4_t base);
octets_t    octree_insert_point(octree_t* tree, size_t index, size_t modind, v3_t pnt, int* octindarr);
void        octree_insert_path(octree_t* tree, size_t index, size_t modind, int* octs14, int* octs54, int* octs94);
void        octree_remove_point(octree_t* tree, v3_t pnt, int* modind, int* octind);
void        octree_log_path(octets_t o, size_t index);
void        octree_comp(octree_t* treea, octree_t* treeb);
int         octree_trace_line(octree_t* tree, v3_t pos, v3_t dir, v4_t* tlf);
void        octree_model_index(octree_t* tree, v3_t pnt, int* modind, int* octind);

#endif

#if __INCLUDE_LEVEL__ == 0

// octets
//     4 5
//     6 7
// 0 1
// 2 3

static const float xsft[8] = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0}; // x shift
static const float ysft[8] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0}; // y shift
static const float zsft[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}; // z shift

static const int horpairs[8] = {1, 0, 3, 2, 5, 4, 7, 6}; // horizontal pairs of octets
static const int verpairs[8] = {2, 3, 0, 1, 6, 7, 4, 5}; // vertical pairs of octets
static const int deppairs[8] = {4, 5, 6, 7, 0, 1, 2, 3}; // depth pairs of octets

octree_t octree_create(v4_t base, int maxlevel)
{
    octree_t tree;
    tree.basecube = base;
    tree.levels   = maxlevel;
    tree.size     = 2;
    tree.octs     = mt_memory_calloc(sizeof(octets_t) * tree.size, NULL, NULL);

    tree.octs[0] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}};
    tree.len     = 1;

    return tree;
}

octree_t octree_clone(octree_t* ptree)
{
    octree_t tree;
    tree.basecube = ptree->basecube;
    tree.levels   = ptree->levels;
    tree.size     = ptree->size;
    tree.octs     = mt_memory_calloc(sizeof(octets_t) * tree.size, NULL, NULL);

    memcpy(tree.octs, ptree->octs, sizeof(octets_t) * tree.size);

    tree.len = ptree->len;

    return tree;
}

void octree_delete(octree_t* tree)
{
    REL(tree->octs);
}

void octree_reset(octree_t* tree, v4_t base)
{
    tree->len     = 1;
    tree->octs[0] = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}};
}

octets_t octree_insert_point(octree_t* tree, size_t index, size_t modind, v3_t pnt, int* octindarr)
{
    float    size   = tree->basecube.w;
    octets_t result = {0};

    for (int level = 0; level < tree->levels; level++)
    {
	size /= 2.0;

	int octet = (int) (pnt.x / size) % 2;
	int yi    = (int) (pnt.y / size) % 2;
	int zi    = (int) (pnt.z / size) % 2;

	if (yi == 0) octet += 2;
	if (zi == 0) octet += 4;

	if (tree->octs[index].oct[octet] == 0)
	{
	    // store first modified octree index

	    if (octindarr != NULL)
	    {
		octindarr[level]     = index;
		octindarr[level + 1] = tree->len;
	    }

	    // store subnode in array

	    tree->octs[index].oct[octet] = tree->len;
	    tree->octs[tree->len++]      = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, modind}};

	    // resize array if needed

	    if (tree->len == tree->size)
	    {
		tree->size += 100000;
		tree->octs = mt_memory_realloc(tree->octs, tree->size * sizeof(octets_t));
		if (tree->octs == NULL) mt_log_debug("not enough memory");
		mt_log_debug("octree array size %lu\033[1A", tree->size * sizeof(octets_t));
	    }
	}

	// increase index and length

	index = tree->octs[index].oct[octet];

	// store debug info

	result.oct[level] = octet;
    }

    return result;
}

void octree_insert_path(octree_t* tree, size_t index, size_t modind, int* octs14, int* octs54, int* octs94)
{
    for (int level = 0; level < tree->levels; level++)
    {
	// because of webgl transform feedback limitations we have to use 3 separate int buffers
	int octet = octs14[level];
	if (level > 3) octet = octs54[level - 4];
	if (level > 7) octet = octs94[level - 8];

	if (tree->octs[index].oct[octet] == 0)
	{
	    // store subnode in array

	    tree->octs[index].oct[octet] = tree->len;
	    tree->octs[tree->len++]      = (octets_t){{0, 0, 0, 0, 0, 0, 0, 0, modind}};

	    // resize array if needed

	    if (tree->len == tree->size)
	    {
		tree->size += 100000;
		tree->octs = mt_memory_realloc(tree->octs, tree->size * sizeof(octets_t));
		if (tree->octs == NULL)
		    mt_log_debug("not enough memory");
	    }
	}

	// increase index and length

	index = tree->octs[index].oct[octet];
    }
}

void octree_remove_point(octree_t* tree, v3_t pnt, int* modind, int* octind)
{
    float size   = tree->basecube.w;
    int   index  = 0;
    int   lindex = 0;
    int   loctet = 0;

    for (int level = 0; level < tree->levels + 1; level++)
    {
	size /= 2.0;

	int octet = (int) (pnt.x / size) % 2;
	int yi    = (int) (pnt.y / size) % 2;
	int zi    = (int) (pnt.z / size) % 2;

	if (yi == 0) octet += 2;
	if (zi == 0) octet += 4;

	if (level == tree->levels)
	{
	    // return original index and octet index

	    *modind = tree->octs[index].oct[8];
	    *octind = lindex;

	    // zero out branch

	    tree->octs[lindex].oct[loctet] = 0;
	}

	lindex = index;
	loctet = octet;
	index  = tree->octs[index].oct[octet];

	if (index == 0) return;
    }
}

void octree_model_index(octree_t* tree, v3_t pnt, int* modind, int* octind)
{
    float size   = tree->basecube.w;
    int   index  = 0;
    int   lindex = 0;
    int   loctet = 0;

    for (int level = 0; level < tree->levels + 1; level++)
    {
	size /= 2.0;

	int octet = (int) (pnt.x / size) % 2;
	int yi    = (int) (pnt.y / size) % 2;
	int zi    = (int) (pnt.z / size) % 2;

	if (yi == 0) octet += 2;
	if (zi == 0) octet += 4;

	if (level == tree->levels)
	{
	    // return original index and octet index

	    *modind = tree->octs[index].oct[8];
	    *octind = lindex;
	}

	lindex = index;
	loctet = octet;
	index  = tree->octs[index].oct[octet];

	if (index == 0) return;
    }
}

void octree_log_path(octets_t o, size_t index)
{
    mt_log_debug(
	"ind %li path %i %i %i %i %i %i %i %i %i %i %i %i",
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

void octree_comp(octree_t* treea, octree_t* treeb)
{
    for (int i = 0; i < treeb->len; i++)
    {
	octets_t octsa = treea->octs[i];
	octets_t octsb = treeb->octs[i];

	for (int j = 0; j < 12; j++)
	{
	    if (octsa.oct[j] != octsb.oct[j])
	    {
		mt_log_debug("OCTREE DIFF AT %i : OCTET : %i : %i != %i", i, j, octsa.oct[j], octsb.oct[j]);
		return;
	    }
	}
    }
}

typedef struct _stck_t
{
    v4_t cube;    // cube dimensions for stack level
    v4_t isps[4]; // is points for stack level cube
    int  octs[4]; // octet numbers for isps
    int  ispsi;   // | 0 level checked | 1 isps ind0 | 2 isps ind1 | 3 isps ind2 | 4 | 5 ispsp len0 | 6 isps len1 | 7 isps len2 |
    int  octi;    // octets index for stack level
} stck_t;

v4_t is_cube_xplane(float x, v3_t lp, v3_t lv)
{
    v4_t r = (v4_t){0.0, 0.0, 0.0, FLT_MAX};
    if (lv.x != 0.0)
    {
	r.w = (x - lp.x) / lv.x;
	r.y = lp.y + lv.y * r.w;
	r.z = lp.z + lv.z * r.w;
	r.x = x;
    }
    return r;
}

v4_t is_cube_yplane(float y, v3_t lp, v3_t lv)
{
    v4_t r = (v4_t){0.0, 0.0, 0.0, FLT_MAX};
    if (lv.y != 0.0)
    {
	r.w = (y - lp.y) / lv.y;
	r.x = lp.x + lv.x * r.w;
	r.z = lp.z + lv.z * r.w;
	r.y = y;
    }
    return r;
}

v4_t is_cube_zplane(float z, v3_t lp, v3_t lv)
{
    v4_t r = (v4_t){0.0, 0.0, 0.0, FLT_MAX};
    if (lv.z != 0.0)
    {
	r.w = (z - lp.z) / lv.z;
	r.x = lp.x + lv.x * r.w;
	r.y = lp.y + lv.y * r.w;
	r.z = z;
    }
    return r;
}

int octree_trace_line(octree_t* tree, v3_t pos, v3_t dir, v4_t* otlf)
{
    int level = 0;

    // under 17 and over 20 shader becomes very slow, why?
    stck_t stck[18];
    stck[0].cube  = tree->basecube;
    stck[0].octi  = 0;
    stck[0].ispsi = 0;

    v4_t act;
    v4_t tlf = tree->basecube;
    v4_t brb = (v4_t){tlf.x + tlf.w, tlf.y - tlf.w, tlf.z - tlf.w, 0.0};

    int  hitc = 0;
    v4_t hitp[4];

    // front side
    act = is_cube_zplane(tlf.z, pos, dir);
    if (act.w < FLT_MAX && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
	hitp[hitc++] = act;

    // back side
    act = is_cube_zplane(brb.z, pos, dir);
    if (act.w < FLT_MAX && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
	hitp[hitc++] = act;

    // left side
    act = is_cube_xplane(tlf.x, pos, dir);
    if (act.w < FLT_MAX && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
	hitp[hitc++] = act;

    // right side
    act = is_cube_xplane(brb.x, pos, dir);
    if (act.w < FLT_MAX && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
	hitp[hitc++] = act;

    // top side
    act = is_cube_yplane(tlf.y, pos, dir);
    if (act.w < FLT_MAX && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
	hitp[hitc++] = act;

    // bottom side
    act = is_cube_yplane(brb.y, pos, dir);
    if (act.w < FLT_MAX && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
	hitp[hitc++] = act;

    // we don't deal with the outside world
    if (hitc < 2) return 0;

    // one hitp have to be in front of the camera
    if (hitp[0].w < 0.0 && hitp[1].w < 0.0) return 0;

    // order side hitpoints
    if (hitp[1].w < hitp[0].w) hitp[0] = hitp[1];

    // inside cube, starting isp will be pos
    if (hitp[0].w < 0.0) hitp[0] = (v4_t){pos.x, pos.y, pos.z, 0.0};

    // store starting isp
    stck[level].isps[0] = hitp[0];

    while (true)
    {
	tlf = stck[level].cube;

	// bingo, we reached bottom, return index of point
	if (level == tree->levels)
	{
	    if (otlf) *otlf = tlf;
	    return tree->octs[stck[level].octi].oct[8];
	}

	// check subnode intersection if needed
	if (stck[level].ispsi == 0)
	{
	    stck[level].ispsi = 128;

	    v4_t brb = (v4_t){tlf.x + tlf.w, tlf.y - tlf.w, tlf.z - tlf.w, 0.0};
	    v4_t hlf = v4_add(brb, v4_scale(v4_sub(tlf, brb), 0.5));

	    hitc    = 1;
	    hitp[0] = stck[level].isps[0];

	    // z div plane
	    act = is_cube_zplane(hlf.z, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y) hitp[hitc++] = act;

	    // x div plane
	    act = is_cube_xplane(hlf.x, pos, dir);
	    if (act.w > 0.0 && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z) hitp[hitc++] = act;

	    // y div plane
	    act = is_cube_yplane(hlf.y, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z) hitp[hitc++] = act;

	    int oct = 0;
	    int pre = -1;

	    for (int i = 0; i < hitc; ++i)
	    {
		// order hitpoints
		if (i < hitc - 1)
		{
		    for (int j = i + 1; j < hitc; ++j)
		    {
			if (hitp[j].w < hitp[i].w)
			{
			    act     = hitp[i];
			    hitp[i] = hitp[j];
			    hitp[j] = act;
			}
		    }
		}

		act = hitp[i];
		oct = 0;

		// calculate octet number for current smallest
		if (act.x > hlf.x) oct = 1;
		if (act.y < hlf.y) oct += 2;
		if (act.z < hlf.z) oct += 4;

		// from the second isp find octet pairs if needed
		if (oct == pre)
		{
		    if (act.x == hlf.x)
			oct = horpairs[oct];
		    else if (act.y == hlf.y)
			oct = verpairs[oct];
		    else if (act.z == hlf.z)
			oct = deppairs[oct];
		}

		pre = oct;

		int octi = tree->octs[stck[level].octi].oct[oct];

		// add to isps if there are subnodes in current cube
		if (octi > 0)
		{
		    int ind               = stck[level].ispsi;
		    int len               = ind & 0x0F;
		    stck[level].octs[len] = oct;
		    stck[level].isps[len] = act;
		    len++;
		    stck[level].ispsi = (ind & 0xF0) | len;
		}
	    }
	}

	// go inside closest subnode

	int cur_len = stck[level].ispsi & 0x0F;

	if (cur_len > 0)
	{
	    int  nxt_ind = (stck[level].ispsi >> 4) & 7;
	    v4_t nxt_isp = stck[level].isps[nxt_ind];
	    int  nxt_oct = stck[level].octs[nxt_ind];

	    float halfs = tlf.w / 2.0;
	    v4_t  ncube = (v4_t){
                tlf.x += xsft[nxt_oct] * halfs,
                tlf.y -= ysft[nxt_oct] * halfs,
                tlf.z -= zsft[nxt_oct] * halfs,
                halfs};

	    nxt_ind++;
	    cur_len--;

	    // increase index and decrease length at current level
	    stck[level].ispsi = 128 | (nxt_ind << 4) | cur_len;

	    int octi = tree->octs[stck[level].octi].oct[nxt_oct];

	    // increase stack level
	    level += 1;

	    // reset containers for the next level
	    stck[level].cube    = ncube;
	    stck[level].ispsi   = 0;
	    stck[level].octi    = octi;
	    stck[level].isps[0] = nxt_isp;
	}
	else
	{
	    // no intersection with subnodes
	    // step back one for possible intersection with a neighbor node
	    stck[level--].ispsi = 0;

	    if (level < 0) return 0;
	}
    }

    return 0;
}

#endif
