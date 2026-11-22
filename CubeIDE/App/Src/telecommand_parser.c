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

static floatsat_err_t parse_cmd_imu(parser_ctx_t *ctx, floatsat_cmd_t *cmd);

static floatsat_err_t parse_cmd_motor(parser_ctx_t *ctx, floatsat_cmd_t *cmd);

static floatsat_err_t parse_cmd_system(parser_ctx_t *ctx, floatsat_cmd_t *cmd);

static floatsat_err_t fill_photo_fields(const struct arg_dbl *angle,
                                        const struct arg_str *time,
                                        const struct arg_int *amount,
                                        photo_info_t *photo,
                                        uint32_t *field_mask);

static int cmd_init_control(void);
static int cmd_init_mission(void);
static int cmd_init_photo(void);
static int cmd_init_imu(void);
static int cmd_init_motor(void);
static int cmd_init_system(void);


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

// Format control [-t <type> -p <params> -a <angle> --debug-id <debug_id> -e -d]
static struct {
    struct arg_str *type;
    struct arg_str *params;
    struct arg_dbl *angle;
    struct arg_lit *enable;
    struct arg_lit *disable;
    struct arg_end *end;
}argtable_control;


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

// Format: photo add -a <angle> -t <time> -n <amount>
static struct {
    struct arg_rex *subcmd;
    struct arg_dbl *angle;
    struct arg_str *time;
    struct arg_int *amount;
    struct arg_end *end;
}argtable_photo_add;

// Format: photo edit <index> -a <angle> -t <time> -n <amount>
static struct {
    struct arg_rex *subcmd;
    struct arg_int *index;
    struct arg_dbl *angle;
    struct arg_str *time;
    struct arg_int *amount;
    struct arg_end *end;
}argtable_photo_edit;

// Format photo rm <index>
static struct {
    struct arg_rex *subcmd;
    struct arg_int *index;
    struct arg_end *end;
}argtable_photo_remove;

//format photo move <src> <dest>
static struct {
    struct arg_rex *subcmd;
    struct arg_int *src;
    struct arg_int *dest;
    struct arg_end *end;
}argtable_photo_move;

// Format: photo reset
static struct {
    struct arg_rex *subcmd;
    struct arg_end *end;
}argtable_photo_reset;

// Format: photo take
static struct {
    struct arg_rex *subcmd;
    struct arg_end *end;
}argtable_photo_take;

/* =========================== imu command =========================== */
// Format imu read [-g -a -m -r -f]
static struct {
    struct arg_rex *subcmd;
    struct arg_lit *opts_gyro;
    struct arg_lit *opts_acc;
    struct arg_lit *opts_mag;
    struct arg_lit *opts_raw;
    struct arg_end *end;
}argtable_imu_read;

// Format imu cal [-g -a -m]

static struct {
    struct arg_rex *subcmd;
    struct arg_lit *opts_gyro;
    struct arg_lit *opts_accel;
    struct arg_lit *opts_mag;
    struct arg_end *end;
}argtable_imu_cal;

/* =========================== motor command =========================== */
// Format motor -p <pwm_val>
static struct {
    struct arg_dbl *pwm_val;
    struct arg_end *end;
}argtable_motor_pwn;

/* =========================== System command =========================== */
// Format: sys [--reset] [--debug-id <debug_id>] 

static struct {
    struct arg_lit *reset;
    struct arg_int *debug_id;
    struct arg_str *invoke_cb;
    struct arg_str *set_rtc;
    struct arg_end *end;
}argtable_system;

