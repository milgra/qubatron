#version 310 es

precision highp float;

in vec4 inValue;
out int[12] outOctet;

uniform vec3 fpori[3];
uniform vec3 fpnew[3];

uniform vec4 basecube;

const float xsft[] = float[8](0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
const float ysft[] = float[8](0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0);
const float zsft[] = float[8](0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0);

// A line point 0
// B line point 1
// C point to project

vec3 project_point(vec3 A, vec3 B, vec3 C)
{
    vec3  d = (C - B) / distance(C, B); // normalized direction vector
    vec3  v = A - B;                    // line vector
    float t = dot(v, d);                // dot ( projection ) product
    return B + t * d;
    /* return B + dot(A - B, normalize(C - B)) * D; */
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

    vec3 pnt = inValue.xyz;

    float d = distance(inValue.xyz, fpori[0]);

    if (d < 30.0)
    {
	float ratio = d / 30.0;
	for (int i = 0; i < 2; i++)
	{
	    vec3 oldbone = normalize(fpori[i + 1] - fpori[i]);
	    vec3 newbone = normalize(fpnew[i + 1] - fpnew[i]);
	    vec3 olddir  = inValue.xyz - fpori[i];

	    if (oldbone != newbone)
	    {
		float newboneangle = acos(dot(oldbone, newbone));
		vec3  axis         = cross(oldbone, newbone);
		/* rotate olddir with newboneangle on axis to get new position */
		vec4 rotquat  = quat_from_axis_angle(axis, newboneangle);
		vec3 newdir   = qrot(rotquat, olddir);
		vec3 newpoint = fpnew[i] + newdir;

		if (i == 0)
		    pnt = newpoint;
		else
		    pnt = pnt + (newpoint - pnt) * ratio;
	    }
	    else
	    {
		vec3 newpoint = fpnew[i] + olddir;
		if (i == 0)
		    pnt = newpoint;
		else
		    pnt = pnt + (newpoint - pnt) * ratio;
	    }
	}
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
