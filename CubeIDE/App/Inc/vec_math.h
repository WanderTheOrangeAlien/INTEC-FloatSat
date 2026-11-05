#ifndef INC_VEC_MATH_H
#define INC_VEC_MATH_H

#include <math.h>
#include "floatsat_types.h"


#define QUATERNION_IDENTITY() ((Quaternion_t){1,0,0,0})


Vec3_t Vec_SMult(const Vec3_t *vec, float scalar); // Scalar multiplication
float Vec_GetAzimuth(const Vec3_t *vec);

Quaternion_t Vec3_toQuaternion(const Vec3_t *vec);
Quaternion_t Quaternion_SMult(const Quaternion_t *q, float s);
float Quaternion_Norm(const Quaternion_t *q);
Quaternion_t Quaternion_Normalize(const Quaternion_t *q);
Quaternion_t Quaternion_Mult(const Quaternion_t *q1, const Quaternion_t *q2);
Quaternion_t Quaternion_Conj(const Quaternion_t *q);
bool Quaternion_ContainsNaN(const Quaternion_t *q);


Vec3_t Vec3_Rotate(const Vec3_t *v, const Quaternion_t *q); 

/* ==================== Non vector math ==================== */
float wrap_180(float in);
float saturate_symmetric(float in, float limit);


#endif