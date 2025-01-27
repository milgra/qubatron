#ifndef renderconn_h
#define renderconn_h

#include "mt_log.c"
#include "mt_math_3d.c"
#include "mt_matrix_4d.c"
#include "readfile.c"
#include <GL/glew.h>
#include <SDL.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "shader.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct renderconn_t
{
    glsha_t sha;
    glsha_t sha_texquad;

    GLuint frg_vbo_in;
    GLuint frg_vao;

    GLuint vao_texquad;
    GLuint vbo_texquad;
    GLuint texture;
    GLuint framebuffer;

    GLuint col1_tex;
    GLuint col2_tex;
    GLuint nrm1_tex;
    GLuint nrm2_tex;
    GLuint oct1_tex;
    GLuint oct2_tex;
} renderconn_t;

renderconn_t renderconn_init();

void         renderconn_update(renderconn_t* rc, float width, float height, v3_t position, v3_t angle, float lighta, uint8_t quality, int maxlevel);
void         renderconn_upload_normals(renderconn_t* cc, void* data, int width, int height, int components, bool dynamic);
void         renderconn_upload_colors(renderconn_t* cc, void* data, int width, int height, int components, bool dynamic);
void         renderconn_upload_octree_quadruplets(renderconn_t* cc, void* data, int width, int height, bool dynamic);
void         renderconn_upload_octree_quadruplets_partial(renderconn_t* rc, void* data, int x, int y, int width, int height, bool dynamic);

#endif

#if __INCLUDE_LEVEL__ == 0

v3_t lightc = {420.0, 200.0, 680.0};