static const cmd_handler_pair_t cmd_handler_table[] = {
    {"control",     parse_cmd_control},
    {"mission",     parse_cmd_mission},
    {"photo",       parse_cmd_photo},
    {"imu",         parse_cmd_imu},
    {"motor",       parse_cmd_motor},
    {"sys",         parse_cmd_system},
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
    cmd_init_imu();
    cmd_init_motor();
    cmd_init_system();

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

static int cmd_init_control(void)
{
    // Format: control -i
    // argtable_control_info.cmd_name      =   arg_rex1(NULL,NULL,"control",NULL,ARG_REX_ICASE,NULL);
    argtable_control_info.info          =   arg_lit1("i","info","Show control information");
    argtable_control_info.end           =   arg_end(CMD_MAX_ERRORS);

    // Format control [-t <type> -p <parasms> -a <angle> --debug-id <debug_id> -e -d]
    argtable_control.type               =   arg_str0("t","type","PID|LQR","Control type");
    argtable_control.params             =   arg_str0("p","params","k_theta,k_i,k_p","Controller gains, separated by commas without spaces");
    argtable_control.angle              =   arg_dbl0("a","angle","float (-180,180]","Target angle");
    argtable_control.enable             =   arg_lit0("e","enable","Enable the control loop");
    argtable_control.disable            =   arg_lit0("d","disable","Disable the control loop");
    argtable_control.end                =   arg_end(CMD_MAX_ERRORS);

    // TODO: Check for NULL pointers!

    return 0;
}

static int cmd_init_mission(void)
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

static int cmd_init_photo(void)
{

    // Format: photo -i
    argtable_photo_info.info            =   arg_lit1("i","info","Show photo information");
    argtable_photo_info.end             =   arg_end(CMD_MAX_ERRORS);

    // Format: photo add -a <angle> -t <time> -d <c>
    argtable_photo_add.subcmd           =   arg_rex1(NULL, NULL, "add", NULL, ARG_REX_ICASE, NULL);
    argtable_photo_add.angle            =   arg_dbl1("a", "angle","float", "Azimuth angle of the photo in the range (-180,180]");
    argtable_photo_add.time             =   arg_str1("t", "time", "HH:mm:ss",
                                                        "Moment to take the photo. Specified as HH:mm:ss of Standard Atlantic Time (UTC-4)");
    argtable_photo_add.amount           =   arg_int1("n", "amount", "uint", "Amount of photos to take");
    argtable_photo_add.end              =   arg_end(CMD_MAX_ERRORS);

    // Format: photo edit <index> -a <angle> -t <time> -d <amount>
    argtable_photo_edit.subcmd          =   arg_rex1(NULL, NULL, "edit", NULL, ARG_REX_ICASE, NULL);
    argtable_photo_edit.index           =   arg_int1(NULL, NULL, "uint8_t", "Index of the photo cmd to edit");
    argtable_photo_edit.angle           =   arg_dbl0("a", "angle","double", "Azimuth angle of the photo in the range (-180,180]o");
    argtable_photo_edit.time            =   arg_str0("t", "time", "HH:mm:ss",
                                                        "Moment to take the photo. Specified as HH:mm:ss of Standard Atlantic Time (UTC-4)");
    argtable_photo_edit.amount        =   arg_int0("n", "amount", "uint", "Amount of photos to take");
    argtable_photo_edit.end             =   arg_end(CMD_MAX_ERRORS);

    // Format photo rm <index>
    argtable_photo_remove.subcmd        =   arg_rex1(NULL, NULL, "rm", NULL, ARG_REX_ICASE, NULL);
    argtable_photo_remove.index         =   arg_int1(NULL, NULL, "uint8_t","Index of the photo to remove");
    argtable_photo_remove.end           =   arg_end(CMD_MAX_ERRORS);
    
    // Format: photo move <src> <dest>
    argtable_photo_move.subcmd          =   arg_rex1(NULL,NULL, "move", NULL, ARG_REX_ICASE, NULL);
    argtable_photo_move.src             =   arg_int1(NULL, NULL, "uint8_t","Index of the entry to move");
    argtable_photo_move.dest            =   arg_int1(NULL, NULL, "uint8_t","Destination index");
    argtable_photo_move.end             =   arg_end(CMD_MAX_ERRORS);


    // Format: photo reset
    argtable_photo_reset.subcmd         =   arg_rex1(NULL, NULL, "reset", NULL, ARG_REX_ICASE, "Reset subcommand");
    argtable_photo_reset.end            =   arg_end(CMD_MAX_ERRORS);

    // Format: photo take
    argtable_photo_take.subcmd          =   arg_rex1(NULL, NULL, "take", NULL, ARG_REX_ICASE, "Take a photo");
    argtable_photo_take.end             =   arg_end(CMD_MAX_ERRORS);

    return 0;
}

static int cmd_init_imu(void)
{
    // Format imu read [-g -a -m -r -f]
    argtable_imu_read.subcmd            =   arg_rex1(NULL, NULL, "read", NULL, ARG_REX_ICASE, "Read the sensor outputs from the IMU");
    argtable_imu_read.opts_gyro         =   arg_lit0("g","gyro","Include gyroscope readings");
    argtable_imu_read.opts_acc          =   arg_lit0("a","acc","Include accelerometer readings");
    argtable_imu_read.opts_mag          =   arg_lit0("m","mag","Include magnetometer readings");

    // Format imu cal [-g -a -m]
    argtable_imu_cal.subcmd             = arg_rex1(NULL, NULL, "cal", NULL, ARG_REX_ICASE, "Calibrate IMU");
    argtable_imu_cal.opts_gyro          = arg_lit0("g","gyro","Calibrate gyroscope");
    argtable_imu_cal.opts_accel         = arg_lit0("a","accel","Calibrate accelerometer");
    argtable_imu_cal.opts_mag           = arg_lit0("m","mag","Calibrate magnetometer");
    argtable_imu_cal.end                = arg_end(CMD_MAX_ERRORS);

    return 0;
}

static int cmd_init_motor(void)
{
    argtable_motor_pwn.pwm_val          =   arg_dbl1("p", "pwm", "float", "PWM value [0-1]");
    argtable_motor_pwn.end              =   arg_end(CMD_MAX_ERRORS);

    return 0;
}

static int cmd_init_system(void){
    argtable_system.reset               = arg_lit0(NULL,"reset","Reset the microcontroller");
    argtable_system.debug_id            = arg_int0(NULL,"debug-id","uint8_t", "Set the debug packet ID");
    argtable_system.invoke_cb           = arg_str0(NULL,"invoke-cb", "string", "Invoke a system callback");
    argtable_system.set_rtc             = arg_str0(NULL, "set-rtc","MM/DD/yy hh:mm:ss","Set the RTC time and date");
    argtable_system.end                 = arg_end(CMD_MAX_ERRORS);
    
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
    int nerrors_arr[2] = {0};

    nerrors_arr[0] = arg_parse(ctx->argc,ctx->argv, (void**)&argtable_control_info);
    nerrors_arr[1] = arg_parse(ctx->argc,ctx->argv, (void**)&argtable_control);

    *cmd = (floatsat_cmd_t){0}; 

    if(!nerrors_arr[0]){
        // Format: control [-i]
        cmd->cmd_id = CMD_ID_CONTROL_INFO;
    
    }else if(!nerrors_arr[1]){
        // Format control [-t <type> -p <params> -a <angle> --debug-id <debug_id> -e -d]

        cmd->cmd_id = CMD_ID_CONTROL_MODIFY;
        if(argtable_control.type->count > 0){
            cmd->control_args.action_flags |= CMD_CONTROL_FLAG_TYPE;
            
            if(strcmp(argtable_control.type->sval[0], "PID") == 0){
                cmd->control_args.control_type = CONTROL_TYPE_PID;
            }else if(strcmp(argtable_control.type->sval[0], "LQR") == 0){
                cmd->control_args.control_type = CONTROL_TYPE_LQR;
            }else {
                LOGE(LOG_TAG,"Error: Invalid control type");
                goto err;
            }

        }

        if(argtable_control.params->count > 0){
            float params[3] = {0};
            GOTO_ON_ERR_LOG(parse_control_params(params, argtable_control.params->sval[0]),
                err, ret, LOG_TAG, "Error: Invalid control param format");

            cmd->control_args.action_flags |= CMD_CONTROL_FLAG_PARAM;

            cmd->control_args.k_theta   = params[0];
            cmd->control_args.k_p       = params[1];
            cmd->control_args.k_i       = params[2];
        }

        if(argtable_control.angle->count > 0){
            float angle = (float)argtable_control.angle->dval[0]; 
            if(!(angle > -180.0f && angle <= 180.0f)){
                goto err;
            }
            cmd->control_args.action_flags |= CMD_CONTROL_FLAG_ANGLE;
            cmd->control_args.angle = (float)argtable_control.angle->dval[0];
        }

        // if(argtable_control.debug_id->count > 0){
        //     if(argtable_control.debug_id->ival[0] < 0){
        //         goto err;
        //     }
        //     cmd->control_args.action_flags |= CMD_CONTROL_FLAG_DBG_ID;
        //     cmd->control_args.debug_id = (uint8_t)argtable_control.debug_id->ival[0];

        // }

        if(argtable_control.enable->count > 0){
            cmd->control_args.action_flags |= CMD_CONTROL_FLAG_ENABLE;
        }else if(argtable_control.disable->count > 0){
            cmd->control_args.action_flags |= CMD_CONTROL_FLAG_DISABLE;
        }
    }
    
    
    else{
        // printf("Errors: [%d %d]\n",nerrors_arr[0], nerrors_arr[1]);
        // printf("INFO\n");
        // arg_print_errors(stdout, argtable_control_info.end, "control");
    
        // printf("SET\n");
        // arg_print_errors(stdout, argtable_control_set.end, "control");
        return ERR_INVALID_CMD;

        
    }

    return ERR_OK;
err:
    memset(cmd, 0, sizeof(floatsat_cmd_t));
    return ERR_INVALID_CMD;


}

STATIC floatsat_err_t parse_control_params(float *params, const char *buffer)
{
    const char *p = buffer;
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
        int mission_id = argtable_mission_set.mission_id->ival[0];
        if(mission_id < 0){
            LOGE(LOG_TAG,"Error: Mission ID cannot be negative");
            goto err;
        }
        cmd->mission_args.mission_id = (uint8_t)mission_id;

    }else{
        return ERR_INVALID_CMD;
    }


    return ERR_OK;

err: 
    memset(cmd, 0, sizeof(floatsat_cmd_t));
    return ERR_INVALID_CMD;
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
    nerrors_arr[2] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_photo_edit);
    nerrors_arr[3] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_photo_remove);
    nerrors_arr[4] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_photo_reset);
    nerrors_arr[5] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_photo_move);
    nerrors_arr[6] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_photo_take);

    if(!nerrors_arr[0]){
        // Format: photo -i
        cmd->cmd_id = CMD_ID_PHOTO_INFO;

    }else if(!nerrors_arr[1]){
        // Format: photo add -a <angle> -t <time> nd <amount>
        cmd->cmd_id = CMD_ID_PHOTO_ADD;

        floatsat_err_t ret = fill_photo_fields(
            argtable_photo_edit.angle,
            argtable_photo_edit.time,
            argtable_photo_edit.amount,
            &cmd->photo_args.photo,
            &cmd->photo_args.action_flags);
        if(ret != ERR_OK){
            goto err;
        }


    }else if(!nerrors_arr[2]){
        // Format: photo edit <index> [-a <angle>] [-t <time>] [-d <amount>]
        cmd->cmd_id = CMD_ID_PHOTO_EDIT;
        
        int index = argtable_photo_edit.index->ival[0];
        if(index < 0){
            LOGE(LOG_TAG, "Error: Negative photo index");
            return ERR_INVALID_CMD;
        }
        cmd->photo_args.index = (uint8_t)index;

       floatsat_err_t ret = fill_photo_fields(
            argtable_photo_edit.angle,
            argtable_photo_edit.time,
            argtable_photo_edit.amount,
            &cmd->photo_args.photo,
            &cmd->photo_args.action_flags
        );
        if(ret != ERR_OK){
            goto err;
        }

    }else if(!nerrors_arr[3]){
        cmd->cmd_id = CMD_ID_PHOTO_REMOVE;
        int index = argtable_photo_remove.index->ival[0];
        if(index < 0){
            LOGE(LOG_TAG,"Error: Negative photo index");
            return ERR_INVALID_ARG;
        }
        cmd->photo_args.index = (uint8_t)index;
    }
    else if(!nerrors_arr[4]){
        // Format: photo reset
        cmd->cmd_id = CMD_ID_PHOTO_RESET;

    }else if(!nerrors_arr[5]){
        cmd->cmd_id = CMD_ID_PHOTO_MOVE;
        if(argtable_photo_move.src->ival[0] < 0 || argtable_photo_move.dest->ival[0] < 0){
            LOGE(LOG_TAG,"Error: Negative photo index");
            return ERR_INVALID_ARG;
        }

        if(argtable_photo_move.src->ival[0] == argtable_photo_move.dest->ival[0]){
            LOGE(LOG_TAG,"Error: Source and destination are the same");
            return ERR_INVALID_ARG;
        }

        cmd->photo_args.index       = (uint8_t)argtable_photo_move.src->ival[0];
        cmd->photo_args.dest_index  = (uint8_t)argtable_photo_move.dest->ival[0];


    } else if(!nerrors_arr[6]){
        cmd->cmd_id = CMD_ID_PHOTO_TAKE;

    } else{
        return ERR_INVALID_CMD;
    }

    return ERR_OK;

