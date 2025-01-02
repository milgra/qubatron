#version 310 es

precision highp float;

in vec4 inValue;
out int[12] outOctet;

uniform vec4 fpori[12];
uniform vec3 fpnew[12];

uniform vec4 basecube;

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
    // go through skeleton point pairs
    // calculate original direction vector from second point
    // calculate angle between new and origin vectors
    // rotate direction vector and add it
    // calculate second original direction vector if affected by other bone

    vec3 pnt         = inValue.xyz; // by default point stays in place
    int  point_count = 12;

    for (int i = 0; i < point_count; i += 2)
    {
	vec3 fpd1 = inValue.xyz - fpori[i].xyz;
	vec3 fpd2 = inValue.xyz - fpori[i + 1].xyz;
	vec3 bone = fpori[i + 1].xyz - fpori[i].xyz;

	float orad = length(bone) + fpori[i].w;
	float nrad = length(fpd1) + length(fpd2);
	float rat  = nrad / orad;

	if (rat < 1.2)
	{
	    if (rat > 1.0)
		rat = (1.3 - rat) / 0.3;
	    else
		rat = 1.0;

	    vec3 oldd0 = inValue.xyz - fpori[i].xyz;
	    /* vec3 oldd1 = inValue.xyz - fpori[i + 1].xyz; */

	    vec3 oldb0 = normalize(fpori[i + 1].xyz - fpori[i].xyz);
	    vec3 newb0 = normalize(fpnew[i + 1] - fpnew[i]);
	    /* vec3 oldb1 = normalize(fpori[i + 2].xyz - fpori[i + 1].xyz); */
	    /* vec3 newb1 = normalize(fpnew[i + 2] - fpnew[i + 1]); */

	    float angle0 = acos(dot(oldb0, newb0));
	    /* float angle1 = acos(dot(oldb1, newb1)); */

	    vec3 axis0 = normalize(cross(oldb0, newb0));
	    /* vec3 axis1 = normalize(cross(oldb1, newb1)); */

	    // TODO rotation quaternions should be precalculated on the CPU per bone

	    vec4 rotq0 = quat_from_axis_angle(axis0, angle0);
	    /* vec4 rotq1 = quat_from_axis_angle(axis1, angle1); */

	    vec3 newd0 = qrot(rotq0, oldd0);
	    /* vec3 newd1 = qrot(rotq1, oldd1); */

	    vec3 newp0 = fpnew[i] + newd0;
	    /* vec3 newp1 = fpnew[i + 1] + newd1; */

	    /* pnt = newp0 + (newp1 - newp0) * ty; */
	    vec3 newpnt = inValue.xyz + (newp0 - inValue.xyz) * rat;
	    if (pnt == inValue.xyz)
		pnt = newpnt;
	    else
		pnt = pnt + (newpnt - pnt) / 2.0;
	}

	/* if (fpori[i + 2].w == 0.0) i += 2; */
    }

    int  levels = 12;
    vec4 cube   = basecube;

    for (int level = 0; level < levels; level++)
    {
	// get octet index

	float size = cube.w / 2.0;

	int octet = int(floor(pnt.x / size)) % 2;
	int yi    = int(floor(pnt.y / size)) % 2;
	int zi    = int(floor(-1.0 * pnt.z / size)) % 2;

	if (yi == 0) octet += 2;
	if (zi == 1) octet += 4;

	cube = vec4(
	    cube.x + xsft[octet] * size,
	    cube.y - ysft[octet] * size,
	    cube.z - zsft[octet] * size,
	    size);

	outOctet[level] = octet;
    }
}
