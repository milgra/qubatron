#version 310 es

precision highp float;

in vec4 inValue;
out int[12] outOctet;

// TODO change these to quaternion queues for speed
uniform vec3 fpori[3];
uniform vec3 fpnew[3];

uniform vec4 basecube;

const float xsft[] = float[8](0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
const float ysft[] = float[8](0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0);
const float zsft[] = float[8](0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0);

void main()
{
    // go through skeleton point pairs
    // calculate original direction vector from second point
    // calculate angle between new and origin vectors
    // rotate direction vector and add it
    // calculate second original direction vector if affected by other bone
    float d = distance(inValue.xyz, fpori[0]);

    vec3 pnt;
    if (d < 30.0)
    {
	vec3 deltav = inValue.xyz - fpori[0];
	pnt         = fpnew[0] + deltav;
    }
    else
	pnt = inValue.xyz;

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
