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

    uint32_t prevticks;
} move = {0};

struct qubatron_t
{
    char          doexit;
    SDL_Window*   window;
    SDL_GLContext context;

    float   winscale;
    int32_t winwth;
    int32_t winhth;

    model_t statmod;
    model_t dynamod;

    octree_t statoctr;
    octree_t dynaoctr;

    skeleton_glc_t skelglc;
    octree_glc_t   octrglc;

    uint8_t rndscale;
    int     octrdpth;
    float   octrsize;

    uint32_t frames;
    uint32_t frametamp;

    float lightangle;
} quba = {0};

void GLAPIENTRY
MessageCallback(
    GLenum        source,
    GLenum        type,
    GLuint        id,
    GLenum        severity,
    GLsizei       length,
    const GLchar* message,
    const void*   userParam)
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

    char* base_path = SDL_GetBasePath();
    char  path[PATH_MAX];

#ifdef EMSCRIPTEN
    snprintf(path, PATH_MAX, "%s/src/qubatron/shaders", base_path);
#else
    snprintf(path, PATH_MAX, "%s", base_path);
#endif

    quba.octrglc = octree_glc_init(path);
    quba.skelglc = skeleton_glc_init(path);

    quba.statmod = model_init();
    quba.dynamod = model_init();

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

    quba.statmod.vertexes    = mt_memory_alloc(3 * 8192, NULL, NULL);
    quba.statmod.normals     = mt_memory_alloc(3 * 8192, NULL, NULL);
    quba.statmod.colors      = mt_memory_alloc(3 * 8192, NULL, NULL);
    quba.statmod.point_count = 5;
    quba.statmod.txwth       = 8192;
    quba.statmod.txhth       = (int) ceilf((float) quba.statmod.point_count / (float) quba.statmod.txwth);

    memcpy(quba.statmod.vertexes, points, 3 * 8192);
    memcpy(quba.statmod.normals, normals, 3 * 8192);
    memcpy(quba.statmod.colors, colors, 3 * 8192);

    // !!! it works only with x 0 first
    quba.statoctr = octree_create((v4_t){0.0, quba.octrsize, quba.octrsize, quba.octrsize}, quba.octrdpth);

    for (int index = 0; index < point_count * 3; index += 3)
    {
	octree_insert_point(
	    &quba.statoctr,
	    0,
	    index / 3,
	    (v3_t){points[index], points[index + 1], points[index + 2]},
	    NULL);
    }

    octree_glc_upload_texbuffer(&quba.octrglc, normals, 0, 0, 8192, 1, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octrglc->nrm1_tex, 8);
    octree_glc_upload_texbuffer(&quba.octrglc, colors, 0, 0, 8192, 1, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octrglc->col1_tex, 6);
    octree_glc_upload_texbuffer(&quba.octrglc, quba.statoctr.octs, 0, 0, quba.statoctr.txwth, quba.statoctr.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octrglc.oct1_tex, 10);

    // init dynamic model

    quba.dynaoctr = octree_create((v4_t){0.0, quba.octrsize, quba.octrsize, quba.octrsize}, quba.octrdpth);

    octree_glc_upload_texbuffer(&quba.octrglc, quba.dynaoctr.octs, 0, 0, quba.dynaoctr.txwth, quba.dynaoctr.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octrglc.oct2_tex, 11);

    // set camera position to see test cube
    move.lookpos = (v3_t){900.0, 900.0, 3000.0};

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

    model_load_flat(&quba.statmod, pntpath, colpath, nrmpath, rngpath);

    quba.statoctr = octree_create((v4_t){0.0, quba.octrsize, quba.octrsize, quba.octrsize}, quba.octrdpth);

    octets_t fstpath;
    octets_t lstpath;

    for (int index = 0; index < quba.statmod.point_count * 3; index += 3)
    {
	octets_t path =
	    octree_insert_point(
		&quba.statoctr,
		0,
		index / 3,
		(v3_t){quba.statmod.vertexes[index], quba.statmod.vertexes[index + 1], quba.statmod.vertexes[index + 2]},
		NULL);

	if (index == 0)
	    fstpath = path;
	else if (index == (quba.statmod.point_count - 3))
	    lstpath = path;
    }

    mt_log_debug("STATIC MODEL");
    model_log_vertex_info(&quba.statmod, 0);
    octree_log_path(fstpath, 0);
    model_log_vertex_info(&quba.statmod, quba.statmod.point_count - 1);
    octree_log_path(lstpath, quba.statmod.point_count - 1);

    // upload static model

    octree_glc_upload_texbuffer(&quba.octrglc, quba.statmod.colors, 0, 0, quba.statmod.txwth, quba.statmod.txhth, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octrglc.col1_tex, 6);
    octree_glc_upload_texbuffer(&quba.octrglc, quba.statmod.normals, 0, 0, quba.statmod.txwth, quba.statmod.txhth, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octrglc.nrm1_tex, 8);
    octree_glc_upload_texbuffer(&quba.octrglc, quba.statoctr.octs, 0, 0, quba.statoctr.txwth, quba.statoctr.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octrglc.oct1_tex, 10);

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
    model_load_flat(&quba.dynamod, pntpath, colpath, nrmpath, rngpath);

    quba.dynaoctr = octree_create((v4_t){0.0, quba.octrsize, quba.octrsize, quba.octrsize}, quba.octrdpth);
    for (int index = 0; index < quba.dynamod.point_count * 3; index += 3)
    {
	octets_t path =
	    octree_insert_point(
		&quba.dynaoctr,
		0,
		index / 3,
		(v3_t){quba.dynamod.vertexes[index], quba.dynamod.vertexes[index + 1], quba.dynamod.vertexes[index + 2]},
		NULL);
	if (index == 0)
	    fstpath = path;
	else if (index == (quba.dynamod.point_count - 3))
	    lstpath = path;
    }

    mt_log_debug("DYNAMIC MODEL");
    model_log_vertex_info(&quba.dynamod, 0);
    model_log_vertex_info(&quba.dynamod, quba.dynamod.point_count - 1);
    octree_log_path(fstpath, 0);
    octree_log_path(lstpath, quba.dynamod.point_count - 3);

    // upload dynamic model

    octree_glc_upload_texbuffer(&quba.octrglc, quba.dynamod.colors, 0, 0, quba.dynamod.txwth, quba.dynamod.txhth, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octrglc.col2_tex, 7);
    octree_glc_upload_texbuffer(&quba.octrglc, quba.dynamod.normals, 0, 0, quba.dynamod.txwth, quba.dynamod.txhth, GL_RGB32F, GL_RGB, GL_FLOAT, quba.octrglc.nrm2_tex, 9);
    octree_glc_upload_texbuffer(&quba.octrglc, quba.dynaoctr.octs, 0, 0, quba.dynaoctr.txwth, quba.dynaoctr.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octrglc.oct2_tex, 11);

    // upload actor to skeleton modifier

    skeleton_glc_alloc_in(&quba.skelglc, quba.dynamod.vertexes, quba.dynamod.point_count * 3 * sizeof(GLfloat));
    skeleton_glc_alloc_out(&quba.skelglc, NULL, quba.dynamod.point_count * sizeof(GLint) * 12);
    skeleton_glc_update(&quba.skelglc, quba.lightangle, quba.dynamod.point_count, quba.octrdpth, quba.octrsize);
    skeleton_glc_update(&quba.skelglc, quba.lightangle, quba.dynamod.point_count, quba.octrdpth, quba.octrsize); // double run to step over double buffer

    // add modified point coords by compute shader

    octree_reset(&quba.dynaoctr, (v4_t){0.0, quba.octrsize, quba.octrsize, quba.octrsize});

    for (int index = 0; index < quba.dynamod.point_count; index++)
    {
	octree_insert_path(
	    &quba.dynaoctr,
	    0,
	    index,
	    &quba.skelglc.octqueue[index * 12]); // 48 bytes stride 12 int
    }

    octree_glc_upload_texbuffer(&quba.octrglc, quba.dynaoctr.octs, 0, 0, quba.dynaoctr.txwth, quba.dynaoctr.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octrglc.oct2_tex, 11);

