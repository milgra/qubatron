// compute shader connector

#ifndef computeconn_h
#define computeconn_h

#include "readfile.c"
#include <GL/glew.h>
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "ku_gl_shader.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct computeconn_t
{
    GLuint cmp_sp;
    GLuint cmp_vbo_in;
    GLuint cmp_vbo_out;
    GLuint cmp_vao;

    GLint oril;
    GLint newl;
    GLint cubl;

    GLint* octqueue;

} computeconn_t;

computeconn_t computeconn_init();
void          computeconn_update(computeconn_t* cc, float lighta, int model_count);
void          computeconn_alloc_in(computeconn_t* cc, void* data, size_t size);
void          computeconn_alloc_out(computeconn_t* cc, void* data, size_t size);

void computeconn_alloc_in(computeconn_t* cc, void* data, size_t size);
void computeconn_alloc_out(computeconn_t* cc, void* data, size_t size);

#endif

#if __INCLUDE_LEVEL__ == 0

computeconn_t computeconn_init()
{
    computeconn_t cc;

    char* base_path = SDL_GetBasePath();
    char  cshpath[PATH_MAX];
    char  dshpath[PATH_MAX];

    snprintf(cshpath, PATH_MAX, "%scsh.c", base_path);
    snprintf(dshpath, PATH_MAX, "%sdsh.c", base_path);

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

void computeconn_update(computeconn_t* cc, float lighta, int model_count)
{

    // switch off fragment stage
    glEnable(GL_RASTERIZER_DISCARD);

    glUseProgram(cc->cmp_sp);

    glBindVertexArray(cc->cmp_vao);

    GLfloat pivot_old[80] =
	// head
	{
	    54.0, 205.0, -90.0, 20.0,
	    54.0, 180.0, -90.0, 22.0,
	    // torso
	    52.0, 110.0, -90.0, 0.0,
	    // right leg
	    60.0, 100.0, -85.0, 20.0,
	    60.0, 70.0, -85.0, 20.0,
	    68.0, 12.0, -75.0, 20.0,
	    75.0, 0.0, -75.0, 20.0,
	    77.0, -20.0, -75.0, 0.0, // end sequence with 0.0 radius
	    // left legs
	    44.0, 70.0, -85.0, 20.0,
	    36.0, 12.0, -75.0, 20.0,
	    29.0, 0.0, -75.0, 20.0,
	    27.0, -20.0, -75.0, 0.0,
	    // right arm
	    60.0, 180.0, -90.0, 23.0,
	    70.0, 130.0, -90.0, 25.0,
	    90.0, 70.0, -90.0, 25.0,
	    90.0, 50.0, -90.0, 0.0}; // end sequence

    glUniform4fv(cc->oril, 20, pivot_old);

    GLfloat pivot_new[60] =
	// head
	{
	    54.0, 205.0, -200.0 + sinf(lighta) * 5.0,
	    54.0, 180.0, -190.0 + sinf(lighta) * 2.0,
	    // torso
	    55.0, 110.0, -190.0 + sinf(lighta) * 1.0,
	    // left leg
	    60.0, 100.0, -190.0 + sinf(lighta) * 5.0,
	    60.0, 70.0, -190.0 + sinf(lighta) * 5.0,
	    65.0, 12.0, -190.0,
	    70.0, 0.0, -190.0,
	    70.0, -20.0, -190.0,
	    // right leg
	    44.0, 70.0, -190.0,
	    36.0, 12.0, -190.0,
	    29.0, 0.0, -190.0,
	    27.0, -20.0, -190.0,
	    // rigth arm
	    60.0, 180.0, -185.0,
	    70.0, 130.0, -185.0,
	    90.0, 70.0, -180.0,
	    90.0, 50.0, -180.0}; // end sequence

    glUniform3fv(cc->newl, 20, pivot_new);

    /* GLfloat pivot_old[16] = */
    /* 	{54.0, 205.0, -90.0, 20.0, */
    /* 	 54.0, 174.0, -90.0, 25.0, */
    /* 	 54.0, 110.0, -90.0, 25.0, */
    /* 	 54.0, 80.0, -90.0, 25.0}; */

    /* glUniform4fv(cc->oril, 4, pivot_old); */

    /* GLfloat pivot_new[12] = */
    /* 	{54.0, 205.0, -200.0 + sinf(lighta) * 10.0, */
    /* 	 54.0, 174.0, -190.0, */
    /* 	 54.0 + sinf(lighta) * 4.0, 110.0, -190.0, */
    /* 	 54.0, 80.0, -190.0}; */

    /* glUniform3fv(cc->newl, 3, pivot_new); */

    GLfloat basecubearr[4] = {0.0, 1800.0, 0.0, 1800.0};
    glUniform4fv(cc->cubl, 1, basecubearr);

    /* GLfloat cmp_data[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}; */
    glBindBuffer(GL_ARRAY_BUFFER, cc->cmp_vbo_in);
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

    glDisable(GL_RASTERIZER_DISCARD);
}

void computeconn_alloc_in(computeconn_t* cc, void* data, size_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, cc->cmp_vbo_in);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void computeconn_alloc_out(computeconn_t* cc, void* data, size_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, cc->cmp_vbo_out);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_READ);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->cmp_vbo_out);
    cc->octqueue = glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, size, GL_MAP_READ_BIT);
}

#endif
