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

void GLAPIENTRY
MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    mt_log_debug("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

void draw_quad()
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat vertexes[] = {
	0.0f, 0.0f, 0.0f, (float) width, 0.0f, 0.0f, 0.0f, (float) height, 0.0f, 0.0f, (float) height, 0.0f, (float) width, 0.0f, 0.0f, (float) width, (float) height, 0.0f};

    glViewport(
	0.0,
	0.0,
	width,
	height);

    glBindBuffer(
	GL_ARRAY_BUFFER,
	vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, 0);

    glBufferData(
	GL_ARRAY_BUFFER,
	sizeof(GLfloat) * 6 * 3,
	vertexes,
	GL_DYNAMIC_DRAW);

    glDrawArrays(
	GL_TRIANGLES,
	0,
	6);

    SDL_GL_SwapWindow(window);
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

cube_t* basecube;

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

/* inserts new cube for a point creating the intermediate octree */

void cube_insert(cube_t* cube, v3_t point, v3_t normal, uint32_t color)
{
    if (cube->size > 1.0)
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

struct glvec4
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

struct glcube_t
{
    struct glvec4 p;   // position of top left corner
    struct glvec4 nor; // normal
    struct glvec4 c;   // color
    GLint         n[8];
};

uint32_t cube_collect(cube_t* cube, struct glcube_t* glcubes, uint32_t* glindex, uint32_t length)
{
    uint32_t index = *glindex;
    if (index < length)
    {
	glcubes[index].p   = (struct glvec4){cube->tlf.x, cube->tlf.y, cube->tlf.z, cube->size};
	glcubes[index].nor = (struct glvec4){cube->nrm.x, cube->nrm.y, cube->nrm.z, 0.0};
	glcubes[index].c   = (struct glvec4){(float) ((cube->color >> 24) & 0xFF) / 255.0, (float) ((cube->color >> 16) & 0xFF) / 255.0, (float) ((cube->color >> 8) & 0xFF) / 255.0, (float) (cube->color & 0xFF) / 255.0};

	for (int o = 0; o < 8; o++)
	{
	    if (cube->nodes[o])
	    {
		*glindex += 1;
		glcubes[index].n[o] = cube_collect(cube->nodes[o], glcubes, glindex, length);
	    }
	    else
	    {
		glcubes[index].n[o] = 0;
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

	cube_insert(basecube, (v3_t){px, py, -1620 + pz}, (v3_t){nx, nz, ny}, (int) cx << 24 | (int) cy << 16 | (int) cz < 8 | 0xFF);

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

void main_init()
{
    srand((unsigned int) time(NULL));

    // opengl init

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

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

    glUseProgram(sha.name);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // vbo

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

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

    char plypath[PATH_MAX];

    snprintf(plypath, PATH_MAX, "%s../abandoned1.ply", base_path);

    p_ply ply = ply_open(plypath, NULL, 0, NULL);

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
    if (!ply_read(ply)) return;
    ply_close(ply);

    mt_log_debug("cube count : %lu", cube_count);
    mt_log_debug("leaf count : %lu", leaf_count);
    size_t buffsize = sizeof(struct glcube_t) * cube_count;
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

    struct glcube_t* glcubes = mt_memory_calloc(buffsize, NULL, NULL);

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

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, buffsize, glcubes, GL_DYNAMIC_COPY); // sizeof(data) only works for statically sized C/C++ arrays.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

    // perspective

    m4_t pers = m4_defaultortho(0.0, width, 0.0, height, -10, 10);

    projection.matrix = pers;

    glUniformMatrix4fv(sha.uni_loc[0], 1, 0, projection.array);

    GLfloat posarr[3] = {position.x, position.y, position.z};

    glUniform3fv(sha.uni_loc[1], 1, posarr);

    GLfloat lightarr[3] = {0.0, 2000.0, -500.0};

    glUniform3fv(sha.uni_loc[3], 1, lightarr);

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

		GLfloat anglearr[3] = {angle.x, angle.y, 0.0};

		glUniform3fv(sha.uni_loc[2], 1, anglearr);
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

    draw_quad();

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
	GLfloat posarr[3] = {position.x, position.y, position.z};

	glUniform3fv(sha.uni_loc[1], 1, posarr);
    }

    lighta += 0.05;
    if (lighta > 6.28) lighta = 0.0;
    GLfloat lightarr[3] = {lightc.x, lightc.y, lightc.z - sinf(lighta) * 200.0};
    /* printf("%f %f %f\n", lightarr[0], lightarr[1], lightarr[2]); */

    glUniform3fv(sha.uni_loc[3], 1, lightarr);

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

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

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