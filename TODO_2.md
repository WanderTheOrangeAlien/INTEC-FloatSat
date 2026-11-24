# TODO 2

Built on 2026-11-22 (end of S3) from the last sections of `CubeIDE/TODO.md`, `WBS_FloatSat_v0.3.md`
(written at `c589b82`) and the code at `72342a2`. IDs in brackets are WBS IDs.

Scope changes from the WBS:
- **BNO085 is out.** The LSM9DS1 + Fusion AHRS is the final attitude source. Dropped: ADC-12, ADC-14,
  ADC-15, ADC-16, and the BNO085 parts of ADC-05, EPS-02, PCB-02, PCB-04 and PCB-05. The IMU
  abstraction (ADC-13) only existed to swap in the BNO085, so it is now optional.
- **OTA is lowest priority** (the professor allows calling it out of scope). It is kept at the end,
  except for one cheap item that must happen early: reserving the bootloader pins on the PCB.

Priorities: **P0** = broken now or blocks other work · **P1** = needed for the missions/defense ·
**P2** = quality, do if time allows · **P3** = OTA.
Owner: M = Miguel, A = Axel (as in the WBS).

Deadlines: end of features S7 (Dec 20) · technical freeze end of S9 (Jan 10) · defense S11.

---

## P0 - Broken now / blocking

- [x] **Boot bug, still half open** [CDH-13] (M). The `!` in `LightSensor_Init` is fixed, but
      `CubeIDE/App/Src/floatsat_init.c:99` still has `&veml7700_arr[3]` (array size is 3, out of
      bounds). Must be `[1]`. Build and boot on the bench to confirm.
- [ ] **I2C mutex** [CDH-01] (M). `floatsat_control.c:153` TODO. The IMU and the light sensors share
      the bus. Must be done before the light sensors get their own task.
- [ ] **Anti-windup: check it really exists** [ADC-06] (M). `TODO.md` says DONE, but in the cascade
      controller (`floatsat_control.c:255-275`) `u_I` is unbounded, `static`, and never reset when
      control is disabled or the target changes. Clamp it (or stop integrating while saturated) and reset
      it on enable/disable.
- [ ] **ESP32 WebSocket client cleanup** (M). Already in `TODO.md`. Slots in `clients[]` are never
      freed and every send goes to `clients[0]`. After a PC-side reconnect, data goes to a dead socket;
      after 4 reconnects nobody receives data. Free the slot on close (`close_fn`) and send to the newest
      client. Without this, the terminal's auto-reconnect only helps when the ESP32 itself rebooted.
- [ ] **Graceful init error handling** [CDH-06] (M). Already in `TODO.md`. One failed module sends
      `main` to `Error_Handler`. Report which peripheral failed and keep running in a degraded/SAFE state
      when possible.

## P1 - Safety (ADCS override) [ADC-10]

- [x] Angular velocity failsafe: threshold with hysteresis (M). The threshold value is still TBD in
      REQ1.2; get it from the air tests / simulation [ADC-03].
- [x] Detect wheel saturation (M): PWM stuck at the limit and unable to hold position (`TODO.md`).
- [x] On trigger (M): disable/reset the integrator, reload default gains, go back to 0°, and send an
      event in telemetry.
- [ ] Suggested (M): stop the motor if no command or heartbeat arrives from the GS for N seconds.
      Right now, if the link drops during a test, nobody can stop the wheel.

## P1 - Core, commands and missions framework

- [x] Mission FSM [CDH-03] (M): INIT, M0, M1, M2, SAFE. Check that `Core_ChangeMission` uses its
      argument and that `CORE_ACTION_CHANGE_MISSION` is consumed. Close the current mission safely when
      switching.
- [x] Allowed commands per mission (M) (`TODO.md` idea: bitmask over the action flags).
- [x] Init sequence [CDH-06] (M): calibration, then stabilize at 0°, then enable control and
      commands. NOTE: Control is disable by default so the floatsat can be tests without air
- [ ] Command responses [CDH-04] (M): structured ACK/NACK with an error code. Validate ranges
      before applying. Add default parameters and a firmware version string.
- [ ] Telemetry rate command for M0 [CDH-05] (M).
- [ ] `shutdown` command [CDH-09] (M): close the mission and stop the motor. Useful without OTA too.
- [ ] Extended telemetry [CDH-08] (M): subsystem health, current mission/state, light sensor values.
      `Telemetry_AddSlow` has no callers yet.

## P1 - Mission 2 (camera)

STM32:
- [x] Delete the photo entry after it is taken (`TODO.md`).
- [x] Delete the `photo move` command (`TODO.md`, parser still has `argtable_photo_move`).
- [x] Handle `on_photo_fail` (`TODO.md`). The ESP32 already sends it; the STM32 only maps
      `on_photo_finish`.
