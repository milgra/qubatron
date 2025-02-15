#ifndef particle_glc_h
#define particle_glc_h

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

typedef struct particle_glc_t
{
    GLuint part_sha;
    GLuint part_vao;
    GLuint part_vbo_in;
    GLuint part_vbo_out;
    GLuint part_unilocs[10];

    GLfloat* position_out;
    GLfloat* speed_out;
    GLuint   out_size;
} particle_glc_t;

particle_glc_t particle_glc_init(char* path);

void particle_glc_update(particle_glc_t* cc, float lighta, int model_count, int maxlevel, float basesize);
void particle_glc_alloc_in(particle_glc_t* cc, void* data, size_t size);
void particle_glc_alloc_out(particle_glc_t* cc, void* data, size_t size);

void particle_glc_alloc_in(particle_glc_t* cc, void* data, size_t size);
void particle_glc_alloc_out(particle_glc_t* cc, void* data, size_t size);

#endif

#if __INCLUDE_LEVEL__ == 0

particle_glc_t particle_glc_init(char* base_path)
{
    particle_glc_t cc;

    char cshpath[PATH_MAX];
    char dshpath[PATH_MAX];

    snprintf(cshpath, PATH_MAX, "%sparticle_vsh.c", base_path);
    snprintf(dshpath, PATH_MAX, "%sparticle_fsh.c", base_path);

    mt_log_debug("loading shaders \n%s\n%s", cshpath, dshpath);

    char* vsh = shader_readfile(cshpath);
    char* fsh = shader_readfile(dshpath);

    cc.part_sha = shader_create(vsh, fsh, 0);

    free(vsh);
    free(fsh);

    // set transform feedback varyings before linking

    const GLchar* feedbackVaryings[] = {"position_out", "speed_out"};
    glTransformFeedbackVaryings(cc.part_sha, 2, feedbackVaryings, GL_SEPARATE_ATTRIBS);

    shader_link(cc.part_sha);

    glBindAttribLocation(cc.part_sha, 0, "position");
    glBindAttribLocation(cc.part_sha, 0, "speed");

    // get uniforms

    char* part_uniforms[] = {"basecube", "maxlevel"};

    for (int index = 0; index < 2; index++)
	cc.part_unilocs[index] = glGetUniformLocation(cc.part_sha, part_uniforms[index]);

    // create vertex array and vertex buffer

    glGenVertexArrays(1, &cc.part_vao);
    glBindVertexArray(cc.part_vao);

    glGenBuffers(1, &cc.part_vbo_in);
    glBindBuffer(GL_ARRAY_BUFFER, cc.part_vbo_in);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    // create and bind result buffer object

    glGenBuffers(1, &cc.part_vbo_out);
    glBindBuffer(GL_ARRAY_BUFFER, cc.part_vbo_out);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return cc;
}

void particle_glc_update(particle_glc_t* cc, float lighta, int model_count, int maxlevel, float basesize)
{
    // switch off fragment stage

    glEnable(GL_RASTERIZER_DISCARD);

    glUseProgram(cc->part_sha);

    glBindVertexArray(cc->part_vao);

    // update uniforms

    GLfloat basecubearr[4] = {0.0, basesize, basesize, basesize};

    glUniform4fv(cc->part_unilocs[2], 1, basecubearr);
    glUniform1i(cc->part_unilocs[3], maxlevel);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->part_vbo_out);

    // get previous state first to avoid feedback buffer stalling

    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->out_size, cc->position_out);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 1, cc->out_size, cc->speed_out);

    // run compute shader

    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, model_count);
    glEndTransformFeedback();
    glFlush();

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

    glDisable(GL_RASTERIZER_DISCARD);
}

void particle_glc_alloc_in(particle_glc_t* cc, void* data, size_t size)
{
    // upload original points

    glBindBuffer(GL_ARRAY_BUFFER, cc->part_vbo_in);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void particle_glc_alloc_out(particle_glc_t* cc, void* data, size_t size)
{
    // create buffer for modified points coming from the shader

    glBindBuffer(GL_ARRAY_BUFFER, cc->part_vbo_out);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_READ);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    cc->position_out = mt_memory_alloc(size, NULL, NULL);
    cc->speed_out    = mt_memory_alloc(size, NULL, NULL);
    cc->out_size     = size;
}

#endif
