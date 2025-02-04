#version 300 es
precision highp float;

in vec3      position;
out vec2     coord;
uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(position, 1.0);
    coord       = position.xy;
}
