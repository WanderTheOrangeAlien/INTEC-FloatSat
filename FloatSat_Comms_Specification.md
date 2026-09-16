# FloatSat comms specification
This documents details the format and workings of the FloatSat communication 
system.  

## 1. Definitions
- Telecommand: A command sent from the ground station to the FloatSat.
- Telemetry: Internal FloatSat data periodically sent to the ground station. 
This data is classified in Fast and Slow, depending on the data source. 
- Info Packet: A packet that contains miscellaneous information about the 
FloatSat, for example, its current photo sequence. These are long text streams, 
and are requested by the ground station. 


## 2. Introduction

Outgoing communications from the FloatSat happen synchronously after every control
update. High priority data (Fast Telemetry) is guaranteed to be sent during the 
FreeRTOS tick of the Telemetry Task. After fast telmetry is sent Slow Telemetry and Info Packets are sent in the remaining time. 


The priority of tasks and overall task structure must guarantee that the 
following assumption always hold true:
> **The Control and Telemetry tasks always run to completion (never interrupted by a task switch)**

The complete supervisor + control loop + telemetry sequence is the following:  


SUPERVISOR
1. Check if it is time to send Slow Telemetry 
2. Check for received commands
3. Update internal state according to commands
4. Generate Info Packets if requested. Then queue it
5. Update control system siignals according to the internal state and wake up control task

CONTROL TASK  
6. Read the IMU  
7. Read reaction wheel encoder speed  
8. Apply madwick filter to estimate the attitude quaternion  
9. Convert to 3D vector and get euler angles. Write them to shared memory [1]  
10. Use euler angles and RW speed as input to the control system  
11. Send appropiate signals to the reaction wheel driver  
12. Wake up Telemetry task  
   
TELEMETRY TASK  
13. Send fast telemetry  
14. Send Slow Telemetry and Info Packets  

After step 12 ends (control loop complete), the Telemetry Task is woken up. 
The task first sends the Fast Telemetry, and then, in the remaining time of the
current FreeRTOS Tick, sends any pending Slow Telemetry and Info pakcets.

Slow Telemetry is guaranteed to be sent on the same tick as the Fast one, however, Info Packets
are not. To address this problem, the comms system divides the the Info Packets
into fixed-length chunks and sends them in the remaining time of the current 
Tick. Before sendig a chunk, it checks whether there is enough time left in the 
tick for it to be sent completely, with a safety margin. If not, it cancels the
transmission and yeilds

_____
[1] This shared memory is a small buffer protected by atomic access (critical 
section), shared only between the Control and Telemetry Tasks.


## Telemetry types

Fast and slow telemetry share a parent structure simply called 
`telemetry_packet_t`. This structure contains a header, and then the fast and 
slow packets structures

### Packet (`telemetry_packet_t`)
| Name     	| Type   	                  | Size 	| Notes                 |
|----------	|--------	                  |------	|----------             |
| type 	    | uint8_t                   | 1   	|                       |
| timestamp | uint64_t                  | 8    	| ms since powered on   |
| fast      | telemetry_packet_fast_t   | 16    |                       |
| slow      | telemetry_packet_slow_t   | TBD   |                       |   


### Fast telemetry (`telemetry_packet_fast_t`)
| Name     	| Type   	| Size 	| Notes                 |
|----------	|--------	|------	|----------             |
| Attitude 	| Vec3_t 	| 12   	|                       |
| RW_Speed 	| float  	| 4    	| Reaction wheel speed  |

Fast telemetry is sent after every control update. 

### Slow Telemetry (`telemetry_packet_slow_t`)

(Place table here)

Slow telemetry contains data from sensors with small rates of change, for 
example, battery and temperature. Moreover, slow telemetry includes subsystem 
health indicators. The packet size is larger than Fast Telemetry, and is sent
at larger intervals


## Info packets

### Header
| Name     	    | Type   	  | Size 	    | Notes                 |
|----------	    |--------	  |------	    |----------             |
| ID            | uint8_t   | 1         | PACKET_TYPE_INFO_HDR  |
| timestamp     | uint64_t  | 8         | ms since boot         |
| nChunks       | uint8_t   | 1         |                       |
| lastChunkSize | uint8_t 	| 1   	    |                       |
| CRC           | uint16_t 	| 2   	    | CRC of the whole message before dividing|

### Chunk
| Name     	    | Type   	  | Size 	                    | Notes                 |
|----------	    |--------	  |------	                    |----------             |
| ID            | uint8_t   | 1                         | PACKET_TYPE_INFO_CHUNK  |
| chunkNumber   | uint8_t   | 1                         |                         |
| Data          | uint8_t[] | `INFO_PACKET_CHUNK_SIZE`  |                         |

## Communication Logic

The role of the Telemetry Task is only to transmit data through UART, it does 
not create or queue data by its own. Data is instead queued into the 
transmission buffer by other taks, through functions exposed by the telemetry 
module. The Control task inserts Fast Telemetry, while the Supervisor Task 
inserts Slow Telemetry and Info packets.  

Slow Telemetry is queued at larger intervals (To be defined) and the Info packets
are only queued after receiving an info request command (for example, `photo -i`)

The data queuing done by the Supervisor has the following logic:  
- Check if it is time to send slow telemetry
  - If so, generate and queue it using `Telemetry_AddSlow`
  - If not, continue 
- Check if an info request command was received
  - If so, generate and queue the requested data using `Telemetry_AddInfo`
  - If not, continue

After the Supervisor runs, the Control Task performs its control loop and, at 
the end, adds Fast Telemetry with `Telemetry_AddFast` 
 
Internally, the Telemetry Module keeps two buffers: one for Fast and Slow 
Telemetry, and one for Info Packets. It also contains a flag that indicates the 
type of data that had been queued, as shown in the following table. The 
Telemetry_AddXX functions set these bits accordingly

| Bit2      |  Bit1       |    Bit0         |
| ----------|-------------|--------------   |
| TELE_INFO | TELE_SLOW   |   TELE_FAST     |

Since Fast and Slow Telemetry are of fixed size, we decided to merge the packets 
into a single one to avoid header overhead. When only sending Fast Telemetry, 
(`TELE_SLOW` bit not set) the packet type is set to `PACKET_TYPE_FAST` and the 
program knows that it has to send only `PACKET_SIZE_FAST` bytes. If we are going 
to also send Slow Telemetry (`TELE_SLOW` bit set), the packet type is changed to
`PACKET_TYPE_EXTENDED` and the program knows to send `PACKET_SIZE_EXTENDED` bytes

This logic therefore requires that Fast Telemetry is always present. If the 
`TELE_FAST` bit is not set when the Telemetry Task reads the flag, it indicates 
an error.

The Info Pakcet can vary in size, that's why a separate buffer is used. 
`Telemetry_AddInfo` takes the size in bytes and divides the data into chunks of 
size `INFO_PACKET_CHUNK_SIZE`. In case the size was not a multiple of this 
number, the lastChunkSize is set as the remaining amount of bytes. This way the 
receiver knows how many chunks to wait before parsing the packet. As explained 
before, the chunks are sent only if there is enough time left in 
the FreeRTOS Tick, otherwise, the transmission is cancelled and the module 
prepares for continuing the transmission next time the task runs.

Each chunk contains a packet type and a chunk number, so the ground station 
knows that the it is receiving pending information.