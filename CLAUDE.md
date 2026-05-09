# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

University embedded systems project (LSFarm / P2FB) with two communicating components:

1. **Embedded C firmware** for a PIC18F4321 (XC8 / MPLAB X). Drives an HD44780 LCD, a joystick + LDR via ADC, two UARTs, and runs a cooperative scheduler.
2. **Java Swing desktop app** (`P2FB/`) that talks to the PIC over the hardware UART and shows the farm GUI.

Authoritative spec: `Enunciat P2FB.pdf`. Functional spec for the central firmware FSM: `esquema_TAD_FARM.md` (Catalan). LCD driver internals: `explicacio_LcTLCD.md` (Catalan).

## Building and Running

### Embedded C
Build via MPLAB X IDE with the XC8 compiler — there is no Makefile. The firmware entry point is `main.c` at the repo root. Configuration bits are set there (`OSC=HS`, etc.) for Fosc = 10 MHz.

The interrupt service routine in `main.c` dispatches `RSI_Timer0()`, `SIO_InterrupcioRX()`, `SIO_InterrupcioTX()`. The main loop calls each module's `*_Motor()` once per iteration.

`exemplesTADS/` is reference material from the lecturer, not part of the build (e.g. `CCODIS.C` is the canonical "motor" pattern; `pic18f4321.h` is the SFR header).

### Java Application
No Maven/Gradle — it is an IntelliJ module (`P2FB/P2FB.iml`).
- Source: `P2FB/src/`
- Dependencies (jars, jSerialComm etc.): `P2FB/lib/`
- Pre-compiled output: `P2FB/out/production/P2FB/`
- Entry point: `Main.java` → `SwingUtilities.invokeLater(() -> new UI())`

To build/run from a terminal:
```
javac -cp "P2FB/lib/*" -d P2FB/out/production/P2FB P2FB/src/*.java
java  -cp "P2FB/out/production/P2FB;P2FB/lib/*" Main
```

### Tests
No automated tests on either side.

## Architecture

### Firmware: cooperative TAD (Abstract Data Type) pattern

Every embedded module follows the same convention, modeled on `exemplesTADS/CCODIS.C`:

- A `*_Init()` called once before `ei()`.
- A `*_Motor()` called every iteration of the main `while (1)` loop. The motor must **never block** — it advances at most one step per call. Long operations are split across many calls via an internal state machine.
- Optional `*_InterrupcioXX()` entry points invoked from the single ISR in `main.c`.

The current main loop (`main.c`) runs: `SIOFARM_Motor → AD_Motor → JOY_Motor → JOY_MotorInterficie → FARM_Motor`. `JOY_MotorInterficie` is a second motor inside `TAD_JOYSTICK` that translates the latest joystick movement / button press into the `MOVE_*` / `SELECT` UI messages on `TAD_SIO`; it must run **after** `JOY_Motor` so it sees the freshly latched edge. `LcMotor` (from `TAD_LCD`) is **not** wired in yet — add it to the loop when the LCD becomes part of the system.

#### Module map

| Module | Role | Hardware |
|---|---|---|
| `TAD_TIMER` | 10-slot virtual timer service driven by Timer0 ISR. 1 tic ≈ 417 µs (Fosc=10 MHz, no prescaler, reload 64496). All other TADs grab a handle via `TI_NewTimer`. | Timer0 |
| `TAD_SIO` | **Hardware UART** (TX=RC6, RX=RC7, ~9600 bps). Interrupt-driven RX/TX queues. This is the channel to the Java app. | EUSART |
| `TAD_SIOFARM` | **Software UART by bit-banging** (TX=RD1, RX=RD0, 1200 bps, 2 Timer0 tics per bit). Used as the local terminal for the date/time prompt. Has its own circular queues; `SIOFARM_Motor()` advances the bit-banging state. | RD0/RD1 |
| `TAD_ADC` | Multiplexed 8-bit ADC (left-justified). Clients call `AD_RegistraCanal(ANx)` to register and get an index; `AD_Motor` round-robins one channel per call. Up to `AD_MAX_CANALS` (4). | ADCON |
| `TAD_JOYSTICK` | 4-direction joystick + button via ADC. `JOY_GetMoviment` returns a direction once per movement (edge detection — must return to centre to fire again). `JOY_BotoNouPremut` likewise fires once per press. The second motor `JOY_MotorInterficie` ships those events out over `TAD_SIO` as `MOVE_UP/DOWN/LEFT/RIGHT` / `SELECT`. | AN0/AN1 + button |
| `TAD_FARM` | **Application FSM**. Coordinates everything else. Owns no hardware directly. See below. | — |
| `TAD_LCD` (`TAD_LCD.c/.h`) | Cooperative HD44780 driver with a 3-deep FIFO of pending orders. All public calls (`LcClear`, `LcGotoXY`, `LcPutChar`, …) just enqueue and return 1/0; `LcMotor` does the work. Pin-out is defined by macros in the header — keep `RS`, `RW`, `E` and the four data pins on disjoint port-B bits. | RB pins per `TAD_LCD.h` |

