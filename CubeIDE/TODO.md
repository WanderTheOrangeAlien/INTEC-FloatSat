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
  
- Move telecommand parsing to the ESP32             PAUSED
  - Test that the parsign is correct

- Supervisor                                        PAUSED
  - Implement changes in internal state through commands   DONE
  - Implement response to info requests
  -       

- A library for measuring the execution time of tasks DONE?

- Fix the logger                                    DOING

- Implement library for reaction wheel control      DONE
- Unit tests for RW control
  
- Implement Madwick filter                          DONE
- Unit tests for Madwick filter

- Implement simple PID control                      DONE
- Unit test PID control

- Implement clibration                              DONE
- Unit test calibration

- Make the debug packet ID be changed from a sys command instead of control, and have the Core handle it.
- Add a debug packet for sending the RTC values

## DEMO 1: Simple PID control



## Good to have
Verbose error log through telemetry packets
