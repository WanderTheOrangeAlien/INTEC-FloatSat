
#ifdef TEST

#include "unity.h"

// Mocks
#include "mock_stm32f4xx_hal.h"         // For HAL_GetTick() used in logging

// Dependencies
#include "argtable3.h"
#include <string.h>
#include <stdlib.h>
#include "floatsat_time.h"
#include "floatsat_log.h"

// File under test
#include "telecommand_parser.h"

void setUp(void)
{
}

void tearDown(void)
{
}



static parser_ctx_t ctx = {0};

// Array of char arrays (NOT POINTERS) because defining a char pointer to a const string puts it in 
// read only mem. For example, char *buffers_control[] places everything in read only mem


void test_get_args(){
    
    char buffers_control[][64] = {
        "    control -i    ",
        "control           -i     ",

        "control -t PID --params=\"1 2 3\"",
        "control --type=PID  -p \"-343.5,0,1234.567\"",
        "control",
    };
    floatsat_err_t ret = ERR_OK;

    // ret = CmdParser_GetArgs(&ctx,buffers_control[0], strlen(buffers_control[0]));
    // TEST_ASSERT_EQUAL_INT(ERR_OK, ret); 
    // TEST_ASSERT_EQUAL_STRING("control",ctx.argv[0]);
    // TEST_ASSERT_EQUAL_STRING("-i",ctx.argv[1]);
    // TEST_ASSERT_EQUAL_INT(2,ctx.argc);

    // ret = CmdParser_GetArgs(&ctx,buffers_control[1], strlen(buffers_control[1]));
    // TEST_ASSERT_EQUAL_INT(ERR_OK, ret);// 
    // TEST_ASSERT_EQUAL_STRING("control",ctx.argv[0]);
    // TEST_ASSERT_EQUAL_STRING("-i",ctx.argv[1]);
    // TEST_ASSERT_EQUAL_INT(2, ctx.argc);

    ret = CmdParser_GetArgs(&ctx,buffers_control[2], strlen(buffers_control[2]));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_STRING("control",ctx.argv[0]);
    TEST_ASSERT_EQUAL_STRING("-t",ctx.argv[1]);
    TEST_ASSERT_EQUAL_STRING("PID",ctx.argv[2]);
    TEST_ASSERT_EQUAL_STRING("--params=1 2 3",ctx.argv[3]);
    TEST_ASSERT_EQUAL_INT(4,ctx.argc);


    ret = CmdParser_GetArgs(&ctx,buffers_control[3], strlen(buffers_control[3]));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_STRING("control",ctx.argv[0]);
    TEST_ASSERT_EQUAL_STRING("--type=PID",ctx.argv[1]);
    TEST_ASSERT_EQUAL_STRING("-p",ctx.argv[2]);
    TEST_ASSERT_EQUAL_STRING("-343.5,0,1234.567",ctx.argv[3]);
    TEST_ASSERT_EQUAL_INT(4,ctx.argc);
}

