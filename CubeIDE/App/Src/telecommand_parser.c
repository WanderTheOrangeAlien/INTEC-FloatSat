#include "telecommand_parser.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "floatsat_time.h"

STATIC floatsat_err_t CmdParser_GetArgs(parser_ctx_t *ctx, uint8_t *msg_body, size_t len);

static floatsat_err_t parse_cmd_control(parser_ctx_t *ctx, floatsat_cmd_t *cmd);
STATIC floatsat_err_t parse_control_params(float *params, const char *buffer);

STATIC floatsat_err_t parse_cmd_mission(parser_ctx_t *ctx, floatsat_cmd_t *cmd);

static floatsat_err_t parse_cmd_photo(parser_ctx_t *ctx, floatsat_cmd_t *cmd);

static int cmd_init_control();
static int cmd_init_mission();
static int cmd_init_photo();

static const char *LOG_TAG = "CMD_PARSER";

/* ========================================================================== */
/* =============================== Arg Tables =============================== */
/* ========================================================================== */

/* =========================== Control command =========================== */
// Format: control [-i]
static struct {
    // struct arg_rex *cmd_name;
    struct arg_lit *info;
    struct arg_end *end;
}argtable_control_info;

// Format: control [-t <type>] <params>
static struct {
    struct arg_str *type;
    struct arg_str *params;
    struct arg_end *end;
}argtable_control_set;

static struct {
    struct arg_dbl *angle;
    struct arg_end *end;
}argtable_control_angle;

/* =========================== Mission command =========================== */
// Format: misson -i
static struct {
    struct arg_lit *info;
    struct arg_end *end;

}argtable_mission_info;

// Format: mission <mission_id>
static struct {
    struct arg_int *mission_id;
    struct arg_end *end;
}argtable_mission_set;

/* =========================== Photo command =========================== */
// Format: photo -i
static struct {
    struct arg_lit *info;
    struct arg_end *end;
}argtable_photo_info;

// Format: photo add -a <angle> -t <time> -d <duration>
static struct {
    struct arg_rex *subcmd;
    struct arg_dbl *angle;
    struct arg_str *time;
    struct arg_int *duration;
    struct arg_end *end;
}argtable_photo_add;

// Format: photo reset
static struct {
    struct arg_rex *subcmd;
    struct arg_end *end;
}argtable_photo_reset;



static const cmd_handler_pair_t cmd_handler_table[] = {
    {"control",     parse_cmd_control},
    {"mission",     parse_cmd_mission},
    {"photo",       parse_cmd_photo},
};
static const size_t cmd_handler_table_len = sizeof(cmd_handler_table) / sizeof(cmd_handler_pair_t);

floatsat_err_t CmdParser_Init(parser_ctx_t *ctx)
{
#ifndef TEST
    static_assert(sizeof(float) == sizeof(void*), "");
#endif
    if(!ctx){
        return ERR_INVALID_ARG;
    }

    memset(ctx,0,sizeof(parser_ctx_t));

    cmd_init_control();
    cmd_init_mission();
    cmd_init_photo();

    return ERR_OK;
}


