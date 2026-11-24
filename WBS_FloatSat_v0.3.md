# WBS - Plataforma educativa FloatSat
Trimestre nov 2026–ene 2027 · v0.3 · 17-nov-2026 (S3) · Reemplaza la v0.2 del 22-oct

Base de esta versión: repositorio `INTEC-FloatSat` al commit `c589b82` (16-nov-2026, 27 commits) y el anteproyecto. Solo se verificó lectura de código y notas del repo: no se compiló ni se corrió nada en hardware. Todo lo que vive fuera del repo (GS, PCB, EPS, CSV y scripts de identificación, estudio del BNO085) figura como "Sin evidencia en repo" hasta que ustedes lo confirmen.

## 1. Cómo leer las tablas
Columnas igual que en la v0.2 (ID, Tarea, Detalle, Tipo, Dep., Sem., Resp., h, REQ, P). Cambia la columna Estado:

| Estado | Significado |
|---|---|
| Hecho | El código o documento existe en el repo y cubre el criterio de la tarea. No implica prueba en banco salvo que se indique |
| Parcial | Existe una parte; "Resta:" dice qué falta |
| Pendiente | Sin rastro en el repo |
| Sin evidencia | La tarea no deja rastro en el repo por naturaleza (hardware, investigación, GS). Confirmar fuera de banda |
| Atrasada | Se añade a Parcial/Pendiente cuando la semana de fin prevista ya cerró (antes de S3) |

## 2. Calendario y capacidad
| Sem. | Fechas | h/persona | Estado |
|---|---|---|---|
| V1 | 19–25 oct | 30 | Cerrada |
| V2 | 26 oct–1 nov | 70 | Cerrada |
| S1 | 2–8 nov | 35 | Cerrada |
| S2 | 9–15 nov | 35 | Cerrada |
| S3 | 16–22 nov | 35 | **En curso** |
| S4 | 23–29 nov | 35 | |
| S5 | 30 nov–6 dic | 35 | |
| S6 | 7–13 dic | 35 | |
| S7 | 14–20 dic | 35 | Fin de implementación de funciones |
| S8 | 21–23 dic | 20 | Parcial |
| SN | 24 dic–3 ene | 30 | Pausa navideña, ritmo reducido |
| S9 | 4–10 ene | 35 | Congelamiento técnico al cierre de la semana |
| S10 | 11–17 ene | 35 | Solo verificación, documentación y ensayos |
| S11 | 18–23 ene | 15 | Defensa |

Capacidad total aproximada de 480 h por persona. El plan suma unas 325 h para Axel y unas 389 h para Miguel (+4 h por CDH-13 y CDH-14), más 70 h de reserva por persona. Quedan ~8.5 semanas hasta el congelamiento técnico (cierre de S9) contando la pausa navideña.

## 3. Supuestos
- Llegada de componentes (H): la v0.2 la suponía en S1. **No hay evidencia de que haya ocurrido**; confirmar. Si no llegaron, ADC-14, PAY-05, EPS-03 y PCB-01 están bloqueadas y R2 está activo.
- Aire comprimido desde S1.
- La actuación se limita al eje de yaw. **Referencia de 0°: la v0.2 supone la de inicialización, pero el código actual la da respecto al norte magnético** (ver ADC-05 y sección 7).
- Fuente de actitud final: BNO085, con LSM9DS1 + Madgwick como línea base y respaldo tras una abstracción de IMU (ADC-13).
- Controlador: cascada del anteproyecto (lazo interno PI de velocidad del cuerpo por giroscopio, lazo externo P de ángulo). LQR fuera de alcance.
- Fuera de alcance: OTA de la propia ESP32, modo degradado autónomo sin ESP32, más de un eje de actuación.

## 4. Punto de partida (ya hecho, no se planifica)
| Área | Hecho |
|---|---|
| Definición | Objetivos, modos de operación, arquitectura funcional, bocetos de las cinco pantallas de la GS, esquemático preliminar de la PCB (anteproyecto) |
| Firmware base | Proyecto CubeMX con FreeRTOS, `floatsat_init` por módulos, códigos de error, flags estrictos del compilador, trazas con Percepio, objetos FreeRTOS creados tras inicializar periféricos, bug del tick HAL/TIM6 resuelto (`410a25b`), recuperación de bus I2C (`I2C_BusRecover`) |
| IMU y estimación | Driver LSM9DS1 con tests (Ceedling), Madgwick, vectores y cuaterniones con tests, bus de datos IMU (`data_bus`) entre Control y Core. Bug de deriva de actitud resuelto: el magnetómetro (mG) dominaba al filtro, ahora se normaliza (`c5c5009`). Sesgo del giroscopio calibrado al arranque |
| Actuador | PWM bidireccional del TB9051, velocidad de rueda por encoder, comando `motor pwm` |
| Control | **Nuevo:** tarea de control a 100 Hz, cascada PI de velocidad (giroscopio z) + P de ángulo con wrap ±180°, saturación a 0.65 de duty y 360 °/s, habilitar/deshabilitar y cambiar ganancias y objetivo en caliente por comando `control` (`c5c5009`) |
| Telecomando | Recepción UART DMA, parser `argtable3`, comandos `control`, `mission`, `photo` (add, edit, remove, reset, take, info), `motor`, `imu`, `sys`; uniones etiquetadas en lugar de puntero void (`d7442ad`). Tests unitarios del parser |
| Telemetría y logging | Paquetes rápido y de información en chunks por UART DMA, logging diferido, paquetes de depuración (IMU, control, RTC). Telecomando y telemetría estables de extremo a extremo (`410a25b`) |
| Fotos (lado STM32) | **Nuevo:** lista de hasta 32 instrucciones, RTC con alarmas, máquina IDLE→MOVING→BUSY, envío de `EXT_CMD_TAKE_PHOTO` a la ESP32 y cierre con `sys --invoke-cb on_photo_finish` (`d7442ad`) |
| Sensores de luz | **Nuevo, sin integrar:** drivers TCA9548A y VEML7700 y wrapper `floatsat_light_sensor` (`c589b82`) |
| Enlace | UART a **500 kbaud** (USART2, PA2/PA3; la v0.2 decía 115200; 5 Mbit/s falló con cables dupont), ESP32-S3 con ESP-IDF, WiFi STA, servidor WebSocket, reenvío UART→WS y WS→UART (`esp-restart` local) |
| Herramientas | FloatSatPlotter (gráfica en vivo, CSV); **nuevo:** `floatsat_term` (consola asíncrona por WebSocket con decodificador de paquetes) y procedimientos de prueba: `motor_sweep`, `angle_log`, `mag_log`, `control_log`, más `mag_cal.py` |