void test_parse_control(void)
{
    int ignore_ret = 0;
    HAL_GetTick_IgnoreAndReturn(ignore_ret);

    char buffers_control[][64] = {
        "    control -i    ",
        "control           -i     ",

        "control -t PID -p \"1,2,3\"",
        "control --type=LQR  -p \"-343.5,0,1234.567\"",
        "control --type=LQR  -p -343.5,0,1234.567",
    };

    char buffers_control_invalid[][64] = {
        "control -p -p -I",
        "control -t LOL -p",
        "control --type=LQR  -p \"-34G,76,24\"",
        "control --type=LQR  -p \"-34,76,24\"\"",
        "control --type=LQR  -p \"-34,76,24\"\"",
        "control --type=LQR  -p \"-34,76,24",
    };

    floatsat_err_t ret = ERR_OK;
    floatsat_cmd_t cmd = {0};
    floatsat_args_set_control_t *args;

    CmdParser_Init(&ctx);

    // ret = CmdParser_Parse(&ctx, buffers_control[0], strlen(buffers_control[0]),&cmd);
    // TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    // TEST_ASSERT_EQUAL_UINT8(CMD_ID_CONTROL_INFO,cmd.cmd_id);

    ret = CmdParser_Parse(&ctx, buffers_control[2], strlen(buffers_control[2]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    args = (floatsat_args_set_control_t*)cmd.params;
    
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_CONTROL_SET, cmd.cmd_id);
    TEST_ASSERT_EQUAL_FLOAT(1.0, args->control_params[0]);
    TEST_ASSERT_EQUAL_FLOAT(2.0, args->control_params[1]);
    TEST_ASSERT_EQUAL_FLOAT(3.0, args->control_params[2]);
    TEST_ASSERT_EQUAL_UINT8(CONTROL_TYPE_PID,args->control_type);
    if(cmd.params != NULL){
        free(cmd.params);
    }

    ret = CmdParser_Parse(&ctx, buffers_control[3], strlen(buffers_control[3]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    args = (floatsat_args_set_control_t*)cmd.params;

    TEST_ASSERT_EQUAL_UINT8(CMD_ID_CONTROL_SET, cmd.cmd_id);
    TEST_ASSERT_EQUAL_FLOAT(-343.5, args->control_params[0]);
    TEST_ASSERT_EQUAL_FLOAT(0, args->control_params[1]);
    TEST_ASSERT_EQUAL_FLOAT(1234.567, args->control_params[2]);
    TEST_ASSERT_EQUAL_UINT8(CONTROL_TYPE_LQR,args->control_type);
        if(cmd.params != NULL){
        free(cmd.params);
    }

    ret = CmdParser_Parse(&ctx, buffers_control[4], strlen(buffers_control[4]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    args = (floatsat_args_set_control_t*)cmd.params;

    TEST_ASSERT_EQUAL_UINT8(CMD_ID_CONTROL_SET, cmd.cmd_id);
    TEST_ASSERT_EQUAL_FLOAT(-343.5, args->control_params[0]);
    TEST_ASSERT_EQUAL_FLOAT(0, args->control_params[1]);
    TEST_ASSERT_EQUAL_FLOAT(1234.567, args->control_params[2]);
    TEST_ASSERT_EQUAL_UINT8(CONTROL_TYPE_LQR,args->control_type);
        if(cmd.params != NULL){
        free(cmd.params);
    }



    // Test all invalid
    size_t n_invalid = 5;
    for (size_t i = 0; i < n_invalid; i++)
    {
        printf("invlaid #%u\n",i);
        ret = CmdParser_Parse(&ctx, buffers_control_invalid[i], strlen(buffers_control_invalid[i]), &cmd);
        TEST_ASSERT_EQUAL_INT(ERR_INVALID_CMD, ret);
    }

    if(cmd.params){
        free(cmd.params);
    }

}


void test_parse_mission()
{
    int ignore_ret = 0;
    HAL_GetTick_IgnoreAndReturn(ignore_ret);

    char buffer_valid[][64] = {
        "  mission -i",
        "  mission -i      ",
        "mission 1",
        "  mission    7859  ",
        "mission 42     ",
    };

    char buffer_invalid[][64] = {
        "misson  ",
        "mission -34",
        "mission 4 6 3",
        "mission -i 12",
        "mission -i34",
    };

    floatsat_err_t ret = ERR_OK;
    floatsat_cmd_t cmd = {0};
    parser_ctx_t ctx = {0};

    CmdParser_Init(&ctx);

    ret = CmdParser_Parse(&ctx, (uint8_t*)buffer_valid[0], strlen(buffer_valid[0]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_MISSION_INFO, cmd.cmd_id);

    ret = CmdParser_Parse(&ctx, (uint8_t*)buffer_valid[1], strlen(buffer_valid[1]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_MISSION_INFO, cmd.cmd_id);

    ret = CmdParser_Parse(&ctx, (uint8_t*)buffer_valid[2], strlen(buffer_valid[2]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_MISSION_SET, cmd.cmd_id);
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)cmd.params);

    ret = CmdParser_Parse(&ctx, (uint8_t*)buffer_valid[3], strlen(buffer_valid[3]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_MISSION_SET, cmd.cmd_id);
    TEST_ASSERT_EQUAL_UINT32(7859U, (uint32_t)cmd.params);

    ret = CmdParser_Parse(&ctx, (uint8_t*)buffer_valid[4], strlen(buffer_valid[4]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_MISSION_SET, cmd.cmd_id);
    TEST_ASSERT_EQUAL_UINT32(42U, (uint32_t)cmd.params);

    /* ================= Invalid tests ================= */

    for (size_t i = 0; i < 5; i++)
    {
        char *buffer = buffer_invalid[i];
        ret = CmdParser_Parse(&ctx, (uint8_t*)buffer,strlen(buffer),&cmd);
        TEST_ASSERT_EQUAL_INT(ERR_INVALID_CMD, ret);
    }
    


}

void test_parse_photo()
{
    char valid_buffers[][64] = {
        "photo -i",
        "photo    -i  ",
        "photo add -a 45 -t 00:40:23 -d 5000",
        "photo add -a 320.5 -t 23:59:59 -d 1234",
        "photo add -a 93.68 -t 15:9:0 -d 200",
        "photo reset"
    };

    char invalid_buffers[][64] ={
        "phtoo",
        "photo add -a -45 -t 23:59:59 -d 1234",
        "photo add -a 361 -t 23:59:59 -d 1234",
        "photo add -a 62 -t 2A:E5:59 -d 1234",
        "photo add -a 63 -t 2A:E5:YY -d 1234",
        "photo add -a 64 -t 2A:E5:YY -d 1234",
        "photo add -a 64 -t 2A:E5:YY -d 1234",
        "photo add -a 64 -t 2A:E5:YY -d 1234",

        "photo add -a 42 -t 10:45:00 -d -10"
        "photo add -a 42 -t 10:45:00 -d",

        "photo add"
        "photo add -a 42"
        "photo add -a 42 -t 04:32:01"

        "photo add -a 42 -t 04:32:01"
    };

    floatsat_err_t ret = ERR_OK;
    floatsat_cmd_t cmd = {0};
    parser_ctx_t ctx = {0};

    photo_info_t *params = NULL;

    CmdParser_Init(&ctx);

    ret = CmdParser_Parse(&ctx, valid_buffers[0], strlen(valid_buffers[0]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_PHOTO_INFO, cmd.cmd_id);
    TEST_ASSERT_NULL(cmd.params);

    ret = CmdParser_Parse(&ctx, valid_buffers[1], strlen(valid_buffers[1]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_PHOTO_INFO, cmd.cmd_id);
    TEST_ASSERT_NULL(cmd.params);


    ret = CmdParser_Parse(&ctx, valid_buffers[2], strlen(valid_buffers[2]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_PHOTO_ADD, cmd.cmd_id);
    params = (photo_info_t*)cmd.params;
    TEST_ASSERT_EQUAL_FLOAT(    45.0f,  params->angle);
    TEST_ASSERT_EQUAL_UINT8(    0U,     params->time.hours);
    TEST_ASSERT_EQUAL_UINT8(    40U,    params->time.minutes);
    TEST_ASSERT_EQUAL_UINT8(    23U,    params->time.seconds);
    TEST_ASSERT_EQUAL_UINT32(   5000U,  params->duration);


    ret = CmdParser_Parse(&ctx, valid_buffers[3], strlen(valid_buffers[3]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_PHOTO_ADD, cmd.cmd_id);
    params = (photo_info_t*)cmd.params;
    TEST_ASSERT_EQUAL_FLOAT(    320.5f,  params->angle);
    TEST_ASSERT_EQUAL_UINT8(    23U,     params->time.hours);
    TEST_ASSERT_EQUAL_UINT8(    59U,    params->time.minutes);
    TEST_ASSERT_EQUAL_UINT8(    59U,    params->time.seconds);
    TEST_ASSERT_EQUAL_UINT32(   1234U,  params->duration);

    ret = CmdParser_Parse(&ctx, valid_buffers[4], strlen(valid_buffers[4]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_PHOTO_ADD, cmd.cmd_id);
    params = (photo_info_t*)cmd.params;
    TEST_ASSERT_EQUAL_FLOAT(    93.68f,params->angle);
    TEST_ASSERT_EQUAL_UINT8(    15U,   params->time.hours);
    TEST_ASSERT_EQUAL_UINT8(    9U,    params->time.minutes);
    TEST_ASSERT_EQUAL_UINT8(    0U,    params->time.seconds);
    TEST_ASSERT_EQUAL_UINT32(   200U,  params->duration);

    ret = CmdParser_Parse(&ctx, valid_buffers[5], strlen(valid_buffers[5]),&cmd);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(CMD_ID_PHOTO_RESET, cmd.cmd_id);
    TEST_ASSERT_NULL(cmd.params);

    /* ================= Invalidity tests ================= */

    for (size_t i = 0; i < 14; i++)
    {
        ret = CmdParser_Parse(&ctx, invalid_buffers[i], strlen(invalid_buffers[i]),&cmd);
        TEST_ASSERT_EQUAL_INT(ERR_INVALID_CMD, ret);
        TEST_ASSERT_NULL(cmd.params);
    }
    


    printf("telecommand_parse_photo() Successful!\n");
}

#endif // TEST
