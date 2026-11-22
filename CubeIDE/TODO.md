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


- Make the debug packet ID be changed from a sys command instead of control, and have the Core handle it. DONE
- Add a debug packet for sending the RTC values DONE

___

- Update the specs to match the firmware changes (new commands, photo now uses the amount of photos instead of duration, changed telemetry packets)
- Failing to init one single module halts the floatsat. Have a more graceful error handling. 
- Add a command to set the RTC time and date
- Add the magnetometer calibration at startup

- IMPORTANT: Give the telemetry packets a 2-byte end delimiter so that the ESP32 can re-sync in the case of an invalid packet type detected **DONE**

- Add failsafe for angular velocity crossing a thereshold
- Add anti-windup for the control loop integral
- Add movement checks during gyro calibration

- Make ESP32 comms robust (consider reading everything into a buffer and parse it asynchrnously, because receiving, parsing and waiting is too prone to error) **DONE**

- We can't access the webserver after a cold start. (When we power on the system, we have to later restart the ESP32 for it to work) Look into that. **NEXT**

- Turn the ESP32 into a wifi access point and give it a URL so we can access it without the IP


## DEMO 1: Simple PID control



## Good to have
Verbose error log through telemetry packets