err:
    memset(cmd, 0, sizeof(floatsat_cmd_t));
    return ERR_INVALID_CMD;
}

static floatsat_err_t fill_photo_fields(const struct arg_dbl *angle,
                                        const struct arg_str *time,
                                        const struct arg_int *amount,
                                        photo_info_t *photo,
                                        uint32_t *field_mask)
{
    if (!angle || !time || !amount || !photo || !field_mask) {
        return ERR_INVALID_ARG;
    }

    memset(photo, 0, sizeof(*photo));
    *field_mask = 0U;

    if (angle->count > 0) {
        float a = (float)angle->dval[0];
        if (!(a > -180.0f && a <= 180.0f)) {   /* also rejects NaN */
            LOGE(LOG_TAG, "Error: Invalid photo angle");
            return ERR_INVALID_ARG;
        }
        photo->angle = a;
        *field_mask |= CMD_PHOTO_FLAG_FIELD_ANGLE;
    }

    if (time->count > 0) {
        /* Parse once, straight into the destination */
        if (Time_ParseString(time->sval[0], &photo->time) != ERR_OK) {
            LOGE(LOG_TAG, "Error: Invalid photo time string");
            return ERR_INVALID_ARG;
        }
        *field_mask |= CMD_PHOTO_FLAG_FIELD_TIME;
    }

    if (amount->count > 0) {
        int d = amount->ival[0];
        if (d <= 0 || d > CMD_MAX_PHOTO_AMOUNT_PER_INSTRUCTION) {
            LOGE(LOG_TAG, "Error: Invalid amount");
            return ERR_INVALID_ARG;
        }
        photo->amount = (uint8_t)d;
        *field_mask |= CMD_PHOTO_FLAG_FIELD_AMOUNT;
    }

    return ERR_OK;
}

