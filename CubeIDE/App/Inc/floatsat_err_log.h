#ifndef FLOATSAT_ERR_LOG_H
#define FLOATSAT_ERR_LOG_H

#include "floatsat_err_log.h"
#include "floatsat_log.h"

#define GOTO_ON_ERR(x,goto_label,ret_val) do{ \
  ret_val = (x);                              \
  if(ret_val != ERR_OK){                      \
    goto goto_label;                          \
  }                                           \
}while(0)

#define GOTO_ON_HAL_ERR(x,goto_label,ret_val) do{ \
  ret_val = (floatsat_err_t)(x);                              \
  ret_val = HAL_ERR_TO_FLOATSAT_ERR(ret_val); \
  if(ret_val != ERR_OK){                      \
    goto goto_label;                          \
  }                                           \
}while(0)

#define GOTO_ON_ERR_LOG(x, goto_label, ret_val, tag, format, ...) do{ \
  ret_val = (x);                              \
  if(ret_val != ERR_OK){                      \
    LOGE(tag, format, ##__VA_ARGS__);         \
    goto goto_label;                          \
  }                                           \
}while(0)

#endif