#ifndef mt_quat_h
#define mt_quat_h

#include "mt_vector_3d.c"
#include "mt_vector_4d.c"

v4_t quat_from_axis_angle(v3_t axis, float angle);
v3_t quat_rotate(v4_t q, v3_t v);

#endif

#if __INCLUDE_LEVEL__ == 0

v4_t quat_from_axis_angle(v3_t axis, float angle)
{
    v4_t  qr;
    float half_angle = angle * 0.5;
    qr.x             = axis.x * sinf(half_angle);
    qr.y             = axis.y * sinf(half_angle);
    qr.z             = axis.z * sinf(half_angle);
    qr.w             = cosf(half_angle);
    return qr;
}

v3_t quat_rotate(v4_t q, v3_t v)
{
    v3_t qv = (v3_t){q.x, q.y, q.z};
    return v3_add(v, v3_scale(v3_cross(qv, v3_add(v3_cross(qv, v), v3_scale(v, q.w))), 2.0));
}

#endif
