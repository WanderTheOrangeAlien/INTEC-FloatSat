
#ifdef TEST

#include "unity.h"

#include "fake_hal_get_tick.h"
#include "floatsat_log.h"



void setUp(void)
{
}

void tearDown(void)
{
}


void test_ringbuffer(void)
{
    /* Characters added by the log module
        Color escape code:      7
        Timestamp:              3 + # digits
        Log Tag:                3 + Tag length (TEST= 4)
        Color reset:            4
        newline:                1
        TOTAL:                  18 + #digits + tag length 
                                    (22 in this case for 1 digit timestamp)
    
    */


    const char LOG_TAG[] = "TEST";
    const ringbuff_t *ringbuff = Log_GetRingbuff(); 
    char out_buffer[256] = {0};
    size_t written = 0U;

    LOGE(LOG_TAG,"Error1!");
    LOGE(LOG_TAG,"Error2!");


    TEST_ASSERT_EQUAL_CHAR_ARRAY(
        LOG_COLOR_E "(0) [TEST] Error1!" LOG_COLOR_RESET "\n" 
        LOG_COLOR_E "(1) [TEST] Error2!" LOG_COLOR_RESET "\n",
        ringbuff->buffer, 60);

    Log_PopNextMsg(out_buffer, &written);
    TEST_ASSERT_EQUAL_size_t(30, written);
    TEST_ASSERT_EQUAL_size_t(30, ringbuff->tail);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(
        LOG_COLOR_E "(1) [TEST] Error2!" LOG_COLOR_RESET "\n",
        &ringbuff->buffer[ringbuff->tail], 30);

    Log_ResetBuffer();
    TEST_ASSERT_EQUAL_size_t(0, ringbuff->head);
    TEST_ASSERT_EQUAL_size_t(0, ringbuff->tail);
    TEST_ASSERT_FALSE(ringbuff->full);

    LOGI(LOG_TAG,"This is a really long message that when expanded to log form occupies 120 characters. " 
        "Counted it!");
    TEST_ASSERT_EQUAL_size_t(120, ringbuff->head);

    // The message should not have been written because it was too big, therefore, the head stays in the same place
    LOGI(LOG_TAG, "36 characters");
    TEST_ASSERT_EQUAL_size_t(120, ringbuff->head);

    Log_PopNextMsg(out_buffer, &written);
    TEST_ASSERT_EQUAL_size_t(120, ringbuff->tail);

    // Test wrap around!
    LOGI(LOG_TAG, "36 characters");
    TEST_ASSERT_EQUAL_CHAR_ARRAY(
        LOG_COLOR_I "(",
        &ringbuff->buffer[ringbuff->tail], 8);
    
    TEST_ASSERT_EQUAL_CHAR_ARRAY(
        "4) [TEST] 36 characters" LOG_COLOR_RESET "\n",
        ringbuff->buffer, 28);


    //TODO: Test popping a message that has wrapped around



}

#endif // TEST