floatsat_err_t CmdParser_Parse(parser_ctx_t *ctx, uint8_t *msg_body, size_t len, floatsat_cmd_t *cmd)
{
    if(!ctx || !msg_body || len == 0 || !cmd){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret;

    memset(cmd, 0, sizeof(floatsat_cmd_t));

    GOTO_ON_ERR(CmdParser_GetArgs(ctx,msg_body,len), err, ret);

    // Search for the appropiate handler based on the command name
    for (size_t i = 0; i < cmd_handler_table_len; i++)
    {
        if(strcmp(ctx->argv[0],cmd_handler_table[i].name) == 0 && cmd_handler_table[i].handler){
            return cmd_handler_table[i].handler(ctx, cmd);
        }
    }

    return ERR_INVALID_CMD;

err:
    return ret;
}


#pragma region init
/* ========================================================================== */
/* ================================= CMD Init =============================== */
/* ========================================================================== */

static int cmd_init_control()
{
    // Format: control -i
    // argtable_control_info.cmd_name      =   arg_rex1(NULL,NULL,"control",NULL,ARG_REX_ICASE,NULL);
    argtable_control_info.info          =   arg_lit1("i","info","Show control information");
    argtable_control_info.end           =   arg_end(CMD_MAX_ERRORS);

    // Format: control [-t <type>] -p <params>
    argtable_control_set.type           =   arg_str0("t", "type", "PID|LQR", "Controller type");
    argtable_control_set.params         =   arg_str1("p","params","<float>,<float>...",NULL);
    argtable_control_set.end            =   arg_end(CMD_MAX_ERRORS);

    // Format: control -a <angle>
    argtable_control_angle.angle        =   arg_dbl1("a","angle","float","Target azimuth angle");
    argtable_control_angle.end          =   arg_end(CMD_MAX_ERRORS);

    // TODO: Check for NULL pointers!

    return 0;
}

static int cmd_init_mission()
{
    // Format: mission -i
    argtable_mission_info.info          =   arg_lit1("i","info","Show mission information");
    argtable_mission_info.end           =   arg_end(CMD_MAX_ERRORS);

    // Format: mission <mission_id>'
    argtable_mission_set.mission_id     =   arg_int1(NULL,NULL,"mission_id_t",NULL);
    argtable_mission_set.end            =   arg_end(CMD_MAX_ERRORS);

    // TODO: Check for NULL pointers!

    return 0;
}

static int cmd_init_photo(){

    // Format: photo -i
    argtable_photo_info.info            =   arg_lit1("i","info","Show photo information");
    argtable_photo_info.end             =   arg_end(CMD_MAX_ERRORS);

    // Format: photo add -a <angle> -t <time> -d <duration>
    argtable_photo_add.subcmd           =   arg_rex1(NULL, NULL, "add", NULL, ARG_REX_ICASE, NULL);
    argtable_photo_add.angle            =   arg_dbl1("a", "angle","double", "Azimuth angles of the photo");
    argtable_photo_add.time             =   arg_str1("t", "time", "HH:mm:ss",
                                                        "Moment to take the photo. Specified as HH:mm:ss of Standard Atlantic Time (UTC-4)");
    argtable_photo_add.duration         =   arg_int1("d", "duration", "uint", "Duration in milliseconds");
    argtable_photo_add.end              =   arg_end(CMD_MAX_ERRORS);

    // Format: photo reset
    argtable_photo_reset.subcmd         =   arg_rex1(NULL, NULL, "reset", NULL, ARG_REX_ICASE, "Reset subcommand");
    argtable_photo_reset.end            =   arg_end(CMD_MAX_ERRORS);


    return 0;
}


#pragma endregion


#pragma region handlers
/* ========================================================================== */
/* ============================== CMD Handlers ============================== */
/* ========================================================================== */

/* =========================== Control command =========================== */

static floatsat_err_t parse_cmd_control(parser_ctx_t *ctx, floatsat_cmd_t *cmd)
{
    if(!ctx || ! cmd){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret = ERR_OK;
    int nerrors_arr[CMD_CONTROL_N_SUBCMD] = {0};

    nerrors_arr[0] = arg_parse(ctx->argc,ctx->argv, (void**)&argtable_control_info);
    nerrors_arr[1] = arg_parse(ctx->argc,ctx->argv, (void**)&argtable_control_set);
    nerrors_arr[2] = arg_parse(ctx->argc,ctx->argv, (void**)&argtable_control_angle);


    *cmd = (floatsat_cmd_t){0}; 

    if(!nerrors_arr[0]){
        // Format: control [-i]
        cmd->cmd_id = CMD_ID_CONTROL_INFO;
    
    }else if(!nerrors_arr[1]){
        // Format: control [-t <type>] <params>
        cmd->cmd_id = CMD_ID_CONTROL_SET;
        
        // TODO: Is there a way to avoid malloc? 
        cmd->params = malloc(sizeof(floatsat_args_set_control_t)); 

        if(cmd->params == NULL){
            return ERR_OUT_OF_MEM;
        }

        // Typed pointer shortcut to avoid casting at every call 
        floatsat_args_set_control_t *cmd_params = (floatsat_args_set_control_t*)cmd->params;

        // Check for controller type
        if(argtable_control_set.type->count > 0){

            if(strcmp(argtable_control_set.type->sval[0],"PID") == 0){
                // PID controller type selected
                cmd_params->control_type = CONTROL_TYPE_PID;

            }else if(strcmp(argtable_control_set.type->sval[0],"LQR") == 0){
                // LQR controller type selected
                cmd_params->control_type = CONTROL_TYPE_LQR;

            }else{
                // Invalid type
                LOGE(LOG_TAG,"Invalid controller type: %s", argtable_control_set.type->sval[0]);
                return ERR_INVALID_CMD;
            }
        }else{
            cmd_params->control_type = CONTROL_TYPE_NO_CHANGE;
        }
        
        // Parse control params into 
        ret = parse_control_params(cmd_params->control_params, argtable_control_set.params->sval[0]);

        if(ret != ERR_OK){
            LOGE(LOG_TAG,"Control set: Invalid parameter string. Error: 0x%04x",ret);
            free(cmd->params);
            cmd->params = NULL;
            return ret;
        }


    }else if(!nerrors_arr[2]){
        float angle = (float)argtable_control_angle.angle->dval[0];

        if(angle < 0 || angle >= 360){
            return ERR_INVALID_CMD;
        }
        cmd->cmd_id = CMD_ID_CONTROL_ANGLE;
        memcpy(&cmd->params, &angle, sizeof(float));

    }else{
        // printf("Errors: [%d %d]\n",nerrors_arr[0], nerrors_arr[1]);
        // printf("INFO\n");
        // arg_print_errors(stdout, argtable_control_info.end, "control");
    
        // printf("SET\n");
        // arg_print_errors(stdout, argtable_control_set.end, "control");
        return ERR_INVALID_CMD;

        
    }

    return ERR_OK;
}

STATIC floatsat_err_t parse_control_params(float *params, const char *buffer)
{
    char *p = (char*)buffer;
    char *endptr = NULL;
    uint8_t param_idx = 0U;
    float new_param = 0.0f;

    while(*p != '\0'){
        new_param = strtof(p, &endptr);

        if(endptr == p || !(*endptr == ',' || *endptr == '\0')){
            // The strtof function found a non-numeric character that is not our 
            // delimiter, therefore, the string was invalid
            return ERR_INVALID_CMD;
        }

        if(param_idx >= CONTROL_MAX_PARAMS){
            // Too many params
            return ERR_TOO_MANY_ARGS;
        }
        params[param_idx++] = new_param;
        
        if(*endptr == '\0'){
            break;
        }
        p = endptr + 1;
    }

    return ERR_OK;
}


/* =========================== Mission command =========================== */
STATIC floatsat_err_t parse_cmd_mission(parser_ctx_t *ctx, floatsat_cmd_t *cmd)
{
    if(!ctx || ! cmd){
        return ERR_INVALID_ARG;
    }

    int nerrors_arr[CMD_MISSION_N_SUBCMD] = {0};

    nerrors_arr[0] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_mission_info);
    nerrors_arr[1] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_mission_set);

    if(!nerrors_arr[0]){
        // Format: mission -i
        cmd->cmd_id = CMD_ID_MISSION_INFO;

    }else if(!nerrors_arr[1]){
        // Format mission <mission_id>
        cmd->cmd_id = CMD_ID_MISSION_SET;

        // WARN: Treating pointer as a value. Is this the best approach? 
        cmd->params = (void*)argtable_mission_set.mission_id->ival[0]; 
    }else{
        return ERR_INVALID_CMD;
    }


    return ERR_OK;;
}