Decisiones tomadas de facto en el código que aún no están en `decision_log` (ver GES-01): baud 500 kbaud; hora absoluta HH:mm:ss (UTC-4) para las fotos con RTC; fin de foto señalizado por `sys --invoke-cb`; acciones diferidas por banderas en el Core; normalización del magnetómetro antes de Madgwick; ESP32-S3 como compañera.

## 5. WBS por subsistema

### CDH - Firmware STM32: base, Core, comandos y misiones
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| CDH-01 | Estabilizar la base | Cerrar el bug del tick de TIM6, hacer funcionar la tarea Logger, mutex del I2C (IMU y sensores de luz comparten bus), condición de carrera entre DMA y parser | IMP | - | V1-V2 | M | 12 | - | P1 | Parcial. Tick y Logger resueltos (`410a25b`, `56586ab`). **Resta:** mutex I2C (`fs_i2c_handle_t.mutex` declarado y sin usar; TODO en `Control_EstimateOrientation`); carrera DMA/parser sin verificar |
| CDH-02 | CRC16 e ICD | CRC16 en todos los paquetes, número de secuencia o timestamp para detectar pérdidas; documentar formato byte a byte | IMP | - | V2 | M | 6 | REQ4.2,REQ7.1 | P1 | Parcial · atrasada. Existe `FloatSat_Comms_Specification.md`. **Resta:** CRC sigue en TODO (`floatsat_telemetry.c:441`, ESP32 lo fija en 0); `Telecommand_Specificatioon.md` quedó atrás (describe `photo add -d`, el código tiene add/edit/remove/move/take, `sys --invoke-cb`, baud 500k) |
| CDH-03 | FSM de misiones en el Core | Lazo del Core sin bloqueo, estados INIT, M0, M1, M2, SAFE y preparación OTA; cambio de misión desde cualquier estado cerrando la actual de forma segura | IMP | CDH-01 | V2 | M | 14 | REQ2.3 | P1 | Parcial · atrasada. Core se despierta por notificación del control. **Resta:** `Core_ChangeMission` ignora el argumento y la bandera `CHANGE_MISSION` no se consume; `Core_Mission_Playground` vacío; no hay estados INIT/SAFE/OTA ni cierre seguro |
| CDH-04 | Respuesta a comandos y parámetros | ACK/NACK con código de error y estado confirmado; respuestas a `-i`; conjunto de parámetros por defecto y rangos válidos, sin cambios parciales si se rechaza | IMP | CDH-03,CDH-02 | S1 | M | 12 | REQ7.1,REQ2.1 | P1 | Parcial · atrasada. Las respuestas `-i` existen como texto de log. **Resta:** sin ACK/NACK estructurado; `Core_SetControl` aplica cada campo por separado sin validar rangos (cambios parciales posibles); sin defaults de respaldo; sin versión de firmware |
| CDH-05 | M0 Playground | Referencia y ganancias en caliente; comando para la cadencia de telemetría | IMP | CDH-04,ADC-07 | S2 | M | 6 | REQ2.1 | P1 | Parcial. Referencia y ganancias en caliente funcionan por `control`. **Resta:** comando de cadencia de telemetría; depende de CDH-04 para ser verificable desde la GS |
| CDH-06 | Secuencia de inicialización | Chequeo de periféricos con reporte de fallas, calibración, estabilizar en 0° y habilitar comandos | IMP | CDH-03,ADC-08,ADC-07 | S3 | M | 6 | REQ7.2 | P1 | Parcial. `Core_Task` calibra al arrancar. **Resta:** reporte de fallas por periférico (hoy un fallo de init llama a `Error_Handler`), estabilizar en 0° y habilitar el control (`control_active` nace en false) |
| CDH-07 | DEC tiempo de las fotos | Tiempo relativo al inicio de la misión frente a hora absoluta UTC-4 | DEC | - | S3 | A+M | 1 | REQ2.2 | P1 | Hecho de facto, sin registrar. El código usa hora absoluta con RTC. **Resta:** registrarla en `decision_log` y resolver CDH-14 |
| CDH-08 | Telemetría extendida | Salud de subsistemas, misión y estado, señales de los tres sensores de luz | IMP | CDH-02,PAY-01 | S4 | M | 4 | REQ4.1 | P1 | Pendiente. `Telemetry_AddSlow` no tiene llamadores; el paquete lento solo tiene `temp` y `v_batt` |
| CDH-09 | Comando `shutdown` | Cierre de misión, motor detenido, paquete `system` de listo para OTA | IMP | CDH-03 | S4 | M | 4 | REQ2.4 | P1 | Pendiente |
| CDH-10 | Secuencia de fotos | Validación (orden temporal, ángulo [0, 359.9], duración, tiempo de maniobra), informe de errores, sin `malloc` en el parser | IMP | CDH-04,CDH-07 | S5 | M | 8 | REQ2.2,REQ7.1 | P1 | Parcial (adelantada). add/edit/remove/reset/take/info hechos, sin `malloc` según `d7442ad`. **Resta:** validación inexistente; `photo move` sin implementar; **el código maneja "cantidad de fotos" y REQ2.2 pide "duración"**: decidir cuál y alinear código, spec y requisitos |
| CDH-11 | Ejecutor de la Misión 2 | Ir al ángulo, esperar estabilidad, PHOTO_REQ a la ESP32, esperar DONE/ERROR con timeout, siguiente elemento; aborto | IMP | CDH-10,COM-05,ADC-09 | S6 | M | 10 | REQ2.2 | P1 | Parcial (adelantada). Alarma a 10 s antes, MOVING→BUSY, envío a la ESP32, cierre por callback. **Resta:** esperar estabilidad real (hoy temporizador fijo `CORE_PHOTO_PREPARATION_TIME_S`, con TODO de medirlo), timeout, ruta de ERROR, aborto |
| CDH-12 | Tests unitarios de lo nuevo | Ceedling para el PID, la FSM de misiones y la validación de fotos | PRU | CDH-04 | S3-S7 | M | 10 | - | P2 | Pendiente. Hay tests de log, vec_math, parser y LSM9DS1; ninguno de control, FSM ni fotos |
| CDH-13 | **Nuevo** Desbloquear el arranque en `main` | Corregir `LightSensor_Init` (condición `\|\| handle->sensors[2]` sin `!`, devuelve `ERR_INVALID_ARG` siempre) y `floatsat_init.c` (`&veml7700_arr[3]` fuera de rango, debe ser `[1]`). Hoy `FloatSat_Init` falla y `main` cae en `Error_Handler`. Verificar compilando y arrancando | IMP | - | S3 | M | 1 | - | P1 | Pendiente |
| CDH-14 | **Nuevo** Fijar y sincronizar la hora del RTC | Comando `sys` para fijar hora (existe `RTC_SetTimeDate`, ningún comando la expone), la GS la envía al conectar; tolerancia a deriva | IMP | CDH-07 | S4 | M | 3 | REQ2.2 | P1 | Pendiente |

