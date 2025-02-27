#ifndef skeleton_glc_h
#define skeleton_glc_h

#include "mt_log.c"
#include <GL/glew.h>
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "mt_memory.c"
#include "mt_vector_4d.c"
#include "shader.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct _parts_t
{
    v4_t head;
    v4_t neck;
    v4_t hip;
    v4_t lleg;
    v4_t rleg;
    v4_t lfoot;
    v4_t rfoot;
    v4_t larm;
    v4_t rarm;
    v4_t lhand;
    v4_t rhand;
} parts_t;

typedef struct skeleton_glc_t
{
    GLuint defo_sha;
    GLuint defo_vao;
    GLuint defo_vbo_in;
    GLuint defo_vbo_out;
    GLuint defo_unilocs[10];

    GLint* octqueue;
    size_t octqueuesize;

    GLfloat skeleton[48];
    v4_t    dir;

    v4_t oribones[12];
    v4_t newbones[12];

    parts_t newparts;

    int   left;
    int   right;
    int   forw;
    int   back;
    float speed;
} skeleton_glc_t;

skeleton_glc_t skeleton_glc_init(char* path);
void           skeleton_glc_update(skeleton_glc_t* cc, float lighta, int model_count, int maxlevel, float basesize);
void           skeleton_glc_alloc_in(skeleton_glc_t* cc, void* data, size_t size);
void           skeleton_glc_alloc_out(skeleton_glc_t* cc, void* data, size_t size);

void skeleton_glc_alloc_in(skeleton_glc_t* cc, void* data, size_t size);
void skeleton_glc_alloc_out(skeleton_glc_t* cc, void* data, size_t size);

void skeleton_glc_move(skeleton_glc_t* cc, int dir);

#endif

#if __INCLUDE_LEVEL__ == 0

// body parts for zombie model
parts_t parts = {

    .head = (v4_t){54.0, 205.0, 22.0, 15.0},
    .neck = (v4_t){54.0, 170.0, 22.0, 15.0},
    .hip  = (v4_t){52.0, 120.0, 22.0, 22.0},

    .larm  = (v4_t){5.0, 170.0, 22.0, 3.0},
    .lhand = (v4_t){-10.0, 70.0, 22.0, 3.0},

    .rarm  = (v4_t){105.0, 170.0, 22.0, 3.0},
    .rhand = (v4_t){120.0, 70.0, 22.0, 3.0},

    .lleg  = (v4_t){38.0, 120.0, 22.0, 6.0},
    .lfoot = (v4_t){8.0, -10.0, 22.0, 8.0},

    .rleg  = (v4_t){70.0, 120.0, 22.0, 6.0},
    .rfoot = (v4_t){100.0, -10.0, 22.0, 8.0}

};

