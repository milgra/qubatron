#ifndef mt_math_3d_h
#define mt_math_3d_h

#include "mt_vector_3d.c"

v3_t l3_project_point(v3_t A, v3_t B, v3_t C);

#endif
#if __INCLUDE_LEVEL__ == 0

// A line point 0
// B line point 1
// C point to project

v3_t l3_project_point(v3_t A, v3_t B, v3_t C)
{
    v3_t  AC      = v3_sub(C, A);
    v3_t  AB      = v3_sub(B, A);
    float dotACAB = v3_dot(AC, AB);
    float dotABAB = v3_dot(AB, AB);
    return v3_add(A, v3_scale(AB, dotACAB / dotABAB));
}

#endif