### ADC - Determinación y control de actitud
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| ADC-01 | Esqueleto de `Control_Update` | Lectura atómica de referencia y parámetros, lazo abierto por comando, `Motor_GetSpeed` | IMP | - | V1 | M | 6 | REQ1.1 | P1 | Hecho |
| ADC-02 | Caracterizar la rueda | Velocidad vs duty, zona muerta, saturación y constante de tiempo | PRU | ADC-01,INT-01 | V2 | M | 8 | REQ1.1 | P1 | Hecho (preliminar). `motor_sweep.py` y corridas `id`/`val` v2. **Los CSV y `floatsat_sysid_preliminar_v2.m` no están en el repo** (`MATLAB/` solo tiene `vectors.m` y `ControlSystem_View_v1.m`): versionarlos |
| ADC-03 | Simulación del lazo en cascada | Python/MATLAB: planta 1/(J·s), actuador con saturación y zona muerta, retardo de 10 ms; sintonía inicial y umbral de ω de seguridad (TBD de REQ1.2) | DIS | ADC-02 | V2 | A | 12 | REQ1.1,REQ1.2 | P1 | Parcial. Hay candidato PI preliminar. **Resta:** confirmar si la simulación incluye zona muerta, saturación y retardo; fijar el umbral de ω (sigue TBD) |
| ADC-04 | Línea base con el LSM9DS1 | Ejes de la IMU, yaw contra ángulos conocidos, deriva de yaw en 10 min con motor apagado y encendido, efecto del motor en el magnetómetro | PRU | INT-01 | V2 | M | 8 | REQ1.1 | P1 | Parcial · atrasada. `angle_log`, `mag_log`, `mag_cal.py`; bug de deriva resuelto. **Resta:** TODO de orientación de ejes (`forward_vec`); deriva en 10 min con motor on/off sin evidencia; offset y escala del magnetómetro fijos en el código (`IMU_MAG_OFFSET`, `IMU_MAG_SCALE`) |
| ADC-05 | DEC interfaz y reporte del BNO085 **y referencia de 0°** | Bus y reporte (Game Rotation Vector sin magnetómetro frente a Rotation Vector). Incluye fijar la referencia de 0°: hoy es norte magnético vía Madgwick 9-DOF; el anteproyecto dice inicialización | DEC | ADC-04,ADC-12 | S1 | A+M | 1 | REQ1.1 | P1 | Pendiente · atrasada |
| ADC-06 | Lazo interno PI | Velocidad del cuerpo (giroscopio) a duty, discretizado a 100 Hz, anti-windup y saturación | IMP | ADC-01,ADC-03 | V2-S1 | M | 10 | REQ1.1 | P1 | Parcial. PI a 100 Hz con saturación (`c5c5009`). **Resta:** anti-windup real: la integral (`u_I`) no se acota ni se reinicia al deshabilitar el control o cambiar el objetivo |
| ADC-07 | Lazo externo P | Error de ángulo con wrap ±180° a consigna de velocidad, con límite | IMP | ADC-06 | S1-S2 | M | 6 | REQ1.1,REQ2.1 | P1 | Hecho. Ganancias por defecto k_theta 0.5, k_p -0.0123, k_i -0.0050 |
| ADC-08 | Calibración al arranque | Sesgo del giroscopio, polaridad motor/encoder, detección de rueda atascada | IMP | ADC-04,CDH-01 | S2 | M | 8 | REQ7.2 | P1 | Parcial. Sesgo del giroscopio hecho (`56586ab`). **Resta:** polaridad, rueda atascada; la calibración de acelerómetro y magnetómetro solo escribe un log |
| ADC-09 | Sintonía con aire | Identificar inercia, actualizar la simulación, escalón 0→90°, perturbaciones, fijar ganancias por defecto | PRU | ADC-07,AIRE | S2-S4 | A+M | 16 | REQ1.1 | P1 | Parcial. En curso con `control_log.py` y las corridas de identificación |
| ADC-10 | Override de seguridad | ω del cuerpo sobre el umbral (con histéresis) o rueda saturada, recargar parámetros por defecto, volver a 0°, evento en telemetría | IMP | ADC-09,CDH-04 | S4 | M | 10 | REQ1.2 | P1 | Pendiente. No hay rastro de supervisor/override en el código (G7 sin implementar) |
| ADC-11 | Verificar el control | Barrido de 12 ángulos e inyección de ganancias inestables | PRU | ADC-10,INT-02 | S7 | M | 6 | REQ1.1,REQ1.2 | P1 | Pendiente |
| ADC-12 | Estudio del BNO085 | Protocolo SH-2/SHTP, reportes y tasas, calibración dinámica, bus, pines, alimentación, montaje | INV | - | V2 | A | 8 | REQ1.1 | P1 | Sin evidencia en repo · atrasada |
| ADC-13 | Abstracción de IMU | Interfaz común con backend LSM9DS1 + Madgwick; Control y Telemetría usan solo esa interfaz | IMP | CDH-01 | V2 | M | 6 | REQ1.1 | P1 | Parcial · atrasada. `data_bus` desacopla consumidores. **Resta:** `Control_EstimateOrientation` sigue llamando a `IMU_ReadData` + Madgwick directo; `IMU_data_t` está atado al LSM9DS1 |
| ADC-14 | BNO085 en banco | Comunicación, reportes a 100 Hz y consumo con cableado provisional | PRU | PCB-01,ADC-05 | S2 | A | 4 | REQ1.1 | P1 | Sin evidencia en repo · atrasada |
| ADC-15 | Driver BNO085 en el STM32 | SHTP, 100 Hz con timestamp, recuperación de errores; segundo backend | IMP | ADC-13,ADC-14,CDH-01 | S5-S6 | M | 18 | REQ1.1,REQ7.2 | P1 | Pendiente. Ninguna referencia al BNO085 en el repo |
| ADC-16 | Calibración y deriva del BNO085 | Calibración dinámica, guardar DCD, deriva de yaw en 10 min, comparar con ADC-04 | PRU | ADC-15,AIRE | S7 | M | 6 | REQ1.1,REQ7.2 | P1 | Pendiente |

