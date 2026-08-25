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