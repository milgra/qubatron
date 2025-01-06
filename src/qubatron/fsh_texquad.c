#version 300 es

precision highp float;

in vec2  varTexcoord;
in vec4  varPos;
out vec4 fragColor;

uniform sampler2D texture_base;

void main()
{
    fragColor = texture2D(texture_base, varTexcoord);
};
