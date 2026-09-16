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
- Research FreeRTOS task trace                      DONE
- Implement periodic task for reading the IMU 

- Implement telecommands                            DONE
- Unit test all telecommands                        DONE

- Fnish telemetry design (add the info requests)    DONE
  - Unit Test Telemetry
  
- Supervisor                                        DOING
  - Implement changes in internal state through commands   DONE
  - Implement response to info requests
  -       

- A library for measuring the execution time of tasks DONE?

- Implement library for reaction wheel control
- Unit tests for RW control
  
- Implement Madwick filter                          DONE
- Unit tests for Madwick filter

- Implement simple PID control
- Unit test PID control

- Implement clibration
- Unit test calibration

## DEMO 1: Simple PID control



## Good to have
Verbose error log through telemetry packets
