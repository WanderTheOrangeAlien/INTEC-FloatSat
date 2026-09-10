#include "floatsat_time.h"


floatsat_err_t Time_ParseString(const char *buffer, floatsat_time_t *time)
{
    if(!buffer || !time){
        return ERR_INVALID_ARG;
    }
    
    unsigned int h, m, s; 
    *time = (floatsat_time_t){0}; 

    int ret = sscanf(buffer,"%u:%u:%u", &h, &m, &s);
    
    if(ret < 3 || ret == EOF){
        return ERR_INVALID_TIME;
    }
    
    if(h > 23 || m > 59 || s > 59){
        return ERR_INVALID_TIME;
    }

    time->hours     = h;
    time->minutes   = m;
    time->seconds   = s;

    return ERR_OK;
}