/* =========================== Photo command =========================== */
static floatsat_err_t parse_cmd_photo(parser_ctx_t *ctx, floatsat_cmd_t *cmd)
{
    if(!ctx || ! cmd){
        return ERR_INVALID_ARG;
    }

    int nerrors_arr[CMD_PHOTO_N_SUBCMD] = {0};

    nerrors_arr[0] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_photo_info);
    nerrors_arr[1] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_photo_add);
    nerrors_arr[2] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_photo_reset);


    if(!nerrors_arr[0]){
        // Format: photo -i
        cmd->cmd_id = CMD_ID_PHOTO_INFO;

    }else if(!nerrors_arr[1]){
        // Format: photo add -a <angle> -t <time> -d <duration>
        cmd->cmd_id = CMD_ID_PHOTO_ADD;

        cmd->params = malloc(sizeof(photo_info_t));
        photo_info_t *params = (photo_info_t*)cmd->params;
        if(!cmd->params){
            return ERR_OUT_OF_MEM;
        }

        // Angle
        if(argtable_photo_add.angle->dval[0] < 0 || argtable_photo_add.angle->dval[0] >= 360){
            goto cleanup;
        }
        params->angle = (float)argtable_photo_add.angle->dval[0];

        // Time
        floatsat_err_t ret = Time_ParseString(argtable_photo_add.time->sval[0], &params->time);
        if(ret != ERR_OK){
            goto cleanup;

        }

        // Duration
        if(argtable_photo_add.duration->ival[0] <= 0){
            goto cleanup;
        }
        params->duration = argtable_photo_add.duration->ival[0];


    }else if(!nerrors_arr[2]){
        // Format: photo reset
        cmd->cmd_id = CMD_ID_PHOTO_RESET;
    }else{
        return ERR_INVALID_CMD;
    }

    return ERR_OK;

