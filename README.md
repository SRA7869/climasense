# ClimaSense: Home Comfort Monitoring & Automation System

A software-only simulation of a smart-home comfort controller, written in C++17. It reads virtual
temperature and humidity sensors and automatically controls a virtual Fan, AC, and Exhaust.

## Problem statement

Controlling a fan, an air conditioner, and an exhaust by hand wastes energy and leaves rooms
uncomfortable: devices run in empty rooms, or nobody switches them on until the room is already
unpleasant. An automatic controller solves that, but it brings a harder problem: **sensors fail**.
A broken sensor can report impossible values (900 degrees C), freeze on one number, or stop
responding. A controller that trusts such data can run an AC forever or switch devices on and off
rapidly. A comfort system therefore has to be automatic **and** robust against bad inputs.

## Objective

Design and build the control software of a smart-home comfort system that:

1. monitors temperature and humidity,
2. classifies comfort and switches Fan, AC, and Exhaust using configurable rules,
3. detects bad sensor data and failed sensors, falls back to a safe state, and recovers automatically,
4. is structured so that real Arduino sensors and relays can replace the simulated ones later
   without rewriting the controller.

Temperature and humidity are enough because comfort depends on both together: 30 degrees C is
tolerable when dry and miserable when humid.

## Features

- Simulated temperature and humidity sensors with realistic drift (thermal inertia) and noise
- Reading validation: not-a-number, out-of-range, stale, and stuck-value detection
- Sensor health tracking with debouncing: failed after 3 bad readings in a row, recovered after 3 good ones
- Comfort levels with hysteresis, so devices do not chatter when a value hovers at a threshold
- Comfort state machine: COMFORTABLE, WARM, HUMID, VERY_UNCOMFORTABLE
- Automation engine that issues a command only when a device actually needs to change
- Fail-safe: all devices switch off while a sensor is failed, and automation resumes on recovery
- Event bus connecting the layers, with a persistent log file (`climasense.log`)
- Thresholds loaded from a validated configuration file (`climasense.conf`)
- Live terminal dashboard: readings, sensor health, comfort state, devices, statistics, recent events
- Multithreaded design (sensor, control, dashboard) using a mutex, a condition variable, and atomics
- Fault injection, automated tests, and AddressSanitizer, UBSan, and ThreadSanitizer builds

### Behaviour

Default thresholds (the supplied `climasense.conf` uses slightly lower ones so the demo reacts faster):

| Condition | Result |
|---|---|
| Temperature at or above 28 C (HIGH) | Fan ON |
| Temperature at or above 33 C (VERY_HIGH) | AC ON, fan stays on |
| Humidity at or above 70 % | Exhaust ON |
| Reading back below the threshold minus the 1.0 hysteresis margin | That device OFF |

| Temperature level | Humidity level | Comfort state |
|---|---|---|
| NORMAL | NORMAL | COMFORTABLE |
| HIGH | NORMAL | WARM |
| NORMAL | HIGH | HUMID |
| HIGH | HIGH | VERY_UNCOMFORTABLE |
| VERY_HIGH | any | VERY_UNCOMFORTABLE |

**Failure handling:** a reading is rejected if it is not a number, outside the plausible range
(temperature -40 to 80 C, humidity 0 to 100 %), older than 2 seconds, or identical 5 times in a row.
One rejected reading changes nothing. After 3 in a row the sensor is declared FAILED, every device
is switched off, and the comfort state holds its last value. After 3 good readings in a row the
sensor is declared RECOVERED and automation resumes.

## Architecture

The system is a pipeline of layers. Each layer talks only to its neighbour, and everything that
would touch hardware sits behind an interface (`ISensor`, `IDevice`).

```
 TemperatureSensor   HumiditySensor        (both implement ISensor; FaultySensor wraps either one)
          \              /
           SensorManager                    polls the sensors, timestamps each reading
                 |
        ThreadSafeQueue                     hands sample batches from the sensor thread to the control thread
                 |
  Validator + SensorHealth                  reject bad readings, declare sensors FAILED / RECOVERED
                 |
 ComfortCalculator (levels, hysteresis)
                 |
 ComfortStateMachine                        COMFORTABLE / WARM / HUMID / VERY_UNCOMFORTABLE
                 |
 AutomationEngine                           rules to device commands, only on change, fail-safe shutdown
                 |
 DeviceManager -> Fan, AC, Exhaust          (all implement IDevice)

 EventBus carries STATE_CHANGED, SENSOR_INVALID, SENSOR_FAILED, SENSOR_RECOVERED, DEVICE_COMMAND
     to subscribers: Logger (climasense.log) and the StatusBoard (statistics, recent events)
 Dashboard thread reads the StatusBoard and redraws the terminal
 Controller owns the whole pipeline and the threads; main only talks to the Controller
```

