#ifndef shader_h
#define shader_h

// #include <GLES2/gl2.h>
#include <GL/glew.h>

char*  shader_readfile(char* name);
GLuint shader_compile(GLenum type, const GLchar* source);
int    shader_link(GLuint program);
GLuint shader_create(const char* vertex_source, const char* fragment_source, int link);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <stdio.h>
#include <stdlib.h>

char* shader_readfile(char* name)
{
    FILE* f      = fopen(name, "rb");
    char* string = NULL;

    if (f != NULL)
    {

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET); /* same as rewind(f); */

	string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;
    }

    return string;
}

GLuint shader_compile(GLenum type, const GLchar* source)
{
    GLint  status, logLength, realLength;
    GLuint shader = 0;

    status = 0;
    shader = glCreateShader(type);

    if (shader > 0)
    {
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

	if (logLength > 0)
	{
	    GLchar log[logLength];

	    glGetShaderInfoLog(shader, logLength, &realLength, log);

	    printf("Shader compile log: %s\n", log);
	}

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE)
	    return 0;
    }
    else
	printf("Cannot create shader\n");

    return shader;
}

int shader_link(GLuint program)
{
    GLint status, logLength, realLength;

    glLinkProgram(program);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength > 0)
    {
	GLchar log[logLength];
	glGetProgramInfoLog(program, logLength, &realLength, log);
	printf("Program link log : %i %s\n", realLength, log);
    }

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE)
	return 1;
    return 0;
}

GLuint shader_create(const char* vertex_source, const char* fragment_source, int link)
{
    GLuint shader = glCreateProgram();

    GLuint vertex_shader = shader_compile(GL_VERTEX_SHADER, vertex_source);
    if (vertex_shader == 0) printf("Failed to compile vertex shader : %s\n", vertex_source);

    GLuint fragment_shader = shader_compile(GL_FRAGMENT_SHADER, fragment_source);
    if (fragment_shader == 0) printf("Failed to compile fragment shader : %s\n", fragment_source);

    glAttachShader(shader, vertex_shader);
    glAttachShader(shader, fragment_shader);

    if (link == 1)
    {
	int success = shader_link(shader);

	if (success != 1)
	    printf("Failed to link shader program\n");

	if (vertex_shader > 0)
	{
	    glDetachShader(shader, vertex_shader);
	    glDeleteShader(vertex_shader);
	}

	if (fragment_shader > 0)
	{
	    glDetachShader(shader, fragment_shader);
	    glDeleteShader(fragment_shader);
	}
    }

    return shader;
}

#endif
