#ifndef skeleton_glc_h
#define skeleton_glc_h

#include "mt_log.c"
#include <GL/glew.h>
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "model.c"
#include "mt_line_3d.c"
#include "mt_memory.c"
#include "mt_quat.c"
#include "mt_vector_4d.c"
#include "shader.c"
#include "zombie.c"
#include <GL/gl.h>
#include <GL/glu.h>

typedef struct skeleton_glc_t
{
    GLuint defo_sha;
    GLuint defo_vao;

    GLuint defo_pnt_vbo_in;
    GLuint defo_pnt_vbo_out;

    GLuint defo_vbo_oct_out;
    GLuint defo_vbo_nrm_out;
    GLuint defo_unilocs[10];

    GLint*   oct_out;
    size_t   oct_out_size;
    GLfloat* nrm_out;
    size_t   nrm_out_size;

    v4_t dir;
    v4_t pos;
    v4_t pdir;

    v4_t path[10];
    int  path_ind;

    int zombie_control;

    int left;
    int right;
    int forw;
    int back;

    zombie_t zombie;

    float speed;
    float angle;

    int frontleg;
    int ragdoll;
} skeleton_glc_t;

skeleton_glc_t skeleton_glc_init(char* path);
void           skeleton_glc_init_zombie(skeleton_glc_t* cc, octree_t* statoctr);
void           skeleton_glc_init_ragdoll(skeleton_glc_t* cc);
void           skeleton_glc_update(skeleton_glc_t* cc, octree_t* statoctr, model_t* statmod, float lighta, int model_count, int maxlevel, float basesize);
void           skeleton_glc_alloc_in(skeleton_glc_t* cc, void* pntdata, void* nrmdata, size_t size);
void           skeleton_glc_alloc_out(skeleton_glc_t* cc, void* data, size_t octsize, size_t nrmsize);
void           skeleton_glc_shoot(skeleton_glc_t* cc, v3_t pos, v3_t direction, v3_t hitpos);

void skeleton_glc_move(skeleton_glc_t* cc, int dir);

#endif

#if __INCLUDE_LEVEL__ == 0

skeleton_glc_t skeleton_glc_init(char* base_path)
{
    skeleton_glc_t cc = {0};

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

    const GLchar* feedbackVaryings[] = {"octets_out", "normal_out"};
    glTransformFeedbackVaryings(cc.defo_sha, 2, feedbackVaryings, GL_SEPARATE_ATTRIBS);

    shader_link(cc.defo_sha);

    glBindAttribLocation(cc.defo_sha, 0, "position");
    glBindAttribLocation(cc.defo_sha, 1, "normal");

    // get uniforms

    char* defo_uniforms[] = {"oldbones", "newbones", "basecube", "maxlevel"};

    for (int index = 0; index < 4; index++)
	cc.defo_unilocs[index] = glGetUniformLocation(cc.defo_sha, defo_uniforms[index]);

    // create vertex array and vertex buffer

    glGenBuffers(1, &cc.defo_pnt_vbo_in);
    glGenBuffers(1, &cc.defo_pnt_vbo_out);

    glGenVertexArrays(1, &cc.defo_vao);
    glBindVertexArray(cc.defo_vao);

    glBindBuffer(GL_ARRAY_BUFFER, cc.defo_pnt_vbo_in);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    glBindBuffer(GL_ARRAY_BUFFER, cc.defo_pnt_vbo_out);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);

    glBindVertexArray(0);

    // create and bind result buffer object

    glGenBuffers(1, &cc.defo_vbo_oct_out);
    glGenBuffers(1, &cc.defo_vbo_nrm_out);

    return cc;
}

void skeleton_glc_init_zombie(skeleton_glc_t* cc, octree_t* statoctr)
{
    cc->dir  = (v4_t){0.0, 0.0, 1.0, 0.0};
    cc->pos  = (v4_t){250.0, 0.0, 600.0, 0.0};
    cc->pdir = (v4_t){0.0, 0.0, 1.0, 0.0};

    cc->path[0] = (v4_t){300.0, 0.0, 900.0, 0.0};
    cc->path[1] = (v4_t){450.0, 0.0, 900.0, 0.0};
    cc->path[2] = (v4_t){450.0, 0.0, 200.0, 0.0};
    cc->path[3] = (v4_t){550.0, 0.0, 200.0, 0.0};
    cc->path[4] = (v4_t){450.0, 0.0, 300.0, 0.0};
    cc->path[5] = (v4_t){300.0, 0.0, 600.0, 0.0};

    cc->zombie         = zombie_init(cc->pos, cc->dir, statoctr);
    cc->zombie_control = 0;

    cc->angle = 0.0;
}

