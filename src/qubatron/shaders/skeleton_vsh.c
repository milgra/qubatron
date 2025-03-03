#version 300 es

precision highp float;

in vec3        position;
flat out ivec4 oct14;
flat out ivec4 oct54;
flat out ivec4 oct94;

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

void main()
{
    // one point can be affected by multiple bones
    // we calculate all new positions, maintain a geometrical center point
    // and finally we move the center point towards the positions based on
    // their weight ( distance from each bone )

    // we will store corner points and ratios

    vec3  corner_points[12];
    float corner_weights[12];
    int   corner_count  = 0;
    vec3  corner_center = position;

    // go through skeleton point pairs

    for (int i = 0; i < 12; i += 2)
    {
	// TODO convert cover shape to capsule, ellipse is too wide

	// calculate position radius, get original and current ratio

	vec3 bone = oldbones[i + 1].xyz - oldbones[i].xyz; // bone vector

	vec3 prad1 = position - oldbones[i].xyz;     // position radius to ellipse focus point 1
	vec3 prad2 = position - oldbones[i + 1].xyz; // position radius to ellipse focus point 2

	float orad = length(bone) + oldbones[i].w;  // original ellipse radius = bone lenght + bone radius
	float prad = length(prad1) + length(prad2); // position radius

	float rat = prad / orad; // radius ratio

	// we are dealing with points inside an 1.3 muiltiplier
	// upper 0.3 will be the gradient force
	// TODO rotation quaternions should be precalculated on the CPU per bone

	if (rat < 1.3)
	{
	    // calculate the new position of position base on current bone

	    // convert portion over 1.0 to a linear ratio between 0.0 and 1.0

	    rat = max(1.0, rat) - 1.0;
	    rat = mix(1.0, 0.0, rat / 0.3);

	    // get angle of original and current bone

	    vec3  ob    = normalize(oldbones[i + 1].xyz - oldbones[i].xyz);
	    vec3  cb    = normalize(newbones[i + 1].xyz - newbones[i].xyz);
	    float angle = acos(dot(ob, cb));

	    // calculate rotation axis

	    vec3 ax  = cross(ob, cb);
	    vec3 odv = position - oldbones[i].xyz; // original direction vector

	    // rotate odv with rotation of bone
	    vec4 rq = quat_from_axis_angle(ob, newbones[i].w); // rotation quaternion
	    odv     = qrot(rq, odv);                           // rotate original dvec with angle on axis

	    vec3 newp;

	    if (ax == vec3(0.0, 0.0, 0.0)) // parallel vectors, using original position
	    {
		newp = newbones[i].xyz + odv;
	    }
	    else
	    {
		vec4 rq    = quat_from_axis_angle(normalize(ax), angle); // rotation quaternion
		vec3 newd0 = qrot(rq, odv);                              // rotate original dvec with angle on axis
		newp       = newbones[i].xyz + newd0;
	    }
	    if (corner_count == 0) corner_center = newp;
	    corner_center += (newp - corner_center) / 2.0;
	    corner_points[corner_count]  = newp;
	    corner_weights[corner_count] = rat;
	    corner_count++;
	}
    }

    // calculate finel position of vertex

    vec3 pnt = corner_center;

    for (int i = 0; i < corner_count; i++)
    {
	vec3 dir = corner_points[i] - corner_center;
	pnt += dir * corner_weights[i];
    }

    //  calculate out octets

    int  octets[12];
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

	octets[level] = octet;
    }

    oct14.x = octets[0];
    oct14.y = octets[1];
    oct14.z = octets[2];
    oct14.w = octets[3];
    oct54.x = octets[4];
    oct54.y = octets[5];
    oct54.z = octets[6];
    oct54.w = octets[7];
    oct94.x = octets[8];
    oct94.y = octets[9];
    oct94.z = octets[10];
    oct94.w = octets[11];
}
