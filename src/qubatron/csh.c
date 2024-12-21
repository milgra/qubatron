#version 310 es

in highp vec4  inValue;
out highp vec4 outValue;

precision highp float;

uniform vec3 fpori;
uniform vec3 fpnew;

void main()
{
    float d = distance(inValue.xyz, fpori);
    vec3  v = fpnew - inValue.xyz;

    outValue = vec4(inValue.xyz + v * (300.0 - d) / 300.0, 1.0);
};
