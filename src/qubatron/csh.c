#version 310 es

precision highp float;

in vec4 inValue;
out int[12] outOctet;

uniform vec3 fpori;
uniform vec3 fpnew;

uniform vec4 basecube;

const float xsft[] = float[8](0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
const float ysft[] = float[8](0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0);
const float zsft[] = float[8](0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0);

void main()
{
    float d = distance(inValue.xyz, fpori);
    vec3  v = fpnew - inValue.xyz;

    vec4 pnt = vec4(inValue.xyz + v * (300.0 - d) / 300.0, 1.0);

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
