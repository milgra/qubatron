#include <GL/glew.h>
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "ku_gl_floatbuffer.c"
#include "ku_gl_shader.c"
#include "mt_log.c"
#include "mt_math_3d.c"
#include "mt_matrix_4d.c"
#include "mt_memory.c"
#include "mt_time.c"
#include "mt_vector.c"
#include "mt_vector_2d.c"
#include "mt_vector_3d.c"
#include "rply.h"

#ifdef EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

char  drag  = 0;
char  quit  = 0;
float scale = 1.0;

int32_t width  = 1200;
int32_t height = 800;

uint32_t frames     = 0;
uint32_t timestamp  = 0;
uint32_t start_time = 0;

SDL_Window*   window;
SDL_GLContext context;

GLuint         vbo;
m4_t           pers;
glsha_t        sha;
matrix4array_t projection = {0};

int   strafe    = 0;
int   forward   = 0;
float anglex      = 0.0;
float speed       = 0.0;
float strafespeed = 0.0;

v3_t angle       = {0.0};
v3_t position    = {440.0, 200.0, -1000.0};
v3_t direction   = {0.0, 0.0, -1.0};
v3_t directionX  = {-1.0, 0.0, 0.0};

v3_t  lightc = {420.0, 210.0, -1100.0};
float lighta = 0.0;

GLfloat* model_vertexes;
GLfloat* model_colors;
GLfloat* model_normals;
size_t   model_count = 0;
size_t   model_index = 0;
GLfloat* trans_vertexes;

void GLAPIENTRY
MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    mt_log_debug("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

char* readfile(char* name)
{
    FILE* f      = fopen(name, "rb");
    char* string = NULL;

    if (f != NULL)
    {

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET); /* same as rewind(f); */

	string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;
    }

    return string;
}

GLuint ssbo;

GLuint frg_vbo_in;
GLuint frg_vao;

