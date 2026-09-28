#ifndef INC_VEC_MATH_H
#define INC_VEC_MATH_H

#include "floatsat_types.h"

// Scalar multiplication
Vec3_t Vec_SMult(const Vec3_t *vec, float scalar);

Quaternion_t Vec3_toQuaternion(Vec3_t *vec);
Quaternion_t Quaternion_SMult(const Quaternion_t *q, float s);
float Quaternion_Norm(const Quaternion_t *q);
Quaternion_t Quaternion_Normalize(const Quaternion_t *q);
Quaternion_t Quaternion_Mult(const Quaternion_t *q1, const Quaternion_t *q2);
Quaternion_t Quaternion_Conj(const Quaternion_t *q);

Vec3_t Vec3_Rotate(const Vec3_t *v, const Quaternion_t *q); 


#endif