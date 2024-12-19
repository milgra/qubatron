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

typedef struct glcube__t
{
    glvec4_t tlf;
    glvec4_t nrm;
    glvec4_t col;
    GLint    oct[8];
} glcube__t;

typedef struct glcubearr_t
{
    glcube__t* cubes;
    size_t     size;
    size_t     len;
    size_t     leaves;
} glcubearr_t;

glcubearr_t glcubearr_create(size_t size, glvec4_t base);
void        glcubearr_delete(glcubearr_t* arr);
void        glcubearr_reset(glcubearr_t* arr, glvec4_t base);
void        glcubearr_insert(glcubearr_t* arr, size_t ind, v3_t pnt, glvec4_t nrm, glvec4_t col);

#endif

#if __INCLUDE_LEVEL__ == 0

static const float glcube_res = 0.5;

// octets
//     4 5
//     6 7
// 0 1
// 2 3

static const int   bocts[4]  = {4, 5, 6, 7};
static const float xsizes[8] = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
static const float ysizes[8] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0};
static const float zsizes[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};

glcubearr_t glcubearr_create(size_t size, glvec4_t base)
{
    glcubearr_t arr;
    arr.size   = size;
    arr.cubes  = mt_memory_alloc(sizeof(glcube__t) * arr.size, NULL, NULL);
    arr.len    = 0;
    arr.leaves = 0;

    arr.cubes[0] = (glcube__t){
	(glvec4_t) base,
	(glvec4_t){0.0, 0.0, 0.0, 0.0},
	(glvec4_t){0.0, 0.0, 0.0, 0.0},
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
    arr->cubes[0] = (glcube__t){
	base,
	(glvec4_t){0.0, 0.0, 0.0, 0.0},
	(glvec4_t){0.0, 0.0, 0.0, 0.0},
	{0, 0, 0, 0, 0, 0, 0, 0}};

    arr->len = 1;
}

/* inserts new cube for a point creating the intermediate octree */

void glcubearr_insert(glcubearr_t* arr, size_t ind, v3_t pnt, glvec4_t nrm, glvec4_t col)
{
    glcube__t cube = arr->cubes[ind];

    // check resolution
    if (cube.tlf.w > glcube_res)
    {
	// check bounds
	v3_t brb = (v3_t){cube.tlf.x + cube.tlf.w, cube.tlf.y - cube.tlf.w, cube.tlf.z - cube.tlf.w};

	/* printf("%f %f %f %f %f %f %f %f %f\n", cube.tlf.x, cube.tlf.y, cube.tlf.z, brb.x, brb.y, brb.z, pnt.x, pnt.y, pnt.z); */

	if (cube.tlf.x <= pnt.x && pnt.x < brb.x &&
	    cube.tlf.y >= pnt.y && pnt.y > brb.y &&
	    cube.tlf.z >= pnt.z && pnt.z > brb.z)
	{
	    // get octet
	    int   octi = 0;                // octet index
	    float half = cube.tlf.w / 2.0; // half size

	    if (cube.tlf.x + half < pnt.x) octi = 1;
	    if (cube.tlf.y - half > pnt.y) octi += 2;
	    if (cube.tlf.z - half > pnt.z) octi = bocts[octi];

	    if (cube.oct[octi] == 0)
	    {
		// create octet in array if needed
		glvec4_t ntlf = (glvec4_t){
		    cube.tlf.x + xsizes[octi] * half,
		    cube.tlf.y - ysizes[octi] * half,
		    cube.tlf.z - zsizes[octi] * half,
		    half};

		arr->cubes[ind].oct[octi] = arr->len;
		arr->cubes[arr->len]      = (glcube__t){ntlf, nrm, col, {0, 0, 0, 0, 0, 0, 0, 0}};

		if (arr->len + 1 == arr->size)
		{
		    // resize array if needed
		    arr->size *= 2;
		    arr->cubes = mt_memory_realloc(arr->cubes, arr->size * sizeof(glcube__t));
		    if (arr->cubes == NULL) mt_log_debug("not enough memory");
		}
		arr->len++;

		if (half < glcube_res) arr->leaves++;
	    }

	    /* printf("insert octet %i index %u\n", octi, cube.oct[octi]); */

	    glcubearr_insert(arr, cube.oct[octi], pnt, nrm, col);
	}
    }
}

#endif