### COM - ESP32 y enlace WiFi
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| COM-01 | Puente bidireccional | WebSocket→UART con el byte de longitud, reconexión del cliente, credenciales fuera del código fuente | IMP | - | V1 | M | 6 | REQ4.2,REQ7.1 | P1 | Parcial · atrasada. WS→UART y UART→WS funcionan. **Resta:** credenciales fijas en `wifi-uart-bridge.c` (y `wifi_module.c` imprime la contraseña en el log); bug de desincronización tras reinicio del STM32 **sin resolver** (`notes.md` bug 7); reconexión del cliente sin evidencia |
| COM-02 | DEC modo WiFi en el laboratorio | SoftAP propio frente a STA en la red de INTEC | DEC | - | V2 | A+M | 1 | REQ6.2 | P1 | Pendiente · atrasada. De facto STA contra redes personales (casa/celulares), sin decisión de laboratorio |
| COM-03 | Aplicar el modo WiFi | Implementar lo decidido con IP o nombre fijo para la GS | IMP | COM-01,COM-02 | S1 | M | 3 | REQ6.2 | P1 | Pendiente · atrasada |
| COM-04 | DEC ruta de V/I/T | La ESP32 publica su propio paquete EPS hacia la GS, o lo envía al STM32 | DEC | - | S1 | A+M | 1 | REQ4.1 | P1 | Pendiente · atrasada. Sin código de INA ni MCP9808 |
| COM-05 | Interfaz de foto STM32↔ESP32 | Mensajes PHOTO_REQ, PHOTO_DONE y ERROR en el ICD | DIS | CDH-02 | S3 | M | 3 | REQ2.2 | P1 | Parcial. Existen `EXT_CMD_TAKE_PHOTO` (`external_cmd.h`) y el retorno por `sys --invoke-cb`. **Resta:** ERROR, metadatos, documentarlo en el ICD |
| COM-06 | Canal de imágenes | JPEG por WebSocket en chunks binarios con id y CRC, separado de la telemetría | IMP | PAY-05,COM-03 | S5 | A | 8 | REQ2.2 | P1 | Pendiente. El buffer de frame de la ESP32 es de 4096 B |
| COM-07 | Prueba del enlace | 30 min a 100 Hz, pérdidas y latencia; RSSI y alcance (resuelve el TBD de REQ6.1) | PRU | COM-03,GS-02 | S4 | A | 6 | REQ4.2,REQ6.1 | P1 | Pendiente |

