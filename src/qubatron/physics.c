#ifndef physics_h
#define physics_h

#include "mt_vector.c"
#include "mt_vector_3d.c"
#include <math.h>
#include <stdio.h>

// mass point

typedef struct _mass_t mass_t;
struct _mass_t
{
    v3_t trans;
    v3_t basis;

    float weight;
    float radius;
    float elasticity;
};

mass_t* mass_alloc(v3_t position, float radius, float weight, float elasticity);

// distance resolver

typedef struct _dres_t dres_t;
struct _dres_t
{
    mass_t* mass_a;
    mass_t* mass_b;

    float distance;
    float elasticity;

    float ratio_a;
    float ratio_b;
};

dres_t*    dres_alloc(mass_t* mass_a, mass_t* mass_b, float elasticity);
void       dres_resetdistance(dres_t* dguard);
void       dres_update(dres_t* dguard, float delta);

#endif

#if __INCLUDE_LEVEL__ == 0

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
    #define M_PI_2 3.14159265358979323846 * 2
#endif

/* dealloc mass point */

void mass_dealloc(void* pointer)
{
    mass_t* mass = pointer;
}

/* creates mass point */

mass_t* mass_alloc(v3_t position, float radius, float weight, float elasticity)
{
    mass_t* result = CAL(sizeof(mass_t), mass_dealloc, NULL);

    result->trans = position;
    result->basis = v3_init(0.0, 0.0, 0.0);

    result->radius     = radius;
    result->weight     = weight;
    result->elasticity = elasticity;

    return result;
}

/* creates distance resolver */

dres_t* dres_alloc(mass_t* mass_a, mass_t* mass_b, float elasticity)
{
    float sumweight = mass_a->weight + mass_b->weight;

    dres_t* dguard = CAL(sizeof(dres_t), NULL, NULL);

    dguard->mass_a     = mass_a;
    dguard->mass_b     = mass_b;
    dguard->distance   = v3_length(v3_sub(mass_a->trans, mass_b->trans));
    dguard->elasticity = elasticity;
    dguard->ratio_a    = mass_b->weight / sumweight;
    dguard->ratio_b    = mass_a->weight / sumweight;

    return dguard;
}

/* resets distance based on actual mass positions */

void dres_resetdistance(dres_t* dguard)
{
    v3_t vector      = v3_sub(dguard->mass_b->trans, dguard->mass_a->trans);
    dguard->distance = v3_length(vector);
}

/* adds forces to masses in dguard to keep up distance */

void dres_update(dres_t* dguard, float ratio)
{

    v3_t massabasis = v3_scale(dguard->mass_a->basis, ratio);
    v3_t massbbasis = v3_scale(dguard->mass_b->basis, ratio);

    v3_t transa = v3_add(dguard->mass_a->trans, massabasis);
    v3_t transb = v3_add(dguard->mass_b->trans, massbbasis);

    v3_t connector = v3_sub(transa, transb);

    float delta = v3_length(connector) - dguard->distance;

    if (dguard->elasticity > 0.0) delta *= dguard->elasticity;

    assert(!isinf(delta));

    if (fabs(delta) > 0.01)
    {
	v3_t a                = v3_resize(connector, -delta * dguard->ratio_a);
	v3_t b                = v3_resize(connector, delta * dguard->ratio_b);
	dguard->mass_a->basis = v3_add(dguard->mass_a->basis, a);
	dguard->mass_b->basis = v3_add(dguard->mass_b->basis, b);
    }
}

#endif
