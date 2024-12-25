#include <GL/glew.h>
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "computeconn.c"
#include "ku_gl_shader.c"
#include "model.c"
#include "mt_log.c"
#include "mt_time.c"
#include "mt_vector_2d.c"
#include "octree.c"
#include "readfile.c"
#include "renderconn.c"

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

int strafe  = 0;
int forward = 0;

float anglex      = 0.0;
float speed       = 0.0;
float strafespeed = 0.0;

v3_t angle       = {0.0};
v3_t position    = {440.0, 200.0, -1000.0};
v3_t direction   = {0.0, 0.0, -1.0};
v3_t directionX  = {-1.0, 0.0, 0.0};

float lighta = 0.0;

uint32_t cnt = 0;
uint32_t ind = 0;

v3_t  offset  = {0.0, 0.0, 0.0};
float scaling = 1.0;

computeconn_t cc;
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

    char* base_path = SDL_GetBasePath();

    // opengl init

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    cc = computeconn_init();
    rc = renderconn_init();

    char plypath[PATH_MAX];

    snprintf(plypath, PATH_MAX, "%s../abandoned1.ply", base_path);

    model_t static_model = model_init(plypath, (v3_t){0.0, 0.0, 0.0});

    renderconn_alloc_normals(&rc, static_model.normals, static_model.point_count * 4 * sizeof(GLfloat), false);
    renderconn_alloc_colors(&rc, static_model.colors, static_model.point_count * 4 * sizeof(GLfloat), false);

    static_octree = octree_create(10000, (v4_t){0.0, 1800.0, 0.0, 1800.0});
    for (int index = 0; index < static_model.point_count * 4; index += 4)
    {
	bool leaf;
	octree_insert_fast(
	    &static_octree,
	    0,
	    index / 4,
	    (v3_t){static_model.vertexes[index], static_model.vertexes[index + 1], -1620 + static_model.vertexes[index + 2]},
	    &leaf);
    }

    mt_log_debug("point count %lu leaf count %lu compacted %f", static_model.point_count, static_octree.leaves, (float) static_octree.leaves / (float) static_model.point_count);
    mt_log_debug("buffer size is %lu bytes", static_octree.size * sizeof(octets_t));

    renderconn_alloc_octree(&rc, static_octree.octs, static_octree.len * sizeof(octets_t), false);

    snprintf(plypath, PATH_MAX, "%s../zombie.ply", base_path);

    dynamic_model = model_init(plypath, (v3_t){-270.0, -600.0, -10.0});

    renderconn_alloc_normals(&rc, dynamic_model.normals, dynamic_model.point_count * 4 * sizeof(GLfloat), true);
    renderconn_alloc_colors(&rc, dynamic_model.colors, dynamic_model.point_count * 4 * sizeof(GLfloat), true);

    dynamic_octree = octree_create(10000, (v4_t){0.0, 1800.0, 0.0, 1800.0});
    for (int index = 0; index < dynamic_model.point_count * 4; index += 4)
    {
	bool leaf;
	octree_insert_fast(
	    &dynamic_octree,
	    0,
	    index / 4,
	    (v3_t){dynamic_model.vertexes[index], dynamic_model.vertexes[index + 1], -1620 + dynamic_model.vertexes[index + 2]},
	    &leaf);
    }

    mt_log_debug("point count %lu leaf count %lu compacted %f", dynamic_model.point_count, dynamic_octree.leaves, (float) dynamic_octree.leaves / (float) dynamic_model.point_count);
    mt_log_debug("buffer size is %lu bytes", dynamic_octree.size * sizeof(octets_t));

    renderconn_alloc_octree(&rc, dynamic_octree.octs, dynamic_octree.len * sizeof(octets_t), true);

    computeconn_alloc_in(&cc, dynamic_model.vertexes, dynamic_model.point_count * 4 * sizeof(GLfloat));
    computeconn_alloc_out(&cc, NULL, dynamic_model.point_count * sizeof(GLint) * 12);
    computeconn_update(&cc, lighta, dynamic_model.point_count);

    // add modified point coords by compute shader

    octree_reset(&dynamic_octree, (v4_t){0.0, 1800.0, 0.0, 1800.0});

    for (int index = 0; index < dynamic_model.point_count * 4; index += 4)
    {
	bool leaf = false;
	octree_insert_fast_octs(
	    &dynamic_octree,
	    0,
	    index / 4,
	    &cc.octqueue[index * 3], // 48 bytes stride 12 int
	    &leaf);
    }

    renderconn_alloc_octree(&rc, dynamic_octree.octs, dynamic_octree.len * sizeof(octets_t), true);

    /* for (int i = 0; i < 100; i++) */
    /* { */
    /* 	printf( */
    /* 	    "%i orind %i nodes : %i | %i | %i | %i | %i | %i | %i | %i\n", */
    /* 	    i, */
    /* 	    cubearr.octs[i].ind, */
    /* 	    cubearr.octs[i].oct[0], */
    /* 	    cubearr.octs[i].oct[1], */
    /* 	    cubearr.octs[i].oct[2], */
    /* 	    cubearr.octs[i].oct[3], */
    /* 	    cubearr.octs[i].oct[4], */
    /* 	    cubearr.octs[i].oct[5], */
    /* 	    cubearr.octs[i].oct[6], */
    /* 	    cubearr.octs[i].oct[7]); */
    /* } */

    /* glBindBuffer(GL_SHADER_STORAGE_BUFFER, cub_ssbo); */
    /* glBufferData(GL_SHADER_STORAGE_BUFFER, cubearr.len * sizeof(octets_t), cubearr.octs, GL_DYNAMIC_COPY); // sizeof(data) only works for statically sized C/C++ arrays. */
    /* glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);                                                              // unbind */

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

    /* for (int i = 0; i < 100; i++) */
    /* { */
    /* 	GLfloat x = (float) (rand() % 800); */
    /* 	GLfloat y = (float) (rand() % 600); */
    /* 	GLfloat z = -200.0 + (float) (rand() % 500); */
    /* 	GLfloat s = (float) (10 + rand() % 90); */
    /* 	/\* glocts[0] = (struct octets_t){{300.0, 300.0, 0.0}, {600.0, 600.0, -300.0}, 300.0, 8}; *\/ */
    /* 	/\* glocts[1] = (struct octets_t){{700.0, 300.0, 0.0}, {600.0, 600.0, -300.0}, 300.0, 8}; *\/ */
    /* 	glocts[i] = (struct octets_t){{x, y, -z, s}}; */
    /* } */

    /* glocts[0] = (struct octets_t){{100.0, 100.0, -200.0, 100.0}}; */
    /* glBindBuffer(GL_SHADER_STORAGE_BUFFER, cub_ssbo); */
    /* glBufferData(GL_SHADER_STORAGE_BUFFER, buffsize, glocts, GL_DYNAMIC_COPY); // sizeof(data) only works for statically sized C/C++ arrays. */
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

    computeconn_update(&cc, lighta, dynamic_model.point_count);

    // add modified point coords by compute shader

    octree_reset(&dynamic_octree, (v4_t){0.0, 1800.0, 0.0, 1800.0});

    for (int index = 0; index < dynamic_model.point_count * 4; index += 4)
    {
	bool leaf = false;
	octree_insert_fast_octs(
	    &dynamic_octree,
	    0,
	    index / 4,
	    &cc.octqueue[index * 3], // 48 bytes stride 12 int
	    &leaf);
    }

    renderconn_alloc_octree(&rc, dynamic_octree.octs, dynamic_octree.len * sizeof(octets_t), true);

    renderconn_update(&rc, width, height, position, angle, lighta);

    SDL_GL_SwapWindow(window);

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
