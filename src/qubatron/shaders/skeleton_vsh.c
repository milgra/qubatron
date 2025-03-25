#version 300 es

precision highp float;

in vec3 position;
in vec3 normal;

flat out int  octets_out[12];
flat out vec3 normal_out;

uniform vec4 oldbones[12];
uniform vec4 newbones[12];

uniform vec4 basecube;
uniform int  maxlevel;

const float xsft[] = float[8](0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
const float ysft[] = float[8](0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0);
const float zsft[] = float[8](0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0);

const float PI   = 3.1415926535897932384626433832795;
const float PI_2 = 1.57079632679489661923;

// A line point 0
// B line point 1
// C point to project

vec3 project_point(vec3 A, vec3 B, vec3 C)
{
    vec3 AC = C - A;
    vec3 AB = B - A;
    return A + dot(AC, AB) / dot(AB, AB) * AB;
}

// A quaternion is a 4D object defined as follows:
// q = [s, v]
// q = [s + xi + yj + zk] // with imaginary parts
// q = [x, y, z, w] // in programming, where w = s

// transform a rotation axis into a quaternion

vec4 quat_from_axis_angle(vec3 axis, float angle)
{
    vec4  qr;
    float half_angle = angle * 0.5;
    qr.x             = axis.x * sin(half_angle);
    qr.y             = axis.y * sin(half_angle);
    qr.z             = axis.z * sin(half_angle);
    qr.w             = cos(half_angle);
    return qr;
}

// P1 = q P0 q-1
// where P1 is the rotated point and q-1 is the inverse of the UNIT quaternion q.

vec3 qrot(vec4 q, vec3 v)
{
    /* vec4 q_conj = quat_conj(q); // inverse quaternion is the conjugate quaternion */
    /* vec4 q_pos  = vec4(v.x, v.y, v.z, 0); */
    /* vec4 q_tmp  = quat_mult(q, q_pos); */
    /* q           = quat_mult(q_tmp, q_conj); */
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

// TODO rotation quaternions should be precalculated on the CPU per bone

void main()
{
    // one point can be affected by multiple bones
    // we calculate all new positions, maintain a geometrical center point
    // and finally we move the center point towards the positions based on
    // their weight ( distance from each bone )

    // we will store corner points and ratios

    vec3  corner_points[12];
    vec3  corner_normals[12];
    float corner_tozerow[12]; // distance to zero weight from center
    int   corner_count  = 0;
    vec3  corner_center = position;
    float tozerow_sum   = 0.0; // summary of to zero weight lengths from center

    // go through skeleton point pairs

    for (int i = 0; i < 6; i += 2)
    {
	vec3 oldbone               = oldbones[i + 1].xyz - oldbones[i].xyz; // bone vector
	vec3 oldbone_midp          = oldbones[i].xyz + oldbone / 2.0;
	vec3 point_on_oldbone      = project_point(oldbones[i].xyz, oldbones[i + 1].xyz, position);
	vec3 point_on_oldbone_v    = point_on_oldbone - oldbones[i].xyz;
	vec3 point_from_oldbone_v  = position - point_on_oldbone;
	vec3 point_from_halfbone_v = point_on_oldbone - oldbone_midp;

	vec3  point_from_center = position - oldbones[i].xyz;
	float effect_distance   = oldbones[i].w;

	float dist = 10000.0;
	if (length(point_from_halfbone_v) < length(oldbone) / 2.0)
	{
	    dist = length(point_from_oldbone_v);
	}
	else
	{
	    dist = min(length(position - oldbones[i].xyz), length(position - oldbones[i + 1].xyz));
	}

	// rectangle check
	float diff = dist - effect_distance;
	if (diff < 0.0)
	{
	    // default values

	    vec3 currbone              = newbones[i + 1].xyz - newbones[i].xyz;
	    vec3 currnormal            = normal;
	    vec3 point_on_currbone_v   = point_on_oldbone_v;
	    vec3 point_from_currbone_v = point_from_oldbone_v;

	    float remdist = effect_distance - dist;

	    /* float bone_ratio = length(currbone) / length(oldbone); // current bone ratio to old bone */
	    /* bone_ratio       = 1.0; */

	    // calculate the new position of position base on current bone
	    // get angle of original and current bone

	    vec3 oldbone_norm  = normalize(oldbone);
	    vec3 currbone_norm = normalize(currbone);

	    // rotate point_from_oldbone_v with rotation of bone

	    vec4 oldbone_rot_quat = quat_from_axis_angle(oldbone_norm, newbones[i].w); // rotation quaternion
	    point_from_currbone_v = qrot(oldbone_rot_quat, point_from_oldbone_v);
	    currnormal            = qrot(oldbone_rot_quat, normal); // rotate original normal with angle on axis

	    // if bones are not parallel, rotate vector and normal with bones axis

	    /* vec3 point_on_currbone_v = point_on_oldbone_v * bone_ratio; */

	    float bones_dot   = dot(oldbone_norm, currbone_norm);
	    float bones_angle = acos(bones_dot);
	    vec3  bones_axis  = cross(oldbone_norm, currbone_norm);

	    if (length(bones_axis) > 0.000001)
	    {
		vec4 bonesangle_rot_quat = quat_from_axis_angle(normalize(bones_axis), bones_angle); // rotation quaternion
		point_on_currbone_v      = qrot(bonesangle_rot_quat, point_on_currbone_v);
		point_from_currbone_v    = qrot(bonesangle_rot_quat, point_from_currbone_v);
		currnormal               = qrot(bonesangle_rot_quat, currnormal);
	    }

	    vec3 currpos = newbones[i].xyz + point_on_currbone_v + point_from_currbone_v;

	    if (corner_count == 0) corner_center = currpos;
	    corner_center += (currpos - corner_center) / 2.0;
	    corner_points[corner_count]  = currpos;
	    corner_normals[corner_count] = currnormal;
	    corner_tozerow[corner_count] = remdist;
	    corner_count++;
	    tozerow_sum += remdist;
	}
    }

    // calculate final position of vertex based on tozerow distances

    vec3 pnt = corner_center;
    vec3 nrm = corner_normals[0];

    if (corner_count > 1)
    {
	for (int i = 0; i < corner_count; i++)
	{
	    float rat = corner_tozerow[i] / tozerow_sum;
	    vec3  dir = corner_points[i] - corner_center;
	    pnt += dir * rat;
	    nrm = (nrm + corner_normals[i]) / 2.0;
	}
    }

    normal_out = nrm;

    //  calculate out octets

    vec4 cube = basecube;

    for (int level = 0; level < maxlevel; level++)
    {
	// get octet index

	float size = cube.w / 2.0;

	int octet = int(floor(pnt.x / size)) % 2;
	int yi    = int(floor(pnt.y / size)) % 2;
	int zi    = int(floor(pnt.z / size)) % 2;

	if (yi == 0) octet += 2;
	if (zi == 0) octet += 4;

	cube = vec4(
	    cube.x + xsft[octet] * size,
	    cube.y - ysft[octet] * size,
	    cube.z - zsft[octet] * size,
	    size);

	octets_out[level] = octet;
    }
}