renderconn_t renderconn_init()
{
    renderconn_t rc;
    // shader

    char* base_path = SDL_GetBasePath();
    char  vshpath[PATH_MAX];
    char  fshpath[PATH_MAX];

#ifdef EMSCRIPTEN
    snprintf(vshpath, PATH_MAX, "%s/src/qubatron/shaders/vsh_render.c", base_path);
    snprintf(fshpath, PATH_MAX, "%s/src/qubatron/shaders/fsh_render.c", base_path);
#else
    snprintf(vshpath, PATH_MAX, "%svsh_render.c", base_path);
    snprintf(fshpath, PATH_MAX, "%sfsh_render.c", base_path);
#endif

    mt_log_debug("loading shaders \n%s\n%s", vshpath, fshpath);

    char* vsh = readfile(vshpath);
    char* fsh = readfile(fshpath);

    rc.sha = shader_create(
	vsh,
	fsh,
	1,
	((const char*[]){"position"}),
	13,
	((const char*[]){"projection", "camfp", "angle_in", "light", "basecube", "dimensions", "coltexbuf_s", "coltexbuf_d", "nrmtexbuf_s", "nrmtexbuf_d", "octtexbuf_s", "octtexbuf_d", "maxlevel"}));

    free(vsh);
    free(fsh);

#ifdef EMSCRIPTEN
    snprintf(vshpath, PATH_MAX, "%s/src/qubatron/shaders/vsh_texquad.c", base_path);
    snprintf(fshpath, PATH_MAX, "%s/src/qubatron/shaders/fsh_texquad.c", base_path);
#else
    snprintf(vshpath, PATH_MAX, "%svsh_texquad.c", base_path);
    snprintf(fshpath, PATH_MAX, "%sfsh_texquad.c", base_path);
#endif
    char* vsh_texquad = readfile(vshpath);
    char* fsh_texquad = readfile(fshpath);

    rc.sha_texquad = shader_create(
	vsh_texquad,
	fsh_texquad,
	2,
	((const char*[]){"position", "texcoord"}),
	2,
	((const char*[]){"projection", "texture_base"}));

    free(vsh_texquad);
    free(fsh_texquad);

    // scene sha

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

    // texquad sha

    // create vertex array object for vertex buffer
    glGenVertexArrays(1, &rc.vao_texquad);
    glBindVertexArray(rc.vao_texquad);

    // create vertex buffer object
    glGenBuffers(1, &rc.vbo_texquad);
    glBindBuffer(GL_ARRAY_BUFFER, rc.vbo_texquad);

    // create attribute for compute shader
    inputAttrib = glGetAttribLocation(rc.sha_texquad.name, "position");
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribPointer(inputAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);

    inputAttrib = glGetAttribLocation(rc.sha_texquad.name, "texcoord");
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribPointer(inputAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (const GLvoid*) 12);

    glGenTextures(1, &rc.col1_tex);
    glBindTexture(GL_TEXTURE_2D, rc.col1_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenTextures(1, &rc.col2_tex);
    glBindTexture(GL_TEXTURE_2D, rc.col2_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenTextures(1, &rc.nrm1_tex);
    glBindTexture(GL_TEXTURE_2D, rc.nrm1_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenTextures(1, &rc.nrm2_tex);
    glBindTexture(GL_TEXTURE_2D, rc.nrm2_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenTextures(1, &rc.oct1_tex);
    glBindTexture(GL_TEXTURE_2D, rc.oct1_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenTextures(1, &rc.oct2_tex);
    glBindTexture(GL_TEXTURE_2D, rc.oct2_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    /* glGenBuffers(1, &vbo); */
    /* glbindbuffer(GL_ARRAY_BUFFER, vbo); */

    /* GLfloat posarr[3] = {position.x, position.y, position.z}; */
    /* glUniform3fv(sha.uni_loc[1], 1, posarr); */

    /* GLfloat lightarr[3] = {0.0, 2000.0, -500.0}; */
    /* glUniform3fv(sha.uni_loc[3], 1, lightarr); */

    // render to texture

    glGenFramebuffers(1, &rc.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rc.framebuffer);

    glGenTextures(1, &rc.texture);
    glBindTexture(GL_TEXTURE_2D, rc.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    /* glBindTexture(GL_TEXTURE_2D, 0); */

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rc.texture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) printf("Failed to create framebuffer at ogl_framebuffer_with_texture ");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    return rc;
}

void renderconn_update(renderconn_t* rc, float width, float height, v3_t position, v3_t angle, float lighta, uint8_t quality, int maxlevel)
{

    // first render scene to texture

    glUseProgram(rc->sha.name);

    glBindFramebuffer(
	GL_FRAMEBUFFER,
	rc->framebuffer);

    glUniform1i(rc->sha.uni_loc[12], maxlevel);

    matrix4array_t projection = {0};

    float ow = width / (6.0 - (float) quality / 2.0);
    float oh = height / (6.0 - (float) quality / 2.0);

    m4_t pers         = m4_defaultortho(0.0, ow, 0.0, oh, -10, 10);
    projection.matrix = pers;
    glUniformMatrix4fv(rc->sha.uni_loc[0], 1, 0, projection.array);

    GLfloat lightarr[3] = {lightc.x, lightc.y, lightc.z - sinf(lighta) * 200.0};
    /* printf("%f %f %f\n", lightarr[0], lightarr[1], lightarr[2]); */
    glUniform3fv(rc->sha.uni_loc[3], 1, lightarr);

    GLfloat anglearr[3] = {angle.x, angle.y, 0.0};

    glUniform3fv(rc->sha.uni_loc[2], 1, anglearr);

    GLfloat posarr[3] = {position.x, position.y, position.z};

    glUniform3fv(rc->sha.uni_loc[1], 1, posarr);

    GLfloat basecubearr[4] = {0.0, 1800.0, 1800.0, 1800.0};

    glUniform4fv(rc->sha.uni_loc[4], 1, basecubearr);

    GLfloat dimensions[2] = {ow, oh};

    glUniform2fv(rc->sha.uni_loc[5], 1, dimensions);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat vertexes[] = {
	0.0f, 0.0f, 0.0f,
	(float) ow, 0.0f, 0.0f,
	0.0f, (float) oh, 0.0f,
	0.0f, (float) oh, 0.0f,
	(float) ow, 0.0f, 0.0f,
	(float) ow, (float) oh, 0.0};

    glViewport(
	0.0,
	0.0,
	ow,
	oh);

    glBindVertexArray(rc->frg_vao);

    glBindBuffer(
	GL_ARRAY_BUFFER,
	rc->frg_vbo_in);

    glBufferData(
	GL_ARRAY_BUFFER,
	sizeof(GLfloat) * 6 * 3,
	vertexes,
	GL_DYNAMIC_DRAW);

    glClearColor(
	0.0,
	0.05,
	0.0,
	1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawArrays(
	GL_TRIANGLES,
	0,
	6);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // render unit quad with texture

    glBindFramebuffer(
	GL_FRAMEBUFFER,
	0);

    glClearColor(
	0.0,
	0.0,
	0.0,
	1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(rc->sha_texquad.name);

    glViewport(
	0.0,
	0.0,
	width,
	height);

    pers              = m4_defaultortho(0.0, ow, 0.0, oh, -10, 10);
    projection.matrix = pers;

    glUniformMatrix4fv(rc->sha_texquad.uni_loc[0], 1, 0, projection.array);

    glBindVertexArray(rc->vao_texquad);

    glActiveTexture(GL_TEXTURE0 + 0);

    glUniform1i(rc->sha_texquad.uni_loc[1], 0);

    glBindTexture(GL_TEXTURE_2D, rc->texture);

    GLfloat vertexes_uni[] = {
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	(float) 2048, 0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, (float) 2048, 0.0f, 0.0f, 1.0f,
	0.0f, (float) 2048, 0.0f, 0.0f, 1.0f,
	(float) 2048, 0.0f, 0.0f, 1.0f, 0.0f,
	(float) 2048, (float) 2048, 0.0f, 1.0f, 1.0f};

    glBindBuffer(
	GL_ARRAY_BUFFER,
	rc->vbo_texquad);

    glBufferData(
	GL_ARRAY_BUFFER,
	sizeof(GLfloat) * 6 * 5,
	vertexes_uni,
	GL_DYNAMIC_DRAW);

    glDrawArrays(
	GL_TRIANGLES,
	0,
	6);

    glEnable(GL_SCISSOR_TEST);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glScissor(width / 2 - 1, height / 2 - 1, 2, 2);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderconn_upload_normals(renderconn_t* rc, void* data, int width, int height, int components, bool dynamic)
{
    glUseProgram(rc->sha.name);

    if (dynamic)
    {
	glActiveTexture(GL_TEXTURE0 + 10);
	glBindTexture(GL_TEXTURE_2D, rc->nrm2_tex);
	glUniform1i(rc->sha.uni_loc[9], 10);
    }
    else
    {
	glActiveTexture(GL_TEXTURE0 + 9);
	glBindTexture(GL_TEXTURE_2D, rc->nrm1_tex);
	glUniform1i(rc->sha.uni_loc[8], 9);
    }

    if (components == 3)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);

    mt_log_debug("uploaded normals dyn %i width %i height %i components %i", dynamic, width, height, components);
    /* GLfloat* arr = data; */
    /* for (int i = 0; i < 12; i += 3) */
    /* 	mt_log_debug("%f %f %f", arr[i], arr[i + 1], arr[i + 2]); */
}

void renderconn_upload_colors(renderconn_t* rc, void* data, int width, int height, int components, bool dynamic)
{
    glUseProgram(rc->sha.name);

    if (dynamic)
    {
	glActiveTexture(GL_TEXTURE0 + 8);
	glBindTexture(GL_TEXTURE_2D, rc->col2_tex);
	glUniform1i(rc->sha.uni_loc[7], 8);
    }
    else
    {
	glActiveTexture(GL_TEXTURE0 + 7);
	glBindTexture(GL_TEXTURE_2D, rc->col1_tex);
	glUniform1i(rc->sha.uni_loc[6], 7);
    }

    if (components == 3)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);

    mt_log_debug("uploaded colors dyn %i width %i height %i components %i", dynamic, width, height, components);
    /* GLfloat* arr = data; */
    /* for (int i = 0; i < 12; i += 3) */
    /* 	mt_log_debug("%f %f %f", arr[i], arr[i + 1], arr[i + 2]); */
}

void renderconn_upload_octree_quadruplets(renderconn_t* rc, void* data, int width, int height, bool dynamic)
{
    glUseProgram(rc->sha.name);

    if (dynamic)
    {
	glActiveTexture(GL_TEXTURE0 + 12);
	glBindTexture(GL_TEXTURE_2D, rc->oct2_tex);
	glUniform1i(rc->sha.uni_loc[11], 12);
    }
    else
    {
	glActiveTexture(GL_TEXTURE0 + 11);
	glBindTexture(GL_TEXTURE_2D, rc->oct1_tex);
	glUniform1i(rc->sha.uni_loc[10], 11);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32I, width, height, 0, GL_RGBA_INTEGER, GL_INT, data);

    /* mt_log_debug("uploaded octree dyn %i width %i height %i", dynamic, width, height); */
    /* GLint* arr = data; */
    /* for (int i = 0; i < 48; i += 12) */
    /* { */
    /* 	printf( */
    /* 	    "nodes : %i | %i | %i | %i | %i | %i | %i | %i index %i\n", */
    /* 	    arr[i + 0], */
    /* 	    arr[i + 1], */
    /* 	    arr[i + 2], */
    /* 	    arr[i + 3], */
    /* 	    arr[i + 4], */
    /* 	    arr[i + 5], */
    /* 	    arr[i + 6], */
    /* 	    arr[i + 7], */
    /* 	    arr[i + 8]); */
    /* } */
}

void renderconn_upload_octree_quadruplets_partial(renderconn_t* rc, void* data, int x, int y, int width, int height, bool dynamic)
{
    glUseProgram(rc->sha.name);

    if (dynamic)
    {
	glActiveTexture(GL_TEXTURE0 + 12);
	glBindTexture(GL_TEXTURE_2D, rc->oct2_tex);
	glUniform1i(rc->sha.uni_loc[11], 12);
    }
    else
    {
	glActiveTexture(GL_TEXTURE0 + 11);
	glBindTexture(GL_TEXTURE_2D, rc->oct1_tex);
	glUniform1i(rc->sha.uni_loc[10], 11);
    }

    glTexSubImage2D(
	GL_TEXTURE_2D,
	0,
	x,
	y,
	width,
	height,
	GL_RGBA_INTEGER,
	GL_INT,
	data);
}

#endif
