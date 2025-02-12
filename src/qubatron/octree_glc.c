#ifndef octree_glc_h
#define octree_glc_h

#include "mt_log.c"
#include "mt_math_3d.c"
#include "mt_matrix_4d.c"
#include <GL/glew.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "shader.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct octree_glc_t
{
    // shaders

    GLuint octr_sha;
    GLuint r2tx_sha;

    GLuint octr_unilocs[20];
    GLuint r2tx_unilocs[10];

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

} octree_glc_t;

octree_glc_t octree_glc_init(char* path);
void         octree_glc_update(octree_glc_t* rc, float width, float height, v3_t position, v3_t angle, float lighta, uint8_t quality, int maxlevel, float basesize);
void         octree_glc_upload_texbuffer(octree_glc_t* rc, void* data, int x, int y, int width, int height, int internalformat, int format, int type, int texture, int uniform);

#endif

#if __INCLUDE_LEVEL__ == 0

v3_t lightc = {420.0, 200.0, 680.0};

octree_glc_t octree_glc_init(char* base_path)
{
    octree_glc_t rc;

    char vshpath[PATH_MAX];
    char fshpath[PATH_MAX];

    snprintf(vshpath, PATH_MAX, "%soctree_vsh.c", base_path);
    snprintf(fshpath, PATH_MAX, "%soctree_fsh.c", base_path);

    mt_log_debug("loading shaders \n%s\n%s", vshpath, fshpath);

    char* vsh = shader_readfile(vshpath);
    char* fsh = shader_readfile(fshpath);

    rc.octr_sha = shader_create(vsh, fsh, 1);

    free(vsh);
    free(fsh);

    glBindAttribLocation(rc.octr_sha, 0, "position");

    // get uniforms

    char* uniforms[] = {
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
	"maxlevel"};

    for (int index = 0; index < 13; index++)
	rc.octr_unilocs[index] = glGetUniformLocation(rc.octr_sha, uniforms[index]);

    snprintf(vshpath, PATH_MAX, "%stexquad_vsh.c", base_path);
    snprintf(fshpath, PATH_MAX, "%stexquad_fsh.c", base_path);

    mt_log_debug("loading shaders \n%s\n%s", vshpath, fshpath);

    char* vsh_texquad = shader_readfile(vshpath);
    char* fsh_texquad = shader_readfile(fshpath);

    rc.r2tx_sha = shader_create(vsh_texquad, fsh_texquad, 1);

    free(vsh_texquad);
    free(fsh_texquad);

    glBindAttribLocation(rc.r2tx_sha, 0, "position");
    glBindAttribLocation(rc.r2tx_sha, 1, "texcoord");

    // get uniforms

    char* r2tx_uniforms[] = {"projection", "texture_base"};

    for (int index = 0; index < 2; index++)
	rc.r2tx_unilocs[index] = glGetUniformLocation(rc.r2tx_sha, r2tx_uniforms[index]);

    // octree renderer

    glGenBuffers(1, &rc.octr_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rc.octr_vbo);

    // only attribute is position for quad to render to

    glGenVertexArrays(1, &rc.octr_vao);
    glBindVertexArray(rc.octr_vao);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

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

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (const GLvoid*) 12);

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

void octree_glc_update(octree_glc_t* rc, float width, float height, v3_t position, v3_t angle, float lighta, uint8_t quality, int maxlevel, float basesize)
{
    glEnable(GL_BLEND);

    // bind scale framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, rc->r2tex_fbo);

    // render octree first

    glUseProgram(rc->octr_sha);

    // update uniforms

    GLfloat basecubearr[4] = {0.0, basesize, basesize, basesize};
    GLfloat lightarr[3]    = {lightc.x, lightc.y, lightc.z - sinf(lighta) * 200.0};
    GLfloat anglearr[3]    = {angle.x, angle.y, 0.0};
    GLfloat posarr[3]      = {position.x, position.y, position.z};

    float   ow            = width / (6.0 - (float) quality / 2.0);
    float   oh            = height / (6.0 - (float) quality / 2.0);
    GLfloat dimensions[2] = {ow, oh};

    m4_t pers = m4_defaultortho(0.0, ow, 0.0, oh, -10, 10);

    matrix4array_t projection = {0};
    projection.matrix         = pers;

    glUniformMatrix4fv(rc->octr_unilocs[0], 1, 0, projection.array);
    glUniform3fv(rc->octr_unilocs[1], 1, posarr);
    glUniform3fv(rc->octr_unilocs[2], 1, anglearr);
    glUniform3fv(rc->octr_unilocs[3], 1, lightarr);
    glUniform4fv(rc->octr_unilocs[4], 1, basecubearr);
    glUniform2fv(rc->octr_unilocs[5], 1, dimensions);
    glUniform1i(rc->octr_unilocs[12], maxlevel);

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

    glUseProgram(rc->r2tx_sha);

    glViewport(0.0, 0.0, width, height);

    pers              = m4_defaultortho(0.0, ow, 0.0, oh, -10, 10);
    projection.matrix = pers;

    // update uniforms

    glUniformMatrix4fv(rc->r2tx_unilocs[0], 1, 0, projection.array);

    glActiveTexture(GL_TEXTURE0 + 0);
    glUniform1i(rc->r2tx_unilocs[1], 0);
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

void octree_glc_upload_texbuffer(
    octree_glc_t* rc,
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
    glUseProgram(rc->octr_sha);
    glActiveTexture(GL_TEXTURE0 + uniform + 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(rc->octr_unilocs[uniform], uniform + 1);

    if (y == 0)
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data); // full upload
    else
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, type, data); // partial upload
}

#endif
