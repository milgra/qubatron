#version 310 es

precision highp float;

in vec4 inValue;
out int[12] outOctet;

uniform vec4 fpori[4];
uniform vec3 fpnew[4];

uniform vec4 basecube;

const float xsft[] = float[8](0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
const float ysft[] = float[8](0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0);
const float zsft[] = float[8](0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0);

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

    vec3 pnt = inValue.xyz; // by default point stays in place

    for (int i = 0; i < 2; i++)
    {
	// check if bone belongs to us
	// all bones have to be vertical

	vec3 prp = project_point(fpori[i].xyz, fpori[i + 1].xyz, inValue.xyz);

	if (prp.y < fpori[i].y && prp.y > fpori[i + 1].y)
	{
	    float dop = distance(inValue.xyz, prp); // distance of original point and projection point

	    // check distance

	    if (dop < fpori[i].w)
	    {
		vec3  oldbone = normalize(fpori[i + 1].xyz - fpori[i].xyz);
		vec3  newbone = normalize(fpnew[i + 1] - fpnew[i]);
		vec3  olddir  = inValue.xyz - fpori[i].xyz;
		float bonelen = distance(fpori[i].xyz, fpori[i + 1].xyz); // length of bone

		pnt = fpnew[i] + olddir;

		if (oldbone != newbone)
		{
		    float dbp = distance(fpori[i].xyz, prp); // distance of vector start point and bone projection point

		    float angle   = acos(dot(oldbone, newbone));
		    vec3  axis    = cross(oldbone, newbone);
		    vec4  rotquat = quat_from_axis_angle(normalize(axis), angle);
		    vec3  newdir  = qrot(rotquat, olddir);
		    vec3  newpnt  = fpnew[i] + newdir;

		    if (bonelen - dbp > 6.0)
		    {
			// top part of the bone, affects mesh alone
			pnt = newpnt;
		    }
		    else
		    {
			float part  = 6.0 - (bonelen - dbp);
			float ratio = part / 6.0;

			// lower part of the bone, affected by second bone
			vec3 oldbonea = normalize(fpori[i + 2].xyz - fpori[i + 1].xyz);
			vec3 newbonea = normalize(fpnew[i + 2] - fpnew[i + 1]);

			if (oldbonea != newbonea)
			{
			    vec3  olddira  = inValue.xyz - fpori[i + 1].xyz;
			    float anglea   = acos(dot(oldbonea, newbonea));
			    vec3  axisa    = cross(oldbonea, newbonea);
			    vec4  rotquata = quat_from_axis_angle(normalize(axisa), anglea);
			    vec3  newdira  = qrot(rotquata, olddira);
			    vec3  newpnta  = fpnew[i + 1] + newdira;

			    pnt = newpnt + (newpnta - newpnt) * ratio;
			}
			else
			{
			    pnt = newpnt;
			}
		    }
		}
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
