// skeleton shader connector

#ifndef skeleconn_h
#define skeleconn_h

#include "mt_log.c"
#include "readfile.c"
#include <GL/glew.h>
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "ku_gl_shader.c"
#include "mt_memory.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct skeleconn_t
{
    GLuint cmp_sp;
    GLuint cmp_vbo_in;
    GLuint cmp_vbo_out;
    GLuint cmp_vao;

    GLint oril;
    GLint newl;
    GLint cubl;

    GLint* octqueue;

    size_t size;
} skeleconn_t;

skeleconn_t   skeleconn_init();
void          skeleconn_update(skeleconn_t* cc, float lighta, int model_count);
void          skeleconn_alloc_in(skeleconn_t* cc, void* data, size_t size);
void          skeleconn_alloc_out(skeleconn_t* cc, void* data, size_t size);

void skeleconn_alloc_in(skeleconn_t* cc, void* data, size_t size);
void skeleconn_alloc_out(skeleconn_t* cc, void* data, size_t size);

#endif

#if __INCLUDE_LEVEL__ == 0

skeleconn_t skeleconn_init()
{
    skeleconn_t cc;

    char* base_path = SDL_GetBasePath();
    char  cshpath[PATH_MAX];
    char  dshpath[PATH_MAX];

#ifdef EMSCRIPTEN
    snprintf(cshpath, PATH_MAX, "%s/src/qubatron/csh.c", base_path);
    snprintf(dshpath, PATH_MAX, "%s/src/qubatron/dsh.c", base_path);
#else
    snprintf(cshpath, PATH_MAX, "%scsh.c", base_path);
    snprintf(dshpath, PATH_MAX, "%sdsh.c", base_path);
#endif

    mt_log_debug("loading shaders %s %s", cshpath, dshpath);

    char* csh = readfile(cshpath);
    char* dsh = readfile(dshpath);

    GLuint cmp_vsh = ku_gl_shader_compile(GL_VERTEX_SHADER, csh);
    GLuint cmp_fsh = ku_gl_shader_compile(GL_FRAGMENT_SHADER, dsh);

    free(csh);
    free(dsh);

    // create compute shader ( it is just a vertex shader with transform feedback )
    cc.cmp_sp = glCreateProgram();

    glAttachShader(cc.cmp_sp, cmp_vsh);
    glAttachShader(cc.cmp_sp, cmp_fsh);

    // we want to capture outvalue in the result buffer
    const GLchar* feedbackVaryings[] = {"outOctet"};
    glTransformFeedbackVaryings(cc.cmp_sp, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

    // link
    ku_gl_shader_link(cc.cmp_sp);

    cc.oril = glGetUniformLocation(cc.cmp_sp, "fpori");
    cc.newl = glGetUniformLocation(cc.cmp_sp, "fpnew");
    cc.cubl = glGetUniformLocation(cc.cmp_sp, "basecube");

    // create vertex array object for vertex buffer
    glGenVertexArrays(1, &cc.cmp_vao);
    glBindVertexArray(cc.cmp_vao);

    // create vertex buffer object
    glGenBuffers(1, &cc.cmp_vbo_in);
    glBindBuffer(GL_ARRAY_BUFFER, cc.cmp_vbo_in);

    // create attribute for compute shader
    GLint inputAttrib = glGetAttribLocation(cc.cmp_sp, "inValue");
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribPointer(inputAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);

    // create and bind result buffer object
    glGenBuffers(1, &cc.cmp_vbo_out);
    glBindBuffer(GL_ARRAY_BUFFER, cc.cmp_vbo_out);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return cc;
}

void skeleconn_update(skeleconn_t* cc, float lighta, int model_count)
{

    // switch off fragment stage
    glEnable(GL_RASTERIZER_DISCARD);

    glUseProgram(cc->cmp_sp);

    glBindVertexArray(cc->cmp_vao);

    GLfloat pivot_old[48] =
	{
	    // head
	    54.0, 205.0, -90.0, 15.0,
	    54.0, 170.0, -90.0, 15.0,
	    // torso
	    54.0, 170.0, -90.0, 22.0,
	    52.0, 120.0, -90.0, 22.0,
	    // right arm
	    105.0, 170.0, -90.0, 3.0,
	    120.0, 70.0, -90.0, 3.0,
	    // left arm
	    5.0, 170.0, -90.0, 3.0,
	    -10.0, 70.0, -90.0, 3.0,
	    // right leg
	    70.0, 120.0, -70.0, 6.0,
	    100.0, -10.0, -70.0, 8.0,
	    // left leg
	    38.0, 120.0, -70.0, 6.0,
	    8.0, -10.0, -70.0, 8.0};

    glUniform4fv(cc->oril, 12, pivot_old);

    GLfloat pivot_new[36] =
	// head
	{
	    54.0 + 340.0, 205.0, -190.0 - 950.0 + sinf(lighta) * 10.0,
	    54.0 + 340.0, 170.0, -190.0 - 950.0,
	    // torso
	    54.0 + 340.0, 170.0, -190.0 - 950.0,
	    55.0 + 340.0, 120.0, -190.0 - 950.0 + sinf(lighta) * 2.0,
	    // right arm
	    105.0 + 340.0, 170.0, -190.0 - 950.0,
	    120.0 + 340.0 + sinf(lighta) * 15.0, 70.0, -190.0 - 950.0 + cosf(lighta) * 15.0,
	    // left arm
	    5.0 + 340.0, 170.0, -190.0 - 950.0,
	    -10.0 + 340.0 + sinf(lighta) * 15.0, 70.0, -190.0 - 950.0 + cosf(lighta) * 15.0,
	    // right leg
	    70.0 + 340.0, 120.0, -170.0 - 950.0,
	    90.0 + 340.0, -10.0, -170 - 950.0 + cosf(lighta) * 15.0,
	    // left leg
	    38.0 + 340.0, 120.0, -170.0 - 950.0,
	    8.0 + 340.0, -10.0, -170.0 - 950.0 + cosf(lighta) * 15.0};

    glUniform3fv(cc->newl, 12, pivot_new);

    GLfloat basecubearr[4] = {0.0, 1800.0, 0.0, 1800.0};
    glUniform4fv(cc->cubl, 1, basecubearr);

    /* GLfloat cmp_data[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}; */
    /* glBindBuffer(GL_ARRAY_BUFFER, cc->cmp_vbo_in); */
    /* glBufferData(GL_ARRAY_BUFFER, model_count * sizeof(GLfloat), model_vertexes, GL_STATIC_DRAW); */
    /* glBindBuffer(GL_ARRAY_BUFFER, 0); */

    glBindBuffer(GL_ARRAY_BUFFER, cc->cmp_vbo_out);
    /* glBufferData(GL_ARRAY_BUFFER, model_count * sizeof(GLfloat), NULL, GL_STATIC_READ); */

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->cmp_vbo_out);

    // run compute shader
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, model_count);
    glEndTransformFeedback();
    glFlush();

    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->size, cc->octqueue);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisable(GL_RASTERIZER_DISCARD);
}

void skeleconn_alloc_in(skeleconn_t* cc, void* data, size_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, cc->cmp_vbo_in);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void skeleconn_alloc_out(skeleconn_t* cc, void* data, size_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, cc->cmp_vbo_out);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_READ);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->cmp_vbo_out);

    /* cc->octqueue = glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, size, GL_MAP_READ_BIT); */
    cc->octqueue = mt_memory_alloc(size, NULL, NULL);
    cc->size     = size;
}

#endif
