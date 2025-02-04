#ifndef glc_octree_h
#define glc_octree_h

#include "mt_log.c"
#include "mt_math_3d.c"
#include "mt_matrix_4d.c"
#include "readfile.c"
#include <GL/glew.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "shader.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct glc_octree_t
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

} glc_octree_t;

glc_octree_t glc_octree_init(char* path);
void         glc_octree_update(glc_octree_t* rc, float width, float height, v3_t position, v3_t angle, float lighta, uint8_t quality, int maxlevel);
void         glc_octree_upload_texbuffer(glc_octree_t* rc, void* data, int x, int y, int width, int height, int internalformat, int format, int type, int texture, int uniform);

#endif

#if __INCLUDE_LEVEL__ == 0

v3_t lightc = {420.0, 200.0, 680.0};

glc_octree_t glc_octree_init(char* base_path)
{
    glc_octree_t rc;

    char vshpath[PATH_MAX];
    char fshpath[PATH_MAX];

    snprintf(vshpath, PATH_MAX, "%soctree_vsh.c", base_path);
    snprintf(fshpath, PATH_MAX, "%soctree_fsh.c", base_path);

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

    snprintf(vshpath, PATH_MAX, "%stexquad_vsh.c", base_path);
    snprintf(fshpath, PATH_MAX, "%stexquad_fsh.c", base_path);

    mt_log_debug("loading shaders \n%s\n%s", vshpath, fshpath);

    char* vsh_texquad = readfile(vshpath);
    char* fsh_texquad = readfile(fshpath);

    rc.r2tx_sha = shader_create(
	vsh_texquad,
	fsh_texquad,
	2,
	((const char*[]){
	    "position",
	    "texcoord"}),
	2,
	((const char*[]){
	    "projection",
	    "texture_base"}));

    free(vsh_texquad);
    free(fsh_texquad);

    // octree renderer

    glGenBuffers(1, &rc.octr_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rc.octr_vbo);

    // only attribute is position for quad to render to

    glGenVertexArrays(1, &rc.octr_vao);
    glBindVertexArray(rc.octr_vao);

    GLint octposAttrib = glGetAttribLocation(rc.octr_sha.name, "position");

    glEnableVertexAttribArray(octposAttrib);
    glVertexAttribPointer(octposAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    // texture buffers

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

    // render to texture renderer

    glGenBuffers(1, &rc.r2tex_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rc.r2tex_vbo);

    // create vertex array object for vertex buffer

    glGenVertexArrays(1, &rc.r2tex_vao);
    glBindVertexArray(rc.r2tex_vao);

    // attributes are position and texcoord

    GLint r2posAttrib = glGetAttribLocation(rc.r2tx_sha.name, "position");
    GLint r2texAttrib = glGetAttribLocation(rc.r2tx_sha.name, "texcoord");

    glEnableVertexAttribArray(r2posAttrib);
    glEnableVertexAttribArray(r2texAttrib);

    glVertexAttribPointer(r2posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
    glVertexAttribPointer(r2texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (const GLvoid*) 12);

    // render to texture texture/framebufffer

    glGenFramebuffers(1, &rc.r2tex_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rc.r2tex_fbo);

    glGenTextures(1, &rc.r2tex_tex);
    glBindTexture(GL_TEXTURE_2D, rc.r2tex_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rc.r2tex_tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) printf("Failed to create framebuffer at ogl_framebuffer_with_texture ");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return rc;
}

void glc_octree_update(glc_octree_t* rc, float width, float height, v3_t position, v3_t angle, float lighta, uint8_t quality, int maxlevel)
{
    // bind scale framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, rc->r2tex_fbo);

    // render octree first

    glUseProgram(rc->octr_sha.name);

    // update uniforms

    GLfloat basecubearr[4] = {0.0, 1800.0, 1800.0, 1800.0};
    GLfloat lightarr[3]    = {lightc.x, lightc.y, lightc.z - sinf(lighta) * 200.0};
    GLfloat anglearr[3]    = {angle.x, angle.y, 0.0};
    GLfloat posarr[3]      = {position.x, position.y, position.z};

    float   ow            = width / (6.0 - (float) quality / 2.0);
    float   oh            = height / (6.0 - (float) quality / 2.0);
    GLfloat dimensions[2] = {ow, oh};

    m4_t pers = m4_defaultortho(0.0, ow, 0.0, oh, -10, 10);

    matrix4array_t projection = {0};
    projection.matrix         = pers;

    glUniformMatrix4fv(rc->octr_sha.uni_loc[0], 1, 0, projection.array);
    glUniform3fv(rc->octr_sha.uni_loc[1], 1, posarr);
    glUniform3fv(rc->octr_sha.uni_loc[2], 1, anglearr);
    glUniform3fv(rc->octr_sha.uni_loc[3], 1, lightarr);
    glUniform4fv(rc->octr_sha.uni_loc[4], 1, basecubearr);
    glUniform2fv(rc->octr_sha.uni_loc[5], 1, dimensions);
    glUniform1i(rc->octr_sha.uni_loc[12], maxlevel);

    // set viewport and color

    glViewport(0.0, 0.0, ow, oh);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw on octree canvas

    GLfloat vertexes[] = {
	0.0f, 0.0f, 0.0f,
	(float) ow, 0.0f, 0.0f,
	0.0f, (float) oh, 0.0f,
	0.0f, (float) oh, 0.0f,
	(float) ow, 0.0f, 0.0f,
	(float) ow, (float) oh, 0.0};

    glBindVertexArray(rc->octr_vao);
    glBindBuffer(GL_ARRAY_BUFFER, rc->octr_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 3, vertexes, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // scale down rendered output to default framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(rc->r2tx_sha.name);

    glViewport(0.0, 0.0, width, height);

    pers              = m4_defaultortho(0.0, ow, 0.0, oh, -10, 10);
    projection.matrix = pers;

    // update uniforms

    glUniformMatrix4fv(rc->r2tx_sha.uni_loc[0], 1, 0, projection.array);

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

    glBindVertexArray(rc->r2tex_vao);
    glBindBuffer(GL_ARRAY_BUFFER, rc->r2tex_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 5, vertexes_uni, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    // draw crosshair

    glEnable(GL_SCISSOR_TEST);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glScissor(width / 2 - 1, height / 2 - 1, 2, 2);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

void glc_octree_upload_texbuffer(
    glc_octree_t* rc,
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
