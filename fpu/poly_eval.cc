/*============================================================================
 * Written for Bochs (x86 achitecture simulator) by
 *            Stanislav Shwartsman (gate at fidonet.org.il)
 * ==========================================================================*/ 

#define FLOAT128

#include "softfloat.h"

/* arithmetical polynomial evauation */
float128 poly_eval(float128 x, float128 *carr, int n, float_status_t &status)
{
    float128 result = carr[n--];

    for(int i=n-1; i >= 0; i--)
    {
       result = float128_mul(result,      x,  status);
       result = float128_add(result, carr[i], status);
    }

    return result;
}
