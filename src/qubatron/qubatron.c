#include <GL/glew.h>
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "dust_glc.c"
#include "model.c"
#include "modelutil.c"
#include "mt_log.c"
#include "mt_quat.c"
#include "mt_time.c"
#include "mt_vector_2d.c"
#include "octree.c"
#include "octree_glc.c"
#include "skeleton_glc.c"

#ifdef EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

/* #define OCTTEST 1 */

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
    int      freemove;
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
    model_t partmod;
    model_t dustmod;

    octree_t statoctr;
    octree_t dynaoctr;

    skeleton_glc_t skelglc;
    particle_glc_t partglc;
    octree_glc_t   octrglc;
    dust_glc_t     dustglc;

    uint8_t rndscale;
    int     octrdpth;
    float   octrsize;

    uint32_t frames;
    uint32_t frametamp;

    float lightangle;

    long zombiecount;
    int  shoot;
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
    quba.partglc = particle_glc_init(path);
    quba.dustglc = dust_glc_init(path);

    quba.statmod = model_init();
    quba.dynamod = model_init();
    quba.partmod = model_init();
    quba.dustmod = model_init();

    quba.statoctr = octree_create(
	(v4_t){0.0, quba.octrsize, quba.octrsize, quba.octrsize},
	quba.octrdpth);

    quba.dynaoctr = octree_create(
	(v4_t){0.0, quba.octrsize, quba.octrsize, quba.octrsize},
	quba.octrdpth);

#ifdef OCTTEST
    // set camera position to see test cube
    move.lookpos = (v3_t){900.0, 900.0, 3000.0};

    modelutil_load_test(
	base_path,
	&quba.octrglc,
	&quba.skelglc,
	&quba.statoctr,
	&quba.dynaoctr,
	&quba.statmod,
	&quba.dynamod);
#else
    modelutil_load_flat(
	base_path,
	&quba.octrglc,
	&quba.skelglc,
	&quba.statoctr,
	&quba.dynaoctr,
	&quba.statmod,
	&quba.dynamod);
#endif

    quba.zombiecount = quba.dynamod.point_count;

    skeleton_glc_init_zombie(&quba.skelglc, &quba.statoctr);

    // init dust

    for (int i = 0; i < 1000; i++)
    {
	v3_t pnt = (v3_t){
	    400.0 + 400.0 * (float) (rand() % 100) / 100.0,
	    300.0 * (float) (rand() % 100) / 100.0,
	    400.0 * (float) (rand() % 100) / 100.0,
	};

	v3_t spd = (v3_t){
	    -0.4 + 0.8 * ((float) (rand() % 100) / 100.0),
	    -0.4 + 0.8 * ((float) (rand() % 100) / 100.0),
	    -0.4 + 0.8 * ((float) (rand() % 100) / 100.0),
	};

	v3_t col = (v3_t){1.2, 1.2, 1.2};

	model_add_point(&quba.dustmod, pnt, spd, col);
    }

    dust_glc_alloc_out(&quba.dustglc, NULL, quba.dustmod.point_count * 3 * sizeof(GLfloat));

    model_append(&quba.dynamod, &quba.dustmod);

    octree_glc_upload_texbuffer_data(
	&quba.octrglc,
	quba.dynamod.colors,                            // buffer
	GL_FLOAT,                                       // data type
	quba.dynamod.point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                            // itemsize
	0,                                              // start offset
	quba.dynamod.point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_COLOR);               // buffer type

    octree_glc_upload_texbuffer_data(
	&quba.octrglc,
	quba.dynamod.normals,                           // buffer
	GL_FLOAT,                                       // data type
	quba.dynamod.point_count * sizeof(GLfloat) * 3, // size
	sizeof(GLfloat) * 3,                            // itemsize
	0,                                              // start offset
	quba.dynamod.point_count * sizeof(GLfloat) * 3, // end offset
	OCTREE_GLC_BUFFER_DYNAMIC_NORMAL);              // buffer type

    mt_log_debug("main init");
}

void main_free()
{
}

