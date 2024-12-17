#version 310 es

in highp vec3  inValue;
out highp vec3 outValue;

precision highp float;

uniform vec3 fpori;
uniform vec3 fpnew;

void main()
{
    float d = distance(inValue, fpori);
    vec3  v = fpnew - inValue;

    outValue = inValue + v * (d / 200.0);
};
