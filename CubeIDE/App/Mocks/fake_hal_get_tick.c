#include "fake_hal_get_tick.h"

uint32_t HAL_GetTick(void)
{
    static count = 0;
    return count++;
}