### OTA - Carga inalámbrica de firmware
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| OTA-01 | Puertos del bootloader y pines | AN2606 y UM1472. USART2 no es puerto del bootloader del F407; elegir USART1 o USART3 revisando lo que la Discovery conecta; fijar BOOT0, NRST y BOOT1 | INV | - | V2 | M | 5 | REQ2.4 | P1 | Pendiente · atrasada. Sin rastro en el repo (ni código ni notas); R1 sigue sin mitigar |
| OTA-02 | Bootloader en banco | BOOT0=1 y pulso en NRST, sincronización 0x7F, Get y Get ID con USB-TTL y luego desde la ESP32 | PRU | OTA-01 | S1 | M | 6 | REQ2.4 | P1 | Pendiente · atrasada |
| OTA-03 | Cliente AN3155 en la ESP32 | Borrado, escritura por bloques con verificación, Go, reintentos. Evaluar `esp-serial-flasher` | IMP | OTA-02 | S2-S3 | M | 16 | REQ2.4 | P1 | Pendiente · atrasada |
| OTA-04 | Flujo OTA completo | Ruta HTTP de subida del `.bin`, CRC32, `shutdown`, flasheo, reinicio, confirmación | IMP | OTA-03,CDH-09 | S5-S6 | M | 12 | REQ2.4 | P1 | Pendiente |
| OTA-05 | Prueba extremo a extremo | 10 ciclos desde la GS con 10/10 éxitos, corte inducido y recuperación | PRU | OTA-04,GS-07 | S7-S8 | M | 6 | REQ2.4 | P1 | Pendiente |

### GS - Estación terrena
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| GS-01 | DEC tecnología de la GS | Página web local (HTML/JS con Three.js y uPlot, sin CDN) frente a extender FloatSatPlotter en Python | DEC | - | V1 | A+M | 1 | REQ5.1 | P1 | Sin evidencia en repo · atrasada. El repo solo tiene herramientas Python (consola y plotter) |
| GS-02 | Esqueleto de la GS | Cliente WebSocket, decodificador de paquetes probado, barra de estado, menú, «Abortar misión» | IMP | GS-01,CDH-02 | V2 | A | 14 | REQ5.1,REQ4.2 | P1 | Parcial. `floatsat_term/protocol.py` y `link.py` ya decodifican y hablan por WebSocket: reutilizables. Sin evidencia de la GS en sí |
| GS-03 | Pantalla Telemetría/Estado | Indicadores, modelo 3D, gráfica de yaw y referencia | IMP | GS-02 | S1-S2 | A | 16 | REQ5.2,REQ5.3,REQ5.4,REQ5.7 | P1 | Sin evidencia en repo · atrasada |
| GS-04 | Pantalla M0 | Consigna, parámetros editados frente a confirmados, gráfica, CSV y limpiar | IMP | GS-03,CDH-04 | S3 | A | 12 | REQ5.5,REQ5.8,REQ5.9,REQ5.10 | P1 | Sin evidencia en repo |
| GS-05 | Pantalla M1 | Orientación, velocidad angular, señales de luz y estado de la misión | IMP | GS-03,CDH-08 | S5 | A | 6 | REQ-M1 | P1 | Pendiente |
| GS-06 | Pantalla M2 | Editor de secuencia con validación local, envío, visor e historial de fotos | IMP | CDH-10,COM-06 | S6-S7 | A | 16 | REQ5.6,REQ2.2 | P1 | Pendiente |
| GS-07 | Pantalla OTA | Selector de `.bin`, progreso por etapas, errores, bloqueo de controles | IMP | GS-03,OTA-04 | S7 | A | 6 | REQ2.4 | P1 | Pendiente |

### PAY - Misión 1 (luz) y Misión 2 (cámara)
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| PAY-01 | Drivers TCA9548A y VEML7700 | En el STM32 con el mutex de I2C, tres canales a 10 Hz o más | IMP | CDH-01,PCB-01 | S2-S3 | M | 10 | REQ-M1 | P1 | Parcial (adelantada). Drivers y wrapper en `c589b82`. **Resta:** `LightSensor_ReadAll` no lo llama ninguna tarea; sin mutex; `HAL_Delay(1)` dentro del ciclo de lectura; sin prueba con hardware; bugs de CDH-13 |
| PAY-02 | Montaje y caracterización de los sensores de luz | Ángulo entre sensores laterales; respuesta frente al ángulo con una lámpara, luz de fondo, T_det | PRU | PCB-01 | S4 | A | 8 | REQ-M1 | P1 | Pendiente |
| PAY-03 | Algoritmo de la Misión 1 | FSM SEARCH, ACQUIRE, TRACK, LOST con las ecuaciones (1) y (2); K_s y timeout; inyectar la referencia al ADCS | IMP | PAY-01,PAY-02,ADC-09,CDH-03 | S4-S5 | M | 12 | REQ-M1 | P1 | Pendiente |
| PAY-04 | Prueba de la Misión 1 con aire | Adquisición, seguimiento de una lámpara en movimiento, pérdida y readquisición | PRU | PAY-03,GS-05 | S6 | A+M | 8 | REQ-M1 | P1 | Pendiente |
| PAY-05 | Driver de la ArduCAM en la ESP32-S3 | OV2640 en JPEG por SPI e I2C, resolución, tamaño y tiempo de captura | IMP | PCB-01 | S2-S4 | A | 14 | REQ2.2 | P1 | Pendiente. `camera_module.c` es un esqueleto (tarea que imprime la frecuencia de CPU) sin SPI ni I2C |
| PAY-06 | Servicio de foto en la ESP32 | PHOTO_REQ, captura, metadatos, envío por el canal de imágenes, DONE o ERROR | IMP | PAY-05,COM-05,COM-06 | S6 | A | 8 | REQ2.2 | P1 | Pendiente. Hoy `handle_esp_command` responde "foto terminada" de inmediato (marcador útil para probar el lado STM32) |

