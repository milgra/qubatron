#version 310 es

precision highp float;

in vec3 position;

uniform mat4 projection;

out vec2 coord;
out vec4 gl_Position;

void main()
{
    gl_Position = projection * vec4(position, 1.0);
    coord       = position.xy;
};
