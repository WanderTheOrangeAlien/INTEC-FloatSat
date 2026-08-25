#include "vec_math.h"


Vec3_t Vec_SMult(const Vec3_t *vec, float scalar)
{
    return (Vec3_t){
        .x = vec->x * scalar,
        .y = vec->y * scalar,
        .z = vec->z * scalar,
    };
}