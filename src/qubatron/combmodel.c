// combined model for dynamic model combination into one model
#ifndef combmodel_h
#define combmodel_h

#include "model.c"
#include "mt_vector.c"

typedef struct _combmodel_t
{
    mt_vector_t models;
    model_t*    model
} combmodel_t;

combmodel_t combmodel_init();
void combmodel_add(combmodel_t* combmodel, model_t* model);
void combmodel_remove(combmodel_t* combmodel, model_t* model);

#endif

#if __INCLUDE_LEVEL__ == 0

#endif