void init_fragment_shader()
{
    // shader

    char* base_path = SDL_GetBasePath();
    char  vshpath[PATH_MAX];
    char  fshpath[PATH_MAX];
    snprintf(vshpath, PATH_MAX, "%svsh.c", base_path);
    snprintf(fshpath, PATH_MAX, "%sfsh.c", base_path);

    mt_log_debug("basepath %s %s", vshpath, fshpath);

    char* vsh = readfile(vshpath);
    char* fsh = readfile(fshpath);

    sha = ku_gl_shader_create(
	vsh,
	fsh,
	1,
	((const char*[]){"position"}),
	4,
	((const char*[]){"projection", "camfp", "angle_in", "light"}));
    free(vsh);
    free(fsh);

    // vbo

    // create vertex array object for vertex buffer
    glGenVertexArrays(1, &frg_vao);
    glBindVertexArray(frg_vao);

    // create vertex buffer object
    glGenBuffers(1, &frg_vbo_in);
    glBindBuffer(GL_ARRAY_BUFFER, frg_vbo_in);

    // create attribute for compute shader
    GLint inputAttrib = glGetAttribLocation(sha.name, "position");
    printf("INPUTATTRIB %i", inputAttrib);
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribPointer(inputAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    /* glGenBuffers(1, &vbo); */
    /* glbindbuffer(GL_ARRAY_BUFFER, vbo); */

    /* GLfloat posarr[3] = {position.x, position.y, position.z}; */
    /* glUniform3fv(sha.uni_loc[1], 1, posarr); */

    /* GLfloat lightarr[3] = {0.0, 2000.0, -500.0}; */
    /* glUniform3fv(sha.uni_loc[3], 1, lightarr); */

    glGenBuffers(1, &ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
}

void run_fragment_shader()
{
    glUseProgram(sha.name);

    m4_t pers         = m4_defaultortho(0.0, width, 0.0, height, -10, 10);
    projection.matrix = pers;
    glUniformMatrix4fv(sha.uni_loc[0], 1, 0, projection.array);

    GLfloat lightarr[3] = {lightc.x, lightc.y, lightc.z - sinf(lighta) * 200.0};
    /* printf("%f %f %f\n", lightarr[0], lightarr[1], lightarr[2]); */
    glUniform3fv(sha.uni_loc[3], 1, lightarr);

    GLfloat anglearr[3] = {angle.x, angle.y, 0.0};

    glUniform3fv(sha.uni_loc[2], 1, anglearr);

    GLfloat posarr[3] = {position.x, position.y, position.z};

    glUniform3fv(sha.uni_loc[1], 1, posarr);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat vertexes[] = {
	0.0f, 0.0f, 0.0f, (float) width, 0.0f, 0.0f, 0.0f, (float) height, 0.0f, 0.0f, (float) height, 0.0f, (float) width, 0.0f, 0.0f, (float) width, (float) height, 0.0f};

    glViewport(
	0.0,
	0.0,
	width,
	height);

    glBindVertexArray(frg_vao);

    glBindBuffer(
	GL_ARRAY_BUFFER,
	frg_vbo_in);

    glBufferData(
	GL_ARRAY_BUFFER,
	sizeof(GLfloat) * 6 * 3,
	vertexes,
	GL_DYNAMIC_DRAW);

    glDrawArrays(
	GL_TRIANGLES,
	0,
	6);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    SDL_GL_SwapWindow(window);
}

GLuint cmp_sp;
GLuint cmp_vbo_in;
GLuint cmp_vbo_out;
GLuint cmp_vao;

GLint oril;
GLint newl;

void init_compute_shader()
{
    char* base_path = SDL_GetBasePath();
    char  cshpath[PATH_MAX];
    char  fshepath[PATH_MAX];

    snprintf(cshpath, PATH_MAX, "%scsh.c", base_path);
    snprintf(fshepath, PATH_MAX, "%sfshe.c", base_path);

    char* csh  = readfile(cshpath);
    char* fshe = readfile(fshepath);

    GLuint cmp_vsh = ku_gl_shader_compile(GL_VERTEX_SHADER, csh);
    GLuint cmp_fsh = ku_gl_shader_compile(GL_FRAGMENT_SHADER, fshe);

    free(csh);
    free(fshe);

    // create compute shader ( it is just a vertex shader with transform feedback )
    cmp_sp = glCreateProgram();

    glAttachShader(cmp_sp, cmp_vsh);
    glAttachShader(cmp_sp, cmp_fsh);

    // we want to capture outvalue in the result buffer
    const GLchar* feedbackVaryings[] = {"outValue"};
    glTransformFeedbackVaryings(cmp_sp, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

    // link
    ku_gl_shader_link(cmp_sp);

    oril = glGetUniformLocation(cmp_sp, "fpori");
    newl = glGetUniformLocation(cmp_sp, "fpnew");

    // create vertex array object for vertex buffer
    glGenVertexArrays(1, &cmp_vao);
    glBindVertexArray(cmp_vao);

    // create vertex buffer object
    glGenBuffers(1, &cmp_vbo_in);
    glBindBuffer(GL_ARRAY_BUFFER, cmp_vbo_in);

    // create attribute for compute shader
    GLint inputAttrib = glGetAttribLocation(cmp_sp, "inValue");
    printf("INPUTATTRIB %i", inputAttrib);
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribPointer(inputAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    // create and bind result buffer object
    glGenBuffers(1, &cmp_vbo_out);
    glBindBuffer(GL_ARRAY_BUFFER, cmp_vbo_out);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void run_compute_shader()
{
    glUseProgram(cmp_sp);
    // switch off fragment stage
    glEnable(GL_RASTERIZER_DISCARD);

    glBindVertexArray(cmp_vao);

    GLfloat pivot_old[3] = {200.0, 300.0, 600.0};

    glUniform3fv(oril, 1, pivot_old);

    GLfloat pivot_new[3] = {200.0 + sinf(lighta) * 100., 300.0, 600.0};
    GLfloat lightarr[3]  = {lightc.x, lightc.y, lightc.z - sinf(lighta) * 200.0};

    glUniform3fv(newl, 1, pivot_new);

    /* GLfloat cmp_data[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}; */
    glBindBuffer(GL_ARRAY_BUFFER, cmp_vbo_in);
    /* glBufferData(GL_ARRAY_BUFFER, model_count * sizeof(GLfloat), model_vertexes, GL_STATIC_DRAW); */
    /* glBindBuffer(GL_ARRAY_BUFFER, 0); */

    glBindBuffer(GL_ARRAY_BUFFER, cmp_vbo_out);
    /* glBufferData(GL_ARRAY_BUFFER, model_count * sizeof(GLfloat), NULL, GL_STATIC_READ); */

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cmp_vbo_out);

    // run compute shader
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, model_count / 3);
    glEndTransformFeedback();
    glFlush();

    glDisable(GL_RASTERIZER_DISCARD);
}

typedef struct _cube_t cube_t;
struct _cube_t
{
    v3_t     tlf;  // top left front coord
    v3_t     brb;  // bottom right back coord
    v3_t     nrm;  // normal vector
    v3_t     cp;   // center point
    float    size; // side size
    float    dia;  // diameter
    uint32_t color;
    cube_t*  nodes[8];
};

cube_t* basecube = NULL;

void cube_describe(void* p, int level)
{
    cube_t* cube = p;
    cube_t  c    = *cube;

    printf("Cube TLF %.2f %.2f %.2f BRB %.2f %.2f %.2f SIZE %.2f COLOR %08x NODES ", c.tlf.x, c.tlf.y, c.tlf.z, c.brb.x, c.brb.y, c.brb.z, c.size, c.color);
    for (int i = 0; i < 8; i++)
    {
	if (c.nodes[i])
	    printf("Y");
	else
	    printf("N");
    }
    printf("\n");
}

void cube_delete(void* p)
{
    cube_t* cube = p;
    for (int i = 0; i < 8; i++)
    {
	if (cube->nodes[i] != NULL)
	{
	    REL(cube->nodes[i]);
	}
    }
}

uint32_t cube_count = 0;
uint32_t leaf_count = 0;

cube_t* cube_create(uint32_t color, v3_t tlf, v3_t brb, v3_t nrm)
{
    cube_t* cube = CAL(sizeof(cube_t), cube_delete, cube_describe);

    cube->color = color;
    cube->size  = brb.x - tlf.x;
    cube->dia   = v3_length(v3_sub(brb, tlf));
    cube->tlf   = tlf;
    cube->brb   = brb;
    cube->cp    = v3_add(cube->tlf, (v3_t){cube->size / 2.0, -cube->size / 2.0, -cube->size / 2.0});
    cube->nrm   = nrm;

    cube_count++;

    return cube;
}

struct glvec4
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glcube_t
{
    struct glvec4 tlf;
    struct glvec4 nrm;
    struct glvec4 col;
    GLint         oct[8];
};

static const int bocts[4] = {4, 5, 6, 7};
static const float xsizes[8] = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
static const float ysizes[8] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0};
static const float zsizes[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};

/* inserts new cube for a point creating the intermediate octree */

void glcube_insert(glcube_t** arr, size_t* ind, size_t* len, size_t* size, v3_t pnt, v4_t nrm, v4_t col)
{
	glcube_t cube = *arr[ind];
	if (cube.tlf.w > 0.5)
	{
		v3_t brb = (v3_t){tlf.x + tlf.w, tlf.y - tlf.w, tlf.x - tlf.w},
		if (cube.tlf.x <= pnt.x && pnt.x < brb.x &&
	            cube.tlf.y >= pnt.y && pnt.y > brb.y &&
	            cube.tlf.z >= pnt.z && pnt.z > brb.z)
	{
	    int   octi    = 0; // octet index
	    float half = cube.tlf.w / 2.0; // half size

	    if (cube.tlf.x + halfsize < point.x) octi = 1;
	    if (cube.tlf.y - halfsize > point.y) octi += 2;
	    if (cube.tlf.z - halfsize > point.z) octi = bocts[octi];

	    if (cube.oct[octi] == 0)
	    {
		float x = cube.tlf.x + xsizes[octet] * half;
		float y = cube.tlf.y - ysizes[octet] * half;
		float z = cube.tlf.z - zsizes[octet] * half;

		size_t nai = *len; // next array index
		
		cube.oct[octi] = nai;
		arr[nai].tlf = (v3_t){x,y,z};
		arr[nai].nrm = nrm;
		arr[nai].col = col;
	        arr[nai].oct = {0,0,0,0,0,0,0,0};
		    
		if (half < 1.0) leaf_count++;

		if (nai + 1 == *size)
		{
			*size *= 2;
			*arr = mt_memory_realloc(*arr, *size);
		}
		*len++;
		    
		/* printf("inserting into cube tlf %.2f %.2f %.2f brb %.2f %.2f %.2f s %.2f at octet %i\n", cube->tlf.x, cube->tlf.y, cube->tlf.z, cube->brb.x, cube->brb.y, cube->brb.z, cube->size, octet); */
	    }

	    *ind = cube.oct[octet];
	    glcube_insert(arr, ind, len, size, pnt, nrm, col);
	}
    }
}

size_t cubearr_size = 1000;
size_t cubearr_length = 0;
size_t cubearr_index = 0;
glcube_t* cubearr = mt_memory_alloc(sizeof(glcube_t)*cubearr_size,NULL,NULL);
cubearr[0] = (glcube_t){
	(v4_t){0.0,1800.0,0.0,1800.0},
	(v4_t){0.0,0.0,0.0,0.0},
	(v4_t){0.0,0.0,0.0,0.0},
	{0,0,0,0,0,0,0,0}};

void cube_insert(cube_t* cube, v3_t point, v3_t normal, uint32_t color)
{
    if (cube->size > 0.5)
    {
	if (cube->tlf.x <= point.x && point.x < cube->brb.x &&
	    cube->tlf.y >= point.y && point.y > cube->brb.y &&
	    cube->tlf.z >= point.z && point.z > cube->brb.z)
	{
	    /* do speed tests on static const vs simple vars */
	    static const int bocts[4] = {4, 5, 6, 7};

	    static const float xsizes[8] = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
	    static const float ysizes[8] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0};
	    static const float zsizes[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};

	    int   octet    = 0;
	    float halfsize = cube->size / 2.0;

	    if (cube->tlf.x + halfsize < point.x) octet = 1;
	    if (cube->tlf.y - halfsize > point.y) octet += 2;
	    if (cube->tlf.z - halfsize > point.z) octet = bocts[octet];

	    if (cube->nodes[octet] == NULL)
	    {
		float x = cube->tlf.x + xsizes[octet] * halfsize;
		float y = cube->tlf.y - ysizes[octet] * halfsize;
		float z = cube->tlf.z - zsizes[octet] * halfsize;

		cube->nodes[octet] = cube_create(
		    color,
		    (v3_t){x, y, z},
		    (v3_t){x + halfsize, y - halfsize, z - halfsize},
		    normal);

		if (halfsize < 1.0) leaf_count++;

		/* printf("inserting into cube tlf %.2f %.2f %.2f brb %.2f %.2f %.2f s %.2f at octet %i\n", cube->tlf.x, cube->tlf.y, cube->tlf.z, cube->brb.x, cube->brb.y, cube->brb.z, cube->size, octet); */
	    }

	    cube_insert(cube->nodes[octet], point, normal, color);

	    /* current color will be that last existing subnode's color
	       TODO : should  be the average of node colors */

	    for (int i = 0; i < 8; i++)
	    {
		if (cube->nodes[i] != NULL)
		{
		    cube->color = cube->nodes[i]->color;
		}
	    }
	}
    }
}


struct glcube_t
{
    struct glvec4 p;   // position of top left corner
    struct glvec4 nor; // normal
    struct glvec4 c;   // color
    GLint         n[8];
};

uint32_t cube_collect(cube_t* cube, struct glcube_t* cubes, uint32_t* glindex, uint32_t length)
{
    uint32_t index = *glindex;
    if (index < length)
    {
	cubes[index].p   = (struct glvec4){cube->tlf.x, cube->tlf.y, cube->tlf.z, cube->size};
	cubes[index].nor = (struct glvec4){cube->nrm.x, cube->nrm.y, cube->nrm.z, 0.0};
	cubes[index].c   = (struct glvec4){(float) ((cube->color >> 24) & 0xFF) / 255.0, (float) ((cube->color >> 16) & 0xFF) / 255.0, (float) ((cube->color >> 8) & 0xFF) / 255.0, (float) (cube->color & 0xFF) / 255.0};

	for (int o = 0; o < 8; o++)
	{
	    if (cube->nodes[o])
	    {
		*glindex += 1;
		cubes[index].n[o] = cube_collect(cube->nodes[o], cubes, glindex, length);
	    }
	    else
	    {
		cubes[index].n[o] = 0;
	    }
	}
    }
    return index;
}

float    px, py, pz, cx, cy, cz, nx, ny, nz;
uint32_t cnt = 0;
uint32_t ind = 0;

v3_t  offset  = {0.0, 0.0, 0.0};
float scaling = 1.0;

float minpx, maxpx, minpy, maxpy, minpz, maxpz, mindx, mindy, mindz, lx, ly, lz;

long nvertices, ntriangles;

static int vertex_cb(p_ply_argument argument)
{
    long eol;
    ply_get_argument_user_data(argument, NULL, &eol);

    if (ind == 0) px = (ply_get_argument_value(argument) - offset.x) * scaling;
    if (ind == 1) pz = (ply_get_argument_value(argument) - offset.y) * scaling;
    if (ind == 2) py = (ply_get_argument_value(argument) - offset.z) * scaling;

    if (ind == 3) cx = ply_get_argument_value(argument);
    if (ind == 4) cy = ply_get_argument_value(argument);
    if (ind == 5) cz = ply_get_argument_value(argument);

    if (ind == 6) nx = ply_get_argument_value(argument);
    if (ind == 7) ny = ply_get_argument_value(argument);
    if (ind == 8)
    {
	nz = ply_get_argument_value(argument);

	model_vertexes[model_index]     = px;
	model_vertexes[model_index + 1] = py;
	model_vertexes[model_index + 2] = pz;

	model_normals[model_index]     = nx;
	model_normals[model_index + 1] = nz;
	model_normals[model_index + 2] = ny;

	model_colors[model_index]     = cx;
	model_colors[model_index + 1] = cy;
	model_colors[model_index + 2] = cz;

	model_index += 3;

	ind = -1;

	if (cnt == 0)
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

	if (cnt % 1000000 == 0) printf("progress %f\n", (float) cnt / (float) nvertices);

	cnt++;
    }

    ind++;

    /* if (cnt < 1000) */
    /* { */
    /* 	printf("%li : %g", ind, ply_get_argument_value(argument)); */
    /* 	if (eol) */
    /* 	    printf("\n"); */
    /* 	else */
    /* 	    printf(" "); */
    /* } */
    return 1;
}

struct glcube_t* glcubes = NULL;
size_t           buffsize;

void main_init()
{
    srand((unsigned int) time(NULL));

    char* base_path = SDL_GetBasePath();

    // opengl init

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    init_compute_shader();
    init_fragment_shader();

    // shader storage buffer object

    basecube = cube_create(
	0,
	(v3_t){0.0, 1800.0, 0.0},
	(v3_t){1800.0, 0.0, -1800.0},
	(v3_t){0.0, 0.0, -1.0});

    /* basecube = cube_create( */
    /* 	0, */
    /* 	(v3_t){100.0, 800.0, -100.0}, */
    /* 	(v3_t){800.0, 100.0, -800.0}); */

    char  plypath[PATH_MAX];
    p_ply ply;

    /* snprintf(plypath, PATH_MAX, "%s../abandoned1.ply", base_path); */

    /* ply = ply_open(plypath, NULL, 0, NULL); */

    /* if (!ply) mt_log_debug("cannot read %s", plypath); */
    /* if (!ply_read_header(ply)) mt_log_debug("cannot read ply header"); */

    /* nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0); */
    /* ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 0); */
    /* ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 1); */
    /* ply_set_read_cb(ply, "vertex", "red", vertex_cb, NULL, 0); */
    /* ply_set_read_cb(ply, "vertex", "green", vertex_cb, NULL, 0); */
    /* ply_set_read_cb(ply, "vertex", "blue", vertex_cb, NULL, 1); */
    /* ply_set_read_cb(ply, "vertex", "nx", vertex_cb, NULL, 0); */
    /* ply_set_read_cb(ply, "vertex", "ny", vertex_cb, NULL, 0); */
    /* ply_set_read_cb(ply, "vertex", "nz", vertex_cb, NULL, 1); */
    /* mt_log_debug("cloud point count : %lu", nvertices); */

    /* model_vertexes = mt_memory_alloc(sizeof(GLfloat) * nvertices * 3, NULL, NULL); */
    /* model_normals  = mt_memory_alloc(sizeof(GLfloat) * nvertices * 3, NULL, NULL); */
    /* model_colors   = mt_memory_alloc(sizeof(GLfloat) * nvertices * 3, NULL, NULL); */
    /* model_count    = nvertices * 3; */
    /* model_index    = 0; */

    /* if (!ply_read(ply)) return; */
    /* ply_close(ply); */

    /* for (int index = 0; index < model_count; index += 3) */
    /* { */
    /* 	cube_insert( */
    /* 	    basecube, */
    /* 	    (v3_t){model_vertexes[index], model_vertexes[index + 1], -1620 + model_vertexes[index + 2]}, */
    /* 	    (v3_t){model_normals[index], model_normals[index + 1], model_normals[index + 2]}, */
    /* 	    (int) model_colors[index] << 24 | (int) model_colors[index + 1] << 16 | (int) model_colors[index + 2] < 8 | 0xFF); */
    /* } */

    /* REL(model_vertexes); */
    /* REL(model_normals); */
    /* REL(model_colors); */

    snprintf(plypath, PATH_MAX, "%s../zombie.ply", base_path);

    ply = ply_open(plypath, NULL, 0, NULL);

    offset.x = -270.0;
    offset.y = -600.0;
    offset.z = -10.0;

    if (!ply) mt_log_debug("cannot read %s", plypath);
    if (!ply_read_header(ply)) mt_log_debug("cannot read ply header");

    nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 1);
    ply_set_read_cb(ply, "vertex", "red", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "green", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "blue", vertex_cb, NULL, 1);
    ply_set_read_cb(ply, "vertex", "nx", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "ny", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "nz", vertex_cb, NULL, 1);
    mt_log_debug("cloud point count : %lu", nvertices);

    model_vertexes = mt_memory_alloc(sizeof(GLfloat) * nvertices * 3, NULL, NULL);
    model_normals  = mt_memory_alloc(sizeof(GLfloat) * nvertices * 3, NULL, NULL);
    model_colors   = mt_memory_alloc(sizeof(GLfloat) * nvertices * 3, NULL, NULL);
    model_count    = nvertices * 3;
    model_index    = 0;

    if (!ply_read(ply)) return;
    ply_close(ply);

    // set compute buffers

    glUseProgram(cmp_sp);

    glBindBuffer(GL_ARRAY_BUFFER, cmp_vbo_in);
    glBufferData(GL_ARRAY_BUFFER, model_count * sizeof(GLfloat), model_vertexes, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, cmp_vbo_out);
    glBufferData(GL_ARRAY_BUFFER, model_count * sizeof(GLfloat), NULL, GL_STATIC_READ);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cmp_vbo_out);
    trans_vertexes = glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, model_count * sizeof(GLfloat), GL_MAP_READ_BIT);

    run_fragment_shader();
    run_compute_shader();
    /* run_compute_shader(); */

    for (int index = 0; index < model_count; index += 3)
    {
	cube_insert(
	    basecube,
	    (v3_t){trans_vertexes[index], trans_vertexes[index + 1], -1620 + trans_vertexes[index + 2]},
	    (v3_t){model_normals[index], model_normals[index + 1], model_normals[index + 2]},
	    (int) model_colors[index] << 24 | (int) model_colors[index + 1] << 16 | (int) model_colors[index + 2] < 8 | 0xFF);
    }

    mt_log_debug("cube count : %lu", cube_count);
    mt_log_debug("leaf count : %lu", leaf_count);
    buffsize = sizeof(struct glcube_t) * cube_count;
    mt_log_debug("buffer size is %lu bytes", buffsize);
    mt_log_debug("minpx %f maxpx %f minpy %f maxpy %f minpz %f maxpz %f mindx %f mindy %f mindz %f\n", minpx, maxpx, minpy, maxpy, minpz, maxpz, mindx, mindy, mindz);

    /* cube_insert(basecube, (v3_t){110.0, 790.0, -110.0}, 0xFFFFFFFF); */
    /* cube_insert(basecube, (v3_t){110.0, 440.0, -110.0}, 0xFFFFFFFF); */
    /* cube_insert(basecube, (v3_t){110.0, 440.0, -790.0}, 0xFFFFFFFF); */
    /* cube_insert(basecube, (v3_t){110.0, 110.0, -110.0}, 0xFFFFFFFF); */
    /* cube_insert(basecube, (v3_t){790.0, 110.0, -790.0}, 0xFFFFFFFF); */

    /* for (int i = 0; i < 20000; i++) */
    /* { */
    /* 	float x = 100.0 + (float) (rand() % 700); */
    /* 	float y = 100.0 + (float) (rand() % 700); */
    /* 	float z = -100.0 - (float) (rand() % 700); */

    /* 	uint32_t c; */
    /* 	c = rand() & 0xff; */
    /* 	c |= (rand() & 0xff) << 8; */
    /* 	c |= (rand() & 0xff) << 16; */
    /* 	c |= (rand() & 0xff) << 24; */

    /* 	cube_insert(basecube, (v3_t){x, y, z}, c); */
    /* } */

    /* int maxcol = 10; */
    /* int maxrow = 10; */

    /* for (int c = 0; c < maxcol; c++) */
    /* { */
    /* 	for (int r = 0; r < maxrow; r++) */
    /* 	{ */
    /* 	    float x = 100.0 + (float) (5 * c); */
    /* 	    float y = 100.0 + (float) (5 * r); */
    /* 	    float z = -110.0; */
    /* 	    cube_insert(basecube, (v3_t){x, y, z}, 0xFFFFFFFF); */
    /* 	    z = -790.0; */
    /* 	    cube_insert(basecube, (v3_t){x, y, z}, 0xFFFFFFFF); */
    /* 	} */
    /* } */

    glcubes = mt_memory_calloc(buffsize, NULL, NULL);

    uint32_t glindex = 0;
    cube_collect(basecube, glcubes, &glindex, cube_count);

    /* for (int i = 0; i < glindex + 1; i++) */
    /* { */
    /* 	printf("%i %.2f %.2f %.2f %.2f nodes : %i | %i | %i | %i | %i | %i | %i | %i\n", i, glcubes[i].p.x, glcubes[i].p.y, glcubes[i].p.z, glcubes[i].p.w, glcubes[i].n[0], glcubes[i].n[1], glcubes[i].n[2], glcubes[i].n[3], glcubes[i].n[4], glcubes[i].n[5], glcubes[i].n[6], glcubes[i].n[7]); */
    /* } */

    /* for (int i = 0; i < 100; i++) */
    /* { */
    /* 	GLfloat x = (float) (rand() % 800); */
    /* 	GLfloat y = (float) (rand() % 600); */
    /* 	GLfloat z = -200.0 + (float) (rand() % 500); */
    /* 	GLfloat s = (float) (10 + rand() % 90); */
    /* 	/\* glcubes[0] = (struct glcube_t){{300.0, 300.0, 0.0}, {600.0, 600.0, -300.0}, 300.0, 8}; *\/ */
    /* 	/\* glcubes[1] = (struct glcube_t){{700.0, 300.0, 0.0}, {600.0, 600.0, -300.0}, 300.0, 8}; *\/ */
    /* 	glcubes[i] = (struct glcube_t){{x, y, -z, s}}; */
    /* } */

    /* glcubes[0] = (struct glcube_t){{100.0, 100.0, -200.0, 100.0}}; */
    /* glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo); */
    /* glBufferData(GL_SHADER_STORAGE_BUFFER, buffsize, glcubes, GL_DYNAMIC_COPY); // sizeof(data) only works for statically sized C/C++ arrays. */
    /* glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);                                  // unbind */

    // perspective

    mt_log_debug("main init");
}

