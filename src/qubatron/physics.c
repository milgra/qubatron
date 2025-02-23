#ifndef physics_h
#define physics_h

#include "mt_vector.c"
#include "mt_vector_2d.c"
#include <math.h>
#include <stdio.h>

// mass point

typedef struct _mass_t mass_t;
struct _mass_t
{
    v2_t trans;
    v2_t basis;

    float weight;
    float radius;
    float elasticity;
};

mass_t* mass_alloc(v2_t position, float radius, float weight, float elasticity);

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

dres_t*    dres_alloc(mass_t* mass_a, mass_t* mass_b, float distance, float elasticity);
void       dres_resetdistance(dres_t* dguard);
void       dres_new(dres_t* dguard, float delta);

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

mass_t* mass_alloc(v2_t position, float radius, float weight, float elasticity)
{
    mass_t* result = CAL(sizeof(mass_t), mass_dealloc, NULL);

    result->trans = position;
    result->basis = v2_init(0.0, 0.0);

    result->radius     = radius;
    result->weight     = weight;
    result->elasticity = elasticity;

    return result;
}

/* creates distance resolver */

dres_t* dres_alloc(mass_t* mass_a, mass_t* mass_b, float distance, float elasticity)
{
    float sumweight = mass_a->weight + mass_b->weight;

    dres_t* dguard = CAL(sizeof(dres_t), NULL, NULL);

    dguard->mass_a     = mass_a;
    dguard->mass_b     = mass_b;
    dguard->distance   = distance;
    dguard->elasticity = elasticity;
    dguard->ratio_a    = mass_b->weight / sumweight;
    dguard->ratio_b    = mass_a->weight / sumweight;

    return dguard;
}

/* resets distance based on actual mass positions */

void dres_resetdistance(dres_t* dguard)
{
    v2_t vector      = v2_sub(dguard->mass_b->trans, dguard->mass_a->trans);
    dguard->distance = v2_length(vector);
}

/* adds forces to masses in dguard to keep up distance */

void dres_new(dres_t* dguard, float ratio)
{

    v2_t massabasis = v2_scale(dguard->mass_a->basis, ratio);
    v2_t massbbasis = v2_scale(dguard->mass_b->basis, ratio);

    v2_t transa = v2_add(dguard->mass_a->trans, massabasis);
    v2_t transb = v2_add(dguard->mass_b->trans, massbbasis);

    v2_t connector = v2_sub(transa, transb);

    float delta = v2_length(connector) - dguard->distance;

    if (dguard->elasticity > 0.0) delta /= dguard->elasticity;

    assert(!isinf(delta));

    if (fabs(delta) > 0.01)
    {
	v2_t a                = v2_resize(connector, -delta * dguard->ratio_a);
	v2_t b                = v2_resize(connector, delta * dguard->ratio_b);
	dguard->mass_a->basis = v2_add(dguard->mass_a->basis, a);
	dguard->mass_b->basis = v2_add(dguard->mass_b->basis, b);
    }
}

#endif
