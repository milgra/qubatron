#ifndef skeleton_glc_h
#define skeleton_glc_h

#include "mt_log.c"
#include <GL/glew.h>
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "mt_memory.c"
#include "mt_quat.c"
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

    v4_t dir;
    v4_t pos;

    v4_t oribones[12];
    v4_t newbones[12];

    parts_t newparts;

    int left;
    int right;
    int forw;
    int back;

    float speed;
    float angle;

    int frontleg;
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

    .larm  = (v4_t){5.0, 170.0, 22.0, 10.0},
    .lhand = (v4_t){-10.0, 70.0, 22.0, 3.0},

    .rarm  = (v4_t){105.0, 170.0, 22.0, 10.0},
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

    cc.dir = (v4_t){0.0, 0.0, 1.0, 0.0};
    cc.pos = (v4_t){250.0, 0.0, 600.0, 0.0};

    cc.newparts.head  = v4_add(cc.newparts.hip, (v4_t){2.0, 86.0, 0.0, 0.0});
    cc.newparts.neck  = v4_add(cc.newparts.hip, (v4_t){2.0, 50.0, 0.0, 0.0});
    cc.newparts.larm  = v4_add(cc.newparts.hip, (v4_t){-47.0, 50.0, 0.0, 0.0});
    cc.newparts.lhand = v4_add(cc.newparts.hip, (v4_t){-62.0, -50.0, 0.0, 0.0});
    cc.newparts.rarm  = v4_add(cc.newparts.hip, (v4_t){53.0, 50.0, 0.0, 0.0});
    cc.newparts.rhand = v4_add(cc.newparts.hip, (v4_t){68.0, -50.0, 0.0, 0.0});

    cc.newparts.lleg  = v4_add(cc.newparts.hip, (v4_t){-20.0, 0.0, 1.0, 0.0});
    cc.newparts.lfoot = v4_add(cc.newparts.hip, (v4_t){-45.0, -130.0, 31.0, 0.0});
    cc.newparts.rleg  = v4_add(cc.newparts.hip, (v4_t){20.0, 0.0, 1.0, 0.0});
    cc.newparts.rfoot = v4_add(cc.newparts.hip, (v4_t){45.0, -130.0, 1.0, 0.0});

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

// A line point 0
// B line point 1
// C point to project

v3_t skeleton_glc_project_point(v3_t A, v3_t B, v3_t C)
{
    v3_t  AC      = v3_sub(C, A);
    v3_t  AB      = v3_sub(B, A);
    float dotACAB = v3_dot(AC, AB);
    float dotABAB = v3_dot(AB, AB);
    return v3_add(A, v3_scale(AB, dotACAB / dotABAB));
}

