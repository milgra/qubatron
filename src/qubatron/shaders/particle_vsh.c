#version 300 es

precision highp float;

in vec3 position;
in vec3 speed;

out vec3 position_out;
out vec3 speed_out;

uniform vec4 basecube;
uniform int  maxlevel;

const float xsft[] = float[8](0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
const float ysft[] = float[8](0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0);
const float zsft[] = float[8](0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0);

const float PI   = 3.1415926535897932384626433832795;
const float PI_2 = 1.57079632679489661923;

void main()
{
    // add gravity to speed, check collosion with speed

    // if speed is big and particle is solid, bounce

    // if speed is slow or not solid particle, stall, particle will be added to static model
}
