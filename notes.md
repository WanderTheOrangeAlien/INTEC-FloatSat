# August

## (08-18-26) On plotting data
I read a bit about Serial Studio. It is an awesome software for our purpose. 
It runs a server that listens to TCP port 7777 and we can send lots of commands
through it. What interests me the most is the io.writeData command. We can
basically have full two-way communication with the microcontroller. This is
very convenient because it would allow us to run an experiment multiple times 
(or run multiple experiments) without needing to reset the microcontroller.

We now need to design the packet structure. Right now I can see 2 options, let's
consider only the 9 values from the IMU

1 - JSON Format  
{  
    acc:    [ [3] [3] [3] ...],  
    gyro:   [ [3] [3] [3] ...],  
    mag:    [ [3] [3] [3] ...],  
}  

2 - Array of 9-dimensional vectores  
{  
    Data: [ [9], [9], [9]]  
}  

I like option 2 more. The parsing might be a little trickier, but we send
much fewer bytes, specially if we could send raw 4-byte floats instead of 
characters

We also need to define the commands we will send to the microcontroller. So 
here are a few questions:

- Do we want to specify which data to log? If so, how will we tell Serial Studio
the type and the length? Can the packet be of arbitrary length and order? (for 
example, combine arbitrary readings like mag with temperature or gyro and acc)
> [!WARNING] 
> Before getting too crazy with packet formats, research about packet
> parsing in Serial Studio

- What actions do we want the microcontroller to perform besides logging?


I suggest using argtable3 for implementing the terminal in the microcontroller

# September
## (09-09-26) On UART baudrate
The max baudrate of the STM32F407 is 5.25 MBit/s @ 16 bit oversampling (default)
The max baudrate of the ESP32-S3 is 5Mbit/s at the default clock source

# October
## (10-02-26) The Timer bug
1. Check peripheral registers. 
   1. DBG_TIM6_STOP bit in the DBGMCU_APB1_FZ register of the DBGMCU (debug MCU) peripheral
   2. UIE bit in the TIM6_DIER register
   3. UIF bit in the TIM6_SR register
   4. UG bit in the TIM6_EGR register
   5. TIM6_CNT register for the count

# (10-10-26) Towards safety critical code
The following compiler flags have been added
-Wall -Wextra -Wpedantic 
-Wconversion -Wsign-conversion -Wdouble-promotion -Wfloat-equal
-Wswitch-default -Wimplicit-fallthrough
-Wuninitialized -Wundef
-Wdangling-pointer -Wshadow -Wcast-qual -Wcast-align
-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
-Wwrite-strings -Waddress -Wformat
-Wunreachable-code -Wmissing-noreturn


(without newlines for copying)
-Wconversion -Wsign-conversion -Wdouble-promotion -Wfloat-equal -Wswitch-default -Wimplicit-fallthrough -Wuninitialized -Wundef -Wdangling-pointer -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wwrite-strings -Waddress -Wformat -Wunreachable-code -Wmissing-noreturn 

# Misc

Trace view command  
dump binary value trace.bin *RecorderDataPtr



# Bugs found
This section is just a few notes on strange bugs I found

1. **Inconsistent UAR**T: 5Mbit/s was too fast for the dupont cables to consistenly carry. Lowered to 500kbit/s
2. **Telecommand only firing once**: Forgot to rearm the info buffer by setting info_buffer_empty to true again
3. **Messages with '\n' characters nit behaving properly**: The newline was used as that end of message marker. Changed it to be a NULL terminator. This required a bit of overhaul. Ultimately, the NULL terminator marks the end of a single message, and the End Of Text character (ETX) now marks the end of the info buffer. changed strchr to memchr to bypass the NULL terminator when looking for the ETX char. In `ringbuff_push`, data_len now includes the NULL terminator of the message
4. **Info messages were "delayed" by one or more after init**: What I mean by this is that calling LOGI for message n will actually print message n-1. This happened because, in the logger task, I forgot to send a notification to itself after the buffer overflow error.