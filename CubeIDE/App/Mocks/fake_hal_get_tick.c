#include "fake_hal_get_tick.h"

#ifdef TEST
uint32_t HAL_GetTick(void)
{
    static count = 0;
    return count++;
}
#endif
