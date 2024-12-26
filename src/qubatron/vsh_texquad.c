#version 310 es

precision highp float;

in vec3 position;
in vec2 texcoord;

out vec4 gl_Position;
out vec2 varTexcoord;
out vec4 varPos;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(position, 1.0);
    varTexcoord = texcoord;
    varPos      = gl_Position;
};