void main_free()
{
}

v4_t quat_from_axis_angle(v3_t axis, float angle)
{
    v4_t  qr;
    float half_angle = angle * 0.5;
    qr.x             = axis.x * sinf(half_angle);
    qr.y             = axis.y * sinf(half_angle);
    qr.z             = axis.z * sinf(half_angle);
    qr.w             = cosf(half_angle);
    return qr;
}

// quaternion rotation
v3_t qrot(v4_t q, v3_t v)
{
    v3_t qv = (v3_t){q.x, q.y, q.z};
    return v3_add(v, v3_scale(v3_cross(qv, v3_add(v3_cross(qv, v), v3_scale(v, q.w))), 2.0));
}

int main_loop(double time, void* userdata)
{
    printf("MAINLOOP\n");
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0)
    {
	if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION)
	{
	    int x = 0, y = 0;
	    SDL_GetMouseState(&x, &y);

	    /* v2_t dimensions = {.x = x * scale, .y = y * scale}; */

	    if (event.type == SDL_MOUSEMOTION)
	    {
		angle.x += event.motion.xrel / 100.0;
		angle.y -= event.motion.yrel / 100.0;

		direction  = v3_rotatearoundy((v3_t){0.0, 0.0, -1.0}, angle.x);
		directionX = v3_rotatearoundy((v3_t){1.0, 0.0, 0.0}, angle.x);

		v4_t axisquat = quat_from_axis_angle(directionX, angle.y);
		direction     = qrot(axisquat, direction);
	    }
	}
	else if (event.type == SDL_QUIT)
	{
	    quit = 1;
	}
	else if (event.type == SDL_WINDOWEVENT)
	{
	    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
	    {
		width  = event.window.data1;
		height = event.window.data2;

		v2_t dimensions = {.x = width * scale, .y = height * scale};

		mt_log_debug("new dimension %f %f scale %f", dimensions.x, dimensions.y, scale);
	    }
	}
	else if (event.type == SDL_KEYUP)
	{
	    switch (event.key.keysym.sym)
	    {
		case SDLK_f:
		    break;

		case SDLK_ESCAPE:
		    break;

		case SDLK_a:
		    strafe = 0;
		    break;

		case SDLK_d:
		    strafe = 0;
		    break;

		case SDLK_w:
		    forward = 0;
		    break;

		case SDLK_s:
		    forward = 0;
		    break;
	    }
	}
	else if (event.type == SDL_KEYDOWN)
	{
	    switch (event.key.keysym.sym)
	    {
		case SDLK_f:
		    break;

		case SDLK_ESCAPE:
		    break;

		case SDLK_a:
		    strafe = 1;
		    break;

		case SDLK_d:
		    strafe = -1;
		    break;

		case SDLK_w:
		    forward = 1;
		    break;

		case SDLK_s:
		    forward = -1;
		    break;
	    }
	}
	else if (event.type == SDL_APP_WILLENTERFOREGROUND)
	{
	}
    }

    // update simulation

    if (SDL_GetTicks() > start_time + 1000)
    {
	mt_log_debug("fps : %u", frames);
	frames     = 0;
	start_time = SDL_GetTicks();
    }

    if (strafe != 0)
    {
	strafespeed += 0.1 * strafe;

	if (strafespeed > 10.0) strafespeed = 10.0;
	if (strafespeed < -10.0) strafespeed = -10.0;
    }
    else
	strafespeed *= 0.9;

    if (forward != 0)
    {
	speed += 0.1 * forward;

	if (speed > 10.0) speed = 10.0;
	if (speed < -10.0) speed = -10.0;
    }
    else
	speed *= 0.9;

    if (strafespeed > 0.0001 || strafespeed < -0.0001)
    {
	v3_t curr_speed = v3_scale(directionX, -strafespeed);
	position        = v3_add(position, curr_speed);
    }

    if (speed > 0.0001 || speed < -0.0001)
    {
	v3_t curr_speed = v3_scale(direction, speed);
	position        = v3_add(position, curr_speed);
    }

    if (strafespeed > 0.0001 || strafespeed < -0.0001 || speed > 0.0001 || speed < 0.0001)
    {
    }

    lighta += 0.05;
    if (lighta > 6.28) lighta = 0.0;

    run_compute_shader();

    if (basecube)
	REL(basecube);

    cube_count = 0;
    leaf_count = 0;

    basecube = cube_create(
	0,
	(v3_t){0.0, 1800.0, 0.0},
	(v3_t){1800.0, 0.0, -1800.0},
	(v3_t){0.0, 0.0, -1.0});

    for (int index = 0;
	 index < model_count;
	 index += 3)
    {
	cube_insert(
	    basecube,
	    (v3_t){trans_vertexes[index], trans_vertexes[index + 1], -1620 + trans_vertexes[index + 2]},
	    (v3_t){model_normals[index], model_normals[index + 1], model_normals[index + 2]},
	    (int) model_colors[index] << 24 | (int) model_colors[index + 1] << 16 | (int) model_colors[index + 2] < 8 | 0xFF);
    }

    buffsize = sizeof(struct glcube_t) * cube_count;

    mt_log_debug("cube count : %lu model count %lu", cube_count, model_count);
    /* mt_log_debug("leaf count : %lu", leaf_count); */
    /* mt_log_debug("buffer size is %lu bytes", buffsize); */
    /* mt_log_debug("minpx %f maxpx %f minpy %f maxpy %f minpz %f maxpz %f mindx %f mindy %f mindz %f\n", minpx, maxpx, minpy, maxpy, minpz, maxpz, mindx, mindy, mindz); */

    if (glcubes != NULL)
	REL(glcubes);
    glcubes = mt_memory_calloc(buffsize, NULL, NULL);

    uint32_t glindex = 0;
    cube_collect(basecube, glcubes, &glindex, cube_count);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, buffsize, glcubes, GL_DYNAMIC_COPY); // sizeof(data) only works for statically sized C/C++ arrays.

    run_fragment_shader();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

    frames++;

    return 1;
}

