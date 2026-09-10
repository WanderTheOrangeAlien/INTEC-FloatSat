#ifndef TELECOMMAND_PARSER_H
#define TELECOMMAND_PARSER_H

#include <stdint.h>
#include "floatsat_types.h"
#include "floatsat_error.h"

#include "argtable3.h"

#define CMD_PARSER_MAX_ARGS         16
#define CMD_MAX_ERRORS              20

#define CMD_CONTROL_N_SUBCMD        2   // Number of subcommands for the control command
#define CMD_MISSION_N_SUBCMD        2   //  Number of subcommands for the mission command
#define CMD_PHOTO_N_SUBCMD          3   //  Number of subcommands for the photo command

typedef struct parser_ctx_t {
    int argc;
    char *argv[CMD_PARSER_MAX_ARGS];
}parser_ctx_t;


typedef struct photo_info_t {
    float angle;
    floatsat_time_t time;
    uint32_t duration;

}photo_info_t;

typedef floatsat_err_t(*cmd_handler_t)(parser_ctx_t *ctx, floatsat_cmd_t *cmd);

typedef struct cmd_handler_pair_t {
    const char *name;
    cmd_handler_t handler;
}cmd_handler_pair_t;


floatsat_err_t CmdParser_Init(parser_ctx_t *ctx);
floatsat_err_t CmdParser_Parse(parser_ctx_t *ctx, uint8_t *msg_body, size_t len, floatsat_cmd_t *cmd);


#ifdef TEST
STATIC floatsat_err_t CmdParser_GetArgs(parser_ctx_t *ctx, uint8_t *msg_body, size_t len);
STATIC floatsat_err_t parse_control_params(float *params, const char *buffer);


#endif



#endif