int main_canmove(v3_t pos)
{
    v4_t  x1tl, x2tl, y1tl, y2tl, z1tl, z2tl;
    float x1len = 100.0, x2len = 100.0, y1len = 100.0, y2len = 100.0, z1len = 100.0, z2len = 100.0;

    int x1ind = octree_trace_line(&quba.statoctr, pos, (v3_t){10.0, 0.0, 0.0}, &x1tl); // horizotnal dirs
    int x2ind = octree_trace_line(&quba.statoctr, pos, (v3_t){-10.0, 0.0, 0.0}, &x2tl);

    int y1ind = octree_trace_line(&quba.statoctr, pos, (v3_t){0.0, 10.0, 0.0}, &y1tl); // vertical dirs
    int y2ind = octree_trace_line(&quba.statoctr, pos, (v3_t){0.0, -10.0, 0.0}, &y2tl);

    int z1ind = octree_trace_line(&quba.statoctr, pos, (v3_t){0.0, 0.0, 10.0}, &z1tl); // depth dirs
    int z2ind = octree_trace_line(&quba.statoctr, pos, (v3_t){0.0, 0.0, -10.0}, &z2tl);

    if (x1ind > 0) x1len = v3_length(v3_sub(v4_xyz(x1tl), pos));
    if (x2ind > 0) x2len = v3_length(v3_sub(v4_xyz(x2tl), pos));
    if (y1ind > 0) y1len = v3_length(v3_sub(v4_xyz(y1tl), pos));
    if (y2ind > 0) y2len = v3_length(v3_sub(v4_xyz(y2tl), pos));
    if (z1ind > 0) z1len = v3_length(v3_sub(v4_xyz(z1tl), pos));
    if (z2ind > 0)
	z2len = v3_length(v3_sub(v4_xyz(z2tl), pos));

    if (x1len < 10.0 || x2len < 10.0 || y1len < 10.0 || y2len < 10.0 || z1len < 10.0 || z2len < 10.0)
	return 0;
    else
	return 1;
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

	    if (event.type == SDL_MOUSEMOTION)
	    {
		move.lookangle.x += event.motion.xrel / 300.0;
		move.lookangle.y -= event.motion.yrel / 300.0;

		move.direction  = v3_rotatearoundy((v3_t){0.0, 0.0, -1.0}, move.lookangle.x);
		move.directionX = v3_rotatearoundy((v3_t){1.0, 0.0, 0.0}, move.lookangle.x);

		v4_t axisquat  = quat_from_axis_angle(move.directionX, move.lookangle.y);
		move.direction = quat_rotate(axisquat, move.direction);
	    }

	    if (event.type == SDL_MOUSEBUTTONDOWN)
	    {
		v4_t tlf       = (v4_t){0.0};
		int  statindex = octree_trace_line(&quba.statoctr, move.lookpos, move.direction, &tlf);
		int  dynaindex = octree_trace_line(&quba.dynaoctr, move.lookpos, move.direction, &tlf);

		quba.shoot = 1;

		if (dynaindex > 0)
		{
		    modelutil_punch_hole_dyna(&quba.octrglc, &quba.skelglc, &quba.partglc, &quba.partmod, dynaindex, &quba.dynamod, move.lookpos, move.direction, tlf, quba.zombiecount + quba.dustmod.point_count);

		    skeleton_glc_shoot(&quba.skelglc, move.lookpos, move.direction, v4_xyz(tlf));
		}
		else if (statindex > 0)
		    modelutil_punch_hole(&quba.octrglc, &quba.partglc, &quba.partmod, &quba.statoctr, &quba.statmod, &quba.dynamod, move.lookpos, move.direction, quba.zombiecount + quba.dustmod.point_count);
	    }

	    if (event.type == SDL_MOUSEBUTTONUP)
	    {
	    }
	}
	else if (event.type == SDL_QUIT)
	{
	    quba.doexit = 1;
	}
	else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
	{
	    quba.winwth = event.window.data1;
	    quba.winhth = event.window.data2;

	    v2_t dimensions = {.x = event.window.data1 * quba.winscale, .y = event.window.data2 * quba.winscale};

	    mt_log_debug("new dimension %f %f scale %f", dimensions.x, dimensions.y, quba.winscale);
	}
	else if (event.type == SDL_KEYUP)
	{
	    if (event.key.keysym.sym == SDLK_a) move.strafe = 0;
	    if (event.key.keysym.sym == SDLK_d) move.strafe = 0;
	    if (event.key.keysym.sym == SDLK_w) move.walk = 0;
	    if (event.key.keysym.sym == SDLK_s) move.walk = 0;
	    if (event.key.keysym.sym == SDLK_g) skeleton_glc_move(&quba.skelglc, 0);
	    if (event.key.keysym.sym == SDLK_j) skeleton_glc_move(&quba.skelglc, 0);
	    if (event.key.keysym.sym == SDLK_y) skeleton_glc_move(&quba.skelglc, 0);
	    if (event.key.keysym.sym == SDLK_h) skeleton_glc_move(&quba.skelglc, 0);
	    if (event.key.keysym.sym == SDLK_r) skeleton_glc_init_ragdoll(&quba.skelglc);
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
	    if (event.key.keysym.sym == SDLK_a) move.strafe = 1;
	    if (event.key.keysym.sym == SDLK_d) move.strafe = -1;
	    if (event.key.keysym.sym == SDLK_w) move.walk = 1;
	    if (event.key.keysym.sym == SDLK_s) move.walk = -1;
	    if (event.key.keysym.sym == SDLK_g) skeleton_glc_move(&quba.skelglc, 1);
	    if (event.key.keysym.sym == SDLK_j) skeleton_glc_move(&quba.skelglc, 2);
	    if (event.key.keysym.sym == SDLK_y) skeleton_glc_move(&quba.skelglc, 3);
	    if (event.key.keysym.sym == SDLK_h) skeleton_glc_move(&quba.skelglc, 4);
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
	    v3_t newpos     = v3_add(move.lookpos, curr_speed);

	    if (move.freemove == 0)
	    {
		if (main_canmove(newpos) == 0)
		{
		    newpos = move.lookpos;
		}

		// detect floor

		v4_t x1tl;
		int  x1ind = octree_trace_line(&quba.statoctr, newpos, (v3_t){0.0, -1.0, 0.0}, &x1tl); // horizotnal dirs

		if (x1ind > 0)
		{
		    newpos.y = x1tl.y + 120.0;
		}
	    }

	    move.lookpos = newpos;
	}

	if (move.walkspeed > 0.0001 || move.walkspeed < -0.0001)
	{
	    v3_t curr_speed = v3_scale(move.direction, move.walkspeed);
	    v3_t newpos     = v3_add(move.lookpos, curr_speed);

	    if (move.freemove == 0)
	    {
		if (main_canmove(newpos) == 0)
		{
		    newpos = move.lookpos;
		}

		// detect floow

		v4_t x1tl;
		int  x1ind = octree_trace_line(&quba.statoctr, newpos, (v3_t){0.0, -1.0, 0.0}, &x1tl); // horizotnal dirs

		if (x1ind > 0)
		{
		    newpos.y = x1tl.y + 120.0;
		}
	    }

	    move.lookpos = newpos;
	}

	quba.lightangle += curr_step / 10.0;
	if (quba.lightangle > 6.28) quba.lightangle = 0.0;