### EPS - Energía y sensado eléctrico y térmico
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| EPS-01 | Medir el consumo actual | Batería y riel de 5 V en reposo y con la rueda al 10, 30 y 50 %; picos | PRU | - | V2 | M | 4 | REQ3.2 | P1 | Sin evidencia en repo · atrasada |
| EPS-02 | Presupuesto de potencia | Consumo medido más ESP32 con WiFi, cámara y BNO085; margen sobre 19.98 Wh para 1 h, límite del D24V50F5, shunt | DIS | EPS-01 | S1 | A | 6 | REQ3.1,REQ3.2 | P1 | Sin evidencia en repo · atrasada |
| EPS-03 | INA219 y MCP9808 en la ESP32 | Lectura a 1–2 Hz, alarma de batería baja y envío según COM-04 | IMP | EPS-02,COM-04,PCB-01 | S5 | A | 8 | REQ4.1 | P1 | Pendiente |

### PCB - Placa de expansión e integración física
No hay archivos de KiCad en el repo: el estado de esta sección no se puede verificar desde aquí.

| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| PCB-01 | Cableado provisional | ESP32, BNO085, sensores de luz, cámara, INA219 y MCP9808 sobre la plataforma | HW | H | S1-S2 | A | 6 | - | P1 | Sin evidencia en repo. El commit de sensores de luz sugiere que ya hay algo cableado: confirmar |
| PCB-02 | Asignación de pines | Tabla definitiva STM32 y ESP32: UART principal, UART del bootloader (OTA-01), BOOT0, NRST, BNO085, I2C, SPI cámara | DIS | OTA-01,ADC-05 | S1 | A+M | 6 | REQ8.1 | P1 | Pendiente · atrasada. Bloqueada por OTA-01 y ADC-05 |
| PCB-03 | DEC ruta de fabricación | Fresado CNC de 1 capa frente a fabricación externa; VEML7700 en breakouts | DEC | - | S1 | A | 1 | REQ8.1 | P1 | Sin evidencia en repo · atrasada |
| PCB-04 | Esquemático final | Completar la Fig. 10 y añadir el BNO085 | DIS | PCB-02,EPS-02 | S1-S2 | A | 12 | REQ8.1 | P1 | Parcial (esquemático preliminar del anteproyecto) |
| PCB-05 | Layout y revisión | Masas y geometría, BNO085 lejos del motor, DRC, revisión cruzada | DIS | PCB-04,PCB-03 | S3-S4 | A | 18 | REQ8.1,REQ8.2 | P1 | Pendiente |
| PCB-06 | Fabricar y probar la PCB v1 | Fabricación, soldadura, continuidad, rieles, escaneo I2C, loopback UART, BOOT0 y NRST | HW | PCB-05 | S5-S6 | A | 16 | REQ8.1 | P1 | Pendiente |
| PCB-07 | Montaje y balanceo | PCB sobre los headers, ajuste de masas, nivelación estable ±1° durante 5 min | PRU | PCB-06 | S7 | A | 6 | REQ8.2 | P1 | Pendiente |

### INT - Integración y verificación
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| INT-01 | Test 0: dar vida al sistema | Arranque estable, IMU y Madgwick, telemetría rápida y logs por el puente WiFi | PRU | CDH-01,COM-01 | V2 | M | 4 | REQ4.2 | P1 | Hecho (`410a25b`) con salvedad: bug 7 de desincronización UART sin resolver |
| INT-02 | M0 con aire desde la GS | Ganancias, ángulo y cambio de misión desde la GS con ACK/NACK visibles | PRU | GS-04,CDH-05,COM-03,AIRE | S3 | A+M | 6 | REQ2.1,REQ2.3 | P1 | Pendiente. Hacer una versión provisional con `floatsat_term` no espera a la GS |
| INT-03 | Plan de verificación | Matriz REQ, método, criterio y tarea que lo prueba | DOC | - | S4 | A+M | 4 | - | P1 | Pendiente |
| INT-04 | Migración a la PCB v1 | Repetir control con el BNO085, sensores de luz, cámara y EPS en la PCB | PRU | PCB-07,ADC-11,ADC-16 | S7 | A+M | 8 | REQ1.1,REQ8.1 | P1 | Pendiente |
| INT-05 | Misiones con el sistema completo | M1 y M2 (5 fotos) desde la GS; cambio de misión desde cada estado; override | PRU | INT-04,PAY-04,PAY-06,CDH-11,GS-06 | S8-SN | A+M | 12 | REQ2.2,REQ2.3,REQ1.2,REQ-M1 | P1 | Pendiente |
| INT-06 | Arranque y autonomía | 20 arranques desde batería con fallas inyectadas; misión de 1 h con V e I registrados | PRU | INT-04,CDH-06,EPS-03 | SN | A+M | 10 | REQ7.2,REQ3.1,REQ3.2 | P1 | Pendiente |
| INT-07 | Regresión y congelamiento | Ejecutar la matriz de INT-03, corregir defectos, etiquetar firmware STM32, ESP32, GS y PCB al cierre de S9 | PRU | INT-03,INT-05,INT-06,OTA-05,COM-07 | S9 | A+M | 24 | - | P1 | Pendiente |
| INT-08 | Prueba con estudiantes y ensayos | 2–3 estudiantes siguen el manual sin ayuda; dos ensayos con video como plan B | PRU | INT-07,GES-06 | S10 | A+M | 8 | - | P2 | Pendiente |

