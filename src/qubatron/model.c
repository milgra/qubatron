// model container

#ifndef model_h
#define model_h

typedef struct model_t
{
    GLfloat* model_vertexes;
    GLfloat* model_colors;
    GLfloat* model_normals;

} model_t;

#endif

#if __INCLUDE_LEVEL__ == 0

void model_load()
{
}

#endif