#ifndef OCTTEST

	skeleton_glc_update(
	    &quba.skelglc,
	    &quba.statoctr,
	    &quba.statmod,
	    quba.lightangle,
	    quba.zombiecount,
	    quba.dynaoctr.levels,
	    quba.dynaoctr.basecube.w);

	// add modified point coords by compute shader

	octree_reset(
	    &quba.dynaoctr,
	    quba.dynaoctr.basecube);

	for (int index = 0; index < quba.zombiecount; index++)
	{
	    octree_insert_path(
		&quba.dynaoctr,
		0,
		index,
		&quba.skelglc.oct_out[index * 12]); // 48 bytes stride 12 int
	}

	if (quba.dustmod.point_count > 0)
	{
	    modelutil_update_dust(
		&quba.dustglc,
		&quba.dustmod,
		&quba.dynamod,
		quba.octrdpth,
		quba.octrsize,
		quba.frames,
		move.lookpos);

	    for (int index = 0; index < quba.dustmod.point_count; index++)
	    {
		octree_insert_point(
		    &quba.dynaoctr,
		    0,
		    quba.zombiecount + index,
		    (v3_t){
			quba.dustmod.vertexes[index * 3 + 0],
			quba.dustmod.vertexes[index * 3 + 1],
			quba.dustmod.vertexes[index * 3 + 2]},
		    NULL);
	    }
	}

	if (quba.partmod.point_count > 0)
	{
	    modelutil_update_particle(
		&quba.partglc,
		&quba.partmod,
		&quba.dynamod,
		quba.octrdpth,
		quba.octrsize,
		quba.frames,
		&quba.statoctr,
		&quba.statmod,
		&quba.octrglc);

	    for (int index = 0; index < quba.partmod.point_count; index++)
	    {
		octree_insert_point(
		    &quba.dynaoctr,
		    0,
		    quba.zombiecount + quba.dustmod.point_count + index,
		    (v3_t){
			quba.partmod.vertexes[index * 3 + 0],
			quba.partmod.vertexes[index * 3 + 1],
			quba.partmod.vertexes[index * 3 + 2]},
		    NULL);
	    }
	}

	// update dynamic octree

	octree_glc_upload_texbuffer_data(
	    &quba.octrglc,
	    quba.dynaoctr.octs,                     // buffer
	    GL_INT,                                 // data type
	    quba.dynaoctr.len * sizeof(GLint) * 12, // size
	    sizeof(GLint) * 4,                      // itemsize
	    0,                                      // start offset
	    quba.dynaoctr.len * sizeof(GLint) * 12, // end offset
	    OCTREE_GLC_BUFFER_DYNAMIC_OCTREE);      // buffer type

	// update dynamic normals
	// !!! in case of desktop opengl, leave modified normals on GPU and use glTexBuffer!!!

	octree_glc_upload_texbuffer_data(
	    &quba.octrglc,
	    quba.skelglc.nrm_out,                           // buffer
	    GL_FLOAT,                                       // data type
	    quba.dynamod.point_count * sizeof(GLfloat) * 3, // size
	    sizeof(GLfloat) * 3,                            // itemsize
	    0,                                              // start offset
	    quba.dynamod.point_count * sizeof(GLfloat) * 3, // end offset
	    OCTREE_GLC_BUFFER_DYNAMIC_NORMAL);              // buffer type

