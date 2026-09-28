
#ifdef TEST

#include "unity.h"

#include "vec_math.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_quaternion_rotation(void)
{
    Vec3_t vec = {0,0,1};
    Quaternion_t rot = {1,2,3,4};
    rot = Quaternion_Normalize(&rot);
    
    Vec3_t res = Vec3_Rotate(&vec, &rot);
    Vec3_t expected = {0.733333,0.666667,0.133333};
    TEST_ASSERT_FLOAT_ARRAY_WITHIN(0.00001f,(float*)&expected, &res, 3);

    vec = (Vec3_t){0,0,1};
    rot = (Quaternion_t){0.275537, -0.837815, -0.461108, 0.097595};
    rot = Quaternion_Normalize(&rot);
    
    res = Vec3_Rotate(&vec, &rot);
    expected = (Vec3_t){-0.417638, 0.371694, -0.829109};
    TEST_ASSERT_FLOAT_ARRAY_WITHIN(0.00001f,(float*)&expected, &res, 3);

    vec = (Vec3_t){1,1,1};
    rot = (Quaternion_t){0.699103, -0.022364, 0.459191, -0.547630};
    rot = Quaternion_Normalize(&rot);
    
    res = Vec3_Rotate(&vec, &rot);
    expected = (Vec3_t){1.390189, -0.858700, -0.574465};
    TEST_ASSERT_FLOAT_ARRAY_WITHIN(0.00001f,(float*)&expected, &res, 3);


}

#endif // TEST
