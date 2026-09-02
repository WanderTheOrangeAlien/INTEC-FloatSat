# Phase 1
We need to perform the following activities

- Run hardware-independent unit tests
- Initialize IMU
- Read from IMU
- Estimate state vector
- Have periodic tasks
- Trace the tasks

## Tasks
- Complete the IMU library
  - Add conversion to metric units                  DONE
  - Add init params test                            DONE
- Research unit test frameworks                     DONE
- Implement IMU library hardware-independent tests  
  - Test for catching invalid registers             DONE
  - Test for correct conversions
- Research FreeRTOS task trace
- Implement periodic task for reading the IMU 

- Implement telecommands                            DOING NOW
- Unit test all telecommands                        NEXT

- Implement library for reaction wheel control
- Unit tests for RW control
  
- Implement Madwick filter (to estimate azimuth)
- Unit tests for Madwick filter

- Implement simple PID control
- Unit test PID control

- Implement clibration
- Unit test calibration

## DEMO 1: Simple PID control
- 