#endif

	// render scene

	v3_t lookpos = move.lookpos;
	if (move.freemove == 0) lookpos.y += sinf(quba.lightangle) * 5.0;

	octree_glc_update(
	    &quba.octrglc,
	    quba.winwth,
	    quba.winhth,
	    lookpos,
	    move.lookangle,
	    quba.lightangle,
	    quba.rndscale,
	    quba.octrdpth,
	    quba.octrsize,
	    quba.shoot);

	quba.shoot = 0;

	SDL_GL_SwapWindow(quba.window);

	quba.frames++;
    }

    return 1;
}

int main(int argc, char* argv[])
{
    quba.rndscale = 10; // render scaling
    quba.octrdpth = 12; // octree depth
    quba.doexit   = 0;
    quba.winscale = 1.0; // windows scaling
    quba.winwth   = 1200;
    quba.winhth   = 800;
    quba.octrsize = 1800.0; // default base octree edge size

    move.lookpos    = (v3_t){400.0, 150.0, 700.0};
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

    while ((option = getopt_long(argc, argv, "vhl:", long_options, &option_index)) != -1)
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
#ifdef EMSCRIPTEN
	    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
#else
	    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
#endif
	);

	if (quba.window != NULL)
	{
	    quba.context = SDL_GL_CreateContext(quba.window);

	    if (quba.context != NULL)
	    {
		GLint GlewInitResult = glewInit();
		if (GLEW_OK != GlewInitResult)
		    mt_log_error("%s", glewGetErrorString(GlewInitResult));

		SDL_GL_GetDrawableSize(
		    quba.window,
		    &quba.winwth,
		    &quba.winhth);

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