/* =========================== imu command =========================== */
static floatsat_err_t parse_cmd_imu(parser_ctx_t *ctx, floatsat_cmd_t *cmd)
{
    if(!ctx || !cmd){
        return ERR_INVALID_ARG;
    }

    int nerrors_arr[2] = {0};

    nerrors_arr[0] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_imu_read);
    nerrors_arr[1] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_imu_cal);

    if(!nerrors_arr[0]){
        cmd->cmd_id = CMD_ID_IMU_READ;

        if(argtable_imu_read.opts_gyro->count > 0){
            cmd->imu_args.action_flags |= CMD_IMU_READ_FLAG_GYRO;
        }

        if(argtable_imu_read.opts_acc->count > 0){
            cmd->imu_args.action_flags |= CMD_IMU_READ_FLAG_ACCEL;
        }

        if(argtable_imu_read.opts_mag->count > 0){
            cmd->imu_args.action_flags |= CMD_IMU_READ_FLAG_MAG;
        }

        if(argtable_imu_read.opts_raw->count > 0){
            cmd->imu_args.action_flags |= CMD_IMU_READ_FLAG_RAW;
        }

    }else if(!nerrors_arr[1]){
        cmd->cmd_id = CMD_ID_IMU_CALIBRATE;

        if(argtable_imu_cal.opts_gyro->count > 0){
            cmd->imu_args.action_flags |= CMD_IMU_CAL_FLAG_GYRO;
        }

        if(argtable_imu_cal.opts_accel->count > 0){
            cmd->imu_args.action_flags |= CMD_IMU_CAL_FLAG_ACCEL;
        }

        if(argtable_imu_cal.opts_mag->count > 0){
            cmd->imu_args.action_flags |= CMD_IMU_CAL_FLAG_MAG;
        }

    }else{
        return ERR_INVALID_CMD;
    }


    return ERR_OK;
}

