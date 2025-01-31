#include <GL/glew.h>
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "model.c"
#include "mt_log.c"
#include "mt_time.c"
#include "mt_vector_2d.c"
#include "octree.c"
#include "readfile.c"
#include "renderconn.c"
#include "skeleconn.c"

#ifdef EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

/* #define OCTTEST 0 */

char drag    = 0;
char quit    = 0;
char octtest = 0;

float   scale  = 1.0;
int32_t width  = 1200;
int32_t height = 800;

uint32_t frames    = 0;
uint32_t timestamp = 0;
uint32_t prevticks = 0;

SDL_Window*   window;
SDL_GLContext context;

int strafe   = 0;
int forward  = 0;
int maxlevel = 12;

float anglex      = 0.0;
float speed       = 0.0;
float strafespeed = 0.0;

v3_t angle       = {0.0};
v3_t position    = {440.0, 200.0, 700.0};
v3_t direction   = {0.0, 0.0, -1.0};
v3_t directionX  = {-1.0, 0.0, 0.0};

float lighta = 0.0;

uint8_t quality = 10;

uint32_t cnt = 0;
uint32_t ind = 0;

v3_t  offset  = {0.0, 0.0, 0.0};
float scaling = 1.0;

skeleconn_t   cc;
renderconn_t  rc;

octree_t static_octree;
octree_t dynamic_octree;

model_t static_model;
model_t dynamic_model;