void skeleton_glc_update(skeleton_glc_t* cc, octree_t* statoctr, model_t* statmod, float lighta, int model_count, int maxlevel, float basesize)
{
    // update body parts

    if (cc->ragdoll > 0)
    {
	cc->ragdoll -= 1;
	if (cc->ragdoll == 0)
	    zombie_init_walk(&cc->zombie);
    }

    v4_t cdir;

    if (cc->zombie_control == 1)
    {
	if (cc->forw)
	    cc->speed += 0.4;
	else if (cc->back)
	    cc->speed -= 0.4;
	else if (cc->left)
	    cc->angle -= 0.05;
	else if (cc->right)
	    cc->angle += 0.05;

	v4_t rotq = quat_from_axis_angle(v3_normalize((v3_t){0.0, 1.0, 0.0}), -cc->angle);
	cdir      = v4_xyzw(quat_rotate(rotq, v4_xyz(cc->dir)));

	cc->speed *= 0.8;
	cc->pos = v4_add(cc->pos, v4_scale(cdir, cc->speed * 2.0));
    }
    else if (cc->ragdoll == 0)
    {
	cdir = v4_sub(cc->path[cc->path_ind], cc->pos);

	if (v3_length(v4_xyz(cdir)) < 20.0)
	{
	    cc->path_ind += 1;
	    if (cc->path_ind == 4) cc->path_ind = 0;
	}

	float angle = -atan2(cdir.x, cdir.z);

	if (cc->angle - angle > M_PI) cc->angle -= 2.0 * M_PI;
	if (angle - cc->angle > M_PI) cc->angle += 2.0 * M_PI;

	cc->angle += (angle - cc->angle) / 16.0;

	v4_t rotq = quat_from_axis_angle(v3_normalize((v3_t){0.0, 1.0, 0.0}), -cc->angle);
	cdir      = v4_xyzw(quat_rotate(rotq, v4_xyz(cc->dir)));

	cc->speed += 0.5;
	cc->speed *= 0.8;
	cc->pos = v4_add(cc->pos, v4_scale(cdir, cc->speed));
    }

    zombie_update(&cc->zombie, statoctr, statmod, lighta, cc->angle, cc->pos, cdir, cc->speed);

    // switch off fragment stage

    glEnable(GL_RASTERIZER_DISCARD);

    glUseProgram(cc->defo_sha);

    glBindVertexArray(cc->defo_vao);

    // update uniforms

    GLfloat basecubearr[4] = {0.0, basesize, basesize, basesize};

    glUniform4fv(cc->defo_unilocs[0], POINT_COUNT, (GLfloat*) cc->zombie.oribones.arr);
    glUniform4fv(cc->defo_unilocs[1], POINT_COUNT, (GLfloat*) cc->zombie.newbones.arr);
    glUniform4fv(cc->defo_unilocs[2], 1, basecubearr);
    glUniform1i(cc->defo_unilocs[3], maxlevel);

    // get previous state first to avoid feedback buffer stalling

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->defo_vbo_oct_out);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->oct_out_size, cc->oct_out);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, cc->defo_vbo_nrm_out);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, cc->nrm_out_size, cc->nrm_out);

    // run compute shader

    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, model_count);
    glEndTransformFeedback();
    glFlush();

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

    glDisable(GL_RASTERIZER_DISCARD);
}

void skeleton_glc_alloc_in(skeleton_glc_t* cc, void* pntdata, void* nrmdata, size_t size)
{
    // upload original points

    glBindBuffer(GL_ARRAY_BUFFER, cc->defo_pnt_vbo_in);
    glBufferData(GL_ARRAY_BUFFER, size, pntdata, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, cc->defo_pnt_vbo_out);
    glBufferData(GL_ARRAY_BUFFER, size, nrmdata, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void skeleton_glc_alloc_out(skeleton_glc_t* cc, void* data, size_t octsize, size_t nrmsize)
{
    // create buffer for modified points coming from the shader

    glBindBuffer(GL_ARRAY_BUFFER, cc->defo_vbo_oct_out);
    glBufferData(GL_ARRAY_BUFFER, octsize, NULL, GL_STREAM_READ);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, cc->defo_vbo_nrm_out);
    glBufferData(GL_ARRAY_BUFFER, nrmsize, NULL, GL_STREAM_READ);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    cc->oct_out      = mt_memory_alloc(octsize, NULL, NULL);
    cc->oct_out_size = octsize;

    cc->nrm_out      = mt_memory_alloc(nrmsize, NULL, NULL);
    cc->nrm_out_size = nrmsize;
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

void skeleton_glc_init_ragdoll(skeleton_glc_t* cc)
{
    if (cc->ragdoll == 0)
    {
	zombie_init_ragdoll(&cc->zombie);
    }
    else
    {
	zombie_init_walk(&cc->zombie);
	cc->ragdoll = 100;
    }
}

void skeleton_glc_shoot(skeleton_glc_t* cc, v3_t pos, v3_t dir, v3_t hit)
{
    zombie_init_ragdoll(&cc->zombie);
    cc->ragdoll += 150;
    cc->speed -= 2.0;
    zombie_shoot(&cc->zombie, pos, dir, hit);
}

#endif