cleanup:
    free(cmd->params);
    cmd->params = NULL;
    return ERR_INVALID_CMD;
}

#pragma endregion

/// @brief Process a message to obtain POSIX style argc and argv params. 
/// There is no dynamic allocation, the same `msg_body` buffer is used, and 
/// parameters are separated by replacing the next space with a NULL character. A NULL terminator is also written in the byte
/// directly after the buffer, so make sure there are len+1 bytes available
/// @param msg_body Pointer to the message body buffer, without the header
/// @param len Length of the message body. 
STATIC floatsat_err_t CmdParser_GetArgs(parser_ctx_t *ctx, uint8_t *msg_body, size_t len)
{
    if(!ctx || !msg_body || len == 0){
        return ERR_INVALID_ARG;
    }

    uint8_t argv_idx = 0U;
    char *p = (char*)msg_body;
    char *end = NULL;
    bool parsing_str = false;

    memset(ctx->argv, 0, CMD_PARSER_MAX_ARGS*sizeof(char*));
    msg_body[len] = '\0';

    //Skip whitespaces
    while(*p == ' '){
        p++;
    }
    if(*p == '\0'){
        return ERR_INVALID_ARG;
    }
    
    while(*p != '\0'){ // starts at a new argument
        
        // Quoted param, skip the " character
        if(*p == '\"'){ 
            p++;
            parsing_str = true;
        }else{
            parsing_str = false;
        }

        // Add the new arg
        if(argv_idx < CMD_PARSER_MAX_ARGS){
            ctx->argv[argv_idx++] = p;
        }else{
            return ERR_INVALID_CMD;
        }

        // Find the end of the current arg and add a NULL terminator
        if(!parsing_str){
            // Step through the arg. Check if it is a 
            while(*p != ' ' && *p != '\0'){

                // long opt with string param (ex: --type="PID")
                if(*p == '\"'){
                    end = strchr(p + 1, '\"');
                    if(end == NULL){
                        return ERR_INVALID_CMD;
                    }
                    // This is a bit convoluted. Eliminate the left quote with a 
                    /// memmove and the left over byte with a NULL
                    memmove(p, p + 1, end - p - 1);
                    *(end - 1) = '\0';
                    p = end;
                    break;
                }
                p++;
            }
            // end of argument found, place a NULL terminator
            *p++ = '\0';

        }else{
            // Find the next " character
            end = strchr(p, '\"');
            if(end == NULL){
                // No matching quote found
                return ERR_INVALID_CMD;
            }

            p = end;
            *p++ = '\0';

        }


        // Find the next argument
        while(*p == ' '){
            p++;
        }
        if(*p == '\0'){
            break;
        }

    }
    ctx->argc = argv_idx;
    return ERR_OK;
} 