### GES - Gestión y documentación
| ID | Tarea | Detalle | Tipo | Dep. | Sem. | Resp. | h | REQ | P | Estado |
|---|---|---|---|---|---|---|---|---|---|---|
| GES-01 | Ordenar el repositorio | Sacar `Debug/` y credenciales del control de versiones, README con la estructura, `decision_log.md` con las decisiones de la sección 4 | DOC | - | V1 | M | 3 | - | P1 | Parcial · atrasada. Se dejó de rastrear los CSV y la caché de Python. **Resta:** siguen versionados `CubeIDE/Debug/` (53 archivos), `trace.bin`, `logs/log.txt`, `sdkconfig.old`, `.asv`, `esp32_out.html`, `.claude/settings.local.json` y `Python/results.zip`; credenciales WiFi en el código; README sin estructura; sin `decision_log.md`; `TODO.md` desactualizado |
| GES-02 | Registro de riesgos | Partir de la sección 7 y revisarlo cada viernes | DOC | - | V1 | A+M | 2 | - | P1 | Sin evidencia en repo |
| GES-03 | Revisión semanal de los viernes | Preparar, presentar y registrar retroalimentación (2 h por persona y semana) | DOC | - | S1-S10 | A+M | 40 | - | P1 | En curso |
| GES-04 | Hito de llegada de componentes | Compuerta H | HW | - | S1 | A+M | 0 | - | P1 | **Confirmar** |
| GES-05 | Requerimientos v1.1 | Cerrar los TBD (umbral de ω, alcance WiFi), redactar REQ-M1 y reconciliar REQ2.2 con el código (duración vs cantidad) | DOC | ADC-10,COM-07 | S5 | A+M | 3 | - | P1 | Pendiente |
| GES-06 | Manual de usuario y de mantenimiento | Encendido, GS, misiones, comandos, OTA, solución de problemas, cómo compilar y probar | DOC | INT-04 | S8-S10 | A+M | 16 | - | P1 | Pendiente |
| GES-07 | Informe final | Actualizar el anteproyecto con el diseño final y los resultados | DOC | INT-04 | S8-S10 | A+M | 40 | - | P1 | Pendiente |
| GES-08 | Video y presentación de defensa | Grabación, edición, diapositivas y guion | DOC | INT-07 | S10-S11 | A+M | 16 | - | P1 | Pendiente |
| GES-09 | Reserva de contingencia | Retraso de componentes, segunda PCB, bootloader o cámara | - | - | S5-S9 | A+M | 140 | - | - | - |

## 6. Estado según el repo (c589b82, 16-nov)

### 6.1 Avance por subsistema
| Subsistema | Semáforo | Resumen |
|---|---|---|
| Base de firmware, telecomando, telemetría, logging | Verde | Estables de extremo a extremo desde el 29-oct; falta CRC y mutex I2C |
| ADCS (control) | Verde/ámbar | Cascada PI+P operativa a 100 Hz con ganancias en caliente; faltan override de seguridad, anti-windup, calibración completa, abstracción de IMU y BNO085 |
| Misión 0 | Ámbar | Funcional por línea de comandos; faltan ACK/NACK, validación de parámetros, cadencia de telemetría y orquestación en el Core |
| Misión 2 (lado STM32) | Ámbar | Planificador con RTC adelantado al plan; sin validación, espera de estabilidad ni aborto; ESP32 es un esqueleto |
| Misión 1 | Rojo | Drivers de luz escritos con dos bugs que bloquean el arranque, sin integrar, sin FSM ni caracterización |
| Enlace WiFi | Ámbar | Puente bidireccional útil a 500 kbaud; desincronización tras reinicio sin resolver; credenciales en el código |
| OTA | Rojo | Cero código y cero notas. OTA-01 (V2) y OTA-02 (S1) vencidas |
| GS | Sin evidencia | Solo consola y plotter en Python; GS-01 sin registrar |
| PCB, EPS, BNO085 (estudio) | Sin evidencia | Nada de esto vive en el repo |

### 6.2 Cumplimiento del plan
Estimación propia, ±15 puntos: de las 18 tareas de Miguel con fin previsto hasta S2 (~129 h de plan), el contenido terminado equivale a ~55 % (~71 h). Además hay ~14 h de trabajo adelantado (fotos con RTC, sensores de luz, `floatsat_term`). Neto: ~65 %, con el control y el tooling de pruebas por delante y todo lo de ESP32, OTA, BNO085 y robustez de comandos por detrás. Esto no mide horas gastadas, solo contenido hecho frente al plan. El trabajo de Axel no se puede medir desde el repo.

### 6.3 Defectos y desviaciones a corregir
1. **El arranque falla en HEAD** (CDH-13): `LightSensor_Init` con condición invertida y un índice fuera de rango; verificado por lectura, no compilado.
2. **Credenciales WiFi en un repo clonable sin autenticación**, incluida la historia, y `wifi_module.c` imprime la contraseña. Rotar las contraseñas, mover los datos a configuración local ignorada por git y limpiar la historia si el repo va a seguir público.
3. **Referencia de 0°** (ADC-05): el yaw sale de Madgwick 9-DOF, así que 0° es el norte magnético y no la orientación de inicialización del anteproyecto, y el motor interfiere con el magnetómetro (limitación documentada).
4. **REQ2.2 vs código**: la foto tiene "cantidad", no "duración de retención".
5. **Fotos por hora absoluta** sin forma de fijar el RTC (CDH-14) ni de sincronizarlo.
6. **Espera fija de 10 s** en lugar de verificar estabilidad antes de fotografiar (CDH-11).
7. **Parámetros sin validar**: un comando `control` puede dejar la configuración a medias y no hay valores por defecto de respaldo (impide cumplir G6 y G7).
8. **Documentación atrasada**: especificación de telecomandos, `TODO.md`, comentario de `usart.c` (aún dice USART1/PB6-PB7; el código usa USART2/PA2-PA3).
9. **Evidencia fuera del repo**: CSV y script de identificación, PCB, GS.