int main(int argc, char* argv[])
{
    mt_log_use_colors(isatty(STDERR_FILENO));
    mt_log_level_info();
    mt_time(NULL);

    printf("Qubatron v" QUBATRON_VERSION " by Milan Toth ( www.milgra.com )\n");

    const char* usage =
	"Usage: cortex [options]\n"
	"\n"
	"  -h, --help                          Show help message and quit.\n"
	"  -v                                  Increase verbosity of messages, defaults to errors and warnings only.\n"
	"\n";

    const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"verbose", no_argument, NULL, 'v'}};

    int option       = 0;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "vh", long_options, &option_index)) != -1)
    {
	switch (option)
	{
	    case '?': printf("parsing option %c value: %s\n", option, optarg); break;
	    case 'v': mt_log_inc_verbosity(); break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    // enable high dpi

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    // init sdl

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0)
    {
	// setup opengl version

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	/* window size should be full screen on phones, scaled down on desktops */

	SDL_DisplayMode displaymode;
	SDL_GetCurrentDisplayMode(0, &displaymode);

	/* if (displaymode.w < 800 || displaymode.h < 400) */
	/* { */
	/*     width  = displaymode.w; */
	/*     height = displaymode.h; */
	/* } */
	/* else */
	/* { */
	/*     width  = displaymode.w * 0.8; */
	/*     height = displaymode.h * 0.8; */
	/* } */

	// create window

	window = SDL_CreateWindow(
	    "Qubatron",
	    SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED,
	    width,
	    height,
	    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

	if (window != NULL)
	{
	    // create context

	    context = SDL_GL_CreateContext(window);

	    if (context != NULL)
	    {
		GLint GlewInitResult = glewInit();
		if (GLEW_OK != GlewInitResult)
		    mt_log_error("%s", glewGetErrorString(GlewInitResult));

		// calculate scaling

		int nw;
		int nh;

		SDL_GL_GetDrawableSize(window, &nw, &nh);

		scale = nw / width;

		// try to set up vsync

		if (SDL_GL_SetSwapInterval(1) < 0)
		    mt_log_error("SDL swap interval error %s", SDL_GetError());

		SDL_SetRelativeMouseMode(SDL_TRUE);

		main_init();

#ifdef EMSCRIPTEN
		// setup the main thread for the browser and release thread with return
		emscripten_request_animation_frame_loop(main_loop, 0);
		return 0;
#else
		// infinite loop til quit
		while (!quit)
		{
		    main_loop(0, NULL);
		}
#endif

		main_free();

		// cleanup

		SDL_GL_DeleteContext(context);
	    }
	    else
		mt_log_error("SDL context creation error %s", SDL_GetError());

	    // cleanup

	    SDL_DestroyWindow(window);
	}
	else
	    mt_log_error("SDL window creation error %s", SDL_GetError());

	// cleanup

	SDL_Quit();
    }
    else
	mt_log_error("SDL init error %s", SDL_GetError());

    return 0;
}