void skeleton_glc_update(skeleton_glc_t* cc, float lighta, int model_count, int maxlevel, float basesize)
{
    // update body parts

    if (cc->forw)
	cc->speed += 0.4;
    else if (cc->back)
	cc->speed -= 0.4;
    else if (cc->left)
	cc->angle -= 0.05;
    else if (cc->right)
	cc->angle += 0.05;

    cc->speed *= 0.8;

    v4_t back_rot = quat_from_axis_angle(v3_normalize(v3_sub(v4_xyz(cc->newparts.hip), v4_xyz(cc->newparts.neck))), cc->angle);
    v3_t curr_dir = quat_rotate(back_rot, (v3_t){cc->dir.x, cc->dir.y, cc->dir.z});

    cc->pos = v4_add(cc->pos, v4_scale((v4_t){curr_dir.x, curr_dir.y, curr_dir.z, 0.0}, cc->speed));

    // project left foot and right foot onto direction vector, if dir vector surpasses the active foot, step

    v3_t lfoot_prjp = skeleton_glc_project_point(v4_xyz(cc->pos), v3_add(v4_xyz(cc->pos), curr_dir), v4_xyz(cc->newparts.lfoot));
    v3_t rfoot_prjp = skeleton_glc_project_point(v4_xyz(cc->pos), v3_add(v4_xyz(cc->pos), curr_dir), v4_xyz(cc->newparts.rfoot));

    v3_t lfd = v3_sub(lfoot_prjp, v4_xyz(cc->pos)); // left foot distance
    v3_t rfd = v3_sub(rfoot_prjp, v4_xyz(cc->pos)); // right foot distance

    int ldirsame = ((lfd.x < 0) == (curr_dir.x < 0)) && ((lfd.y < 0) == (curr_dir.y < 0)) && ((lfd.z < 0) == (curr_dir.z < 0));
    int rdirsame = ((rfd.x < 0) == (curr_dir.x < 0)) && ((rfd.y < 0) == (curr_dir.y < 0)) && ((rfd.z < 0) == (curr_dir.z < 0));

    /* mt_log_debug("llen %f rlen %f ldirsame %i rdirsame %i", llen, rlen, ldirsame, rdirsame); */

    if (cc->frontleg == 1) // right
    {
	if (rdirsame == 0)
	{
	    cc->newparts.lfoot = v4_add(cc->pos, v4_xyzw(v3_resize(curr_dir, 50.0)));

	    v4_t left_rot = quat_from_axis_angle(v3_normalize(v3_sub(v4_xyz(cc->newparts.hip), v4_xyz(cc->newparts.neck))), M_PI / 2.0);
	    v3_t left_dir = quat_rotate(left_rot, curr_dir);

	    cc->newparts.lfoot   = v4_add(cc->newparts.lfoot, v4_xyzw(v3_resize(left_dir, 40.0)));
	    cc->newparts.lfoot.y = 10.0;
	    cc->frontleg         = 0;
	}
    }
    else
    {
	if (ldirsame == 0)
	{
	    cc->newparts.rfoot = v4_add(cc->pos, v4_xyzw(v3_resize(curr_dir, 50.0)));

	    v4_t right_rot = quat_from_axis_angle(v3_normalize(v3_sub(v4_xyz(cc->newparts.hip), v4_xyz(cc->newparts.neck))), -M_PI / 2.0);
	    v3_t right_dir = quat_rotate(right_rot, curr_dir);

	    cc->newparts.rfoot = v4_add(cc->newparts.rfoot, v4_xyzw(v3_resize(right_dir, 40.0)));
	    /* cc->newparts.rfoot   = v4_add(cc->newparts.lfoot, v4_xyzw(v3_resize((v3_t){curr_dir.z, curr_dir.y, curr_dir.x}, 30.0))); */
	    cc->newparts.rfoot.y = 10.0;
	    cc->frontleg         = 1;
	}
    }

    v4_t hipv  = (v4_t){0.0, 120.0 + sin(lighta) * 5.0, 0.0, cc->angle};
    v4_t headv = (v4_t){2.0, 86.0 + cos(lighta) * 2.0, 0.0 + sin(lighta) * 3.0, 0.0};
    v4_t neckv = (v4_t){2.0, 50.0 + cos(lighta) * 2.0, 0.0 - sin(lighta) * 3.0, 0.0};
    v4_t larmv = v4_xyzw(quat_rotate(back_rot, (v3_t){-47.0, 50.0, 0.0}));
    v4_t lhndv = v4_xyzw(quat_rotate(back_rot, (v3_t){-62.0 + sin(lighta) * 15.0, -50.0, 30.0}));
    v4_t rarmv = v4_xyzw(quat_rotate(back_rot, (v3_t){53.0, 50.0, 0.0}));
    v4_t rhndv = v4_xyzw(quat_rotate(back_rot, (v3_t){68.0 + cos(lighta) * 15.0, -50.0, 30.0}));
    v4_t llegv = v4_xyzw(quat_rotate(back_rot, (v3_t){-20.0, 0.0, 1.0}));
    v4_t rlegv = v4_xyzw(quat_rotate(back_rot, (v3_t){20.0, 0.0, 1.0}));

    cc->newparts.hip   = v4_add(cc->pos, hipv);
    cc->newparts.head  = v4_add(cc->newparts.hip, headv);
    cc->newparts.neck  = v4_add(cc->newparts.hip, neckv);
    cc->newparts.larm  = v4_add(cc->newparts.hip, larmv);
    cc->newparts.lhand = v4_add(cc->newparts.hip, lhndv);
    cc->newparts.rarm  = v4_add(cc->newparts.hip, rarmv);
    cc->newparts.rhand = v4_add(cc->newparts.hip, rhndv);
    cc->newparts.lleg  = v4_add(cc->newparts.hip, llegv);
    cc->newparts.rleg  = v4_add(cc->newparts.hip, rlegv);

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