skeleton_glc_t skeleton_glc_init(char* base_path)
{
    skeleton_glc_t cc = {0};

    v4_t oribones[] = {
	parts.head,
	parts.neck,
	parts.neck,
	parts.hip,
	parts.rarm,
	parts.rhand,
	parts.larm,
	parts.lhand,
	parts.rleg,
	parts.rfoot,
	parts.lleg,
	parts.lfoot};

    memcpy(cc.oribones, oribones, sizeof(v4_t) * 12);

    cc.newparts = parts;

    cc.newparts.hip.x += 250.0; // starting position
    cc.newparts.hip.z += 600.0; // starting position

    cc.dir = (v4_t){0.0, 0.0, 1.0, 0.0};

    char cshpath[PATH_MAX];
    char dshpath[PATH_MAX];

    snprintf(cshpath, PATH_MAX, "%sskeleton_vsh.c", base_path);
    snprintf(dshpath, PATH_MAX, "%sskeleton_fsh.c", base_path);

    mt_log_debug("loading shaders \n%s\n%s", cshpath, dshpath);

    char* vsh = shader_readfile(cshpath);
    char* fsh = shader_readfile(dshpath);

    cc.defo_sha = shader_create(vsh, fsh, 0);

    free(vsh);
    free(fsh);

    // set transform feedback varyings before linking

    const GLchar* feedbackVaryings[] = {"oct14", "oct54", "oct94"};
    glTransformFeedbackVaryings(cc.defo_sha, 3, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

    shader_link(cc.defo_sha);

    glBindAttribLocation(cc.defo_sha, 0, "position");

    // get uniforms

    char* defo_uniforms[] = {"oldbones", "newbones", "basecube", "maxlevel"};

    for (int index = 0; index < 4; index++)
	cc.defo_unilocs[index] = glGetUniformLocation(cc.defo_sha, defo_uniforms[index]);

    // create vertex array and vertex buffer

    glGenVertexArrays(1, &cc.defo_vao);
    glBindVertexArray(cc.defo_vao);

    glGenBuffers(1, &cc.defo_vbo_in);
    glBindBuffer(GL_ARRAY_BUFFER, cc.defo_vbo_in);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    glBindVertexArray(0);

    // create and bind result buffer object

    glGenBuffers(1, &cc.defo_vbo_out);

    return cc;
}

void skeleton_glc_update(skeleton_glc_t* cc, float lighta, int model_count, int maxlevel, float basesize)
{
    // update body parts

    if (cc->forw)
	cc->speed += 0.1;
    else if (cc->back)
	cc->speed -= 0.1;
    else if (cc->left)
	cc->speed -= 0.1;
    else if (cc->right)
	cc->speed -= 0.1;
    else
	cc->speed *= 0.8;

    cc->newparts.hip = v4_add(cc->newparts.hip, v4_scale(cc->dir, cc->speed));

    cc->newparts.head  = v4_add(cc->newparts.hip, (v4_t){2.0, 86.0, 0.0, 0.0});
    cc->newparts.neck  = v4_add(cc->newparts.hip, (v4_t){2.0, 50.0, 0.0, 0.0});
    cc->newparts.larm  = v4_add(cc->newparts.hip, (v4_t){-47.0, 50.0, 0.0, 0.0});
    cc->newparts.lhand = v4_add(cc->newparts.hip, (v4_t){-62.0, -50.0, 0.0, 0.0});
    cc->newparts.rarm  = v4_add(cc->newparts.hip, (v4_t){53.0, 50.0, 0.0, 0.0});
    cc->newparts.rhand = v4_add(cc->newparts.hip, (v4_t){68.0, -50.0, 0.0, 0.0});

    cc->newparts.lleg  = v4_add(cc->newparts.hip, (v4_t){-20.0, 0.0, 1.0});
    cc->newparts.lfoot = v4_add(cc->newparts.hip, (v4_t){-45.0, -130.0, 1.0});
    cc->newparts.rleg  = v4_add(cc->newparts.hip, (v4_t){+20.0, 0.0, 1.0});
    cc->newparts.rfoot = v4_add(cc->newparts.hip, (v4_t){+45.0, -130.0, 1.0});

    // switch off fragment stage

    glEnable(GL_RASTERIZER_DISCARD);

    glUseProgram(cc->defo_sha);

    glBindVertexArray(cc->defo_vao);

    v4_t newbones[] = {
	cc->newparts.head,
	cc->newparts.neck,
	cc->newparts.neck,
	cc->newparts.hip,
	cc->newparts.rarm,
	cc->newparts.rhand,
	cc->newparts.larm,
	cc->newparts.lhand,
	cc->newparts.rleg,
	cc->newparts.rfoot,
	cc->newparts.lleg,
	cc->newparts.lfoot};

    // update uniforms

    GLfloat basecubearr[4] = {0.0, basesize, basesize, basesize};

    glUniform4fv(cc->defo_unilocs[0], 12, (GLfloat*) cc->oribones);
    glUniform4fv(cc->defo_unilocs[1], 12, (GLfloat*) newbones);
    glUniform4fv(cc->defo_unilocs[2], 1, basecubearr);
    glUniform1i(cc->defo_unilocs[3], maxlevel);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->defo_vbo_out);

    // get previous state first to avoid feedback buffer stalling

    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->octqueuesize, cc->octqueue);

    // run compute shader

    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, model_count);
    glEndTransformFeedback();
    glFlush();

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

    glDisable(GL_RASTERIZER_DISCARD);
}

void skeleton_glc_alloc_in(skeleton_glc_t* cc, void* data, size_t size)
{
    // upload original points

    glBindBuffer(GL_ARRAY_BUFFER, cc->defo_vbo_in);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void skeleton_glc_alloc_out(skeleton_glc_t* cc, void* data, size_t size)
{
    // create buffer for modified points coming from the shader

    glBindBuffer(GL_ARRAY_BUFFER, cc->defo_vbo_out);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_READ);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    cc->octqueue     = mt_memory_alloc(size, NULL, NULL);
    cc->octqueuesize = size;
}

void skeleton_glc_move(skeleton_glc_t* cc, int dir)
{
    if (dir == 0)
    {
	cc->left  = 0;
	cc->right = 0;
	cc->forw  = 0;
	cc->back  = 0;
    }
    if (dir == 1)
    {
	cc->left = 1;
    }
    if (dir == 2)
    {
	cc->right = 1;
    }
    if (dir == 3)
    {
	cc->forw = 1;
    }
    if (dir == 4)
    {
	cc->back = 1;
    }
}

#endif
