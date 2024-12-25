#ifndef renderconn_h
#define renderconn_h

#include "mt_math_3d.c"
#include "mt_matrix_4d.c"
#include "readfile.c"
#include <GL/glew.h>
#include <SDL.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ku_gl_shader.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct renderconn_t
{
    glsha_t sha;

    GLuint oct_ssbo_d;
    GLuint nrm_ssbo_d;
    GLuint col_ssbo_d;

    GLuint oct_ssbo_s;
    GLuint nrm_ssbo_s;
    GLuint col_ssbo_s;

    GLuint frg_vbo_in;
    GLuint frg_vao;
} renderconn_t;

renderconn_t renderconn_init();

void         renderconn_update(renderconn_t* rc, float width, float height, v3_t position, v3_t angle, float lighta);
void         renderconn_alloc_normals(renderconn_t* cc, void* data, size_t size, bool dynamic);
void         renderconn_alloc_colors(renderconn_t* cc, void* data, size_t size, bool dynamic);
void         renderconn_alloc_octree(renderconn_t* cc, void* data, size_t size, bool dynamic);

#endif

#if __INCLUDE_LEVEL__ == 0

v3_t lightc = {420.0, 210.0, -1100.0};

renderconn_t renderconn_init()
{
    renderconn_t rc;
    // shader

    char* base_path = SDL_GetBasePath();
    char  vshpath[PATH_MAX];
    char  fshpath[PATH_MAX];
    snprintf(vshpath, PATH_MAX, "%svsh.c", base_path);
    snprintf(fshpath, PATH_MAX, "%sfsh.c", base_path);

    char* vsh = readfile(vshpath);
    char* fsh = readfile(fshpath);

    rc.sha = ku_gl_shader_create(
	vsh,
	fsh,
	1,
	((const char*[]){"position"}),
	5,
	((const char*[]){"projection", "camfp", "angle_in", "light", "basecube"}));
    free(vsh);
    free(fsh);

    // vbo

    // create vertex array object for vertex buffer
    glGenVertexArrays(1, &rc.frg_vao);
    glBindVertexArray(rc.frg_vao);

    // create vertex buffer object
    glGenBuffers(1, &rc.frg_vbo_in);
    glBindBuffer(GL_ARRAY_BUFFER, rc.frg_vbo_in);

    // create attribute for compute shader
    GLint inputAttrib = glGetAttribLocation(rc.sha.name, "position");
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribPointer(inputAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    /* glGenBuffers(1, &vbo); */
    /* glbindbuffer(GL_ARRAY_BUFFER, vbo); */

    /* GLfloat posarr[3] = {position.x, position.y, position.z}; */
    /* glUniform3fv(sha.uni_loc[1], 1, posarr); */

    /* GLfloat lightarr[3] = {0.0, 2000.0, -500.0}; */
    /* glUniform3fv(sha.uni_loc[3], 1, lightarr); */

    glGenBuffers(1, &rc.oct_ssbo_s);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rc.oct_ssbo_s);

    glGenBuffers(1, &rc.nrm_ssbo_s);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rc.nrm_ssbo_s);

    glGenBuffers(1, &rc.col_ssbo_s);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, rc.col_ssbo_s);

    glGenBuffers(1, &rc.oct_ssbo_d);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, rc.oct_ssbo_d);

    glGenBuffers(1, &rc.nrm_ssbo_d);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rc.nrm_ssbo_d);

    glGenBuffers(1, &rc.col_ssbo_d);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, rc.col_ssbo_d);

    return rc;
}

void renderconn_update(renderconn_t* rc, float width, float height, v3_t position, v3_t angle, float lighta)
{
    glUseProgram(rc->sha.name);

    matrix4array_t projection = {0};

    m4_t pers         = m4_defaultortho(0.0, width, 0.0, height, -10, 10);
    projection.matrix = pers;
    glUniformMatrix4fv(rc->sha.uni_loc[0], 1, 0, projection.array);

    GLfloat lightarr[3] = {lightc.x, lightc.y, lightc.z - sinf(lighta) * 200.0};
    /* printf("%f %f %f\n", lightarr[0], lightarr[1], lightarr[2]); */
    glUniform3fv(rc->sha.uni_loc[3], 1, lightarr);

    GLfloat anglearr[3] = {angle.x, angle.y, 0.0};

    glUniform3fv(rc->sha.uni_loc[2], 1, anglearr);

    GLfloat posarr[3] = {position.x, position.y, position.z};

    glUniform3fv(rc->sha.uni_loc[1], 1, posarr);

    GLfloat basecubearr[4] = {0.0, 1800.0, 0.0, 1800.0};

    glUniform4fv(rc->sha.uni_loc[4], 1, basecubearr);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat vertexes[] = {
	0.0f, 0.0f, 0.0f, (float) width, 0.0f, 0.0f, 0.0f, (float) height, 0.0f, 0.0f, (float) height, 0.0f, (float) width, 0.0f, 0.0f, (float) width, (float) height, 0.0f};

    glViewport(
	0.0,
	0.0,
	width,
	height);

    glBindVertexArray(rc->frg_vao);

    glBindBuffer(
	GL_ARRAY_BUFFER,
	rc->frg_vbo_in);

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
}

void renderconn_alloc_normals(renderconn_t* rc, void* data, size_t size, bool dynamic)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dynamic ? rc->nrm_ssbo_d : rc->nrm_ssbo_s);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY); // sizeof(data) only works for statically sized C/C++ arrays.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);                           // unbind
}

void renderconn_alloc_colors(renderconn_t* rc, void* data, size_t size, bool dynamic)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dynamic ? rc->col_ssbo_d : rc->col_ssbo_s);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY); // sizeof(data) only works for statically sized C/C++ arrays.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);                           // unbind
}

void renderconn_alloc_octree(renderconn_t* rc, void* data, size_t size, bool dynamic)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dynamic ? rc->oct_ssbo_d : rc->oct_ssbo_s);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY); // sizeof(data) only works for statically sized C/C++ arrays.
    /* glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, cubearr.len * sizeof(octets_t), cubearr.octs); // sizeof(data) only works for statically sized C/C++ arrays. */
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
}

#endif