#### `TAD_FARM` state machine

The high-level startup flow, implemented as `estat` in `TAD_FARM.c`:

1. `ESTAT_DEMANAR_DATA` — print the prompt to the SIOFARM terminal.
2. `ESTAT_LLEGIR_DATA` — read `DD/MM HH:MM:SS` over SIOFARM with echo and backspace handling. Validate strictly (slash/space/colon positions and numeric ranges).
3. `ESTAT_ESPERAR_JAVA` — wait on the **hardware** UART (`TAD_SIO`) for a line that begins with `INITIALIZE:`. Only then transition to operating mode.
4. `ESTAT_FUNCIONAMENT` — normal operation (the broader spec — animals, products, joystick events, rebellion mode — is described in `esquema_TAD_FARM.md`; not all of it is implemented yet).

`TAD_FARM` keeps an outbound string queue (`cadenaPendent` / `MotorSortida`) so that printing to SIOFARM never blocks the main loop. **While a string is being drained, RX is intentionally held back** (`if (EstaEnviantCadena()) return;`) — keep this when extending the FSM.

### Java app — P2FB

| File | Role |
|---|---|
| `Main.java` | Entry point; launches UI on the EDT |
| `UI.java` | All Swing layout — card layout, animals panel + consumption/histogram panel |
| `Controller.java` | Serial port management (jSerialComm), inbound parsing, business logic |
| `FarmConstants.java` | String constants for every serial command |
| `Consum.java` | Custom `JPanel` that draws the bar-chart histogram |

**Serial protocol** (ASCII, `:` and `$` delimited):
- Java → PIC: `INITIALIZE:<name>$<t1>$<t2>$<t3>$<t4>`, `GET_PRODUCTS`, `GET_ANIMALS`, `RESET`, `CONSUME:<id>`, `SLEEP:<species>$<num>`, `START_REBELLION`, `STOP_REBELLION`
- PIC → Java: `DATA_PRODUCTS:<l>$<p>$<o>$<x>`, `DATA_ANIMALS:<species>$<num>$<state>`, `FINISH`, `SLEEP_SUCCESSFUL`, `SLEEP_UNSUCCESSFUL`, plus joystick events `MOVE_UP|DOWN|LEFT|RIGHT`, `SELECT`

UI: 24-slot animal grid (3×8), 6 control buttons, semi-transparent serial-log overlay, and a separate consumption histogram screen. Animal images live in `P2FB/assets/` and follow the pattern `<species>_<state>.png` (e.g. `porc_awake.png`).

The joystick on the PIC sends directional events; `Controller` translates them into focus changes and actions on the GUI.

## Conventions

- Naming and inline comments are in **Catalan** (`Motor`, `EnviaCaracter`, `HiHaCaracter`, `Pendent`, etc.). Match existing style when editing — don't translate identifiers.
- All firmware modules expose `*_Init` + `*_Motor`; if you add a new TAD, follow the same shape and register its motor in `main.c`'s loop.
- Never block inside a motor. If something needs to take longer than one tick, give it its own state variable and split it across calls (see `TAD_LCD` and `TAD_SIOFARM` for examples).
- Static module-level state is the norm; there is no dynamic allocation. Keep new TADs the same.
