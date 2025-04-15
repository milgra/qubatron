#ifndef dust_glc_h
#define dust_glc_h

#include "mt_log.c"
#include "mt_math_3d.c"
#include "mt_matrix_4d.c"
#include "mt_memory.c"
#include <GL/glew.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "shader.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct dust_glc_t
{
    GLuint dust_sha;
    GLuint dust_vao;
    GLuint dust_vbo_pos_in;
    GLuint dust_vbo_spd_in;
    GLuint dust_vbo_pos_out;
    GLuint dust_vbo_spd_out;
    GLuint dust_unilocs[10];

    GLfloat* pos_out;
    GLfloat* spd_out;
    GLuint   out_size;
} dust_glc_t;

dust_glc_t dust_glc_init(char* path);

void dust_glc_update(dust_glc_t* cc, int model_count, int maxlevel, float basesize, v3_t campos);

void dust_glc_alloc_in(dust_glc_t* cc, void* posdata, void* spddata, size_t size);
void dust_glc_alloc_out(dust_glc_t* cc, void* data, size_t size);

#endif

#if __INCLUDE_LEVEL__ == 0

dust_glc_t dust_glc_init(char* base_path)
{
    dust_glc_t cc;

    char cshpath[PATH_MAX];
    char dshpath[PATH_MAX];

    snprintf(cshpath, PATH_MAX, "%sdust_vsh.c", base_path);
    snprintf(dshpath, PATH_MAX, "%sdust_fsh.c", base_path);

    mt_log_debug("loading shaders \n%s\n%s", cshpath, dshpath);

    char* vsh = shader_readfile(cshpath);
    char* fsh = shader_readfile(dshpath);

    cc.dust_sha = shader_create(vsh, fsh, 0);

    free(vsh);
    free(fsh);

    // set transform feedback varyings before linking

    const GLchar* feedbackVaryings[] = {"pos_out", "spd_out"};
    glTransformFeedbackVaryings(cc.dust_sha, 2, feedbackVaryings, GL_SEPARATE_ATTRIBS);

    shader_link(cc.dust_sha);

    glBindAttribLocation(cc.dust_sha, 0, "pos");
    glBindAttribLocation(cc.dust_sha, 1, "spd");

    // get uniforms

    char* dust_uniforms[] = {"basecube", "maxlevel", "campos"};

    for (int index = 0; index < 3; index++)
	cc.dust_unilocs[index] = glGetUniformLocation(cc.dust_sha, dust_uniforms[index]);

    glGenBuffers(1, &cc.dust_vbo_pos_in);
    glGenBuffers(1, &cc.dust_vbo_spd_in);
    glGenBuffers(1, &cc.dust_vbo_pos_out);
    glGenBuffers(1, &cc.dust_vbo_spd_out);

    // create vertex array and vertex buffer

    glGenVertexArrays(1, &cc.dust_vao);
    glBindVertexArray(cc.dust_vao);

    glBindBuffer(GL_ARRAY_BUFFER, cc.dust_vbo_pos_in);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    glBindBuffer(GL_ARRAY_BUFFER, cc.dust_vbo_spd_in);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    glBindVertexArray(0);

    return cc;
}

void dust_glc_update(dust_glc_t* cc, int model_count, int maxlevel, float basesize, v3_t campos)
{
    // switch off fragment stage

    glEnable(GL_RASTERIZER_DISCARD);

    glUseProgram(cc->dust_sha);

    glBindVertexArray(cc->dust_vao);

    // update uniforms

    GLfloat basecubearr[4] = {0.0, basesize, basesize, basesize};
    GLfloat camposarr[3]   = {campos.x, campos.y, campos.z};

    glUniform4fv(cc->dust_unilocs[0], 1, basecubearr);
    glUniform1i(cc->dust_unilocs[1], maxlevel);
    glUniform3fv(cc->dust_unilocs[2], 1, camposarr);

    // get previous state first to avoid feedback buffer stalling

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->dust_vbo_pos_out);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, cc->dust_vbo_spd_out);

    // run compute shader

    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, model_count);
    glEndTransformFeedback();
    glFlush();

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->dust_vbo_pos_out);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->out_size, cc->pos_out);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, cc->dust_vbo_spd_out);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->out_size, cc->spd_out);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, 0);

    glDisable(GL_RASTERIZER_DISCARD);
}

void dust_glc_alloc_in(dust_glc_t* cc, void* posdata, void* spddata, size_t size)
{
    // upload original points

    glBindBuffer(GL_ARRAY_BUFFER, cc->dust_vbo_pos_in);
    glBufferData(GL_ARRAY_BUFFER, size, posdata, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, cc->dust_vbo_spd_in);
    glBufferData(GL_ARRAY_BUFFER, size, spddata, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void dust_glc_alloc_out(dust_glc_t* cc, void* data, size_t size)
{
    // create buffer for modified points coming from the shader

    glBindBuffer(GL_ARRAY_BUFFER, cc->dust_vbo_pos_out);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_READ);
    glBindBuffer(GL_ARRAY_BUFFER, cc->dust_vbo_spd_out);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_READ);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    cc->pos_out  = mt_memory_alloc(size, NULL, NULL);
    cc->spd_out  = mt_memory_alloc(size, NULL, NULL);
    cc->out_size = size;
}

#endif