**Threads.** The sensor thread polls every 200 ms and pushes batches into the queue. The control
thread pops them and runs the pipeline. The dashboard thread redraws every 500 ms from a copy of the
status board. The main thread plays the room and the fault injector. The design rule is to share as
little as possible:

| Shared data | Written by | Read by | Protection |
|---|---|---|---|
| Sample queue | sensor thread | control thread | mutex and condition variable |
| Status board | control thread, main | dashboard thread, main | mutex; readers copy a snapshot |
| Sensor targets, fault mode | main | sensor thread | `std::atomic` |
| Everything else in the pipeline | control thread only | not shared | none needed |

UML class, sequence, and state diagrams for all of this are in
[docs/architecture.md](docs/architecture.md).

## Technologies used

| Area | Technology |
|---|---|
| Language | C++17 (standard library only, no external libraries) |
| Concurrency | `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic` |
| Time and randomness | `<chrono>`, `<random>` |
| Build | GNU Make, `g++` |
| Testing | A small custom test framework (`tests/test.h`) |
| Debugging and checking | AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer, gdb |
| Documentation | Markdown, Mermaid diagrams |
| Version control | Git |
| Environment | Ubuntu on WSL (Linux and macOS should also work) |

## How to run the project

Requirements: `g++` with C++17 support, `make`, and a POSIX system. On Ubuntu:

```
sudo apt update && sudo apt install -y build-essential
```

Then, from the project folder:

```
git clone https://github.com/SRA7869/climasense.git
cd climasense
make run
```

| Command | What it does |
|---|---|
| `make` | Builds `./climasense` |
| `make run` | Builds and runs the demo scenario (about 12 seconds) |
| `make test` | Builds and runs the automated tests |
| `make asan` | Runs the tests under AddressSanitizer and UBSan |
| `make tsan` | Runs the whole system under ThreadSanitizer |
| `make clean` | Removes build output |

For the live dashboard, use a terminal at least **30 rows tall and 70 columns wide**. If output is
piped to a file, the live redraw is skipped and only the final frame is printed.

**What the demo does:** the room heats up and the Fan, Exhaust, and AC switch on one by one. At about
4 s a fault is injected into the temperature sensor; after three rejected readings the sensor is
declared failed and every device switches off. At about 6 s the fault is cleared, the sensor
recovers, and the devices switch back on. At about 7 s the room cools down and the devices switch off.

### Configuration

`climasense.conf` is read from the current directory. Missing keys keep their defaults.

| Key | Default | Meaning |
|---|---|---|
| `temp_high` | 28.0 | C, temperature level HIGH (fan) |
| `temp_very_high` | 33.0 | C, temperature level VERY_HIGH (AC) |
| `humidity_high` | 70.0 | %RH, humidity level HIGH (exhaust) |
| `hysteresis` | 1.0 | margin before a level drops back |
| `fan_runs_with_ac` | true | keep the fan on while the AC is on |

The loader treats the file as untrusted input: bad lines are reported with their line number, and an
inconsistent result (for example `temp_high` above `temp_very_high`) falls back to the defaults.
`bad.conf` is an example of an invalid file.

## Project structure

```
climasense/
  include/        headers (interfaces, classes, templates)
  src/            implementations and main.cpp
  tests/          test framework and test files
  docs/           architecture.md (UML diagrams as Mermaid text)
  climasense.conf configuration used by the demo
  bad.conf        example of an invalid configuration
  Makefile        build, run, test, and sanitizer targets
  README.md
```

| Layer | Files | Responsibility |
|---|---|---|
| Sensors | `ISensor`, `TemperatureSensor`, `HumiditySensor`, `FaultySensor`, `SensorManager` | produce readings, inject faults |
| Validation | `Validator`, `SensorHealth` | judge readings and sensors |
| Comfort | `ComfortCalculator`, `ComfortStateMachine` | levels with hysteresis, then named states |
| Events | `Event`, `EventBus` | decoupled notifications |
| Automation | `AutomationEngine` | rules to device commands |
| Devices | `IDevice`, `VirtualDevices`, `DeviceManager` | virtual Fan, AC, Exhaust |
| Orchestration | `Controller` | owns the pipeline and the threads |
| Monitoring | `Logger`, `Monitor`, `Dashboard` | log file, statistics, terminal UI |
| Support | `Config`, `ThreadSafeQueue`, `Console` | config loading, blocking queue, safe printing |

## Sample output

Event log from one run (`climasense.log`; times vary by a sample or two):