## 7. Decisiones pendientes, en orden
| ID | Decisión | Sem. | Depende de | Nota |
|---|---|---|---|---|
| GS-01 | Tecnología de la GS | V1 | - | **Vencida** |
| COM-02 | Modo WiFi en el laboratorio | V2 | - | **Vencida**; de facto STA personal |
| OTA-01 | UART del bootloader y pines | V2 | - | **Vencida**; bloquea PCB-02 |
| ADC-05 | Interfaz y reporte del BNO085 + referencia de 0° | S1 | ADC-04,ADC-12 | **Vencida**; bloquea PCB-02 |
| COM-04 | Ruta de V/I/T | S1 | - | **Vencida** |
| PCB-03 | Ruta de fabricación | S1 | - | **Vencida** |
| CDH-07 | Tiempo de las fotos | S3 | - | Tomada de facto (absoluta): registrar |
| CDH-10 | Duración vs cantidad en la foto (REQ2.2) | S3 | - | Nueva |

## 8. Riesgos y alertas
| ID | Riesgo | Disparador | Mitigación |
|---|---|---|---|
| R1 | El UART del enlace no sirve para OTA | OTA-01 confirma que USART2 no es puerto del bootloader del F407 | Segundo UART (USART1 o USART3) solo para flashear, reservado en PCB-02. **Activo:** OTA-01 lleva ~3 semanas vencida y es el ítem de mayor riesgo |
| R2 | Componentes atrasados | H no se cumple en S1 | ADC-14, PAY-01, PAY-05 y EPS-03 se corren; el control y la GS siguen con el hardware actual. **Verificar si ya está activo** |
| R3 | Plazo de fabricación externa | PCB-03 elige fabricación externa | Pedir antes de S5; PCB-01 permite seguir integrando |
| R4 | El driver del BNO085 se atrasa o falla | ADC-15 no estable al cierre de S6 | Volver al LSM9DS1 sin tocar el control. **Cercano:** ADC-05, ADC-12 y ADC-14 están vencidas; propuesta: si no están cerradas al fin de S4, el BNO085 pasa a trabajo futuro |
| R5 | La red de INTEC bloquea el WiFi | COM-02 | SoftAP de la ESP32. Sin decisión aún |
| R6 | Carga de Miguel | Semanas de S1 a S6 con más de 30 h de firmware | Mover a Axel tareas de ESP32 o de pruebas, o pasar CDH-12 e INT-08 a P3. **Activo:** firmware, ESP32 y OTA recaen casi por completo en él |
| R7 | **Nuevo.** Regresiones sin verificación de compilación y arranque | Commit que rompe el boot (como `c589b82`) | Compilar y hacer smoke test de arranque en banco antes de empujar a `main`; opcional ramas por tarea |
| R8 | **Nuevo.** Pérdida de desincronización del UART ESP32↔STM32 | Bug 7 sin resolver | Resolver en COM-01; afecta REQ4.2 (no perder actitud) y la OTA |
| R9 | **Nuevo.** Evidencia y trabajo fuera del repo | CSV, scripts, PCB y GS sin versionar | Carpeta `MATLAB/sysid/` y enlace o carpeta para PCB y GS; decidir antes de INT-03 |

Notas de revisión del anteproyecto, a corregir en el informe final:
1. La sección VI dice que la ESP32 pone NRST en alto para entrar al bootloader. El AN3155 requiere BOOT0 en alto y luego un pulso bajo en NRST.
2. La sección VI describe la OTA en pasado, pero en el repo todavía no existe.
3. El comentario de `usart.c` habla de USART1 en PB6/PB7, pero el código configura USART2 en PA2/PA3.
4. **Nueva:** el anteproyecto no menciona el baud de 500 kbaud ni que la referencia de yaw depende del magnetómetro.

## 9. Cambios respecto a la v0.2
- Actualizado el estado de todas las tareas contra el commit `c589b82`; ver leyenda en la sección 1.
- Sección 4 ampliada con control en cascada, fotos con RTC, `floatsat_term`, normalización del magnetómetro, bus de datos IMU y recuperación de I2C; corregido el baud (500 kbaud).
- Nuevas tareas: CDH-13 (desbloquear el arranque) y CDH-14 (fijar y sincronizar el RTC). ADC-05 absorbe la decisión de la referencia de 0°. CDH-10 absorbe la decisión duración vs cantidad. GES-05 reconcilia REQ2.2.
- Nueva sección 6 con el estado por subsistema, el cumplimiento del plan y los defectos abiertos.
- Marcadas como vencidas las tareas y decisiones cuya semana de fin ya cerró.
- Riesgos R7 a R9 nuevos; R1, R2, R4 y R6 con señal de activación.
- Sin cambios de horas salvo las dos tareas nuevas (+4 h de Miguel). No se replanificaron semanas: las tareas vencidas conservan su semana original hasta que ustedes decidan el nuevo orden.