#endif

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

bool main_loop(double time, void* userdata)
{
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0)
    {
	if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION)
	{
	    int x = 0, y = 0;
	    SDL_GetMouseState(&x, &y);

	    /* v2_t dimensions = {.x = x * quba.winscale, .y = y * quba.winscale}; */

	    if (event.type == SDL_MOUSEMOTION)
	    {
		move.lookangle.x += event.motion.xrel / 300.0;
		move.lookangle.y -= event.motion.yrel / 300.0;

		move.direction  = v3_rotatearoundy((v3_t){0.0, 0.0, -1.0}, move.lookangle.x);
		move.directionX = v3_rotatearoundy((v3_t){1.0, 0.0, 0.0}, move.lookangle.x);

		v4_t axisquat  = quat_from_axis_angle(move.directionX, move.lookangle.y);
		move.direction = qrot(axisquat, move.direction);
	    }

	    if (event.type == SDL_MOUSEBUTTONDOWN)
	    {
		modelutil_punch_hole(&quba.octrglc, &quba.statoctr, &quba.statmod, move.lookpos, move.direction);
	    }
	}
	else if (event.type == SDL_QUIT)
	{
	    quba.doexit = 1;
	}
	else if (event.type == SDL_WINDOWEVENT)
	{
	    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
	    {
		quba.winwth = event.window.data1;
		quba.winhth = event.window.data2;

		v2_t dimensions = {.x = event.window.data1 * quba.winscale, .y = event.window.data2 * quba.winscale};

		mt_log_debug("new dimension %f %f scale %f", dimensions.x, dimensions.y, quba.winscale);
	    }
	}
	else if (event.type == SDL_KEYUP)
	{
	    if (event.key.keysym.sym == SDLK_a) move.strafe = 0;
	    if (event.key.keysym.sym == SDLK_d) move.strafe = 0;
	    if (event.key.keysym.sym == SDLK_w) move.walk = 0;
	    if (event.key.keysym.sym == SDLK_s) move.walk = 0;
	    if (event.key.keysym.sym == SDLK_1) quba.rndscale = 1;
	    if (event.key.keysym.sym == SDLK_2) quba.rndscale = 2;
	    if (event.key.keysym.sym == SDLK_3) quba.rndscale = 3;
	    if (event.key.keysym.sym == SDLK_4) quba.rndscale = 4;
	    if (event.key.keysym.sym == SDLK_5) quba.rndscale = 5;
	    if (event.key.keysym.sym == SDLK_6) quba.rndscale = 6;
	    if (event.key.keysym.sym == SDLK_7) quba.rndscale = 7;
	    if (event.key.keysym.sym == SDLK_8) quba.rndscale = 8;
	    if (event.key.keysym.sym == SDLK_9) quba.rndscale = 9;
	    if (event.key.keysym.sym == SDLK_0) quba.rndscale = 10;
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
		    move.strafe = 1;
		    break;

		case SDLK_d:
		    move.strafe = -1;
		    break;

		case SDLK_w:
		    move.walk = 1;
		    break;

		case SDLK_s:
		    move.walk = -1;
		    break;
	    }
	}
	else if (event.type == SDL_APP_WILLENTERFOREGROUND)
	{
	}
    }

    // update simulation

    uint32_t ticks = SDL_GetTicks();
    uint32_t delta = ticks - move.prevticks;

    if (delta > 16)
    {
	move.prevticks = ticks;

	if (delta > 1000) delta = 1000;
	if (delta < 16) delta = 16;
	float whole_step = 10.0;
	float curr_step  = whole_step / (1000.0 / (float) delta);

	if (move.strafe != 0)
	{
	    move.strafespeed += curr_step * move.strafe;

	    if (move.strafespeed > 10.0) move.strafespeed = 10.0;
	    if (move.strafespeed < -10.0) move.strafespeed = -10.0;
	}
	else
	    move.strafespeed *= 0.9;

	if (move.walk != 0)
	{
	    move.walkspeed += curr_step * move.walk;

	    if (move.walkspeed > 10.0) move.walkspeed = 10.0;
	    if (move.walkspeed < -10.0) move.walkspeed = -10.0;
	}
	else
	    move.walkspeed *= 0.9;

	if (move.strafespeed > 0.0001 || move.strafespeed < -0.0001)
	{
	    v3_t curr_speed = v3_scale(move.directionX, -move.strafespeed);
	    move.lookpos    = v3_add(move.lookpos, curr_speed);
	}

	if (move.walkspeed > 0.0001 || move.walkspeed < -0.0001)
	{
	    v3_t curr_speed = v3_scale(move.direction, move.walkspeed);
	    move.lookpos    = v3_add(move.lookpos, curr_speed);
	}

	quba.lightangle += curr_step / 10.0;
	if (quba.lightangle > 6.28) quba.lightangle = 0.0;

#ifndef OCTTEST

	skeleton_glc_update(&quba.skelglc, quba.lightangle, quba.dynamod.point_count, quba.octrdpth, quba.octrsize);

	// add modified point coords by compute shader

	octree_reset(&quba.dynaoctr, (v4_t){0.0, quba.octrsize, quba.octrsize, quba.octrsize});

	for (int index = 0; index < quba.dynamod.point_count; index++)
	{
	    octree_insert_path(
		&quba.dynaoctr,
		0,
		index,
		&quba.skelglc.octqueue[index * 12]); // 48 bytes stride 12 int
	}

	octree_glc_upload_texbuffer(&quba.octrglc, quba.dynaoctr.octs, 0, 0, quba.dynaoctr.txwth, quba.dynaoctr.txhth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, quba.octrglc.oct2_tex, 11);
#endif

	/* mt_time(NULL); */
	octree_glc_update(&quba.octrglc, quba.winwth, quba.winhth, move.lookpos, move.lookangle, quba.lightangle, quba.rndscale, quba.octrdpth, quba.octrsize);

	SDL_GL_SwapWindow(quba.window);
	/* mt_time("Render"); */

	quba.frames++;
    }

    return 1;
}