- [x] Timeout while waiting for the ESP32's finish/fail callback, plus an abort path [CDH-11].
- [NO] Wait for real stability (angle error and ω under a threshold for some time) instead of the fixed
      `CORE_PHOTO_PREPARATION_TIME_S` [CDH-11].
- [x] Validate the photo list [CDH-10]: enough time to maneuver
      between entries.

ESP32:
- [ ] Turn on the real OV2640 driver [PAY-05] (A/M). `#include "Arducam_OV2640.h"` is still commented
      out, so `wifi-uart-bridge.c` only sends the embedded test image. Test capture, pick the resolution,
      measure image size and capture time. Check the SPI `mode` TODO in `Arducam_OV2640.c:44`.
- [ ] Camera task stack is still 2048 B (`ESP32/main/include/project_defs.h:19`). Raise it to 4096 and
      check the high-water mark.
- [x] Use `photo_amount` (today one photo is taken whatever the amount).
- [ ] Photo metadata (photo list index, timestamp, angle) in the camera header, so the GS can name and
      show the images [PAY-06].

## P1 - Mission 1 (Sunchaser)

- [ ] Light sensor task (M): call `LightSensor_ReadAll` at 10 Hz or more (no caller yet), using the I2C
      mutex. Remove any `HAL_Delay` from the read loop [PAY-01].
- [ ] Mount the sensors and characterize them [PAY-02] (A): angle between the side sensors, response
      vs. angle with a lamp, background light, detection threshold.
- [ ] Mission skeleton, then the FSM [PAY-03] (M): SEARCH, ACQUIRE, TRACK, LOST using equations (1)
      and (2) from the proposal. Feed the angle reference into the control loop. Tune K_s and the timeout.
- [ ] Test with air [PAY-04] (A+M): acquire, track a moving lamp, lose and re-acquire it.

## P1 - ADCS with the LSM9DS1 as the final sensor

- [X] **Decide the 0° reference** [ADC-05] (A+M): magnetic north (current behavior) or the orientation
      at startup (what the proposal says). Record it in the decision log. (The 0 degree reference is the magnetic north)
- [ ] Magnetometer calibration (M): `IMU_MAG_OFFSET` / `IMU_MAG_SCALE` are hard-coded. At least redo
      them with the final mounting (`mag_cal.py`). Measure how much the motor disturbs the magnetometer.
- [ ] Movement check during gyro calibration (M) (`TODO.md`).
- [X] Confirm the IMU axis orientation (M) (`floatsat_control.c:16` TODO).
- [ ] Startup checks [ADC-08] (M): motor/encoder polarity, stuck wheel.
- [ ] Yaw drift over 10 min, motor on and off [ADC-04] (M).
- [ ] Finish air tuning and fix the default gains [ADC-09] (A+M). Commit the sysid CSVs and
      `floatsat_sysid_preliminar_v2.m` to `MATLAB/`.
- [ ] Control verification [ADC-11] (M): sweep of 12 angles plus injecting unstable gains (this must
      trip the override).

## P1 - Link and telemetry

- [ ] CRC16 on the packets [CDH-02] (M). `floatsat_telemetry.c:470` TODO, and the ESP32 sets it to 0.
      The end markers help resync but do not detect corrupted bytes.
- [ ] Link test [COM-07] (A): 30 min at 100 Hz, losses and latency, RSSI and range (closes the WiFi
      range TBD).
- [ ] Decide the V/I/T route [COM-04] (A+M): the ESP32 publishes its own EPS packet, or forwards it to
      the STM32.
- [ ] INA219 + MCP9808 driver on the ESP32 [EPS-03] (A): 1-2 Hz, low-battery alarm.

## P1 - Ground station (A)

- [ ] Decide the technology [GS-01] (web page vs. Python). `Python/floatsat_term/protocol.py` and
      `link.py` already decode every packet and can be reused (camera and reconnection included).
- [ ] Skeleton [GS-02]: WebSocket client, status bar, menu, "Abort mission".
- [ ] Telemetry/status screen [GS-03]: indicators, 3D model, yaw vs. reference plot.
- [ ] M0 screen [GS-04]: setpoint, edited vs. confirmed parameters (needs ACK/NACK), plot, CSV.
- [ ] M1 screen [GS-05]: orientation, ω, light signals, mission state.
- [ ] M2 screen [GS-06]: photo list editor with local validation, image viewer and history.
- [ ] Set the RTC when connecting (`TODO.md`). The `sys` command already exists.
- [ ] Real ground station integration (`TODO.md` NEXT 2).

## P1 - Power and PCB (A)

