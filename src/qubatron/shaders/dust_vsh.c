#version 300 es

precision highp float;

in vec3 pos;
in vec3 spd;

out vec3 pos_out;
out vec3 spd_out;

uniform vec4 basecube;
uniform int  maxlevel;
uniform vec3 campos;

float random(vec3 pos)
{
    return fract(sin(dot(pos, vec3(64.25375463, 23.27536534, 86.29678483))) * 59482.7542);
}

void main()
{
    vec3 nspd = spd;
    vec3 npos = pos + spd;

    if (length(campos - npos) < 100.0) npos += npos - campos;

    if (npos.x < 400.0) npos.x = 800.0;
    if (npos.y < 0.0) npos.y = 300.0;
    if (npos.z < 0.0) npos.z = 400.0;

    if (npos.x > 800.0) npos.x = 400.0;
    if (npos.y > 300.0) npos.y = 0.0;
    if (npos.z > 400.0) npos.z = 0.0;

    pos_out = npos;
    spd_out = nspd;
}