int main(int argc, char* argv[])
{
    quba.rndscale = 10;
    quba.octrdpth = 12;
    quba.doexit   = 0;
    quba.winscale = 1.0;
    quba.winwth   = 1200;
    quba.winhth   = 800;
    quba.octrsize = 1800.0;

    move.lookpos    = (v3_t){440.0, 200.0, 700.0};
    move.direction  = (v3_t){0.0, 0.0, -1.0};
    move.directionX = (v3_t){-1.0, 0.0, 0.0};

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
	    case 'l': quba.octrdpth = atoi(optarg); break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0)
    {
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

	// create window

	mt_log_debug("create window  %i %i", quba.winwth, quba.winhth);

	quba.window = SDL_CreateWindow(
	    "Qubatron",
	    SDL_WINDOWPOS_CENTERED,
	    SDL_WINDOWPOS_CENTERED,
	    quba.winwth,
	    quba.winhth,
	    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
#ifndef EMSCRIPTEN
		SDL_WINDOW_ALLOW_HIGHDPI |
#endif
		SDL_WINDOW_RESIZABLE);

	if (quba.window != NULL)
	{
	    quba.context = SDL_GL_CreateContext(quba.window);

	    if (quba.context != NULL)
	    {
		GLint GlewInitResult = glewInit();
		if (GLEW_OK != GlewInitResult)
		    mt_log_error("%s", glewGetErrorString(GlewInitResult));

		SDL_GL_GetDrawableSize(quba.window, &quba.winwth, &quba.winhth);

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
		while (!quba.doexit)
		    main_loop(0, NULL);
#endif

		main_free();

		SDL_GL_DeleteContext(quba.context);
	    }
	    else
		mt_log_error("SDL context creation error %s", SDL_GetError());

	    SDL_DestroyWindow(quba.window);
	}
	else
	    mt_log_error("SDL window creation error %s", SDL_GetError());

	SDL_Quit();
    }
    else
	mt_log_error("SDL init error %s", SDL_GetError());

    return 0;
}
