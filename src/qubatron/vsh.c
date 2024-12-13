#version 310 es

in vec3 position;

uniform mat4 projection;

out highp vec2 coord;
out highp vec4 gl_Position;

void main()
{
    gl_Position = projection * vec4(position, 1.0);
    coord       = position.xy;
};
