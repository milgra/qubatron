#version 310 es

in highp vec3 position;
in highp vec2 texcoord;

out highp vec4 gl_Position;
out highp vec2 varTexcoord;
out highp vec4 varPos;

void main()
{
    gl_Position = vec4(position, 1.0);
    varTexcoord = texcoord;
    varPos      = gl_Position;
};
