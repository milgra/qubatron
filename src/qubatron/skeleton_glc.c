#ifndef skeleton_glc_h
#define skeleton_glc_h

#include "mt_log.c"
#include <GL/glew.h>
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "mt_memory.c"
#include "mt_vector_3d.c"
#include "shader.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct skeleton_glc_t
{
    GLuint defo_sha;
    GLuint defo_vao;
    GLuint defo_vbo_in;
    GLuint defo_vbo_out;
    GLuint defo_unilocs[10];

    GLint* octqueue;
    size_t octqueuesize;

    v3_t head;
    v3_t neck;
    v3_t hip;
    v3_t dir;
    v3_t lleg;
    v3_t rleg;
    v3_t lfoot;
    v3_t rfoot;
    v3_t larm;
    v3_t rarm;
    v3_t lhand;
    v3_t rhand;

    GLfloat skeleton[48];
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

GLfloat skeleton_old[48] =
    {
	// head
	54.0, 205.0, 22.0, 15.0,
	54.0, 170.0, 22.0, 15.0,
	// torso
	54.0, 170.0, 22.0, 22.0,
	52.0, 120.0, 22.0, 22.0,
	// right arm
	105.0, 170.0, 22.0, 3.0,
	120.0, 70.0, 22.0, 3.0,
	// left arm
	5.0, 170.0, 22.0, 3.0,
	-10.0, 70.0, 22.0, 3.0,
	// right leg
	70.0, 120.0, 22.0, 6.0,
	100.0, -10.0, 22.0, 8.0,
	// left leg
	38.0, 120.0, 22.0, 6.0,
	8.0, -10.0, 22.0, 8.0};

skeleton_glc_t skeleton_glc_init(char* base_path)
{
    skeleton_glc_t cc;

    cc.dir = (v3_t){0.0, 0.0, 1.0};
    cc.hip = (v3_t){52.0, 120.0, 22.0};

    cc.head = v3_add(cc.hip, (v3_t){2.0, 85.0, 0.0});
    cc.neck = v3_add(cc.hip, (v3_t){2.0, 50.0, 0.0});

    cc.larm  = v3_add(cc.hip, (v3_t){-47.0, 50.0, 1.0});
    cc.lhand = v3_add(cc.hip, (v3_t){-62.0, -50.0, 1.0});

    cc.rarm  = v3_add(cc.hip, (v3_t){53.0, 50.0, 1.0});
    cc.rhand = v3_add(cc.hip, (v3_t){68.0, -50.0, 1.0});

    cc.lleg  = v3_add(cc.hip, (v3_t){-14.0, 0.0, 1.0});
    cc.lfoot = v3_add(cc.hip, (v3_t){-44.0, -130.0, 1.0});

    cc.rleg  = v3_add(cc.hip, (v3_t){18.0, 0.0, 1.0});
    cc.rfoot = v3_add(cc.hip, (v3_t){48.0, -130.0, 1.0});

    cc.hip.z += 600.0;

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
    // switch off fragment stage

    glEnable(GL_RASTERIZER_DISCARD);

    glUseProgram(cc->defo_sha);

    glBindVertexArray(cc->defo_vao);

    // original skeleton

    float dx = 250.0;
    float dz = 600.0;

    /* cc->hip.z += 1.0; */

    cc->lleg  = v3_add(cc->hip, (v3_t){-20.0, 0.0, 1.0});
    cc->rleg  = v3_add(cc->hip, (v3_t){+20.0, 0.0, 1.0});
    cc->lfoot = v3_add(cc->hip, (v3_t){-45.0, -130.0, 1.0});
    cc->rfoot = v3_add(cc->hip, (v3_t){+45.0, -130.0, 1.0});

    // modified skeleton

    GLfloat skeleton_new[36] =
	{
	    // head
	    54.0 + dx, 205.0, 20.0 + dz + sinf(lighta) * 10.0,
	    54.0 + dx, 170.0, 20.0 + dz,
	    // torso
	    54.0 + dx, 170.0, 20.0 + dz,
	    55.0 + dx, 120.0, 20.0 + dz + sinf(lighta) * 2.0,
	    // right arm
	    105.0 + dx, 170.0, 20.0 + dz,
	    120.0 + dx + sinf(lighta) * 15.0, 70.0, 20.0 + dz + cosf(lighta) * 15.0,
	    // left arm
	    5.0 + dx, 170.0, 20.0 + dz,
	    -10.0 + dx + sinf(lighta) * 15.0, 70.0, 20.0 + dz + cosf(lighta) * 15.0,
	    // right leg
	    70.0 + dx, 120.0, 20.0 + dz,
	    90.0 + dx, -10.0, 20 + dz + cosf(lighta) * 15.0,
	    // left leg
	    38.0 + dx, 120.0, 20.0 + dz,
	    8.0 + dx, -10.0, 20.0 + dz + cosf(lighta) * 15.0};

    // update uniforms

    GLfloat basecubearr[4] = {0.0, basesize, basesize, basesize};

    glUniform4fv(cc->defo_unilocs[0], 12, skeleton_old);
    glUniform3fv(cc->defo_unilocs[1], 12, skeleton_new);
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

#endif