- [ ] Measure current draw [EPS-01]: idle and with the wheel at 10/30/50 %, plus peaks.
- [ ] Power budget [EPS-02]: measured draw + ESP32 WiFi + camera (no BNO085); margin on 19.98 Wh
      for 1 h; D24V50F5 limit.
- [ ] Final pin table [PCB-02]: main UART, I2C, camera SPI/I2C, ESP32 EN. **Also reserve the bootloader
      UART (USART1 or USART3), BOOT0 and NRST lines for OTA**, even though OTA is P3. Leaving them out of
      the PCB rules OTA out for good.
- [ ] Manufacturing decision [PCB-03] → schematic [PCB-04] → layout and review [PCB-05] → build and
      test [PCB-06] → mounting and balance, level within ±1° for 5 min [PCB-07].

## P1 - Integration and verification (A+M)

- [ ] M0 with air, driven from the GS (or a temporary version with `floatsat_term`) [INT-02].
- [ ] Verification plan [INT-03]: REQ → method → criterion → test.
- [ ] Move everything to the PCB [INT-04].
- [ ] Full missions: M1, and M2 with 5 photos, from the GS; mission change from every state;
      override [INT-05].
- [ ] 20 battery boots with injected faults; 1 h mission logging V and I [INT-06].
- [ ] Regression and tag all firmware at the freeze, end of S9 [INT-07].
- [ ] Create subsystem failure tests (`TODO.md` NEXT 2).

## P1 - Docs and repo

- [ ] Update the specs (`TODO.md`): new commands, photo amount instead of duration, telemetry packets
      with end markers, camera packets, `sys --invoke-cb` callbacks, 500 kbaud, AP + mDNS.
- [ ] Requirements v1.1 [GES-05]: ω threshold, WiFi range, write REQ-M1, change REQ2.2 to "amount".
- [ ] `decision_log.md` [GES-01]: 500 kbaud, absolute RTC time for photos, photo amount, AP + mDNS
      WiFi mode, no BNO085, 0° reference, Fusion AHRS.
- [ ] Repo cleanup [GES-01]: stop tracking `CubeIDE/Debug/` (53 files), `trace.bin`, `logs/`,
      `sdkconfig.old`, `Python/results.zip`, `.claude/settings.local.json`.
- [ ] WiFi passwords are now in Kconfig, but the old ones are still in the git history. Change those
      passwords if the repo is public.
- [ ] Fix the `usart.c` comment (says USART1 PB6/PB7, code uses USART2 PA2/PA3).
- [ ] User and maintenance manual [GES-06], final report [GES-07], defense video and slides [GES-08].
- [ ] Student test plus two recorded rehearsals [INT-08] (S10).

## P2 - Tests and nice-to-haves

- [ ] Ceedling tests: PID/cascade, mission FSM, photo list validation [CDH-12].
- [ ] Older unit tests still open in `TODO.md`: telemetry, RW control, calibration, IMU conversions.
- [ ] IMU abstraction [ADC-13]: optional now that the BNO085 is gone.
- [ ] Verbose error log through telemetry packets (`TODO.md` "Good to have").

## P3 - OTA (last; can be declared out of scope)

- [ ] Bootloader UART and pins [OTA-01] (M): AN2606/UM1472; USART2 is not a bootloader port on the F407.
      *(Pin reservation is pulled up into the PCB section.)*
- [ ] Bootloader on the bench [OTA-02] (M): BOOT0=1 + NRST pulse, 0x7F sync, Get / Get ID, first
      with USB-TTL, then from the ESP32.
- [ ] AN3155 client on the ESP32 [OTA-03] (M): erase, block write with verify, Go, retries. Look at
      `esp-serial-flasher`.
- [ ] Full flow [OTA-04] (M): HTTP upload of the `.bin`, CRC32, `shutdown`, flash, reboot,
      confirmation.
- [ ] GS OTA screen [GS-07] (A).
- [ ] End-to-end test [OTA-05]: 10/10 cycles, forced power cut and recovery.

---

## Done since the WBS (for the record)

- UART desync between the ESP32 and the STM32 fixed: new parser plus end markers (`a02f9a6`, notes bug 8) [COM-01, INT-01]
- Control loop working (`a02f9a6`); STM32 resets the ESP32 on boot; `on_bridge_active` handshake
- RTC set command (`sys`) [CDH-14, GS sync still pending]
- ESP32 access point + mDNS (`floatsat`), selectable in menuconfig [COM-02/03, decision still to record]
- WiFi credentials moved to Kconfig [COM-01]
- JPEG over WebSocket with header and chunks; most of the ESP32 bugs from the review fixed (`72342a2`) [COM-06]
- `floatsat_term`: image reception and auto-reconnect
- `LightSensor_Init` condition fixed [CDH-13, the index bug is still open]
