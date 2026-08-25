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
  - Add conversion to metric units                DONE
  - Add init params test                          DONE
- Research unit test frameworks
- Implement IMU library hardware-independent tests
  - Test for catching invalid registers
  - Test for correct conversions
- Research FreeRTOS task trace
- Implement periodic task for reading the IMU