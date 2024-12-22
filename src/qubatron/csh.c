#version 310 es

in highp vec4  inValue;
out highp vec4 outValue;
out ivec3      outOctets;

precision highp float;

uniform vec3 fpori;
uniform vec3 fpnew;

void main()
{
    float d = distance(inValue.xyz, fpori);
    vec3  v = fpnew - inValue.xyz;

    outValue = vec4(inValue.xyz + v * (300.0 - d) / 300.0, 1.0);

    outOctets.x = 10;
    outOctets.y = 11;
    outOctets.z = 12;
}
