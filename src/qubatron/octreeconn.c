#ifndef octreeconn_h
#define octreeconn_h

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

typedef struct octreeconn_t
{
    // shaders

    glsha_t octr_sha;
    glsha_t r2tx_sha;

    // vbo and vao for octree shader

    GLuint octr_vbo;
    GLuint octr_vao;

    // render to texture vao, vbo, fbo and texture

    GLuint r2tex_vao;
    GLuint r2tex_vbo;
    GLuint r2tex_tex;
    GLuint r2tex_fbo;

    // texture buffers for color, normal and octree

    GLuint col1_tex;
    GLuint col2_tex;
    GLuint nrm1_tex;
    GLuint nrm2_tex;
    GLuint oct1_tex;
    GLuint oct2_tex;

} octreeconn_t;

octreeconn_t octreeconn_init();
void         octreeconn_update(octreeconn_t* rc, float width, float height, v3_t position, v3_t angle, float lighta, uint8_t quality, int maxlevel);
void         octreeconn_upload_texbuffer(octreeconn_t* rc, void* data, int x, int y, int width, int height, int internalformat, int format, int type, int texture, int uniform);

#endif

#if __INCLUDE_LEVEL__ == 0

v3_t lightc = {420.0, 200.0, 680.0};

octreeconn_t octreeconn_init()
{
    octreeconn_t rc;

    char* base_path = SDL_GetBasePath();
    char  vshpath[PATH_MAX];
    char  fshpath[PATH_MAX];

#ifdef EMSCRIPTEN
    snprintf(vshpath, PATH_MAX, "%s/src/qubatron/shaders/vsh_octree.c", base_path);
    snprintf(fshpath, PATH_MAX, "%s/src/qubatron/shaders/fsh_octree.c", base_path);
#else
    snprintf(vshpath, PATH_MAX, "%svsh_octree.c", base_path);
    snprintf(fshpath, PATH_MAX, "%sfsh_octree.c", base_path);
#endif

    mt_log_debug("loading shaders \n%s\n%s", vshpath, fshpath);

    char* vsh = readfile(vshpath);
    char* fsh = readfile(fshpath);

    rc.octr_sha = shader_create(
	vsh,
	fsh,
	1,
	((const char*[]){
	    "position"}),
	13,
	((const char*[]){
	    "projection",
	    "camfp",
	    "angle_in",
	    "light",
	    "basecube",
	    "dimensions",
	    "coltexbuf_s",
	    "coltexbuf_d",
	    "nrmtexbuf_s",
	    "nrmtexbuf_d",
	    "octtexbuf_s",
	    "octtexbuf_d",
	    "maxlevel"}));

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

    rc.r2tx_sha = shader_create(
	vsh_texquad,
	fsh_texquad,
	2,
	((const char*[]){"position", "texcoord"}),
	2,
	((const char*[]){"projection", "texture_base"}));

    free(vsh_texquad);
    free(fsh_texquad);

    // scene sha

    // create vertex array object for octree rendering
    glGenVertexArrays(1, &rc.octr_vao);
    glBindVertexArray(rc.octr_vao);

    // create vertex buffer object for octree rendering
    glGenBuffers(1, &rc.octr_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rc.octr_vbo);

    // create attribute for compute shader
    GLint inputAttrib = glGetAttribLocation(rc.octr_sha.name, "position");
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribPointer(inputAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    // texquad sha

    // create vertex array object for vertex buffer
    glGenVertexArrays(1, &rc.r2tex_vao);
    glBindVertexArray(rc.r2tex_vao);

    // create vertex buffer object
    glGenBuffers(1, &rc.r2tex_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rc.r2tex_vbo);

    // create attribute for compute shader
    inputAttrib = glGetAttribLocation(rc.r2tx_sha.name, "position");
    glEnableVertexAttribArray(inputAttrib);
    glVertexAttribPointer(inputAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);

    inputAttrib = glGetAttribLocation(rc.r2tx_sha.name, "texcoord");
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

    glGenFramebuffers(1, &rc.r2tex_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rc.r2tex_fbo);

    glGenTextures(1, &rc.r2tex_tex);
    glBindTexture(GL_TEXTURE_2D, rc.r2tex_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    /* glBindTexture(GL_TEXTURE_2D, 0); */

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rc.r2tex_tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) printf("Failed to create framebuffer at ogl_framebuffer_with_texture ");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    return rc;
}

void octreeconn_update(octreeconn_t* rc, float width, float height, v3_t position, v3_t angle, float lighta, uint8_t quality, int maxlevel)
{

    // first render scene to texture

    glUseProgram(rc->octr_sha.name);

    glBindFramebuffer(
	GL_FRAMEBUFFER,
	rc->r2tex_fbo);

    glUniform1i(rc->octr_sha.uni_loc[12], maxlevel);

    matrix4array_t projection = {0};

    float ow = width / (6.0 - (float) quality / 2.0);
    float oh = height / (6.0 - (float) quality / 2.0);

    m4_t pers         = m4_defaultortho(0.0, ow, 0.0, oh, -10, 10);
    projection.matrix = pers;
    glUniformMatrix4fv(rc->octr_sha.uni_loc[0], 1, 0, projection.array);

    GLfloat lightarr[3] = {lightc.x, lightc.y, lightc.z - sinf(lighta) * 200.0};
    /* printf("%f %f %f\n", lightarr[0], lightarr[1], lightarr[2]); */
    glUniform3fv(rc->octr_sha.uni_loc[3], 1, lightarr);

    GLfloat anglearr[3] = {angle.x, angle.y, 0.0};

    glUniform3fv(rc->octr_sha.uni_loc[2], 1, anglearr);

    GLfloat posarr[3] = {position.x, position.y, position.z};

    glUniform3fv(rc->octr_sha.uni_loc[1], 1, posarr);

    GLfloat basecubearr[4] = {0.0, 1800.0, 1800.0, 1800.0};

    glUniform4fv(rc->octr_sha.uni_loc[4], 1, basecubearr);

    GLfloat dimensions[2] = {ow, oh};

    glUniform2fv(rc->octr_sha.uni_loc[5], 1, dimensions);

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

    glBindVertexArray(rc->octr_vao);

    glBindBuffer(
	GL_ARRAY_BUFFER,
	rc->octr_vbo);

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

    glUseProgram(rc->r2tx_sha.name);

    glViewport(
	0.0,
	0.0,
	width,
	height);

    pers              = m4_defaultortho(0.0, ow, 0.0, oh, -10, 10);
    projection.matrix = pers;

    glUniformMatrix4fv(rc->r2tx_sha.uni_loc[0], 1, 0, projection.array);

    glBindVertexArray(rc->r2tex_vao);

    glActiveTexture(GL_TEXTURE0 + 0);

    glUniform1i(rc->r2tx_sha.uni_loc[1], 0);

    glBindTexture(GL_TEXTURE_2D, rc->r2tex_tex);

    GLfloat vertexes_uni[] = {
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	(float) 2048, 0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, (float) 2048, 0.0f, 0.0f, 1.0f,
	0.0f, (float) 2048, 0.0f, 0.0f, 1.0f,
	(float) 2048, 0.0f, 0.0f, 1.0f, 0.0f,
	(float) 2048, (float) 2048, 0.0f, 1.0f, 1.0f};

    glBindBuffer(
	GL_ARRAY_BUFFER,
	rc->r2tex_vbo);

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

void octreeconn_upload_texbuffer(
    octreeconn_t* rc,
    void*         data,
    int           x,
    int           y,
    int           width,
    int           height,
    int           internalformat,
    int           format,
    int           type,
    int           texture,
    int           uniform)
{
    glUseProgram(rc->octr_sha.name);
    glActiveTexture(GL_TEXTURE0 + uniform + 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(rc->octr_sha.uni_loc[uniform], uniform + 1);

    if (y == 0)
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data); // full upload
    else
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, type, data); // partial upload
}

#endif