void GLAPIENTRY
MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    mt_log_debug("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

void main_init()
{
    srand((unsigned int) time(NULL));

    // opengl init

#ifndef EMSCRIPTEN
    /* glEnable(GL_DEBUG_OUTPUT); */
    /* glDebugMessageCallback(MessageCallback, 0); */
#endif

    cc = skeleconn_init();
    rc = renderconn_init();

    dynamic_model = model_init();
    static_model  = model_init();

#ifdef OCTTEST
    int point_count = 5;

    GLfloat points[3 * 8192] = {
	10.0, 690.0, 10.0,
	10.0, 340.0, 10.0,
	10.0, 340.0, 690.0,
	10.0, 10.0, 10.0,
	690.0, 10.0, 690.0};

    GLfloat normals[3 * 8192] = {
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0};

    GLfloat colors[3 * 8192] = {
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0};

    static_model.vertexes    = mt_memory_alloc(3 * 8192, NULL, NULL);
    static_model.normals     = mt_memory_alloc(3 * 8192, NULL, NULL);
    static_model.colors      = mt_memory_alloc(3 * 8192, NULL, NULL);
    static_model.point_count = 5;
    static_model.txwth       = 8192;
    static_model.txhth       = (int) ceilf((float) static_model.point_count / (float) static_model.txwth);

    memcpy(static_model.vertexes, points, 3 * 8192);
    memcpy(static_model.normals, normals, 3 * 8192);
    memcpy(static_model.colors, colors, 3 * 8192);

    // !!! it works only with x 0 first
    static_octree = octree_create((v4_t){0.0, 1800.0, 1800.0, 1800.0}, maxlevel);

    for (int index = 0; index < point_count * 3; index += 3)
    {
	octree_insert_point(
	    &static_octree,
	    0,
	    index / 3,
	    (v3_t){points[index], points[index + 1], points[index + 2]});
    }

    for (int i = 0; i < static_octree.len; i++)
    {
	printf(
	    "AFTER nodes ind %i : %i | %i | %i | %i | %i | %i | %i | %i orind %i\n",
	    i,
	    static_octree.octs[i].oct[0],
	    static_octree.octs[i].oct[1],
	    static_octree.octs[i].oct[2],
	    static_octree.octs[i].oct[3],
	    static_octree.octs[i].oct[4],
	    static_octree.octs[i].oct[5],
	    static_octree.octs[i].oct[6],
	    static_octree.octs[i].oct[7],
	    static_octree.octs[i].oct[8]);
    }

    renderconn_upload_normals(&rc, normals, 8192, 1, 3, false);
    renderconn_upload_colors(&rc, colors, 8192, 1, 3, false);
    renderconn_upload_octree_quadruplets(&rc, static_octree.octs, static_octree.txwth, static_octree.txhth, false);

    // init dynamic model

    dynamic_octree = octree_create((v4_t){0.0, 1800.0, 1800.0, 1800.0}, maxlevel);
    renderconn_upload_octree_quadruplets(&rc, dynamic_octree.octs, dynamic_octree.txwth, dynamic_octree.txhth, true);

    // set rendering mode to octtest
    octtest = 1;

    quality = 10;

    // set camera position to see test cube
    position = (v3_t){900.0, 900.0, 3000.0};

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
#else

    char* base_path = SDL_GetBasePath();

    char pntpath[PATH_MAX];
    char nrmpath[PATH_MAX];
    char colpath[PATH_MAX];
    char rngpath[PATH_MAX];

    char* scenepath = "abandoned.ply";
    snprintf(pntpath, PATH_MAX, "%s/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%s/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%s/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%s/%s.rng", base_path, scenepath);
    #ifdef EMSCRIPTEN
    snprintf(pntpath, PATH_MAX, "%s/res/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%s/res/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%s/res/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%s/res/%s.rng", base_path, scenepath);
    #endif

    model_load_flat(&static_model, pntpath, colpath, nrmpath, rngpath);

    static_octree = octree_create((v4_t){0.0, 1800.0, 1800.0, 1800.0}, maxlevel);
    octets_t pathf;
    octets_t pathl;
    for (int index = 0; index < static_model.point_count * 3; index += 3)
    {
	octets_t path =
	    octree_insert_point(
		&static_octree,
		0,
		index / 3,
		(v3_t){static_model.vertexes[index], static_model.vertexes[index + 1], static_model.vertexes[index + 2]});

	if (index == 0)
	    pathf = path;
	else if (index == (static_model.point_count - 3))
	    pathl = path;
    }

    mt_log_debug("STATIC MODEL");
    model_log_vertex_info(&static_model, 0);
    model_log_vertex_info(&static_model, static_model.point_count - 1);
    octree_log_path(pathf, 0);
    octree_log_path(pathl, static_model.point_count - 3);

    // upload static model

    renderconn_upload_normals(&rc, static_model.normals, static_model.txwth, static_model.txhth, static_model.comps, false);
    renderconn_upload_colors(&rc, static_model.colors, static_model.txwth, static_model.txhth, static_model.comps, false);
    renderconn_upload_octree_quadruplets(&rc, static_octree.octs, static_octree.txwth, static_octree.txhth, false);

    /* float* octs = mt_memory_calloc(8192 * 4, NULL, NULL); */
    /* renderconn_upload_octree_quadruplets(&rc, octs, 8192, 1, false); */

    scenepath = "zombie.ply";
    snprintf(pntpath, PATH_MAX, "%s/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%s/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%s/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%s/%s.rng", base_path, scenepath);
    #ifdef EMSCRIPTEN
    snprintf(pntpath, PATH_MAX, "%s/res/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%s/res/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%s/res/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%s/res/%s.rng", base_path, scenepath);
    #endif
    model_load_flat(&dynamic_model, pntpath, colpath, nrmpath, rngpath);

    dynamic_octree = octree_create((v4_t){0.0, 1800.0, 1800.0, 1800.0}, maxlevel);
    for (int index = 0; index < dynamic_model.point_count * 3; index += 3)
    {
	octets_t path =
	    octree_insert_point(
		&dynamic_octree,
		0,
		index / 3,
		(v3_t){dynamic_model.vertexes[index], dynamic_model.vertexes[index + 1], dynamic_model.vertexes[index + 2]});
	if (index == 0)
	    pathf = path;
	else if (index == (dynamic_model.point_count - 3))
	    pathl = path;
    }

    mt_log_debug("DYNAMIC MODEL");
    model_log_vertex_info(&dynamic_model, 0);
    model_log_vertex_info(&dynamic_model, dynamic_model.point_count - 1);
    octree_log_path(pathf, 0);
    octree_log_path(pathl, dynamic_model.point_count - 3);

    // upload dynamic model

    renderconn_upload_normals(&rc, dynamic_model.normals, dynamic_model.txwth, dynamic_model.txhth, dynamic_model.comps, true);
    renderconn_upload_colors(&rc, dynamic_model.colors, dynamic_model.txwth, dynamic_model.txhth, dynamic_model.comps, true);
    renderconn_upload_octree_quadruplets(&rc, dynamic_octree.octs, dynamic_octree.txwth, dynamic_octree.txhth, true);

    // upload actor to skeleton modifier

    skeleconn_alloc_in(&cc, dynamic_model.vertexes, dynamic_model.point_count * 3 * sizeof(GLfloat));
    skeleconn_alloc_out(&cc, NULL, dynamic_model.point_count * sizeof(GLint) * 12);
    skeleconn_update(&cc, lighta, dynamic_model.point_count, maxlevel);
    skeleconn_update(&cc, lighta, dynamic_model.point_count, maxlevel); // double run to step over double buffer

    GLint* arr = cc.octqueue;
    for (int i = 0; i < 48; i += 12)
    {
	printf(
	    "nodes : %i | %i | %i | %i | %i | %i | %i | %i | %i | %i | %i | %i\n",
	    arr[i + 0],
	    arr[i + 1],
	    arr[i + 2],
	    arr[i + 3],
	    arr[i + 4],
	    arr[i + 5],
	    arr[i + 6],
	    arr[i + 7],
	    arr[i + 8],
	    arr[i + 9],
	    arr[i + 10],
	    arr[i + 11]);
    }

    // add modified point coords by compute shader

    octree_reset(&dynamic_octree, (v4_t){0.0, 1800.0, 1800.0, 1800.0});

    for (int index = 0; index < dynamic_model.point_count; index++)
    {
	octree_insert_path(
	    &dynamic_octree,
	    0,
	    index,
	    &cc.octqueue[index * 12]); // 48 bytes stride 12 int
    }

    renderconn_upload_octree_quadruplets(&rc, dynamic_octree.octs, dynamic_octree.txwth, dynamic_octree.txhth, true);

#endif

    mt_log_debug("main init");
}

void main_free()
{
}

// A line point 0
// B line point 1
// C point to project

v3_t project_point(v3_t A, v3_t B, v3_t C)
{
    v3_t  AC      = v3_sub(C, A);
    v3_t  AB      = v3_sub(B, A);
    float dotACAB = v3_dot(AC, AB);
    float dotABAB = v3_dot(AB, AB);
    float dotDiv  = dotACAB / dotABAB;
    return v3_add(A, v3_scale(AB, dotDiv));
}

typedef struct _tempcubes_t
{
    char arr[30][30][30];
} tempcubes_t;

// 0 - non-voxel
// 1 - sphere voxel
// 2 - wall voxel
// 3 - final voxel

void touch_neighbours(tempcubes_t* cubes, int x, int y, int z, int size)
{
    for (int cx = x - 1; cx < x + 2; cx++)
    {
	for (int cy = y - 1; cy < y + 2; cy++)
	{
	    for (int cz = z - 1; cz < z + 2; cz++)
	    {
		if (cx > 0 && cx < size && cy > 0 && cy < size && cz > 0 && cz < size)
		{
		    if (cubes->arr[cx][cy][cz] == 1)
		    {
			cubes->arr[cx][cy][cz] = 3;
			touch_neighbours(cubes, cx, cy, cz);
		    }
		}
	    }
	}
    }
}

void main_shoot()
{
    // check collosion between direction vector and static and dynamic voxels

    int index = octree_trace_line(&static_octree, position, direction);

    mt_log_debug("shoot, index %i", index);

    // remove all possible points in a sphere from the octree, collect all points and modified index range
    // from the collected points create new points pushed into the wall in anti-normal direction
    // add the new points to the octree, collect modified index range
    // update modified index ranges on the gpu with the new data

    v3_t pt  = (v3_t){static_model.vertexes[index * 3], static_model.vertexes[index * 3 + 1], static_model.vertexes[index * 3 + 2]};
    v3_t nrm = (v3_t){static_model.normals[index * 3], static_model.normals[index * 3 + 1], static_model.normals[index * 3 + 2]};
    v3_t col = (v3_t){static_model.colors[index * 3], static_model.colors[index * 3 + 1], static_model.colors[index * 3 + 2]};

    mt_log_debug("pt %f %f %f", pt.x, pt.y, pt.z);

    int minind = 0;
    int maxind = 0;

    // cover all grid points inside a sphere, starting with box

    int division = 2;
    for (int i = 0; i < static_octree.levels; i++) division *= 2;

    float step = static_octree.basecube.w / (float) division;

    int         size  = 15;
    tempcubes_t cubes = {0};

    for (int cx = -size; cx < size; cx++)
    {
	for (int cy = -size; cy < size; cy++)
	{
	    for (int cz = -size; cz < size; cz++)
	    {
		float dx = cx * step;
		float dy = cy * step;
		float dz = cz * step;

		if (dx * dx + dy * dy + dz * dz < (size * step) * (size * step))
		{
		    cubes.arr[cx + size][cy + size][cz + size] = 1;

		    int orind  = 0;
		    int octind = 0;

		    octree_remove_point(&static_octree, (v3_t){pt.x + dx, pt.y + dy, pt.z + dz}, &orind, &octind);

		    if (octind > 0)
		    {
			cubes.arr[cx + size][cy + size][cz + size] = 2;

			mt_log_debug("setting %i %i %i to 2", cx + size, cy + size, cz + size);

			if (minind == 0) minind = octind;
			if (maxind == 0) maxind = octind;

			if (octind < minind) minind = octind;
			if (octind > maxind) maxind = octind;
		    }
		}
	    }
	}
    }

    // remove closest side of tempcubes divided by removed voxels

    int ax = size;
    int ay = size;
    int az = size;
    if (nrm.x > 0.0) ax += 4;
    if (nrm.x <= 0.0) ax -= 4;
    if (nrm.y > 0.0) ay += 4;
    if (nrm.y <= 0.0) ay -= 4;
    if (nrm.z > 0.0) az += 4;
    if (nrm.z <= 0.0) az -= 4;

    touch_neighbours(&cubes, ax, ay, az, size * 2);

    /* for (int x = 0; x < 20; x++) */
    /* { */
    /* 	for (int y = 0; y < 20; y++) */
    /* 	{ */
    /* 	    for (int z = 0; z < 20; z++) */
    /* 	    { */
    /* 		if (cubes.arr[x][y][z] == 3) */
    /* 		    mt_log_debug("%i %i %i", x, y, z); */

    /* 	    } */
    /* 	} */
    /* } */

    mt_log_debug("minind %i maxind %i", minind, maxind);

    if (minind > 0)
    {
	int sy = (minind * 3) / 8192;
	int ey = (maxind * 3) / 8192;

	int noi = minind * 3 - (minind * 3) % 8192;

	GLint* data = (GLint*) static_octree.octs;

	mt_log_debug("noi %i", noi);
	mt_log_debug("sy %i ey %i", sy, ey);

	renderconn_upload_octree_quadruplets_partial(
	    &rc,
	    data + noi * 4,
	    0,
	    sy,
	    8192,
	    ey - sy + 1,
	    false);

	// hole is created, create cone

	size_t len = static_octree.len;
	size_t siz = static_octree.size;

	mt_log_debug("Creating conex");

	for (int cx = -size; cx < size; cx++)
	{
	    for (int cy = -size; cy < size; cy++)
	    {
		for (int cz = -size; cz < size; cz++)
		{
		    float dx = cx * step;
		    float dy = cy * step;
		    float dz = cz * step;

		    v3_t cp = (v3_t){pt.x + dx, pt.y + dy, pt.z + dz};

		    if (dx * dx + dy * dy + dz * dz > size * step - 3.0)
		    {
			if (cubes.arr[cx + size][cy + size][cz + size] == 3 || cubes.arr[cx + size][cy + size][cz + size] == 2)
			{
			    model_add_point(&static_model, cp, nrm, (v3_t){1.0, 1.0, 1.0});

			    octree_insert_point(
				&static_octree,
				0,
				static_model.point_count - 1,
				cp);
			}
		    }
		}
	    }
	}

	/* sy = (len * 3) / 8192; */
	/* ey = (siz * 3) / 8192; */

	/* noi = len * 3 - (len * 3) % 8192; */

	/* mt_log_debug("noi %i", noi); */
	/* mt_log_debug("sy %i ey %i", sy, ey); */

	renderconn_upload_normals(&rc, static_model.normals, static_model.txwth, static_model.txhth, static_model.comps, false);
	renderconn_upload_colors(&rc, static_model.colors, static_model.txwth, static_model.txhth, static_model.comps, false);
	renderconn_upload_octree_quadruplets(&rc, static_octree.octs, static_octree.txwth, static_octree.txhth, false);

	/* renderconn_upload_octree_quadruplets_partial( */
	/*     &rc, */
	/*     data + noi * 4, */
	/*     0, */
	/*     sy, */
	/*     8192, */
	/*     ey - sy + 1, */
	/*     false); */
    }
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

bool main_loop(double time, void* userdata)
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
		angle.x += event.motion.xrel / 300.0;
		angle.y -= event.motion.yrel / 300.0;

		direction  = v3_rotatearoundy((v3_t){0.0, 0.0, -1.0}, angle.x);
		directionX = v3_rotatearoundy((v3_t){1.0, 0.0, 0.0}, angle.x);

		v4_t axisquat = quat_from_axis_angle(directionX, angle.y);
		direction     = qrot(axisquat, direction);
	    }

	    if (event.type == SDL_MOUSEBUTTONDOWN)
	    {
		main_shoot();
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

		v2_t dimensions = {.x = event.window.data1 * scale, .y = event.window.data2 * scale};

		mt_log_debug("new dimension %f %f scale %f", dimensions.x, dimensions.y, scale);
	    }
	}
	else if (event.type == SDL_KEYUP)
	{
	    if (event.key.keysym.sym == SDLK_a) strafe = 0;
	    if (event.key.keysym.sym == SDLK_d) strafe = 0;
	    if (event.key.keysym.sym == SDLK_w) forward = 0;
	    if (event.key.keysym.sym == SDLK_s) forward = 0;
	    if (event.key.keysym.sym == SDLK_1) quality = 1;
	    if (event.key.keysym.sym == SDLK_2) quality = 2;
	    if (event.key.keysym.sym == SDLK_3) quality = 3;
	    if (event.key.keysym.sym == SDLK_4) quality = 4;
	    if (event.key.keysym.sym == SDLK_5) quality = 5;
	    if (event.key.keysym.sym == SDLK_6) quality = 6;
	    if (event.key.keysym.sym == SDLK_7) quality = 7;
	    if (event.key.keysym.sym == SDLK_8) quality = 8;
	    if (event.key.keysym.sym == SDLK_9) quality = 9;
	    if (event.key.keysym.sym == SDLK_0) quality = 10;
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

    uint32_t ticks = SDL_GetTicks();
    uint32_t delta = ticks - prevticks;

    if (delta > 16)
    {
	prevticks = ticks;

	if (delta > 1000) delta = 1000;
	if (delta < 16) delta = 16;
	float whole_step = 10.0;
	float curr_step  = whole_step / (1000.0 / (float) delta);

	if (strafe != 0)
	{
	    strafespeed += curr_step * strafe;

	    if (strafespeed > 10.0) strafespeed = 10.0;
	    if (strafespeed < -10.0) strafespeed = -10.0;
	}
	else
	    strafespeed *= 0.9;

	if (forward != 0)
	{
	    speed += curr_step * forward;

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

	lighta += curr_step / 10.0;
	if (lighta > 6.28) lighta = 0.0;

	if (octtest == 0)
	{
	    skeleconn_update(&cc, lighta, dynamic_model.point_count, maxlevel);

	    // add modified point coords by compute shader

	    octree_reset(&dynamic_octree, (v4_t){0.0, 1800.0, 1800.0, 1800.0});

	    for (int index = 0; index < dynamic_model.point_count; index++)
	    {
		octree_insert_path(
		    &dynamic_octree,
		    0,
		    index,
		    &cc.octqueue[index * 12]); // 48 bytes stride 12 int
	    }

	    renderconn_upload_octree_quadruplets(&rc, dynamic_octree.octs, dynamic_octree.txwth, dynamic_octree.txhth, true);
	}

	/* mt_time(NULL); */
	renderconn_update(&rc, width, height, position, angle, lighta, quality, maxlevel);

	SDL_GL_SwapWindow(window);
	/* mt_time("Render"); */

	frames++;
    }

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
	"  -t, --octtest                       Start with octree test scene\n"
	"  -l, --levels                        Maximum ocree depth\n"
	"\n";

    const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"verbose", no_argument, NULL, 'v'}};

    int option       = 0;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "vht", long_options, &option_index)) != -1)
    {
	switch (option)
	{
	    case '?': printf("parsing option %c value: %s\n", option, optarg); break;
	    case 'v': mt_log_inc_verbosity(); break;
	    case 't': octtest = 1; break;
	    case 'l': maxlevel = atoi(optarg); break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    // enable high dpi

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    // init sdl

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0)
    {
	// setup opengl version

#ifdef EMSCRIPTEN
	mt_log_inc_verbosity();
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#endif

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
	    SDL_WINDOWPOS_CENTERED,
	    SDL_WINDOWPOS_CENTERED,
	    width,
	    height,
	    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
#ifndef EMSCRIPTEN
		SDL_WINDOW_ALLOW_HIGHDPI |
#endif
		SDL_WINDOW_RESIZABLE);

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

		mt_log_debug("DRAWABLE %i %i", nw, nh);

		width  = nw;
		height = nh;

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