static floatsat_err_t parse_cmd_motor(parser_ctx_t *ctx, floatsat_cmd_t *cmd)
{
    if(!ctx || !cmd){
        return ERR_INVALID_ARG;
    }

    int nerrors_arr[2] = {0};

    *cmd = (floatsat_cmd_t){0};

    nerrors_arr[0] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_motor_pwn);

    if(!nerrors_arr[0]){
        cmd->cmd_id = CMD_ID_MOTOR_PWM_SET;

        if(argtable_motor_pwn.pwm_val->count > 0){
            float val = (float)argtable_motor_pwn.pwm_val->dval[0];

            if(!(val >= -1.0f && val <= 1.0f)){
                LOGE(LOG_TAG,"Invalid PWM value. Must be in range [-1, 1]");
                goto err;
            }
            cmd->motor_args.pwm_val = val;

        }

    }else{
        return ERR_INVALID_CMD;
    }
    
    return ERR_OK;

err:
    memset(cmd, 0, sizeof(floatsat_cmd_t));
    return ERR_INVALID_ARG;
}

static floatsat_err_t parse_cmd_system(parser_ctx_t *ctx, floatsat_cmd_t *cmd)
{
    if(!ctx || !cmd){
        return ERR_INVALID_ARG;
    }

    int nerrors_arr[2] = {0};

    nerrors_arr[0] = arg_parse(ctx->argc, ctx->argv, (void**)&argtable_system);

    if(!nerrors_arr[0]){
        cmd->cmd_id = CMD_ID_SYSCTL;

        if(argtable_system.reset->count > 0){
            cmd->sysctl_args.action_flags |= CMD_SYSCTL_FLAG_RESET;
        }

        if(argtable_system.debug_id->count > 0){
            int debug_id = argtable_system.debug_id->ival[0];
            if(debug_id < 0 ){
                LOGE(LOG_TAG,"Error: Negative debug ID");
                return ERR_INVALID_CMD;
            }
            cmd->sysctl_args.debug_id = (uint8_t)debug_id;
            cmd->sysctl_args.action_flags |= CMD_SYSCTL_FLAG_DBG_ID;
        }

        if(argtable_system.invoke_cb->count > 0){
            if(strlen(argtable_system.invoke_cb->sval[0]) > CMD_SYSCTL_MAX_CB_NAME_LEN){
                LOGE(LOG_TAG,"Error: Callback name too long. Max length is %u characters", CMD_SYSCTL_MAX_CB_NAME_LEN);
                return ERR_INVALID_ARG;
            }

            cmd->sysctl_args.action_flags |= CMD_SYSCTL_FLAG_INVOKE_CB;
            strncpy(cmd->sysctl_args.callback_name, argtable_system.invoke_cb->sval[0], CMD_SYSCTL_MAX_CB_NAME_LEN);
        }

        if(argtable_system.set_rtc->count > 0){
            if(strlen(argtable_system.set_rtc->sval[0]) > CMD_SYSCTL_MAX_RTC_DATETIME_LEN){
                LOGE(LOG_TAG, "Error: RTC datetime too long.");
                return ERR_INVALID_CMD;
            }

            cmd->sysctl_args.action_flags |= CMD_SYSCTL_FLAG_SET_RTC;
            Time_ParseTimeDateString(argtable_system.set_rtc->sval[0], &cmd->sysctl_args.rtc_time, &cmd->sysctl_args.rtc_date);


        }
        
    }
    
    else{
    	return ERR_INVALID_CMD;
    }


    return ERR_OK;
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
                    memmove(p, p + 1, (size_t)(end - p - 1));
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