```
=== run started 2026-10-04 14:32:07 ===
[     0 ms] NOTE             config: temp_high=27.0 temp_very_high=32.0 humidity_high=65.0 ...
[     0 ms] SYSTEM_STARTED   controller started
[   300 ms] STATE_CHANGED    COMFORTABLE -> WARM
[   300 ms] DEVICE_COMMAND   FAN ON
[  1000 ms] STATE_CHANGED    WARM -> VERY_UNCOMFORTABLE
[  1000 ms] DEVICE_COMMAND   EXHAUST ON
[  1800 ms] DEVICE_COMMAND   AC ON
[  4200 ms] SENSOR_INVALID   Temperature: OUT_OF_RANGE
[  4400 ms] SENSOR_INVALID   Temperature: OUT_OF_RANGE
[  4600 ms] SENSOR_INVALID   Temperature: OUT_OF_RANGE
[  4600 ms] SENSOR_FAILED    Temperature sensor failed
[  4600 ms] DEVICE_COMMAND   FAN OFF
[  4600 ms] DEVICE_COMMAND   AC OFF
[  4600 ms] DEVICE_COMMAND   EXHAUST OFF
[  6600 ms] SENSOR_RECOVERED Temperature sensor recovered
[  6600 ms] DEVICE_COMMAND   FAN ON
[  6600 ms] DEVICE_COMMAND   AC ON
[  6600 ms] DEVICE_COMMAND   EXHAUST ON
[  8000 ms] DEVICE_COMMAND   AC OFF
[  9000 ms] STATE_CHANGED    VERY_UNCOMFORTABLE -> WARM
[  9000 ms] DEVICE_COMMAND   EXHAUST OFF
[ 10500 ms] STATE_CHANGED    WARM -> COMFORTABLE
[ 10500 ms] DEVICE_COMMAND   FAN OFF
```

Final dashboard frame:

```
==================== ClimaSense ====================
 Uptime 12.0 s   |   samples 60
----------------------------------------------------
 Temperature   24.31 C   OK
   min 24.31   avg 30.12   max 35.70
 Humidity      47.80 %   OK
   min 49.70   avg 66.40   max 87.90
----------------------------------------------------
 Comfort state : COMFORTABLE
 Devices       : FAN OFF | AC OFF | EXHAUST OFF
----------------------------------------------------
 Statistics
   invalid readings 3   failures 1   recoveries 1
   state changes 4   device switches 12
   seconds in state: COMFORTABLE 1.9  WARM 2.2
                     HUMID 0.0  VERY_UNCOMFORTABLE 7.9
----------------------------------------------------
 Recent events
   [ 6600 ms] SENSOR_RECOVERED  Temperature sensor recovered
   [ 7000 ms] >>> room conditions returning to normal
   ...
====================================================
```

The maximum temperature stays near 35.7, never 999, because rejected readings are not counted in
the statistics.

## Testing

`make test` runs deterministic tests with exact inputs and expected outputs. Expected summary:

```
50 tests, 227 checks, 0 failed
```

| Area | What is verified |
|---|---|
| Validator | NaN, range limits (inclusive), stale readings, stuck values |
| Sensor health | Failure after 3 consecutive bad readings, recovery after 3 good ones, streak resets |
| Fault injector | Each fault mode, and that a stuck sensor jumps back to the real value when cleared |
| Comfort calculator | Levels, hysteresis, chatter prevention, custom thresholds |
| State machine | Every level combination and transition reporting |
| Automation engine | Device rules, commands only on change, fail-safe shutdown and recovery |
| Devices | Idempotent switching, routing, on-time accounting |
| Config loader | Valid files, bad lines, inconsistent thresholds, NaN values, missing file |
| Event bus and queue | Ordering, typed subscriptions, follow-up events, close and drain, cross-thread delivery |
| Statistics and dashboard | Min, max, mean, recent-event limit, frame content and constant height |
| Controller | Heat-up, fail-safe and recovery, a single bad reading, statistics exclude bad data, clean shutdown, log file |

`Controller::processBatch` is public, so the whole decision pipeline is tested with hand-made
readings and no threads or randomness. `make asan` checks for memory errors and undefined behaviour,
and `make tsan` checks the live multithreaded system for data races.

## Design decisions

- **Interfaces at the hardware boundary** (`ISensor`, `IDevice`), so everything above them is hardware-independent.
- **Events between layers**, so logging and statistics were added without changing the state machine or the engine.
- **Hysteresis and debouncing**, so noise cannot flip the system.
- **Fail-safe default**: when inputs cannot be trusted, devices go off rather than keep acting on stale decisions.
- **Share as little as possible between threads.**
- **Design for testability**: time is a parameter, logic functions are pure, the control cycle is a public function.
- **Validate all external input**, including the configuration file.

## Future work

- **Arduino port.** Write an `ISensor` for a real sensor (for example a DHT22) and an `IDevice` that
  drives a relay pin, and replace the threads with a `loop()` that calls `processBatch`. The
  validation, comfort, automation, and event code does not change.
- Per-sensor failure policy: a failed temperature sensor could shut down only the Fan and AC.
- Minimum on/off times for compressor protection, and log rotation for long runs.

## Team members

| Name | Role |
|---|---|
| SK Rahemat Alli | Design and implementation |

*(Add any teammates as extra rows. If this was an individual project, keep the single row.)*
