#include <GL/glew.h>
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "model.c"
#include "modelutil.c"
#include "mt_log.c"
#include "mt_time.c"
#include "mt_vector_2d.c"
#include "octree.c"
#include "octree_glc.c"
#include "skeleton_glc.c"

#ifdef EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

/* #define OCTTEST 0 */

struct movement_t
{
    v3_t lookangle;
    v3_t lookpos;

    v3_t direction;
    v3_t directionX;

    int   walk;
    float walkspeed;

    int   strafe;
    float strafespeed;
} movement;

struct qubatron_t
{
    SDL_Window*   window;
    SDL_GLContext context;
    char          exit_flag;
    float         window_scale;

    skeleton_glc_t skel_glc;
    octree_glc_t   octr_glc;

    model_t static_model;
    model_t dynamic_model;

    octree_t static_octree;
    octree_t dynamic_octree;

    uint8_t render_detail;
    int     octree_depth;
    float   octree_size;
} quba;

int32_t width  = 1200;
int32_t height = 800;

uint32_t frames    = 0;
uint32_t timestamp = 0;
uint32_t prevticks = 0;

float lighta = 0.0;

void GLAPIENTRY
MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    mt_log_debug("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

void main_init()
{
    srand((unsigned int) time(NULL));

    quba.render_detail = 10;
    quba.octree_depth  = 12;
    quba.exit_flag     = 0;
    quba.window_scale  = 1.0;
    quba.octree_size   = 1800.0;

    movement.lookpos    = (v3_t){440.0, 200.0, 700.0};
    movement.direction  = (v3_t){0.0, 0.0, -1.0};
    movement.directionX = (v3_t){-1.0, 0.0, 0.0};

    // opengl init

#ifndef EMSCRIPTEN
/* glEnable(GL_DEBUG_OUTPUT); */
/* glDebugMessageCallback(MessageCallback, 0); */
#endif

    char* base_path = SDL_GetBasePath();
    char  path[PATH_MAX];

#ifdef EMSCRIPTEN
    snprintf(path, PATH_MAX, "%s/src/qubatron/shaders", base_path);
#else
    snprintf(path, PATH_MAX, "%s", base_path);
#endif

    quba.octr_glc = octree_glc_init(path);
    quba.skel_glc = skeleton_glc_init(path);

    quba.static_model  = model_init();
    quba.dynamic_model = model_init();

#ifdef OCTTEST

    int point_count = 5;

    GLfloat points[3 * 8192] = {
	10.0, 690.0, 10.0,
	10.0, 340.0, 10.0,
	10.0, 340.0, 690.0,
	10.0, 10.0, 10.0,
	690.0, 10.0, 690.0};

    GLfloat normals[3 * 8192] = {
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0};

    GLfloat colors[3 * 8192] = {
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0};

    quba.static_model.vertexes    = mt_memory_alloc(3 * 8192, NULL, NULL);
    quba.static_model.normals     = mt_memory_alloc(3 * 8192, NULL, NULL);
    quba.static_model.colors      = mt_memory_alloc(3 * 8192, NULL, NULL);
    quba.static_model.point_count = 5;
    quba.static_model.txwth       = 8192;
    quba.static_model.txhth       = (int) ceilf((float) quba.static_model.point_count / (float) quba.static_model.txwth);

    memcpy(quba.static_model.vertexes, points, 3 * 8192);
    memcpy(quba.static_model.normals, normals, 3 * 8192);
    memcpy(quba.static_model.colors, colors, 3 * 8192);

    // !!! it works only with x 0 first
    quba.static_octree = octree_create((v4_t){0.0, quba.octree_size, quba.octree_size, quba.octree_size}, quba.octree_depth);

    for (int index = 0; index < point_count * 3; index += 3)
    {
	octree_insert_point(
	    &quba.static_octree,
	    0,
	    index / 3,
	    (v3_t){points[index], points[index + 1], points[index + 2]},
	    NULL);
    }

    octree_glc_upload_texbuffer(&quba.octr_glc, normals, 0, 0, 8192, 1, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octr_glc->nrm1_tex, 8);
    octree_glc_upload_texbuffer(&quba.octr_glc, colors, 0, 0, 8192, 1, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octr_glc->col1_tex, 6);
    octree_glc_upload_texbuffer(&quba.octr_glc, quba.static_octree.octs, 0, 0, quba.static_octree.txwth, quba.static_octree.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octr_glc.oct1_tex, 10);

    // init dynamic model

    quba.dynamic_octree = octree_create((v4_t){0.0, quba.octree_size, quba.octree_size, quba.octree_size}, quba.octree_depth);

    octree_glc_upload_texbuffer(&quba.octr_glc, quba.dynamic_octree.octs, 0, 0, quba.dynamic_octree.txwth, quba.dynamic_octree.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octr_glc.oct2_tex, 11);

    // set camera position to see test cube
    movement.lookpos = (v3_t){900.0, 900.0, 3000.0};

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

    char pntpath[PATH_MAX];
    char nrmpath[PATH_MAX];
    char colpath[PATH_MAX];
    char rngpath[PATH_MAX];

    char* scenepath = "abandoned.ply";
    snprintf(pntpath, PATH_MAX, "%s%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%s%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%s%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%s%s.rng", base_path, scenepath);
    #ifdef EMSCRIPTEN
    snprintf(pntpath, PATH_MAX, "%sres/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%sres/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%sres/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%sres/%s.rng", base_path, scenepath);
    #endif

    model_load_flat(&quba.static_model, pntpath, colpath, nrmpath, rngpath);

    quba.static_octree = octree_create((v4_t){0.0, quba.octree_size, quba.octree_size, quba.octree_size}, quba.octree_depth);
    octets_t pathf;
    octets_t pathl;
    for (int index = 0; index < quba.static_model.point_count * 3; index += 3)
    {
	octets_t path =
	    octree_insert_point(
		&quba.static_octree,
		0,
		index / 3,
		(v3_t){quba.static_model.vertexes[index], quba.static_model.vertexes[index + 1], quba.static_model.vertexes[index + 2]},
		NULL);

	if (index == 0)
	    pathf = path;
	else if (index == (quba.static_model.point_count - 3))
	    pathl = path;
    }

    mt_log_debug("STATIC MODEL");
    model_log_vertex_info(&quba.static_model, 0);
    octree_log_path(pathf, 0);
    model_log_vertex_info(&quba.static_model, quba.static_model.point_count - 1);
    octree_log_path(pathl, quba.static_model.point_count - 1);

    // upload static model

    octree_glc_upload_texbuffer(&quba.octr_glc, quba.static_model.colors, 0, 0, quba.static_model.txwth, quba.static_model.txhth, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octr_glc.col1_tex, 6);
    octree_glc_upload_texbuffer(&quba.octr_glc, quba.static_model.normals, 0, 0, quba.static_model.txwth, quba.static_model.txhth, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octr_glc.nrm1_tex, 8);
    octree_glc_upload_texbuffer(&quba.octr_glc, quba.static_octree.octs, 0, 0, quba.static_octree.txwth, quba.static_octree.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octr_glc.oct1_tex, 10);

    scenepath = "zombie.ply";
    snprintf(pntpath, PATH_MAX, "%s%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%s%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%s%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%s%s.rng", base_path, scenepath);
    #ifdef EMSCRIPTEN
    snprintf(pntpath, PATH_MAX, "%sres/%s.pnt", base_path, scenepath);
    snprintf(nrmpath, PATH_MAX, "%sres/%s.nrm", base_path, scenepath);
    snprintf(colpath, PATH_MAX, "%sres/%s.col", base_path, scenepath);
    snprintf(rngpath, PATH_MAX, "%sres/%s.rng", base_path, scenepath);
    #endif
    model_load_flat(&quba.dynamic_model, pntpath, colpath, nrmpath, rngpath);

    quba.dynamic_octree = octree_create((v4_t){0.0, quba.octree_size, quba.octree_size, quba.octree_size}, quba.octree_depth);
    for (int index = 0; index < quba.dynamic_model.point_count * 3; index += 3)
    {
	octets_t path =
	    octree_insert_point(
		&quba.dynamic_octree,
		0,
		index / 3,
		(v3_t){quba.dynamic_model.vertexes[index], quba.dynamic_model.vertexes[index + 1], quba.dynamic_model.vertexes[index + 2]},
		NULL);
	if (index == 0)
	    pathf = path;
	else if (index == (quba.dynamic_model.point_count - 3))
	    pathl = path;
    }

    mt_log_debug("DYNAMIC MODEL");
    model_log_vertex_info(&quba.dynamic_model, 0);
    model_log_vertex_info(&quba.dynamic_model, quba.dynamic_model.point_count - 1);
    octree_log_path(pathf, 0);
    octree_log_path(pathl, quba.dynamic_model.point_count - 3);

    // upload dynamic model

    octree_glc_upload_texbuffer(&quba.octr_glc, quba.dynamic_model.colors, 0, 0, quba.dynamic_model.txwth, quba.dynamic_model.txhth, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octr_glc.col2_tex, 7);
    octree_glc_upload_texbuffer(&quba.octr_glc, quba.dynamic_model.normals, 0, 0, quba.dynamic_model.txwth, quba.dynamic_model.txhth, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octr_glc.nrm2_tex, 9);
    octree_glc_upload_texbuffer(&quba.octr_glc, quba.dynamic_octree.octs, 0, 0, quba.dynamic_octree.txwth, quba.dynamic_octree.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octr_glc.oct2_tex, 11);

    // upload actor to skeleton modifier

    skeleton_glc_alloc_in(&quba.skel_glc, quba.dynamic_model.vertexes, quba.dynamic_model.point_count * 3 * sizeof(GLfloat));
    skeleton_glc_alloc_out(&quba.skel_glc, NULL, quba.dynamic_model.point_count * sizeof(GLint) * 12);
    skeleton_glc_update(&quba.skel_glc, lighta, quba.dynamic_model.point_count, quba.octree_depth, quba.octree_size);
    skeleton_glc_update(&quba.skel_glc, lighta, quba.dynamic_model.point_count, quba.octree_depth, quba.octree_size); // double run to step over double buffer

    // add modified point coords by compute shader

    octree_reset(&quba.dynamic_octree, (v4_t){0.0, quba.octree_size, quba.octree_size, quba.octree_size});

    for (int index = 0; index < quba.dynamic_model.point_count; index++)
    {
	octree_insert_path(
	    &quba.dynamic_octree,
	    0,
	    index,
	    &quba.skel_glc.octqueue[index * 12]); // 48 bytes stride 12 int
    }

    octree_glc_upload_texbuffer(&quba.octr_glc, quba.dynamic_octree.octs, 0, 0, quba.dynamic_octree.txwth, quba.dynamic_octree.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octr_glc.oct2_tex, 11);

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

	    /* v2_t dimensions = {.x = x * quba.window_scale, .y = y * quba.window_scale}; */

	    if (event.type == SDL_MOUSEMOTION)
	    {
		movement.lookangle.x += event.motion.xrel / 300.0;
		movement.lookangle.y -= event.motion.yrel / 300.0;

		movement.direction  = v3_rotatearoundy((v3_t){0.0, 0.0, -1.0}, movement.lookangle.x);
		movement.directionX = v3_rotatearoundy((v3_t){1.0, 0.0, 0.0}, movement.lookangle.x);

		v4_t axisquat      = quat_from_axis_angle(movement.directionX, movement.lookangle.y);
		movement.direction = qrot(axisquat, movement.direction);
	    }

	    if (event.type == SDL_MOUSEBUTTONDOWN)
	    {
		modelutil_punch_hole(&quba.octr_glc, &quba.static_octree, &quba.static_model, movement.lookpos, movement.direction);
	    }
	}
	else if (event.type == SDL_QUIT)
	{
	    quba.exit_flag = 1;
	}
	else if (event.type == SDL_WINDOWEVENT)
	{
	    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
	    {
		width  = event.window.data1;
		height = event.window.data2;

		v2_t dimensions = {.x = event.window.data1 * quba.window_scale, .y = event.window.data2 * quba.window_scale};

		mt_log_debug("new dimension %f %f scale %f", dimensions.x, dimensions.y, quba.window_scale);
	    }
	}
	else if (event.type == SDL_KEYUP)
	{
	    if (event.key.keysym.sym == SDLK_a) movement.strafe = 0;
	    if (event.key.keysym.sym == SDLK_d) movement.strafe = 0;
	    if (event.key.keysym.sym == SDLK_w) movement.walk = 0;
	    if (event.key.keysym.sym == SDLK_s) movement.walk = 0;
	    if (event.key.keysym.sym == SDLK_1) quba.render_detail = 1;
	    if (event.key.keysym.sym == SDLK_2) quba.render_detail = 2;
	    if (event.key.keysym.sym == SDLK_3) quba.render_detail = 3;
	    if (event.key.keysym.sym == SDLK_4) quba.render_detail = 4;
	    if (event.key.keysym.sym == SDLK_5) quba.render_detail = 5;
	    if (event.key.keysym.sym == SDLK_6) quba.render_detail = 6;
	    if (event.key.keysym.sym == SDLK_7) quba.render_detail = 7;
	    if (event.key.keysym.sym == SDLK_8) quba.render_detail = 8;
	    if (event.key.keysym.sym == SDLK_9) quba.render_detail = 9;
	    if (event.key.keysym.sym == SDLK_0) quba.render_detail = 10;
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
		    movement.strafe = 1;
		    break;

		case SDLK_d:
		    movement.strafe = -1;
		    break;

		case SDLK_w:
		    movement.walk = 1;
		    break;

		case SDLK_s:
		    movement.walk = -1;
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

	if (movement.strafe != 0)
	{
	    movement.strafespeed += curr_step * movement.strafe;

	    if (movement.strafespeed > 10.0) movement.strafespeed = 10.0;
	    if (movement.strafespeed < -10.0) movement.strafespeed = -10.0;
	}
	else
	    movement.strafespeed *= 0.9;

	if (movement.walk != 0)
	{
	    movement.walkspeed += curr_step * movement.walk;

	    if (movement.walkspeed > 10.0) movement.walkspeed = 10.0;
	    if (movement.walkspeed < -10.0) movement.walkspeed = -10.0;
	}
	else
	    movement.walkspeed *= 0.9;

	if (movement.strafespeed > 0.0001 || movement.strafespeed < -0.0001)
	{
	    v3_t curr_speed  = v3_scale(movement.directionX, -movement.strafespeed);
	    movement.lookpos = v3_add(movement.lookpos, curr_speed);
	}

	if (movement.walkspeed > 0.0001 || movement.walkspeed < -0.0001)
	{
	    v3_t curr_speed  = v3_scale(movement.direction, movement.walkspeed);
	    movement.lookpos = v3_add(movement.lookpos, curr_speed);
	}

	lighta += curr_step / 10.0;
	if (lighta > 6.28) lighta = 0.0;

#ifndef OCTTEST

	skeleton_glc_update(&quba.skel_glc, lighta, quba.dynamic_model.point_count, quba.octree_depth, quba.octree_size);

	// add modified point coords by compute shader

	octree_reset(&quba.dynamic_octree, (v4_t){0.0, quba.octree_size, quba.octree_size, quba.octree_size});

	for (int index = 0; index < quba.dynamic_model.point_count; index++)
	{
	    octree_insert_path(
		&quba.dynamic_octree,
		0,
		index,
		&quba.skel_glc.octqueue[index * 12]); // 48 bytes stride 12 int
	}

	octree_glc_upload_texbuffer(&quba.octr_glc, quba.dynamic_octree.octs, 0, 0, quba.dynamic_octree.txwth, quba.dynamic_octree.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octr_glc.oct2_tex, 11);
#endif

	/* mt_time(NULL); */
	octree_glc_update(&quba.octr_glc, width, height, movement.lookpos, movement.lookangle, lighta, quba.render_detail, quba.octree_depth, quba.octree_size);

	SDL_GL_SwapWindow(quba.window);
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
	    case 'l': quba.octree_depth = atoi(optarg); break;
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

	quba.window = SDL_CreateWindow(
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

	if (quba.window != NULL)
	{
	    // create context

	    quba.context = SDL_GL_CreateContext(quba.window);

	    if (quba.context != NULL)
	    {
		GLint GlewInitResult = glewInit();
		if (GLEW_OK != GlewInitResult)
		    mt_log_error("%s", glewGetErrorString(GlewInitResult));

		int nw;
		int nh;

		SDL_GL_GetDrawableSize(quba.window, &nw, &nh);

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
		while (!quba.exit_flag)
		{
		    main_loop(0, NULL);
		}
#endif

		main_free();

		// cleanup

		SDL_GL_DeleteContext(quba.context);
	    }
	    else
		mt_log_error("SDL context creation error %s", SDL_GetError());

	    // cleanup

	    SDL_DestroyWindow(quba.window);
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
