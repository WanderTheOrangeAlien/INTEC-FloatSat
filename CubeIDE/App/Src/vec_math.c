#include "vec_math.h"
#include "math.h"

Vec3_t Vec_SMult(const Vec3_t *vec, float scalar)
{
    return (Vec3_t){
        .x = vec->x * scalar,
        .y = vec->y * scalar,
        .z = vec->z * scalar,
    };
}

Quaternion_t Quaternion_SMult(const Quaternion_t *q, float s)
{
    return (Quaternion_t){
        .a = q->a *s,
        .b = q->b *s,
        .c = q->c *s,
        .d = q->d *s,
    };
}

float Quaternion_Norm(const Quaternion_t *q)
{
    return sqrtf(q->a*q->a + q->b*q->b + q->c*q->c + q->d*q->d);
}

Quaternion_t Quaternion_Normalize(const Quaternion_t *q)
{
    float norm = Quaternion_Norm(q);
    return Quaternion_SMult(q, 1/norm); 
}

/// @brief Simply adds the scalar component to the quaternion as zero. 
/// X,Y and Z components are kept the same
/// @param vec 
/// @return 
Quaternion_t Vec3_toQuaternion(Vec3_t *vec)
{
    return (Quaternion_t){
        .a = 0,
        .b = vec->x,
        .c = vec->y,
        .d = vec->z
    };
}

Quaternion_t Quaternion_Mult(const Quaternion_t *q1, const Quaternion_t *q2)
{
    return (Quaternion_t){
        .a = (q1->a * q2->a) - (q1->b * q2->b) - (q1->c * q2->c) - (q1->d * q2->d),
        .b = (q1->a * q2->b) + (q1->b * q2->a) + (q1->c * q2->d) - (q1->d * q2->c),
        .c = (q1->a * q2->c) - (q1->b * q2->d) + (q1->c * q2->a) + (q1->d * q2->b),
        .d = (q1->a * q2->d) + (q1->b * q2->c) - (q1->c * q2->b) + (q1->d * q2->a)
    };
}

Quaternion_t Quaternion_Conj(const Quaternion_t *q)
{
    return (Quaternion_t){
        .a = q->a,
        .b = -q->b,
        .c = -q->c,
        .d = -q->d
    };
}

/// @brief Rotate vector using the rotation described by a quaternion
/// @param vec Vector to rotate
/// @param q Rotation quaternion (Must be an unit quaternion)
/// @return 
Vec3_t Vec3_Rotate(const Vec3_t *v, const Quaternion_t *q)
{
    Quaternion_t q_conj = Quaternion_Conj(q);
    Quaternion_t vec = Vec3_toQuaternion(v);

    Quaternion_t temp = Quaternion_Mult(q, &vec);
    temp = Quaternion_Mult(&temp, &q_conj);

    return (Vec3_t) {
        .x = temp.b,
        .y = temp.c,
        .z = temp.d
    };

    
}